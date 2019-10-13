#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensorR;
VL53L0X sensorL;

#define led1 15
#define led2 13
#define in2 12
#define in4 4

const int freq = 50000;
const int M1FChannel = 0, M2FChannel = 1;
const int resolution = 8;

byte pwmI, pwmA ;
int distR, distL, dd=5, calibR = 0,calibL = 0, offsetR = 43, offsetL = 60;
int bat,sol, imgcount=0, psize;
byte bb[8],bc[8], metab[6] ;
byte tcmode=0, tctype=0, tctime=0;
bool lowOrbit=true, picavail=false, insight=false;

unsigned long currenttime,targettime,lasttime=0;
unsigned long check_wifi = 30000;

WiFiUDP udp;
char incomingPacket[7];  // buffer for incoming packets

IPAddress ServerIP(192,168,4,1);

const char* host = "esp32";
const char* ssid = "PiFi.internal";
const char* password = "letmeaccessyourdata";

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

void intToByte (){
 bc[0]= (bat >> 24) & 0xFF;
 bc[1]= (bat >> 16) & 0xFF;
 bc[2]= (bat >> 8) & 0xFF;
 bc[3]= bat  & 0xFF;
 bc[4]= (sol >> 24) & 0xFF;
 bc[5]= (sol >> 16) & 0xFF;
 bc[6]= (sol >> 8) & 0xFF;
 bc[7]= sol  & 0xFF;
}

void ByteToint (){
bat = (uint32_t) bb[0] << 24;
bat |=  (uint32_t) bb[1] << 16;
bat |= (uint32_t) bb[2] << 8;
bat |= (uint32_t) bb[3]; 

sol = (uint32_t) bb[4] << 24;
sol |=  (uint32_t) bb[5] << 16;
sol |= (uint32_t) bb[6] << 8;
sol |= (uint32_t) bb[7]; 

}

TaskHandle_t Task1;

void setup(void) {

  pinMode(led1, OUTPUT); 
  pinMode(led2, OUTPUT); 
  pinMode(0, OUTPUT);
  pinMode(16, OUTPUT);


  WiFi.disconnect(true);
  delay(500);
  //register event handler
  WiFi.mode(WIFI_STA);
  delay(1000);
  //Initiate connection
  WiFi.begin(ssid, password);
 
  
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    while (1) {
      delay(1000);
    }
  }
  
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
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
      } else {
      }
    }
  });
  server.begin();
  
  ledcSetup(M1FChannel, freq, resolution);
  ledcSetup(M2FChannel, freq, resolution);
 
  ledcAttachPin(in2, M1FChannel);
  ledcAttachPin(in4, M2FChannel);
  
  ledcWrite(M1FChannel, 0);
  ledcWrite(M2FChannel, 0);
  
  digitalWrite(0, LOW);
  digitalWrite(16, LOW);

  //initialize 2 Distance-Sensors
  Wire.begin(2,14);
  digitalWrite(16, HIGH);
  delay(150);
  sensorR.init(true);
  delay(100);
  sensorR.setMeasurementTimingBudget(200000);
  sensorR.setAddress((uint8_t)25);
  digitalWrite(0, HIGH);
  delay(150);
  sensorL.init(true);
  delay(100);
  sensorL.setMeasurementTimingBudget(200000);

  udp.begin(WiFi.localIP(),2000);

//calibration
digitalWrite(led2, HIGH);
calibR=0;
calibL=0;
 for (int i = 0; i < 10; i++) {
    calibR=calibR+sensorR.readRangeSingleMillimeters();
    delay(200);
    calibL=calibL+sensorL.readRangeSingleMillimeters();
    delay(200);
  }
  offsetR = (calibR/10)-20;
  offsetL = (calibL/10)-20;

