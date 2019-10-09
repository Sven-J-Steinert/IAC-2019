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
#define in2 12
#define in4 15

const int freq = 50000;
const int M1FChannel = 0, M2FChannel = 1;
const int resolution = 8;

int r = 140, i =0, a=70; 
byte pwmI, pwmA ;
int distR, distL, dd=5, distT=0, calibR = 0,calibL = 0, offsetR = 4300, offsetL = 6000 ;
int bat,sol, imgcount=0, psize;
byte bb[8],bc[8], metab[6] ;
byte tcmode=0, tctype, tctime, tcmode2=0, tctype2, tctime2;
bool lowOrbit=false;


unsigned long currenttime,targettime,targettime2;

WiFiUDP udp;
char incomingPacket[24];  // buffer for incoming packets
char controll = '1';       // data variable

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
  //calcOffset();
  
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
   server.handleClient();
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

 /*
  xTaskCreatePinnedToCore(
                    Task1code,   // Task function. 
                    "Task1",     // name of task. 
                    10000,       // Stack size of task 
                    NULL,        // parameter of the task 
                    1,           // priority of the task 
                    &Task1,      // Task handle to keep track of created task 
                    0);          // pin task to core 0  
*/
                   
}

void loop(void) {
 readUDP();
Wire.requestFrom(0x50, 8); // request 1 byte from slave device address 4
int u=0;
while(Wire.available()) // slave may send less than requested
 {
 bb[u]= Wire.read();
 Serial.println(bb[u]);
 u++;
 }
 
 ByteToint();
 bat=map(bat,0,1023,0,6600);
 sol=map(sol,0,1023,0,6600);
 intToByte();

   
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
   /*
if ((dd<5)&&(dd>-5)){
    int distT;
    distT = round((distR+distL)/2);
    ddT = distT-a;

    if (distT > 80){
      pwmA=max(pwmA+20, 0);
    }
    if (distT < 50){
      pwmI=max(pwmI+20, 0);
    }*/
    

    ledcWrite(M1FChannel, pwmA);
    ledcWrite(M2FChannel, pwmI);

currenttime = millis();
if (currenttime > targettime){
     digitalWrite(led1, HIGH);
    delay(200);
    digitalWrite(led1, LOW);
  if (tcmode == 1){
      if (tctype == 1){
        //move to low Orbit
             lowOrbit = true;
             ledcWrite(M1FChannel, 255);
             ledcWrite(M2FChannel, 55);
             delay(4000);
    
        while (dd>5 || dd<-5) {
        distR = sensorR.readRangeSingleMillimeters()-(offsetR/100);
        delay(200);
        distL = sensorL.readRangeSingleMillimeters()-(offsetL/100);
        delay(200);
        dd = distR -distL;
        ledcWrite(M1FChannel, 100);
        ledcWrite(M2FChannel, 95);
      }}
    if (tctype == 2){
        //move to high Orbit
            lowOrbit = false;
            ledcWrite(M1FChannel, 150);
            ledcWrite(M2FChannel, 150);
            delay(3500);

            while (dd>5 || dd<-5) {
            distR = sensorR.readRangeSingleMillimeters()-(offsetR/100);
            delay(200);
            distL = sensorL.readRangeSingleMillimeters()-(offsetL/100);
            delay(200);
            dd = distR -distL;
            ledcWrite(M1FChannel, 250);
            ledcWrite(M2FChannel, 50);
            }
    }}
 if (tcmode == 2){
    digitalWrite(led1, HIGH);
    delay(20);
    digitalWrite(led1, LOW);
    delay(20);
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
       
        // send meta
       udp.beginPacket(ServerIP, 2002);
       udp.write(metab[0]);
       udp.write(metab[1]);
       udp.write(metab[2]);
       udp.write(metab[3]);
       udp.write(metab[4]);
       udp.write(metab[5]);
       udp.endPacket();
       udp.flush();

       digitalWrite(led1, LOW);
    }
    resetTC();
}
      

    

/*
 if (currenttime > targettime2){
    if (tcmode2 == 1){
      if (tctype2 == 1){
        //move to low Orbit
      }
      if (tctype2 == 2){
        //move to high Orbit
      }
    }
    if (tcmode2 == 2){
      if (tctype2 == 1){
        // photo res = 640x480
      }
      if (tctype2 == 2){
        // photo res = 1024x768
      }
      if (tctype2 == 3){
        // photo res = 1600x1200
      }
    }
    }
*/
}

void Task1code( void * pvParameters ){

  for(;;){
 server.handleClient();
 
udp.beginPacket(ServerIP, 2000);
 udp.write(bc[0]);
 udp.write(bc[1]);
 udp.write(bc[2]);
 udp.write(bc[3]);
 udp.endPacket();
 udp.flush();
delay(200);
udp.beginPacket(ServerIP, 2001);
 udp.write(bc[4]);
 udp.write(bc[5]);
 udp.write(bc[6]);
 udp.write(bc[7]);
 udp.endPacket();
 udp.flush();
 delay(800);
}
  
}


void calcPWM()
{
  if (lowOrbit == true){
    pwmI = 85;
    pwmA = 130;  
  }else{
    pwmI = 100;
    pwmA = 150;
  }

}

void calcOffset()
{
  a=r-70;
  if (a>=90){
    offsetR = 2800;
    offsetL = 3800;
 }
  if ((a<90)&(a>=65)){
    offsetR = 3300;
    offsetL = 4400;
  } 
  if ((a<65)&(a>=35)){
    offsetR = 4000;
    offsetL = 5300;
  } 
  if (a<35){
    offsetR = 5200;
    offsetL = 5500;
  } 
}


void readUDP()
{
  int packetSize = udp.parsePacket();
  if (packetSize)
  { 
    int len = udp.read(incomingPacket, 24);
   /* if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    */
    
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
}else{
    tcmode2 = incomingPacket[3];
    tctype2 = incomingPacket[7];
    tctime2 = incomingPacket[11];
    targettime2 = millis()+ tctime2*1000;
} 

 udp.beginPacket(ServerIP, 2005);
 for (int v=0;v<24;v++){
   udp.write(incomingPacket[v]);
 }
 udp.endPacket();
 udp.flush();
delay(100);
 udp.beginPacket(ServerIP, 2005);
 udp.write(tcmode);
 udp.write(tctype);
 udp.write(tctime);
 udp.endPacket();
 udp.flush();

 for (int v=0;v<24;v++){
   incomingPacket[v]=0;
 }
   }
}

void resetTC()
{
    tcmode = 0;
    tctype = 0;
    tctime = 0;
}
