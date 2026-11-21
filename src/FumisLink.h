#ifndef FumisLink_h
#define FumisLink_h

#include <Arduino.h>
#include "FumisFrame.h"

// Fumis bus (38400 8N1, half-duplex, the stove is the master, see FumisFrame.h): the only code that touches the serial port.
// Each attempt of a Read, Write or SetDateTime is one exchange: wait for a stove Poll, send the Request, read the Response.
// Every error is retried. The V1 board loopback is detected on the first successful Read.
// (original: the SERIALCOM_* layer and the frame and retry parts of the fumis* functions, see FumisLink.cpp)
class FumisLink
{
public:
    // the port must be opened by the application at BAUDRATE, 8N1
    static constexpr uint32_t BAUDRATE = 38400;

    // RX receives back what TX sends: Yes (V1 board) or No (V2 board), Unknown until the first successful Read
    enum class LoopBack : uint8_t
    {
        Unknown,
        Yes,
        No
    };

    // mapped to Palazzetti::CommandResult by its callers
    enum class Result : uint8_t
    {
        Ok,
        Timeout,   // frame not complete in time (Poll: POLL_TIMEOUT_MS, echo or Response: FRAME_TIMEOUT_MS)
        BadFrame,  // Response with a wrong type or checksum
        Collision, // V1 board: the echo differs from the Request, something else was on the bus
        Mismatch,  // a write read back with another value than expected
        Error,     // the Request could not be written to the stream
    };

    // the stream connected to the stove bus, already opened
    void setSerial(Stream &port) { _port = &port; }
    bool hasSerial() const { return _port != nullptr; }

    // Read the 8 bytes at addr (zeroed on failure), up to 3 attempts
    Result read(uint16_t addr, uint8_t (&data)[FumisFrame::DATA_SIZE]);
    // read, then keep the byte or the word (little endian), value is unchanged on failure
    // (readByte gives a uint16_t too, to suit the variables of Palazzetti)
    Result readByte(uint16_t addr, uint16_t &value);
    Result readWord(uint16_t addr, uint16_t &value);

    // Write, then read back: Mismatch if the value read is not the expected one, up to 2 attempts
    // writeByte: expected differs from value for a register the stove changes at once (e.g. a command it clears once run)
    // writeWord: low byte at addr, then high byte at addr + 1
    Result writeByte(uint16_t addr, uint8_t value, uint8_t expected);
    Result writeWord(uint16_t addr, uint16_t value);

    // weekDay: 1 = Monday .. 7 = Sunday, up to 3 attempts
    Result setDateTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t weekDay, uint8_t day, uint8_t month, uint8_t yearSince2000);

    LoopBack loopBack() const { return _loopBack; }

private:
    // frame time limits (original: 3000ms for every frame in var_10, checked after each select of 2300ms, myData.4216 + myData.4220)
    // NOTE : the longest measured "select" is ~35ms
    // Poll: the stove sends one at least every ~67ms
    static constexpr unsigned long POLL_TIMEOUT_MS = 250;
    // echo or Response: received during the Request or right after it (11 bytes: 2.9ms), plus the ESP delay to make them available
    static constexpr unsigned long FRAME_TIMEOUT_MS = 30;

    template <typename Op>
    Result retry(uint8_t attempts, Op op);
    Result exchange(const FumisFrame &request, FumisFrame *response = nullptr);
    Result writeChecked(uint16_t addr, uint16_t value, bool wordMode, uint16_t expected);

    Result acceptResponse(const FumisFrame &frame);
    Result readFrame(FumisFrame &frame);
    Result readResponse(FumisFrame &frame);
    Result readBytes(uint8_t *dst, uint8_t count, unsigned long startTime, unsigned long limitMs);
    Result waitPoll();
    bool waitAvailable(unsigned long timeoutMs);
    void discardInput();

    Stream *_port = nullptr;
    LoopBack _loopBack = LoopBack::Unknown; // (original: comPortNumber == 0 for Yes, myData.4180)
    // sFumisComData.4 (original: fumisComStatus, a state machine where only "closed" was tested): not reproduced
    //   0 closed: the port is opened once by the application and never closed
    //   1 open and idle, 2 waiting for a Poll, 3 Poll received, 4 waiting for a Response, 5 Response received:
    //   now the order of the code in exchange, waitPoll and readResponse
    // CommandResult dword_433248 = CommandResult::OK;      // sFumisComData.8 last error of the serial layer : always set but never get (not reproduced)
    bool _rxAligned = false;           // (not in original) a valid Response was just read: the next RX byte starts a stove frame
    unsigned long _lastResponseMs = 0; // (not in original) millis() of that Response
};

#endif