if (offsetR>200 || offsetL>200){
  calibR=0;
  calibL=0;
  for (int i = 0; i < 10; i++) {
    calibR=calibR+sensorR.readRangeSingleMillimeters();
    delay(200);
    calibL=calibL+sensorL.readRangeSingleMillimeters();
    delay(200);
  }
  offsetR = (calibR/10)-15;
  offsetL = (calibL/10)-15;
}
  
  byte oR[4],oL[4];
 oR[0]= (offsetR >> 24) & 0xFF;
 oR[1]= (offsetR >> 16) & 0xFF;
 oR[2]= (offsetR >> 8) & 0xFF;
 oR[3]= offsetR  & 0xFF;
 oL[0]= (offsetL >> 24) & 0xFF;
 oL[1]= (offsetL >> 16) & 0xFF;
 oL[2]= (offsetL >> 8) & 0xFF;
 oL[3]= offsetL  & 0xFF;
 
 udp.beginPacket(ServerIP, 2005);
 udp.write(oR[1]);
 udp.write(oR[2]);
 udp.write(oR[2]);
 udp.write(oR[3]);
 udp.endPacket();
 udp.flush();
 delay(100);
 udp.beginPacket(ServerIP, 2005);
 udp.write(oL[1]);
 udp.write(oL[2]);
 udp.write(oL[2]);
 udp.write(oL[3]);
 udp.endPacket();
 udp.flush();
 delay(100);
digitalWrite(led2, LOW);
delay(2000);

    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);

// Go to Zero , Updatable
while (dd>3 || dd<-3) {
   server.handleClient();
    distR = sensorR.readRangeSingleMillimeters()-offsetR;
    delay(200);
    distL = sensorL.readRangeSingleMillimeters()-offsetL;
    delay(200);
    dd = distR -distL;
    if ((dd<3)&&(dd>-3)){
      ledcWrite(M1FChannel, 0);
      ledcWrite(M2FChannel, 0);
    }
    if (dd>3){
      ledcWrite(M1FChannel, 70);
      ledcWrite(M2FChannel, 0);
    }
    if (dd<-3){
      ledcWrite(M1FChannel, 0);
      ledcWrite(M2FChannel, 70);
    }
}
      ledcWrite(M1FChannel, 0);
      ledcWrite(M2FChannel, 0);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  delay(500);


  xTaskCreatePinnedToCore(
                    Task1code,   // Task function. 
                    "Task1",     // name of task. 
                    10000,       // Stack size of task 
                    NULL,        // parameter of the task 
                    1,           // priority of the task 
                    &Task1,      // Task handle to keep track of created task 
                    1);          // pin task to core 1  

                   
}

void loop(void) {
    
    Wire.requestFrom(0x50, 8); // request 1 byte from slave device address 4
    byte u=0;
    while(Wire.available()) // slave may send less than requested
      {
        bb[u]= Wire.read();
        u++;
      }
 
    ByteToint();
    bat=map(bat,0,1023,0,6600);
    sol=map(sol,0,1023,0,6600);
    intToByte();
    delay(10);
   
    distR = sensorR.readRangeSingleMillimeters()-offsetR;
    delay(220);
    distL = sensorL.readRangeSingleMillimeters()-offsetL;
    delay(220);
    dd = distR-distL;
    
    pwmI = 70;
    pwmA = 135; 
    //pwmA = 130;  
    
    if (dd>5){
      int val = pwmA +(4*dd);
      pwmA = round( min(val, 255));
    }else if (dd<-5){
      int val = pwmI +(-4*dd);
      pwmI = round( min(val, 255));
    }
    
    ledcWrite(M1FChannel, pwmA);
    ledcWrite(M2FChannel, pwmI);


// run TC 
currenttime = millis();
if (currenttime > targettime){
  if (tcmode == 1){
    digitalWrite(led1, HIGH);
      if (tctype == 1){
        //move to low Orbit
             ledcWrite(M1FChannel, 255);
             ledcWrite(M2FChannel, 80);
             delay(1500);
        ledcWrite(M1FChannel, 100);
        ledcWrite(M2FChannel, 100);
        delay(1500);
        digitalWrite(led1, LOW);
      }
    if (tctype == 2){
        //move to high Orbit
            ledcWrite(M1FChannel, 200);
            ledcWrite(M2FChannel, 180);
            delay(3000);
            digitalWrite(led1, LOW);
    }
    tcmode = 0;
    tctype = 0;
    tctime = 0;
  }
 if (tcmode == 2){
    digitalWrite(led1, HIGH);
    delay(100);
    digitalWrite(led1, LOW);
    delay(50);
    digitalWrite(led1, HIGH);
      imgcount++;
      metab[0]=imgcount;
      if (tctype == 1){
        // photo res = 640x480
        metab[1]=1;
        psize = random(350, 600);
      }
      if (tctype == 2){
        // photo res = 1024x768
        metab[1]=2;
        psize = random(600, 900);
      }
      if (tctype == 3){
        // photo res = 1600x1200
        metab[1]=3;
        psize = random(900, 1500);
      }
      
       metab[2]= (psize >> 24) & 0xFF;
       metab[3]= (psize >> 16) & 0xFF;
       metab[4]= (psize >> 8) & 0xFF;
       metab[5]= psize  & 0xFF;

       picavail=true;

       digitalWrite(led1, LOW);
    tcmode = 0;
    tctype = 0;
    tctime = 0;
    }
    

}
    
}

