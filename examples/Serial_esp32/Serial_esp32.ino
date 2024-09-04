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
size_t myReadSerial(void *buf, size_t count) { return Serial2.read((char *)buf, count); }
size_t myWriteSerial(const void *buf, size_t count) { return Serial2.write((const uint8_t *)buf, count); }
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
    if (myPala.initialize(myOpenSerial, myCloseSerial, mySelectSerial, myReadSerial, myWriteSerial, myDrainSerial, myFlushSerial, myUSleep) == Palazzetti::CommandResult::OK)
        Serial.println("initialize OK");
    else
        Serial.println("initialize Failed");
}

void loop()
{
    float setPoint;
    if (myPala.getSetPoint(&setPoint) == Palazzetti::CommandResult::OK)
        Serial.printf("myPala.getSetPoint : %.2f\r\n", setPoint);
    else
        Serial.println("myPala.getSetPoint failed");

    float T1, T2, T3, T4, T5;
    if (myPala.getAllTemps(&T1, &T2, &T3, &T4, &T5) == Palazzetti::CommandResult::OK)
        Serial.printf("myPala.getAllTemps : T1=%.2f T2=%.2f T3=%.2f T4=%.2f T5=%.2f\r\n", T1, T2, T3, T4, T5);
    else
        Serial.println("myPala.getAllTemps failed");

    uint16_t STATUS, LSTATUS, FSTATUS;
    if (myPala.getStatus(&STATUS, &LSTATUS, &FSTATUS) == Palazzetti::CommandResult::OK)
        Serial.printf("myPala.GetStatus : STATUS=%d LSTATUS=%d FSTATUS=%d\r\n", STATUS, LSTATUS, FSTATUS);
    else
        Serial.println("myPala.GetStatus failed");

    delay(30000);
}