#ifndef FumisLink_h
#define FumisLink_h

#include <Arduino.h>
#include "FumisFrame.h"

// Fumis bus (38400 8N1, half-duplex polled bus, the stove is the master, see FumisFrame.h):
// the only code that touches the serial port
//  - Read, Write and SetDateTime of stove memory, with their retries
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

    // the results the serial layer can give (mapped to Palazzetti::CommandResult by its callers)
    enum class Result : uint8_t
    {
        Ok,
        CommunicationError, // timeout, invalid frame or echo mismatch: worth retrying
        Error,              // port closed, or write error
    };

    // the stream to talk through, and the functions that open and close its serial port
    void setSerial(Stream &port, OpenFn open, CloseFn close);
    // true if the stream and both functions are set
    bool hasSerial() const { return m_port && m_open && m_close; }

    void open();
    void close();

    // Read the 8 bytes at addr into data (zeroed on failure)
    // up to 4 attempts, the port is reopened before each new attempt; only a CommunicationError is retried
    Result read(uint16_t addr, uint8_t (&data)[FumisFrame::DATA_SIZE]);

    // Write 1 byte at addr, one attempt
    // (the retry belongs to the caller: it covers the read-back check, which is about what the stove does with the value)
    Result write(uint16_t addr, uint8_t value);

    // Set the stove date and time (weekDay: 1 = Monday .. 7 = Sunday)
    // up to 3 attempts, any error is retried
    Result setDateTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t weekDay, uint8_t day, uint8_t month, uint8_t yearSince2000);

    LoopBack loopBack() const { return m_loopBack; }

private:
    static constexpr uint32_t BAUDRATE = 38400;
    // myData.4216 + myData.4220 (timeval type originally), original: set by fumisOpenSerial
    // NOTE : the original value is 2300 but the longest measured "select" is ~35ms
    static constexpr unsigned long SELECT_TIMEOUT_MS = 100;

    void reopen();

    // The retry policy: run op up to attempts times until it gives Ok
    //  - reopenBetween: close and reopen the port before each new attempt
    //  - onlyCommunicationError: stop at once on any other error
    template <typename Op>
    Result retry(uint8_t attempts, bool reopenBetween, bool onlyCommunicationError, Op op);

    // One exchange with the stove: wait for a Poll, send the Request (Read, Write or SetDateTime),
    // then if response is given, read the stove Response into it
    Result exchange(const FumisFrame &request, FumisFrame *response = nullptr);

    Result acceptResponse(const FumisFrame &frame);
    Result readFrame(FumisFrame &frame);
    Result readResponse(FumisFrame &frame);
    Result receive(FumisFrame &frame, uint8_t &bufPosition, unsigned long startTime);
    Result waitPoll();

    // stream helpers (original: the select, read, drain and flush calls of the SERIALCOM_* layer)
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
