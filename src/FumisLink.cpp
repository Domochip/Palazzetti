#include "FumisLink.h"

// The original SERIALCOM_* layer and the frame and retry parts of the fumis* functions:
//  - SERIALCOM_OpenComport, SERIALCOM_CloseComport: the open and close functions given by the application
//    (open: always 8 data bits and 1 stop bit)
//  - SERIALCOM_Flush: discardInput
//  - SERIALCOM_ReceiveBuf: readBytes
//  - SERIALCOM_SendBuf (and fumisSendRequest): the send part of exchange
//  - fumisOpenSerial, fumisCloseSerial: open, close
//  - fumisWaitRequest: waitPoll (wait for the stove Poll) and readResponse (read the Response)
//  - the retry loops of fumisComReadBuff and fumisComSetDateTime: read and setDateTime, with retry

void FumisLink::setSerial(Stream &port, OpenFn open, CloseFn close)
{
    m_port = &port;
    m_open = open;
    m_close = close;
}

// (original: fumisOpenSerial)
// (original: serialPortModel 1 also flushed the port here)
void FumisLink::open()
{
    m_open(BAUDRATE);
    portOpen = true;
    rxAligned = false;
}

// (original: fumisCloseSerial)
void FumisLink::close()
{
    m_close();
    portOpen = false;
    rxAligned = false;
}

// Close and reopen the port: after a read error (receive) and before each new Read attempt (retry)
// (not a separate function in original)
// (original: also flushed the port, now left to waitPoll, which always flushes after a reopen since rxAligned is cleared)
void FumisLink::reopen()
{
    close();
    open();
}

template <typename Op>
FumisLink::Result FumisLink::retry(uint8_t attempts, bool reopenBetween, bool stopOnError, Op op)
{
    Result res = Result::Error;
    for (uint8_t attempt = 0; attempt < attempts; attempt++) // (original: counted down)
    {
        if (attempt > 0 && reopenBetween)
        {
            // (original: close, flush, sleep, open, sleep)
            reopen();
            delayMicroseconds(1);
        }

        res = op();
        if (res == Result::Ok)
            return res;
        if (stopOnError && res == Result::Error)
            return res;
    }
    return res;
}

// (original: fumisComReadBuff, 4 attempts in local_2c)
FumisLink::Result FumisLink::read(uint16_t addr, uint8_t (&data)[FumisFrame::DATA_SIZE])
{
    memset(data, 0, FumisFrame::DATA_SIZE);

    FumisFrame request = FumisFrame::read(addr);
    FumisFrame response; // var_28

    Result res = retry(4, true, true, [&]()
                       { return exchange(request, &response); });
    if (res == Result::Ok)
        memcpy(data, response.data(), FumisFrame::DATA_SIZE);
    return res;
}

// (original: one send of fumisComWrite; its 2 attempts (var_34) stay in Palazzetti::fumisComWrite, which adds the read-back check)
FumisLink::Result FumisLink::write(uint16_t addr, uint8_t value)
{
    FumisFrame request = FumisFrame::write(addr, value); // var_2C
    return exchange(request);
}

// (original: fumisComSetDateTime, 3 attempts in local_6c)
FumisLink::Result FumisLink::setDateTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t weekDay, uint8_t day, uint8_t month, uint8_t yearSince2000)
{
    FumisFrame request = FumisFrame::setDateTime(second, minute, hour, weekDay, day, month, yearSince2000); // local_60

    return retry(3, false, false, [&]()
                 { return exchange(request); });
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
    if (m_port->write(request.bytes, FumisFrame::SIZE) != FumisFrame::SIZE)
        return Result::Error;
    m_port->flush(); // wait for the end of the transmission (original: tcdrain)

    // loopBack (V1 board): RX receives back what was just sent, read it and check it is our Request
    // (original: read it without checking, only when comPortNumber == 0)
    if (m_loopBack == LoopBack::Yes)
    {
        FumisFrame echo;
        res = readFrame(echo);
        if (res != Result::Ok)
            return res;
        if (echo != request)
            return Result::Collision; // (not in original) echo differs: collision on the bus
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

// Wait up to timeoutMs for at least one received byte (original: the select call of SERIALCOM_ReceiveBuf)
bool FumisLink::waitAvailable(unsigned long timeoutMs)
{
    unsigned long startTime = millis();
    while (m_port->available() == 0)
    {
        if (millis() - startTime >= timeoutMs)
            return false;
    }
    return true;
}

// Discard the received bytes not read yet (original: SERIALCOM_Flush, tcflush)
void FumisLink::discardInput()
{
    while (m_port->read() != -1)
        ;
}

// Read exactly count bytes into dst
// (original: SERIALCOM_ReceiveBuf and the receive loops of its callers)
// returns Timeout if they are not all received 250ms after startTime (the start of the frame being waited for)
// (original: a select or read error reopened the port; a Stream has none)
FumisLink::Result FumisLink::readBytes(uint8_t *dst, uint8_t count, unsigned long startTime)
{
    uint8_t received = 0; // var_18
    while (received < count)
    {
        if (millis() - startTime > 250) // NOTE : the original value is 3000 but the stove sends a Poll at least every ~67ms
            return Result::Timeout;

        if (!waitAvailable(SELECT_TIMEOUT_MS))
            continue; // nothing received within SELECT_TIMEOUT_MS

        // read the bytes already received, up to count
        while (received < count && m_port->available() > 0)
            dst[received++] = m_port->read();

        delayMicroseconds(1);
    }
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
    if (!alignedFrame)
        discardInput();
    rxAligned = false;

    unsigned long startTime = millis(); // var_10 (instead of time())

    Result res = readBytes(frame.bytes, FumisFrame::SIZE, startTime);
    while (res == Result::Ok)
    {
        // Fast path (not in original): right after a valid Response, the buffer is aligned on the stove's next frame
        if (alignedFrame && frame.isValid(FumisFrame::Type::Poll))
        {
            delayMicroseconds(2000); // 2ms guard: the stove listens ~0.2ms after the end of its Poll
            return Result::Ok;
        }
        alignedFrame = false;

        // Otherwise rely on silence: a Poll is followed by ~28ms of silence, other frames by at most ~9ms.
        if (waitAvailable(13))
        {
            // a byte arriving within 13ms means this is not a Poll frame: slide by one byte (the new byte becomes the last one)
            memmove(frame.bytes, frame.bytes + 1, FumisFrame::SIZE - 1);
            res = readBytes(frame.bytes + FumisFrame::SIZE - 1, 1, startTime);
        }
        else if (frame.isValid(FumisFrame::Type::Poll))
            return Result::Ok;
        else
            res = readBytes(frame.bytes, FumisFrame::SIZE, startTime); // (not in original) full frame then silence but not a valid Poll: drop it, the next byte starts a new frame
    }
    return res;
}

// Read the next FumisFrame::SIZE bytes, whatever they are (Response, or echo of the Request on the V1 board)
FumisLink::Result FumisLink::readFrame(FumisFrame &frame)
{
    frame = FumisFrame();
    return readBytes(frame.bytes, FumisFrame::SIZE, millis()); // var_10 (instead of time())
}

// Check that a frame just read is a valid stove Response
FumisLink::Result FumisLink::acceptResponse(const FumisFrame &frame)
{
    if (!frame.isValid(FumisFrame::Type::Response))
        return Result::BadFrame;

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
