#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <time.h>

#define TRIG_PIN        12
#define ECHO_PIN        14
#define SOUND_SPEED     0.034
#define AP_SSID         "ESP8266-AP"
#define AP_PASSWORD     "TestPassword"

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);

String speed = "0";
long duration;
float distanceCM = 0, curDistance = 0, lastDistance = 0;

float speed_var = 0;
unsigned short counter = 0;

bool isCounting = false;
long int start, end;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

float getDistance(){
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH, 12000);  // set timeout to 12 miliseconds
    if(!duration){  // in case of timeout
        duration = getDistance();
    }
    distanceCM = (duration * SOUND_SPEED) / 2; 

    return distanceCM;
}

void getSpeed(){
    counter++;
    float temp_speed = 0;
    if(!isCounting){
        start = millis();
        isCounting = true;
    }

    end = millis();        
    lastDistance = curDistance;
    curDistance = getDistance();

    start = millis();
    temp_speed = abs(((curDistance - lastDistance) / ((end - start) / 1000.0))/ 27.777777777778);    // converting from cm/s to km/h

    printf("temp_speed = %f\n", temp_speed);

    if(temp_speed < 100){
        speed_var += temp_speed;
    }
    
    if(counter == 4){
        speed_var /= 4;
        speed = String(speed_var);
        speed_var = 0;
        counter = 0;
    }
}

void setup() {
    Serial.begin(9600);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    // If you want to connect to a WiFi network instead
    // WiFi.begin(LSSID, LPASSWORD);           
    // while (WiFi.status() != WL_CONNECTED) {
    //     delay(500);
    //     Serial.print(".");
    // }

    // If you want to create a Access Point
    WiFi.softAP(AP_SSID,AP_PASSWORD);          
    WiFi.softAPConfig(local_ip, gateway, subnet);

    delay(50);

    Serial.print("IP Address: ");           
    Serial.println(WiFi.localIP());

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

    printf("Distance (cm): %f\n",distanceCM);
}
