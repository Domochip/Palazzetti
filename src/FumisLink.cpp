#include "FumisLink.h"

// The original SERIALCOM_* layer and the frame part of the fumis* functions:
//  - SERIALCOM_OpenComport, SERIALCOM_CloseComport, SERIALCOM_Flush: m_serial.open/close/flush are called directly
//    (open: always 8 data bits and 1 stop bit)
//  - SERIALCOM_ReceiveBuf: receive
//  - SERIALCOM_SendBuf (and fumisSendRequest): the send part of exchange
//  - fumisOpenSerial, fumisCloseSerial: open, close
//  - fumisWaitRequest: waitPoll (wait for the stove Poll) and readResponse (read the Response)

bool FumisLink::hasSerial() const
{
    return m_serial.open && m_serial.close && m_serial.select && m_serial.read &&
           m_serial.write && m_serial.drain && m_serial.flush && m_serial.uSleep;
}

// (original: fumisOpenSerial)
FumisLink::Result FumisLink::open()
{
    rxAligned = false;
    int res = m_serial.open(BAUDRATE);

    // (original: serialPortModel 1 also flushed the port here)
    if (res < 0)
    {
        portOpen = false;
        return Result::Error;
    }

    portOpen = true;
    return Result::Ok;
}

// (original: fumisCloseSerial)
void FumisLink::close()
{
    m_serial.close();
    portOpen = false;
    rxAligned = false;
}

// Close and reopen the port after a read error (not a separate function in original)
// (original: also flushed the port, now left to waitPoll, which always flushes after a reopen since rxAligned is cleared)
FumisLink::Result FumisLink::reopen()
{
    close();
    return open();
}

// One exchange with the stove (not a separate function in original):
// wait for a stove Poll, send the Request (Read, Write or SetDateTime), then if response is given, read the stove Response into it
// returns Error if the port is closed (original: checked by fumisComReadBuff, fumisComWrite and fumisComSetDateTime)
FumisLink::Result FumisLink::exchange(const FumisFrame &request, FumisFrame *response)
{
    if (!portOpen)
        return Result::Error;

    // Wait for a Poll (original: skipped when _MBTYPE == 1, which this library never sets)
    Result res = waitPoll();
    if (res != Result::Ok)
        return res;

    // Send the Request (original: fumisSendRequest + SERIALCOM_SendBuf)
    size_t totalBytesWritten = 0;
    while (totalBytesWritten < FumisFrame::SIZE)
    {
        ssize_t bytesWritten = m_serial.write(request.bytes + totalBytesWritten, FumisFrame::SIZE - totalBytesWritten);
        if (bytesWritten < 0)
            return Result::Error;
        totalBytesWritten += bytesWritten;
    }
    m_serial.drain();

    // loopBack (V1 board): RX receives back what was just sent, read it and check it is our Request
    // (original: read it without checking, only when comPortNumber == 0)
    if (m_loopBack == LoopBack::Yes)
    {
        FumisFrame echo;
        res = readFrame(echo);
        if (res != Result::Ok)
            return res;
        if (echo != request)
            return Result::CommunicationError; // (not in original) echo differs: collision on the bus
    }

    if (!response)
        return Result::Ok;

    if (m_loopBack != LoopBack::Unknown)
        return readResponse(*response);

    // (not in original) loopBack detection, on the first successful Read:
    // the first frame received after the Request is either its echo (V1 board) or the stove Response (V2 board)
    // a Response cannot be identical to the Request in practice (for 0x201C read first, it would need STATUS 28 among others)
    res = readFrame(*response);
    if (res != Result::Ok)
        return res;

    if (*response == request)
    {
        res = readResponse(*response);
        if (res == Result::Ok)
            m_loopBack = LoopBack::Yes;
        return res;
    }

    res = acceptResponse(*response);
    if (res == Result::Ok)
        m_loopBack = LoopBack::No;
    return res;
}

