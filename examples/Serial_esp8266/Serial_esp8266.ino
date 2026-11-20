#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Palazzetti.h>

SoftwareSerial swSer(D1, D4); // SoftwareSerial is used for log (Serial is used for Stove communication on D7,D8)

Palazzetti myPala;

// Open and close the serial port connected to the stove (the library does the rest through the stream)
void myOpenSerial(uint32_t baudrate)
{
    Serial.begin(baudrate);
    Serial.pins(15, 13); // swap ESP8266 pins to alternative positions (GPIO15 as TX and GPIO13 as RX)
}
void myCloseSerial()
{
    Serial.end();
    // set TX PIN to OUTPUT HIGH to avoid stove bus blocking
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
}

void setup()
{
    swSer.begin(38400);
    swSer.println("swSer started");

    if (myPala.initialize(Serial, myOpenSerial, myCloseSerial) == Palazzetti::CommandResult::OK)
        swSer.println("initialize OK");
    else
        swSer.println("initialize Failed");
}

void loop()
{
    Palazzetti::SetPointData setPointData;
    if (myPala.getSetPoint(setPointData) == Palazzetti::CommandResult::OK)
        swSer.printf("myPala.getSetPoint : SETP=%.2f SECO=%.2f BECO=%d\r\n", setPointData.SETP, setPointData.SECO, setPointData.BECO);
    else
        swSer.println("myPala.getSetPoint failed");

    Palazzetti::AllTempsData temps;
    if (myPala.getAllTemps(temps) == Palazzetti::CommandResult::OK)
        swSer.printf("myPala.getAllTemps : T1=%.2f T2=%.2f T3=%.2f T4=%.2f T5=%.2f\r\n", temps.T1, temps.T2, temps.T3, temps.T4, temps.T5);
    else
        swSer.println("myPala.getAllTemps failed");

    Palazzetti::StatusData status;
    if (myPala.getStatus(status) == Palazzetti::CommandResult::OK)
        swSer.printf("myPala.getStatus : STATUS=%d LSTATUS=%d FSTATUS=%d\r\n", status.STATUS, status.LSTATUS, status.FSTATUS);
    else
        swSer.println("myPala.getStatus failed");

    delay(30000);
}