#ifndef FumisFrame_h
#define FumisFrame_h

#include <stdint.h>
#include <string.h>

// Fumis bus frame: 38400 8N1, half-duplex polled bus, the stove is the master
//   the stove sends a Poll frame, then the box may send one Request frame (Read, Write or SetDateTime)
//   a Read is answered by a Response frame right after it
// frame (11 bytes):
//   byte 0      type (Type)
//   bytes 1..8  data (Response: 8 data bytes ; Read/Write: address lo, hi ; Write: + value ; SetDateTime: s, m, h, weekday, d, M, y-2000)
//   byte 9      not used by this library (0 in frames sent, ignored in frames received)
//   byte 10     checksum = sum of bytes 0..9
// No Arduino dependency, so it can be unit tested on a PC
struct FumisFrame
{
    static constexpr uint8_t SIZE = 11;
    static constexpr uint8_t CHECKSUM_POS = SIZE - 1;
    static constexpr uint8_t DATA_SIZE = 8; // data bytes of a Response frame (bytes 1..8)

    // Frame type (byte 0 of a frame)
    enum class Type : uint8_t
    {
        Poll = 0x00,        // stove -> box : the stove listens, the box may send one Request (plzbridge: "Request" in fumisWaitRequest)
        Write = 0x01,       // box -> stove Request : write 1 byte (lo, hi, value)
        Read = 0x02,        // box -> stove Request : read 8 bytes (lo, hi)
        Response = 0x02,    // stove -> box : response to a Read (8 data bytes)
        SetDateTime = 0x06, // box -> stove Request : set date and time
    };

    uint8_t bytes[SIZE] = {};

    Type type() const { return static_cast<Type>(bytes[0]); }
    // the 8 data bytes (bytes 1..8)
    const uint8_t *data() const { return bytes + 1; }

    // the word at bytes[i], low byte first (the stove byte order)
    // (not named word: Arduino defines a word() macro)
    static uint16_t wordAt(const uint8_t *bytes, uint8_t i) { return bytes[i] | (bytes[i + 1] << 8); }

    // sum of bytes 0..9, the value expected in byte 10 (original: iChkSum, var_10)
    uint8_t checksum() const
    {
        uint8_t chk = 0;
        for (uint8_t i = 0; i < CHECKSUM_POS; i++)
            chk += bytes[i];
        return chk;
    }
    // true if the checksum (byte 10) matches bytes 0..9 (original: iChkSum, returning 0 or -508)
    bool hasValidChecksum() const { return checksum() == bytes[CHECKSUM_POS]; }
    // true if the frame has this type and a valid checksum (original: parseRxBuffer, which also advanced the state of the exchange)
    bool isValid(Type t) const { return type() == t && hasValidChecksum(); }

    bool operator==(const FumisFrame &other) const { return memcmp(bytes, other.bytes, SIZE) == 0; }
    bool operator!=(const FumisFrame &other) const { return !(*this == other); }

    // a frame to send: type, data bytes 1..7 (others are 0) and checksum
    static FumisFrame make(Type t, uint8_t b1 = 0, uint8_t b2 = 0, uint8_t b3 = 0, uint8_t b4 = 0, uint8_t b5 = 0, uint8_t b6 = 0, uint8_t b7 = 0)
    {
        FumisFrame f;
        f.bytes[0] = static_cast<uint8_t>(t);
        f.bytes[1] = b1;
        f.bytes[2] = b2;
        f.bytes[3] = b3;
        f.bytes[4] = b4;
        f.bytes[5] = b5;
        f.bytes[6] = b6;
        f.bytes[7] = b7;
        f.bytes[CHECKSUM_POS] = f.checksum();
        return f;
    }
    // Read Request: read the 8 bytes at addr
    static FumisFrame read(uint16_t addr) { return make(Type::Read, addr & 0xFF, addr >> 8); }
    // Write Request: write 1 byte at addr
    static FumisFrame write(uint16_t addr, uint8_t value) { return make(Type::Write, addr & 0xFF, addr >> 8, value); }
    // SetDateTime Request (weekDay: 1 = Monday .. 7 = Sunday)
    static FumisFrame setDateTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t weekDay, uint8_t day, uint8_t month, uint8_t yearSince2000)
    {
        return make(Type::SetDateTime, second, minute, hour, weekDay, day, month, yearSince2000);
    }
};

#endif
