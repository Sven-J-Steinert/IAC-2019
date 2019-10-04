#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensorR;
VL53L0X sensorL;

#define led1 4
#define led2 13
#define in2 15
#define in4 12

const int freq = 50000;
const int M1FChannel = 0, M2FChannel = 1;
const int resolution = 8;

int r = 140, i = 1+(8/r);  // r = radius to center, i = rpmA/rpmI
byte pwmI, pwmA ;
int distR, distL, dd=5, distT=0, calibR = 0,calibL = 0, offsetL = 5100, offsetR = 4400;
byte bb[4] ;

WiFiUDP udp;
byte incomingPacket[1];  // buffer for incoming packets
char controll = '1';       // data variable

const char* host = "esp32";
//const char* ssid = "PiFi.internal";
//const char* password = "letmeaccessyourdata";
const char* ssid = "hack.me.if.you.can";
const char* password = "NikJanSve201823";

WebServer server(80);

/*
 * Server Index Page
 */
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<p>Orbiter esp32-webupdate</p>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

  
void intToByte ( int i){
 bb[0]= (i >> 24) & 0xFF;
 bb[1]= (i >> 16) & 0xFF;
 bb[2]= (i >> 8) & 0xFF;
 bb[3]= i  & 0xFF;
}


void setup(void) {
  
    Serial.begin(115200);
  
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
  
  udp.begin(2000);
  
  ledcSetup(M1FChannel, freq, resolution);
  ledcSetup(M2FChannel, freq, resolution);
 
  ledcAttachPin(in2, M1FChannel);
  ledcAttachPin(in4, M2FChannel);
  
  ledcWrite(M1FChannel, 0);
  ledcWrite(M2FChannel, 0);

  r = 140;
  calcPWM();
  
  pinMode(led1, OUTPUT); 
  pinMode(led2, OUTPUT); 
  pinMode(0, OUTPUT);
  pinMode(16, OUTPUT);
  
  digitalWrite(0, LOW);
  digitalWrite(16, LOW);
  
  Wire.begin(2,14);
  
  digitalWrite(16, HIGH);
  delay(150);
  sensorR.init(true);
  delay(100);
  sensorR.setMeasurementTimingBudget(200000);
  sensorR.setAddress((uint8_t)25);
  //sensor.init();
  //sensor.setTimeout(500);

  digitalWrite(0, HIGH);
  delay(150);
  sensorL.init(true);
  delay(100);
  sensorL.setMeasurementTimingBudget(200000);

    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);


while (dd>3 || dd<-3) {
    distR = sensorR.readRangeSingleMillimeters()-(offsetR/100);
    delay(200);
    distL = sensorL.readRangeSingleMillimeters()-(offsetL/100);
    delay(200);
    dd = distR -distL;
    if ((dd<3)&&(dd>-3)){
      ledcWrite(M1FChannel, 0);
      ledcWrite(M2FChannel, 0);
    }
    if (dd>3){
      ledcWrite(M1FChannel, 50);
      ledcWrite(M2FChannel, 0);
    }
    if (dd<-3){
      ledcWrite(M1FChannel, 0);
      ledcWrite(M2FChannel, 50);
    }
}
      ledcWrite(M1FChannel, 0);
      ledcWrite(M2FChannel, 0);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  delay(500);
}

void loop(void) {
  
readUDP();

if (controll == '1'){
  server.handleClient();
  
    distR = sensorR.readRangeSingleMillimeters()-(offsetR/100);
    delay(200);
    distL = sensorL.readRangeSingleMillimeters()-(offsetL/100);
    delay(200);
    dd = distR -distL;
       
    calcPWM();
    
    if (dd>5){
      int val = pwmA +(4*dd);
      pwmA = round( min(val, 255));
    }
    if (dd<-5){
      int val = pwmI +(-4*dd);
      pwmI = round( min(val, 255));
    }
    
    ledcWrite(M1FChannel, pwmA);
    ledcWrite(M2FChannel, pwmI);
}

if (controll == '2'){
  server.handleClient();
  ledcWrite(M1FChannel, 0);
  ledcWrite(M2FChannel, 0);
  delay(2000);
  digitalWrite(led2, HIGH);
 for (int i = 0; i < 50; i++) {
    distR = sensorR.readRangeSingleMillimeters();
    delay(200);
    distL = sensorL.readRangeSingleMillimeters();
    delay(200);
    calibR = calibR+distR;
    calibL = calibL+distL;
 }
    offsetR = round(((calibR/50)-80)*100);
    offsetL = round(((calibL/50)-80)*100);

IPAddress ServerIP(192,168,178,142);
udp.beginPacket(ServerIP, 2000);
 intToByte(offsetR);
 udp.write(bb,4);
 intToByte(offsetL);
 udp.write(bb,4);
 udp.endPacket();
 udp.flush();
  digitalWrite(led2, LOW);
 controll = '0';
}

if (controll == '3'){
    
    server.handleClient();
    ledcWrite(M1FChannel, 0);
    ledcWrite(M2FChannel, 0);
IPAddress ServerIP(192,168,178,142);
udp.beginPacket(ServerIP, 2000);
 intToByte(offsetR);
 udp.write(bb,4);
 udp.endPacket();
 udp.flush();
 delay(200);
 udp.beginPacket(ServerIP, 2000);
 intToByte(offsetL);
 udp.write(bb,4);
 udp.endPacket();
 udp.flush();
delay(200);
}

}




void calcPWM()
{
pwmI = round(1.05*(((2*3.14*r/180)+3)/0.165));
pwmA = round(((2*3.14*(r+80)/180)+3)/0.165);
}

void readUDP()
{
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    digitalWrite(led1, HIGH);
    delay(100);
    digitalWrite(led1, LOW);
    int len = udp.read(incomingPacket, 7);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    char c = incomingPacket[0];
    controll = c;
    
   }
}
