#include <Arduino.h>
#include <Palazzetti.h>

// Serial is used for log (baudrate=38400)
// Serial2 is used for stove communication

Palazzetti myPala;

// Open and close the serial port connected to the stove (the library does the rest through the stream)
void myOpenSerial(uint32_t baudrate)
{
    Serial2.begin(baudrate, SERIAL_8N1, 23, 5); // set ESP32 pins to match hat position (IO23(RX)/IO5(TX))
}
void myCloseSerial()
{
    Serial2.end();
    // set TX PIN to OUTPUT HIGH to avoid stove bus blocking
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);
}

void setup()
{
    Serial.begin(38400);
    Serial.println("Serial started");

    if (myPala.initialize(Serial2, myOpenSerial, myCloseSerial) == Palazzetti::CommandResult::OK)
        Serial.println("initialize OK");
    else
        Serial.println("initialize Failed");
}

void loop()
{
    Palazzetti::SetPointData setPointData;
    if (myPala.getSetPoint(setPointData) == Palazzetti::CommandResult::OK)
        Serial.printf("myPala.getSetPoint : SETP=%.2f SECO=%.2f BECO=%d\r\n", setPointData.SETP, setPointData.SECO, setPointData.BECO);
    else
        Serial.println("myPala.getSetPoint failed");

    Palazzetti::AllTempsData temps;
    if (myPala.getAllTemps(temps) == Palazzetti::CommandResult::OK)
        Serial.printf("myPala.getAllTemps : T1=%.2f T2=%.2f T3=%.2f T4=%.2f T5=%.2f\r\n", temps.T1, temps.T2, temps.T3, temps.T4, temps.T5);
    else
        Serial.println("myPala.getAllTemps failed");

    Palazzetti::StatusData status;
    if (myPala.getStatus(status) == Palazzetti::CommandResult::OK)
        Serial.printf("myPala.getStatus : STATUS=%d LSTATUS=%d FSTATUS=%d\r\n", status.STATUS, status.LSTATUS, status.FSTATUS);
    else
        Serial.println("myPala.getStatus failed");

    delay(30000);
}