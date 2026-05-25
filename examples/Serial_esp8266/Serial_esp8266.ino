#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Palazzetti.h>

SoftwareSerial swSer(D1, D4); // SoftwareSerial is used for log (Serial is used for Stove communication on D7,D8)

Palazzetti myPala;

// Serial management functions
int myOpenSerial(uint32_t baudrate)
{
    Serial.begin(baudrate);
    Serial.pins(15, 13); // swap ESP8266 pins to alternative positions (GPIO15 as TX and GPIO13 as RX)
    return 0;
}
void myCloseSerial()
{
    Serial.end();
    // set TX PIN to OUTPUT HIGH to avoid stove bus blocking
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
}
int mySelectSerial(unsigned long timeout)
{
    size_t avail;
    unsigned long startmillis = millis();
    while ((avail = Serial.available()) == 0 && (startmillis + timeout) > millis())
        ;

    return avail;
}
ssize_t myReadSerial(void *buf, size_t count) { return Serial.read((char *)buf, count); }
ssize_t myWriteSerial(const void *buf, size_t count) { return Serial.write((const uint8_t *)buf, count); }
int myDrainSerial()
{
    Serial.flush(); // On ESP, Serial.flush() is drain
    return 0;
}
int myFlushSerial()
{
    Serial.flush();
    while (Serial.read() != -1)
        ; // flush RX buffer
    return 0;
}
void myUSleep(unsigned long usecond) { delayMicroseconds(usecond); }

void setup()
{
    swSer.begin(38400);
    swSer.println("swSer started");

    Palazzetti::SerialAdapter serial = {
        .open = myOpenSerial,
        .close = myCloseSerial,
        .select = mySelectSerial,
        .read = myReadSerial,
        .write = myWriteSerial,
        .drain = myDrainSerial,
        .flush = myFlushSerial,
        .uSleep = myUSleep};

    if (myPala.initialize(serial) == Palazzetti::CommandResult::OK)
        swSer.println("initialize OK");
    else
        swSer.println("initialize Failed");
}

void loop()
{
    float setPoint;
    if (myPala.getSetPoint(&setPoint) == Palazzetti::CommandResult::OK)
        swSer.printf("myPala.getSetPoint : %.2f\r\n", setPoint);
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