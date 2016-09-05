// This #include statement was automatically added by the Particle IDE.
#include "MQTT/MQTT.h"

#include "SimpleFIFO.h"


//Moisture
int moistMeasureInterval = 100;   //ms between each measurement
#define moistBufferSize 100         //samples to buffer before we overwrite

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

int moistPin = A1;

typedef struct {
    uint32_t millis;
    uint64_t unix;
} offsetTime;
offsetTime offset = { 0, 0 };

typedef struct {
    uint64_t stamp;
    float value;
} floatSample;

char messageOutBuffer[200];

SimpleFIFO<floatSample, moistBufferSize> moistBuffer;

Timer moistTimer(moistMeasureInterval, readMoisture);
Timer syncClock(1000, syncTime);

String deviceId;

void setup()
{
    deviceId = "moist_" + System.deviceID();
    syncTime();
    Serial.begin(9600);
    // pinMode(moistPin,INPUT); //Don't set on analog read
    moistTimer.start();
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

void readMoisture(){
    float value = analogRead(moistPin) / 4096.0;
    floatSample reading = { currentTime(), value };
    moistBuffer.enqueue(reading);
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
    while(moistBuffer.count() > 0){
        floatSample moist = moistBuffer.dequeue();
        String firstPart = "{ \"time\": ";
        String secondPart = printuint64(moist.stamp);
        sprintf (messageOutBuffer, ", \"value\": %f }", moist.value);
        if(Serial.available()){
            Serial.println(firstPart + secondPart + String(messageOutBuffer));
        }
        mqtt.publish("test/uppsalaWorkshop/" + deviceId + "/moisture", firstPart + secondPart + String(messageOutBuffer));
    }
}
