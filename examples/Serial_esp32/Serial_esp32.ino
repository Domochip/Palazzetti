#include <Arduino.h>
#include <Palazzetti.h>

// Serial is used for log (baudrate=38400)
// Serial2 is used for stove communication

Palazzetti myPala;

// Serial management functions
int myOpenSerial(uint32_t baudrate)
{
    Serial2.begin(baudrate, SERIAL_8N1, 23, 5); // set ESP32 pins to match hat position (IO23(RX)/IO5(TX))
    return 0;
}
void myCloseSerial()
{
    Serial2.end();
    // set TX PIN to OUTPUT HIGH to avoid stove bus blocking
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);
}
int mySelectSerial(unsigned long timeout)
{
    size_t avail;
    unsigned long startmillis = millis();
    while ((avail = Serial2.available()) == 0 && (startmillis + timeout) > millis())
        ;

    return avail;
}
ssize_t myReadSerial(void *buf, size_t count) { return Serial2.read((char *)buf, count); }
ssize_t myWriteSerial(const void *buf, size_t count) { return Serial2.write((const uint8_t *)buf, count); }
int myDrainSerial()
{
    Serial2.flush(); // On ESP, Serial2.flush() is drain
    return 0;
}
int myFlushSerial()
{
    Serial2.flush();
    while (Serial2.read() != -1)
        ; // flush RX buffer
    return 0;
}
void myUSleep(unsigned long usecond) { delayMicroseconds(usecond); }

void setup()
{
    Serial.begin(38400);
    Serial.println("Serial started");

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