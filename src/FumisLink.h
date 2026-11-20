#ifndef FumisLink_h
#define FumisLink_h

#include <Arduino.h>
#include "FumisFrame.h"

// Fumis bus (38400 8N1, half-duplex polled bus, the stove is the master, see FumisFrame.h):
// the only code that touches the serial port
//  - Read, Write and SetDateTime of stove memory, with their retries (one rule: reopen the port, then retry, whatever the error)
//  - each attempt is one exchange: wait for the moment the stove listens (after its Poll), send one Request, read the Response
//  - handles the V1 board loopback (RX receives back what TX sends), detected on the first successful Read
// it knows nothing of what the registers mean
// (original: the SERIALCOM_* layer and the frame and retry parts of the fumis* functions, see FumisLink.cpp)
class FumisLink
{
public:
    // Open the serial port of the stream at baudrate, 8N1 (board specific: pins, UART swap...)
    using OpenFn = std::function<void(uint32_t baudrate)>;
    // Close the serial port of the stream (board specific: e.g. drive TX HIGH so the stove bus is not blocked)
    using CloseFn = std::function<void()>;

    // RX receives back what TX sends (V1 board): Yes, or not (V2 board): No
    // Unknown until the first successful Read with the stove decides it (see exchange)
    enum class LoopBack : uint8_t
    {
        Unknown,
        Yes,
        No
    };

    // the results the serial layer can give (mapped to Palazzetti::CommandResult by its callers), all of them are retried
    enum class Result : uint8_t
    {
        Ok,
        Timeout,   // the expected frame is not complete in time (Poll: POLL_TIMEOUT_MS, echo or Response: FRAME_TIMEOUT_MS)
        BadFrame,  // the Response has a wrong type or checksum
        Collision, // V1 board: the echo differs from the Request sent, something else was on the bus
        Mismatch,  // a write was read back with another value than expected
        Error,     // port closed, or write error
    };

    // the stream to talk through, and the functions that open and close its serial port
    void setSerial(Stream &port, OpenFn open, CloseFn close);
    // true if the stream and both functions are set
    bool hasSerial() const { return m_port && m_open && m_close; }

    void open();
    void close();

    // Read the 8 bytes at addr into data (zeroed on failure)
    // up to 3 attempts
    Result read(uint16_t addr, uint8_t (&data)[FumisFrame::DATA_SIZE]);
    // Read the byte at addr into value (0..255, unchanged on failure), as read
    // (value is a uint16_t like the one of readWord, to suit the uint16_t variables of Palazzetti)
    Result readByte(uint16_t addr, uint16_t &value);
    // Read the word (little endian) at addr into value (unchanged on failure), as read
    Result readWord(uint16_t addr, uint16_t &value);

    // Write 1 byte at addr, then read it back: Mismatch if it is not expected
    // (expected differs from value for registers the stove changes at once, e.g. a command it clears once run)
    // up to 2 attempts, each with a single Read exchange as read-back
    Result writeByte(uint16_t addr, uint8_t value, uint8_t expected);
    // Write the word (little endian) at addr, as low byte at addr then high byte at addr + 1, then read it back: Mismatch if it differs
    // up to 2 attempts, each with a single Read exchange as read-back
    Result writeWord(uint16_t addr, uint16_t value);

    // Set the stove date and time (weekDay: 1 = Monday .. 7 = Sunday)
    // up to 3 attempts
    Result setDateTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t weekDay, uint8_t day, uint8_t month, uint8_t yearSince2000);

    LoopBack loopBack() const { return m_loopBack; }

private:
    static constexpr uint32_t BAUDRATE = 38400;
    // Time limits to receive a frame (original: one 3000ms limit in var_10 for every frame,
    // checked after each select whose own timeout was 2300ms, myData.4216 + myData.4220 set by fumisOpenSerial)
    // NOTE : the longest measured "select" (wait for the next received bytes) is ~35ms
    // Poll: the stove sends one at least every ~67ms, so 250ms leaves room for about 3 missed Polls
    static constexpr unsigned long POLL_TIMEOUT_MS = 250;
    // echo or Response: the echo is received while the Request is sent, the Response follows it at once (11 bytes: 2.9ms),
    // so 30ms leaves a wide margin for the delay of the ESP in moving received bytes to the Stream buffer
    static constexpr unsigned long FRAME_TIMEOUT_MS = 30;

    void reopen();

    // The retry policy: run op up to attempts times until it gives Ok, whatever the error,
    // the port is closed and reopened before each new attempt
    template <typename Op>
    Result retry(uint8_t attempts, Op op);

    // One exchange with the stove: wait for a Poll, send the Request (Read, Write or SetDateTime),
    // then if response is given, read the stove Response into it
    Result exchange(const FumisFrame &request, FumisFrame *response = nullptr);

    // writeByte and writeWord: wordMode writes the high byte too, expected is compared with the byte or word read back
    Result writeChecked(uint16_t addr, uint16_t value, bool wordMode, uint16_t expected);

    Result acceptResponse(const FumisFrame &frame);
    Result readFrame(FumisFrame &frame);
    Result readResponse(FumisFrame &frame);
    Result readBytes(uint8_t *dst, uint8_t count, unsigned long startTime, unsigned long limitMs);
    Result waitPoll();

    // stream helpers (original: the select and tcflush calls of the SERIALCOM_* layer)
    bool waitAvailable(unsigned long timeoutMs);
    void discardInput();

    Stream *m_port = nullptr;
    OpenFn m_open;
    CloseFn m_close;
    LoopBack m_loopBack = LoopBack::Unknown; // (original: comPortNumber == 0 for Yes, myData.4180) detected on the first successful Read
    // sFumisComData.4 (original: fumisComStatus, a state machine where only "closed" is still tested)
    //   0 closed, 1 open and idle, 2 waiting for a Poll, 3 Poll received, 4 waiting for a Response, 5 Response received
    //   the steps 1..5 are now the order of the code in exchange, waitPoll and readResponse
    bool portOpen = false;
    // CommandResult dword_433248 = CommandResult::OK;      // sFumisComData.8 last error of the serial layer : always set but never get (not reproduced)
    bool rxAligned = false;           // (not in original) a valid stove Response was just read: RX buffer starts at the next stove frame
    unsigned long lastResponseMs = 0; // (not in original) millis() when that Response was read
};

#endif
