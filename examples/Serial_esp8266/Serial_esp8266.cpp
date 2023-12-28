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
    // TX/GPIO15 is pulled down and so block the stove bus by default...
    pinMode(15, OUTPUT); // set TX PIN to OUTPUT HIGH
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
size_t myReadSerial(void *buf, size_t count) { return Serial.read((char *)buf, count); }
size_t myWriteSerial(const void *buf, size_t count) { return Serial.write((const uint8_t *)buf, count); }
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
    swSer.println("Start Of swSer");
    if (myPala.initialize(myOpenSerial, myCloseSerial, mySelectSerial, myReadSerial, myWriteSerial, myDrainSerial, myFlushSerial, myUSleep))
        swSer.println("initialize OK");
    else
        swSer.println("initialize Failed");
}

void loop()
{
    float setPoint;
    if (myPala.getSetPoint(&setPoint))
        swSer.printf("myPala.getSetPoint : %.2f\r\n", setPoint);
    else
        swSer.println("myPala.getSetPoint failed");

    float T1, T2, T3, T4, T5;
    if (myPala.getAllTemps(&T1, &T2, &T3, &T4, &T5))
        swSer.printf("myPala.getAllTemps : T1=%.2f T2=%.2f T3=%.2f T4=%.2f T5=%.2f\r\n", T1, T2, T3, T4, T5);
    else
        swSer.println("myPala.getAllTemps failed");

    uint16_t STATUS, LSTATUS;
    if (myPala.getStatus(&STATUS, &LSTATUS))
        swSer.printf("myPala.GetStatus : STATUS=%d LSTATUS=%d\r\n", STATUS, LSTATUS);
    else
        swSer.println("myPala.GetStatus failed");

    delay(30000);
}