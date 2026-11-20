#ifndef FumisLink_h
#define FumisLink_h

#include <Arduino.h>
#include "FumisFrame.h"

// UART side of the Fumis bus (38400 8N1, half-duplex polled bus, the stove is the master, see FumisFrame.h):
// the only code that touches the serial port
//  - finds the moment the stove listens (after its Poll), sends one Request and reads the Response
//  - handles the V1 board loopback (RX receives back what TX sends), detected on the first successful Read
// (original: the SERIALCOM_* layer and the frame part of the fumis* functions, see FumisLink.cpp)
class FumisLink
{
public:
    using OpenSerialFn = std::function<int(uint32_t baudrate)>;
    using CloseSerialFn = std::function<void()>;
    using SelectSerialFn = std::function<int(unsigned long timeout)>;
    using ReadSerialFn = std::function<ssize_t(void *buf, size_t count)>;
    using WriteSerialFn = std::function<ssize_t(const void *buf, size_t count)>;
    using DrainSerialFn = std::function<int()>;
    using FlushSerialFn = std::function<int()>;
    using USleepFn = std::function<void(unsigned long usec)>;

    struct SerialAdapter
    {
        // Open a Serial. Returns 0 on success, -1 on error.
        OpenSerialFn open;
        // Close Serial.
        CloseSerialFn close;
        // Returns 1 if data available, 0 if none, -1 on error.
        SelectSerialFn select;
        // Returns bytes read (>= 0) or -1 on error.
        ReadSerialFn read;
        // Returns bytes written (>= 0) or -1 on error.
        WriteSerialFn write;
        // Wait for transmission. Returns 0 on success, -1 on error.
        DrainSerialFn drain;
        // Flush pending I/O. Returns 0 on success, -1 on error.
        FlushSerialFn flush;
        // Suspend execution for usec microseconds.
        USleepFn uSleep;
    };

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
        CommunicationError, // timeout, invalid frame, read error or echo mismatch: worth retrying
        Error,              // port closed, cannot be (re)opened, or write error
    };

    void setSerial(const SerialAdapter &serial) { m_serial = serial; }
    // true if every function of the serial adapter is set
    bool hasSerial() const;

    Result open();
    void close();
    Result reopen();
    // suspend execution for usec microseconds (through the serial adapter)
    void uSleep(unsigned long usec) { m_serial.uSleep(usec); }

    // One exchange with the stove: wait for a Poll, send the Request (Read, Write or SetDateTime),
    // then if response is given, read the stove Response into it
    Result exchange(const FumisFrame &request, FumisFrame *response = nullptr);

    LoopBack loopBack() const { return m_loopBack; }

private:
    static constexpr uint32_t BAUDRATE = 38400;

    Result acceptResponse(const FumisFrame &frame);
    Result readFrame(FumisFrame &frame);
    Result readResponse(FumisFrame &frame);
    Result receive(FumisFrame &frame, uint8_t &bufPosition, unsigned long startTime);
    Result waitPoll();

    SerialAdapter m_serial;
    LoopBack m_loopBack = LoopBack::Unknown; // (original: comPortNumber == 0 for Yes, myData.4180) detected on the first successful Read
    // sFumisComData.4 (original: fumisComStatus, a state machine where only "closed" is still tested)
    //   0 closed, 1 open and idle, 2 waiting for a Poll, 3 Poll received, 4 waiting for a Response, 5 Response received
    //   the steps 1..5 are now the order of the code in exchange, waitPoll and readResponse
    bool portOpen = false;
    // CommandResult dword_433248 = CommandResult::OK;      // sFumisComData.8 last error of the serial layer : always set but never get (not reproduced)
    unsigned long selectSerialTimeoutMs = 100; // myData.4216 + myData.4220 (timeval type originially), original: set by fumisOpenSerial
                                               // NOTE : the original value is 2300 but the longest measured "select" is ~35ms
    bool rxAligned = false;                    // (not in original) a valid stove Response was just read: RX buffer starts at the next stove frame
    unsigned long lastResponseMs = 0;          // (not in original) millis() when that Response was read
};

#endif
