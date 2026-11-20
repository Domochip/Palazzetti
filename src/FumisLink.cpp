#include "FumisLink.h"

// The original SERIALCOM_* layer and the frame and retry parts of the fumis* functions:
//  - SERIALCOM_OpenComport, SERIALCOM_CloseComport, fumisOpenSerial, fumisCloseSerial: not reproduced,
//    the application opens the port once at BAUDRATE, 8N1, and it is never closed
//    (original: serialPortModel 1 also flushed the port when opening it)
//  - SERIALCOM_Flush: discardInput
//  - SERIALCOM_ReceiveBuf: readBytes
//  - SERIALCOM_SendBuf (and fumisSendRequest): the send part of exchange
//  - fumisWaitRequest: waitPoll and readResponse
//  - fumisComRead (without and with wordMode): readByte, readWord
//  - the retry loops of fumisComReadBuff, fumisComWrite and fumisComSetDateTime: read, writeByte/writeWord and setDateTime, with retry

// Run op up to attempts times until it gives Ok, on the same port (waitPoll discards the stale RX bytes)
// (original: three different loops: Read closed, flushed and reopened the port and retried only the communication errors,
//  Write and SetDateTime retried any error without reopening the port;
//  reopening the port is likely meant for a Linux tty (e.g. a USB adapter), it does not help with the built-in UART of an ESP)
template <typename Op>
FumisLink::Result FumisLink::retry(uint8_t attempts, Op op)
{
    Result res = Result::Error;
    for (uint8_t attempt = 0; attempt < attempts; attempt++) // (original: counted down)
    {
        res = op();
        if (res == Result::Ok)
            return res;
    }
    return res;
}

// (original: fumisComReadBuff, 4 attempts in local_2c, now 3)
FumisLink::Result FumisLink::read(uint16_t addr, uint8_t (&data)[FumisFrame::DATA_SIZE])
{
    memset(data, 0, FumisFrame::DATA_SIZE);

    FumisFrame request = FumisFrame::read(addr);
    FumisFrame response; // var_28

    Result res = retry(3, [&]()
                       { return exchange(request, &response); });
    if (res == Result::Ok)
        memcpy(data, response.data(), FumisFrame::DATA_SIZE);
    return res;
}

// (original: fumisComRead without wordMode: the same Read as for a word, only the low byte is kept)
FumisLink::Result FumisLink::readByte(uint16_t addr, uint16_t &value)
{
    uint8_t data[FumisFrame::DATA_SIZE];
    Result res = read(addr, data);
    if (res == Result::Ok)
        value = data[0];
    return res;
}

// (original: fumisComRead with wordMode, var_10)
FumisLink::Result FumisLink::readWord(uint16_t addr, uint16_t &value)
{
    uint8_t data[FumisFrame::DATA_SIZE];
    Result res = read(addr, data);
    if (res == Result::Ok)
        value = data[0] | (data[1] << 8);
    return res;
}

FumisLink::Result FumisLink::writeByte(uint16_t addr, uint8_t value, uint8_t expected)
{
    return writeChecked(addr, value, false, expected);
}

FumisLink::Result FumisLink::writeWord(uint16_t addr, uint16_t value)
{
    return writeChecked(addr, value, true, value);
}

// (original: fumisComWrite, 2 attempts in var_34, result in var_38)
// Custom: the read-back, a single Read exchange in each attempt (original was not checking the write)
FumisLink::Result FumisLink::writeChecked(uint16_t addr, uint16_t value, bool wordMode, uint16_t expected)
{
    FumisFrame lowRequest = FumisFrame::write(addr, value & 0xFF);  // var_2C
    FumisFrame highRequest = FumisFrame::write(addr + 1, value >> 8); // var_2C (second use)
    FumisFrame readBackRequest = FumisFrame::read(addr);
    FumisFrame readBackResponse;

    return retry(2, [&]()
                 {
        Result res = exchange(lowRequest);
        if (res == Result::Ok && wordMode)
            res = exchange(highRequest);
        if (res != Result::Ok)
            return res;

        res = exchange(readBackRequest, &readBackResponse);
        if (res != Result::Ok)
            return res;
        uint16_t readBack = readBackResponse.data()[0] | (readBackResponse.data()[1] << 8); // var_30
        if (!wordMode)
            readBack &= 0xFF;
        return (readBack == expected) ? Result::Ok : Result::Mismatch; });
}

// (original: fumisComSetDateTime, 3 attempts in local_6c)
FumisLink::Result FumisLink::setDateTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t weekDay, uint8_t day, uint8_t month, uint8_t yearSince2000)
{
    FumisFrame request = FumisFrame::setDateTime(second, minute, hour, weekDay, day, month, yearSince2000); // local_60

    return retry(3, [&]()
                 { return exchange(request); });
}

