// This #include statement was automatically added by the Particle IDE.
#include "MQTT/MQTT.h"

#include "SimpleFIFO.h"

//TODO: Add buttons via interupt and send them to a specific measurement.

#define MIN_CM 2.0
#define MAX_CM 400.0

//Distance
int distanceMeasureInterval = 100;   //ms between each measurement
#define distanceBufferSize 100         //samples to buffer before we overwrite

void receiveMQTT(char* topic, byte* payload, unsigned int length);

// Uncomment this if you want the device to only run application code, e.g. out in the wild!
// To reflash the firmware, set the device in safe mode:
// https://docs.particle.io/guide/getting-started/modes/photon/#safe-mode

// SYSTEM_MODE(MANUAL);

/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT mqtt(server, 1883, callback);
 * want to use domain name,
 * MQTT mqtt("www.sample.com", 1883, callback);
 **/
MQTT mqtt("op-en.se", 1883, receiveMQTT);

int trigPin = D4;
int echoPin = D5;

long startTime = 0;
long endTime = 0;

typedef struct {
    uint32_t millis;
    uint64_t unix;
} offsetTime;
offsetTime offset = { 0, 0 };

typedef struct {
    uint64_t stamp;
    int16_t value;
} smallSample;

char messageOutBuffer[200];

SimpleFIFO<smallSample, distanceBufferSize> distanceBuffer;

Timer trigTimer(distanceMeasureInterval, trigger_ultrasound);
Timer untrigTimer(1, untrigger_ultrasound, true); //one-shot timer
Timer syncClock(1000, syncTime);

String deviceId;

void setup()
{
    deviceId = "happiness_" + System.deviceID();
    syncTime();
    Serial.begin(9600);
    pinMode(trigPin,OUTPUT);
    pinMode(echoPin,INPUT);
    attachInterrupt(echoPin, echoISR, CHANGE);
    trigTimer.start();
    syncClock.start();
    // connect to the server
    RGB.control(true);
    RGB.color(255, 0, 0);
    mqtt.connect(deviceId);
    // mqtt.subscribe("/trafic_sensor_" + System.deviceID() + "/control/*");
    RGB.color(0, 255, 0);
}

void syncTime(){
    offset.unix = (uint64_t)Time.now() * 1000;
    offset.millis = millis();
}

String printuint64(uint64_t num )
{
    const static char toAscii[] = "0123456789";

    char buffer[65];            //because you might be doing binary
    char* p = &buffer[64];      //this pointer writes into the buffer, starting at the END

    // zero to terminate a C type string
    *p = 0;

    // do digits until the number reaches zero
    do
    {
        // working on the least significant digit
        //put an ASCII digit at the front of the string
        *(--p) = toAscii[(int)(num % 10)];

        //knock the least significant digit off the number
        num = num / 10;
    } while (num != 0);

    // Serial.println(p);
    return String(p);
}

uint64_t currentTime(){
    uint32_t millisSinceSync = millis() - offset.millis;
    return offset.unix + millisSinceSync;
}

void trigger_ultrasound()
{
    digitalWriteFast(trigPin, HIGH);
    untrigTimer.start();
}

void untrigger_ultrasound()
{
    digitalWriteFast(trigPin, LOW);
}

// recieve message
void receiveMQTT(char* topic, byte* payload, unsigned int length) {
    // char p[length + 1];
    // memcpy(p, payload, length);
    // p[length] = NULL;
    // String message(p);

    // if(topic == "soundMeasureInterval"){
    //     soundMeasureInterval = message.toInt();
    // }else if(topic == "gasMeasureInterval"){
    //     gasMeasureInterval = message.toInt();
    // }else if(topic == "distanceMeasureInterval"){
    //     distanceMeasureInterval = message.toInt();
    // }
}

void loop()
{
    while(distanceBuffer.count() > 0){
        smallSample distance = distanceBuffer.dequeue();
        String firstPart = "{ \"time\": ";
        String secondPart = printuint64(distance.stamp);
        sprintf (messageOutBuffer, ", \"value\": %d }", distance.value);
        if(Serial.available()){
            Serial.println(firstPart + secondPart + String(messageOutBuffer));
        }
        mqtt.publish("test/uppsalaWorkshop/" + deviceId + "/distance", firstPart + secondPart + String(messageOutBuffer));
    }
}

/*
 *
 *
 * Ultrasonic ranging module HC - SR04
 *
 * provides 2cm - 400cm non-contact measurement function, the ranging accuracy can reach to 3mm.
 * The modules includes ultrasonic transmitters, receiver and control circuit.
 *
 * The basic principle of work:
 *
 * (1) Using IO trigger for at least 10us high level signal,
 * (2) The Module automatically sends eight 40 kHz and detect whether there is a pulse signal back.
 * (3) IF the signal back, through high level , time of high output IO duration is the time from
 *     sending ultrasonic to returning.
 *
 *     Test distance = high level time × velocity of sound (340M/S) / 2
 *
 */

void echoISR(){
    if(pinReadFast(echoPin) == HIGH){
        startTime = micros();
    }else{
        endTime = micros();
        long duration = endTime - startTime;
        float distance = duration / 29.41176 / 2.0;
        int16_t mm;
        if (distance < MIN_CM || distance > MAX_CM){
            mm = -1.0;
        }else{
            mm = (uint16_t)(distance * 10);
        }
        smallSample reading = { currentTime(), mm };
        distanceBuffer.enqueue(reading);
    }
}
