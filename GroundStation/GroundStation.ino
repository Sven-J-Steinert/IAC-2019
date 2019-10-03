#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h>

#ifndef STASSID
#define STASSID "PiFi.internal"
#define STAPSK  "letmeaccessyourdata"
#endif

const char* host = "esp8266-webupdate";
const char* ssid = STASSID;
const char* password = STAPSK;

#define IR_PIN 12
#define Servo_PIN 13

Servo myservo;  // create servo object to control a servo
int pos = 170;    // variable to store the servo position
byte x;
int i=0,t=0;
bool ir=false;

ESP8266WebServer server(80);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><p>Groundstation esp8266-webupdate</p><input type='file' name='update'><input type='submit' value='Update'></form>";

void setup(void) {
  pinMode(IR_PIN, INPUT);
  myservo.attach(Servo_PIN); 
  myservo.write(pos);
  
  Serial.begin(115200);  Serial.println();  Serial.println("Booting Sketch...");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    MDNS.begin(host);
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
    server.begin();
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  } else {
    Serial.println("WiFi Failed");
  }
}

void loop(void) {
   x = digitalRead(12);
  if (x == 0){
    ir=true;
    i=0;
   }
if ((x == 1)&&(i>=50)){
  ir=false;
}
  i++;
if (ir==true){
  t++;
}


if ((ir == true)&&(t>=10)){
    myservo.write(pos);              
    pos -= 2;
    t=0;                     
  if(pos <= 6){
    pos=170;
    myservo.write(pos); 
    delay(1000);
    t=0;i=0;
  }
}

if (ir == false){
  pos=170;
   myservo.write(pos);
}

// Serial.print("x:");Serial.print(x);Serial.print(" IR: ");Serial.print(ir);Serial.print(" i:");Serial.print(i);Serial.print(" t:");Serial.println(t);

  server.handleClient();
  MDNS.update();
  delay(30);
}