// One exchange: wait for a Poll, send the Request, then read the Response into response if given
// (not a separate function in original; fumisComReadBuff, fumisComWrite and fumisComSetDateTime also checked the port was open)
FumisLink::Result FumisLink::exchange(const FumisFrame &request, FumisFrame *response)
{
    // (original: skipped when _MBTYPE == 1, which this library never sets)
    Result res = waitPoll();
    if (res != Result::Ok)
        return res;

    // (original: fumisSendRequest + SERIALCOM_SendBuf, then tcdrain)
    if (m_port->write(request.bytes, FumisFrame::SIZE) != FumisFrame::SIZE)
        return Result::Error;
    m_port->flush();

    // V1 board: RX receives back what was just sent (original: read without checking, only when comPortNumber == 0)
    if (m_loopBack == LoopBack::Yes)
    {
        FumisFrame echo;
        res = readFrame(echo);
        if (res != Result::Ok)
            return res;
        if (echo != request)
            return Result::Collision; // (not in original)
    }

    if (!response)
        return Result::Ok;

    if (m_loopBack != LoopBack::Unknown)
        return readResponse(*response);

    // (not in original) loopback detection, on the first successful Read:
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

// Wait up to timeoutMs for a received byte (original: the select call of SERIALCOM_ReceiveBuf)
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

// (original: SERIALCOM_Flush, tcflush)
void FumisLink::discardInput()
{
    while (m_port->read() != -1)
        ;
}

// Read exactly count bytes, Timeout if they are not all received limitMs after startTime
// (original: SERIALCOM_ReceiveBuf and the receive loops of its callers: a select with a fixed timeout, then a check of the elapsed time;
//  a select or read error closed and reopened the port, a Stream has none)
FumisLink::Result FumisLink::readBytes(uint8_t *dst, uint8_t count, unsigned long startTime, unsigned long limitMs)
{
    uint8_t received = 0; // var_18
    while (received < count)
    {
        unsigned long elapsed = millis() - startTime;
        if (elapsed >= limitMs || !waitAvailable(limitMs - elapsed))
            return Result::Timeout;

        while (received < count && m_port->available() > 0)
            dst[received++] = m_port->read();

        delayMicroseconds(1);
    }
    return Result::Ok;
}

// Wait for a stove Poll: the stove then listens and the box may send one Request
// (original: fumisWaitRequest, which also read the Response, now readResponse)
FumisLink::Result FumisLink::waitPoll()
{
    FumisFrame frame;

    // (not in original) keep the RX buffer for 8ms after a valid Response: the stove sends its next frame ~0.1ms after it,
    // so the buffer then starts exactly at that frame; otherwise discard it
    bool alignedFrame = rxAligned && (millis() - lastResponseMs) < 8;
    if (!alignedFrame)
        discardInput();
    rxAligned = false;

    unsigned long startTime = millis(); // var_10 (instead of time())

    Result res = readBytes(frame.bytes, FumisFrame::SIZE, startTime, POLL_TIMEOUT_MS);
    while (res == Result::Ok)
    {
        // (not in original) fast path on the aligned buffer
        if (alignedFrame && frame.isValid(FumisFrame::Type::Poll))
        {
            delayMicroseconds(2000); // 2ms guard: the stove listens ~0.2ms after the end of its Poll
            return Result::Ok;
        }
        alignedFrame = false;

        // otherwise rely on silence: a Poll is followed by ~28ms of silence, other frames by at most ~9ms,
        // so a byte arriving within 13ms means this is not a Poll: slide by one byte
        if (waitAvailable(13))
        {
            memmove(frame.bytes, frame.bytes + 1, FumisFrame::SIZE - 1);
            res = readBytes(frame.bytes + FumisFrame::SIZE - 1, 1, startTime, POLL_TIMEOUT_MS);
        }
        else if (frame.isValid(FumisFrame::Type::Poll))
            return Result::Ok;
        else
            res = readBytes(frame.bytes, FumisFrame::SIZE, startTime, POLL_TIMEOUT_MS); // (not in original) full frame then silence but not a Poll: drop it
    }
    return res;
}

// Read the next frame, whatever it is (Response, or echo of the Request on the V1 board)
FumisLink::Result FumisLink::readFrame(FumisFrame &frame)
{
    frame = FumisFrame();
    return readBytes(frame.bytes, FumisFrame::SIZE, millis(), FRAME_TIMEOUT_MS); // var_10 (instead of time())
}

FumisLink::Result FumisLink::acceptResponse(const FumisFrame &frame)
{
    if (!frame.isValid(FumisFrame::Type::Response))
        return Result::BadFrame;

    // the next RX byte is the start of the stove's next frame (see waitPoll)
    rxAligned = true;
    lastResponseMs = millis();
    return Result::Ok;
}

// (part of fumisWaitRequest in original, used when the state was 4)
FumisLink::Result FumisLink::readResponse(FumisFrame &frame)
{
    Result res = readFrame(frame);
    if (res != Result::Ok)
        return res;
    return acceptResponse(frame);
}
