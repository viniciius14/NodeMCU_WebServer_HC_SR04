#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <string>

#define TRIG_PIN    12
#define ECHO_PIN    14
#define SOUND_SPEED 0.034
#define AP_SSID        "ESP8266-AP"
#define AP_PASSWORD    "TestPassword"

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);

String speed = "0";
long duration;
float distanceCm = 0, speedKmph = 0, curDistance = 0, lastDistance = 0;
bool isCounting = false;
long int start, end;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

float getDistance(){
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    // Sets the TRIG_PIN on HIGH state for 10 micro seconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
    duration = pulseIn(ECHO_PIN, HIGH);
    distanceCm = duration * SOUND_SPEED/2;

    return distanceCm;
}

void getSpeed(){
    if(!isCounting){
        start = millis();
        isCounting = true;
    }
    else{
        end = millis();        
        lastDistance = curDistance;
        curDistance = getDistance();
        // Serial.printf("curDistance: %f lastDistance: %f end: %ld start: %ld\n", curDistance, lastDistance, end, start);
        speedKmph = (curDistance - lastDistance) / ((end - start) / 1000.0 );
        if(curDistance >= 2000){    //ignore reading
            curDistance = lastDistance;
        }
        else{
            start = millis();
            if(speedKmph < 1 && speedKmph > -1){
                speed = 0;
            }
            else{
                if(speedKmph > 100 || speedKmph < -100){}
                else{
                    speedKmph > 0 ? speedKmph : speedKmph = speedKmph * -1;
                    speed = String(speedKmph);
                }
            }
        }
    }
}

void setup() {
    Serial.begin(9600);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    // WiFi.begin(LSSID, LPASSWORD);           // Connect to a WiFi
    // while (WiFi.status() != WL_CONNECTED) {
    //     delay(500);
    //     Serial.print(".");
    // }

    WiFi.softAP(AP_SSID,AP_PASSWORD);          
    WiFi.softAPConfig(local_ip, gateway, subnet);

    delay(50);

    Serial.print("IP Address: ");           
    Serial.println(WiFi.localIP());

    getSpeed();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Number Display</title><style>html,body{width:100%;height:100%;margin:0;padding:0;box-sizing:border-box}.container{display:flex;flex-direction:column;justify-content:center;align-items:center;height:100%}.speed{font-family:sans-serif;font-size:12vw;font-weight:bold;max-width:5em;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.unit{font-family:sans-serif;font-size:6vw;text-transform:uppercase;margin-top:10px}</style></head><body><div class=\"container\"><text class=\"speed\"x=\"50\"y=\"50\"text-anchor=\"middle\" dominant-baseline=\"central\">" + speed + "</text><div class=\"unit\">km/h</div></div></body></html>";

        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
        response->addHeader("Cache-Control", "no-cache");// Add Cache-Control header
        response->addHeader("Refresh", "1"); // Add Refresh header with a value of 1 second
        request->send(response);
    });
    server.onNotFound(notFound);
    Serial.println("Starting server.");
    server.begin();
}

void loop() {
    getSpeed();
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
    delay(750);
}
