# Palazzetti

Arduino library to communicate with Fumis/Palazzetti stoves (Fumis Alpha 65 motherboard) on ESP8266 and ESP32.  
It reads status, temperatures, setpoint, fan speeds, counters, etc. and can power the stove on/off, change the setpoint, fan speeds, chrono program, etc.

## Usage

The application gives a `Stream` connected to the stove bus (38400 8N1) and two board specific functions: one that opens the serial port (pins, UART swap...) and one that closes it (drive TX HIGH so the stove bus is not blocked).  
The library does the rest: it waits for the stove Poll, sends the request and reads the response, with retries.

```cpp
#include <Palazzetti.h>

Palazzetti myPala;

// ESP8266: Serial is used for the stove, on its alternative pins
void myOpenSerial(uint32_t baudrate)
{
    Serial.begin(baudrate);
    Serial.pins(15, 13); // TX on GPIO15 (D8), RX on GPIO13 (D7)
}

void myCloseSerial()
{
    Serial.end();
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH); // keep TX HIGH to avoid blocking the stove bus
}

void setup()
{
    if (myPala.initialize(Serial, myOpenSerial, myCloseSerial) == Palazzetti::CommandResult::OK)
    {
        Palazzetti::StatusData status;
        myPala.getStatus(status);
    }
}
```

Every function returns a `Palazzetti::CommandResult`: `OK`, `COMMUNICATION_ERROR` (no answer or invalid frame from the stove) or `ERROR`.

Some adapters loop TX back to RX, so the requests sent are received back. The library detects it automatically on the first exchange with the stove and handles it, nothing has to be configured. `getLoopBack()` gives the result: `Yes`, `No`, or `Unknown` until the stove has answered.

See `examples/Serial_esp8266` and `examples/Serial_esp32` for complete sketches.

# Hardware

The hardware part of this project is in the WPalaControl repository: schematics and PCB to build the adapter that connects to the stove.  
https://github.com/Domochip/WPalaControl