void Task1code( void * pvParameters ){

  for(;;){

    // if wifi is down, try reconnecting every 5 seconds
  if ((WiFi.status() != WL_CONNECTED) && (millis() > check_wifi)) {
    WiFi.disconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(ssid, password);
    udp.begin(WiFi.localIP(),2000);
    check_wifi = millis() + 5000;
  }
   
    int packetSize = udp.parsePacket();
  if (packetSize){ 
    int len = udp.read(incomingPacket, 7);
    digitalWrite(led1, HIGH);
    delay(100);
    digitalWrite(led1, LOW);
    
   if (tcmode==0){
 
    tcmode = incomingPacket[0] - 0x30;
    tctype = incomingPacket[2] - 0x30;
    String x1,x2,x3;
    x1 = String(incomingPacket[4]-0x30);
    x2 = String(incomingPacket[5]-0x30);
    x3 = String(incomingPacket[6]-0x30);
    String myString = x1+x2+x3; 
    tctime = myString.toInt();
    targettime = millis() + tctime*1000;

    udp.beginPacket(ServerIP, 2006);
    udp.write(tcmode);
    udp.write(tctype);
    udp.write(tctime);
    udp.endPacket();
    udp.flush();

    incomingPacket[0]=0;
    incomingPacket[1]=0;
    incomingPacket[2]=0;
    incomingPacket[3]=0;
    incomingPacket[4]=0;
    incomingPacket[5]=0;
    incomingPacket[6]=0;
    incomingPacket[7]=0;
    }

    }
    
if (currenttime > lasttime+2000){
// Send TM Bat-Voltage Sol-Voltage
 udp.beginPacket(ServerIP, 2000);
 udp.write(bc[0]);
 udp.write(bc[1]);
 udp.write(bc[2]);
 udp.write(bc[3]);
 udp.endPacket();
 udp.flush();
 delay(100);
udp.beginPacket(ServerIP, 2001);
 udp.write(bc[4]);
 udp.write(bc[5]);
 udp.write(bc[6]);
 udp.write(bc[7]);
 udp.endPacket();
 udp.flush();
 lasttime=millis();
}


if (picavail){
  // send Photo meta
       udp.beginPacket(ServerIP, 2002);
       udp.write(metab[0]);
       udp.write(metab[1]);
       udp.write(metab[2]);
       udp.write(metab[3]);
       udp.write(metab[4]);
       udp.write(metab[5]);
       udp.endPacket();
       udp.flush();
       picavail=false;
}
   
  } 
}


