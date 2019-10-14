#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h>

const char* ssid ="PiFi.internal";
const char* pass ="letmeaccessyourdata";

const char* host = "esp8266-webupdate";

#define IR_PIN 12
#define Servo_PIN 13


unsigned int localPort = 2000; // local port to listen for UDP packets

IPAddress ServerIP(192,168,4,1);
 
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


Servo myservo;  // create servo object to control a servo
int pos = 170;    // variable to store the servo position
byte x;
int i=0,t=0, c=0;
bool ir=false, nostop= false;

unsigned long check_wifi = 5000;

ESP8266WebServer server(80);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><p>Groundstation esp8266-webupdate</p><input type='file' name='update'><input type='submit' value='Update'></form>";


void setup()
{
  pinMode(IR_PIN, INPUT);
  myservo.attach(Servo_PIN); 
  myservo.write(pos);
  
  Serial.begin(115200);
    Serial.println();
 
  initNetwork();
  
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

    //Start UDP
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
   pos=170;
   myservo.write(pos);

}




void loop()
{
  // if wifi is down, try reconnecting every 5 seconds
  if ((WiFi.status() != WL_CONNECTED) && (millis() > check_wifi)) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    udp.begin(localPort);
    check_wifi = millis() + 5000;
  }


// get IR
   x = digitalRead(12);
  // IR refresh
  if (x == 0){
    ir=true;
    i=0;
   }
   // IR loss
if ((x == 1)&&(i>=150)){
  ir=false;
}
  i++;
  // connected count
if ((ir==true) || nostop){
  t++;
  c++;
}


if (((ir == true)&&(t>=10)&&(c>=120)) || ((nostop)&&(t>=10)) ){
  // make Servo Steps
    myservo.write(pos);              
    pos -= 1;
    t=0;                     
  if(pos <= 6){
  // finish routine
    pos=170;
    myservo.write(pos); 
    t=0;i=0;c=0;
    nostop = false;
    for (int a = 0; a < 5; a++) {
     udp.beginPacket(ServerIP, 1999);
     udp.write(1);
     udp.endPacket();
     udp.flush();
  }
  }
}

if ((pos < 160)&&(pos > 155)){
 udp.beginPacket(ServerIP, 1998);
 udp.write(1);
 udp.endPacket();
 udp.flush();
 nostop = true;
}

if (ir == false){
 // pos=170;
 //  myservo.write(pos);
   c=0;
}

  server.handleClient();
  MDNS.update();
  delay(20);

}




void initNetwork()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("UDP server on port %d\n", localPort);
  udp.begin(localPort);
}