// Receive the next bytes of the frame being read into frame.bytes[bufPosition..FumisFrame::SIZE-1]
// (original: SERIALCOM_ReceiveBuf, now with the timeout and the reopen on read error of its callers)
// returns:
//  - Ok: bufPosition is advanced by the bytes received (0 if nothing arrived within selectSerialTimeoutMs)
//  - CommunicationError: more than 250ms since startTime, or read error (the port is then reopened and the frame is lost)
//  - Error: the port could not be reopened
FumisLink::Result FumisLink::receive(FumisFrame &frame, uint8_t &bufPosition, unsigned long startTime)
{
    if (millis() - startTime > 250) // NOTE : the original value is 3000 but the stove sends a Poll at least every ~67ms
        return Result::CommunicationError;

    int res = m_serial.select(selectSerialTimeoutMs);
    if (res == 0)
        return Result::Ok; // nothing received within selectSerialTimeoutMs

    if (res > 0)
        res = m_serial.read(frame.bytes + bufPosition, FumisFrame::SIZE - bufPosition); // var_18

    // select error, read error, or nothing read although select reported data: reopen the port, the frame is lost
    if (res <= 0)
    {
        Result reopenRes = reopen();
        return (reopenRes != Result::Ok) ? reopenRes : Result::CommunicationError;
    }

    bufPosition += res;
    m_serial.uSleep(1);
    return Result::Ok;
}

// Wait for a stove Poll frame: the stove then listens and the box may send one Request
// (original: fumisWaitRequest, which also read the Response, depending on the state set by the caller: that part is now readResponse)
FumisLink::Result FumisLink::waitPoll()
{
    FumisFrame frame; // the Poll frame is only checked, not returned

    // Discard RX buffer content, except right after a valid stove Response:
    // the stove sends its next frame ~0.1ms after a Response, so the RX buffer then holds the start of a fresh, aligned frame.
    // keep the buffer for 8ms at most.
    bool alignedFrame = rxAligned && (millis() - lastResponseMs) < 8; // (not in original) the next FumisFrame::SIZE RX bytes are exactly the stove's next frame
    if (!alignedFrame && m_serial.flush() < 0)
        return Result::CommunicationError;
    rxAligned = false;

    unsigned long startTime = millis(); // var_10 (instead of time())
    uint8_t bufPosition = 0;

    while (true)
    {
        Result res = receive(frame, bufPosition, startTime);
        if (res != Result::Ok)
            return res;

        if (bufPosition == FumisFrame::SIZE)
        {
            // Fast path (not in original): right after a valid Response, the buffer is aligned on the stove's next frame
            if (alignedFrame && frame.isValid(FumisFrame::Type::Poll))
            {
                m_serial.uSleep(2000); // 2ms guard: the stove listens ~0.2ms after the end of its Poll
                return Result::Ok;
            }
            alignedFrame = false;

            // Otherwise rely on silence: a Poll is followed by ~28ms of silence, other frames by at most ~9ms.
            // A byte arriving within 13ms means this is not a Poll frame: slide by one byte and keep reading
            if (m_serial.select(13))
            {
                bufPosition--;
                memmove(frame.bytes, frame.bytes + 1, bufPosition);
            }
            else if (frame.isValid(FumisFrame::Type::Poll))
                return Result::Ok;
            else
                bufPosition = 0; // (not in original) full frame then silence but not a valid Poll: drop it, the next byte starts a new frame
        }
    }
}

// Read the next FumisFrame::SIZE bytes, whatever they are (Response, or echo of the Request on the V1 board)
FumisLink::Result FumisLink::readFrame(FumisFrame &frame)
{
    unsigned long startTime = millis(); // var_10 (instead of time())
    uint8_t bufPosition = 0;
    frame = FumisFrame();

    while (bufPosition < FumisFrame::SIZE)
    {
        Result res = receive(frame, bufPosition, startTime);
        if (res != Result::Ok)
            return res;
    }
    return Result::Ok;
}

// Check that a frame just read is a valid stove Response
FumisLink::Result FumisLink::acceptResponse(const FumisFrame &frame)
{
    if (!frame.isValid(FumisFrame::Type::Response))
        return Result::CommunicationError;

    // valid Response read : the next RX byte is the start of the stove's next frame
    rxAligned = true;
    lastResponseMs = millis();
    return Result::Ok;
}

// Read the stove Response frame, which starts right after the Read Request sent by the box (or after its echo)
// (part of fumisWaitRequest in original, used when the state was 4)
FumisLink::Result FumisLink::readResponse(FumisFrame &frame)
{
    Result res = readFrame(frame);
    if (res != Result::Ok)
        return res;
    return acceptResponse(frame);
}
