#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Arduino_GC9B72.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>



#include <Adafruit_GFX.h>
#include <Lato_Black14pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include "Jarvis_Round250x250.h"

// XIAO ESP32C3 only breaks out GPIO 2,3,4,5,6,7,8,9,10,20,21 (D0-D10).
// GPIO 11-14 (used by the S3 example this driver ships with) do not exist
// on this board's header, so the panel wiring must use these instead.
#define TFT_SCLK D8   // D8
#define TFT_MOSI D10  // D10
#define TFT_CS   D5   // D5
#define TFT_DC   D6  // D6
#define TFT_RST  D7  // D7
#define TFT_BL   D4   // D4

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_GC9B72(bus, TFT_RST, 0 /*rotation*/, false /*IPS*/, 360, 360);

const char* SSID = "Sherhome";
const char* PASSWORD = "4803493771";


//Sending HTTP GET request to: {"status": "Sleeping", "cpu_percent": 3.5, "gpu_percent": 9.0, "memory_total_gb": 62.4, "memory_used_gb": 12.0, "memory_percent": 19.2}

struct jstate
{
  String status;
  float cpu_pct;
  float gpu_pct;  //utilization
  float gpu_temp;
  float mem_pct;  //64GB total
  bool display_On; 

};

jstate JStatus;
String lastState = "";  //set a null state, persists across loop() calls

uint16_t graphgreen=gfx->color565(0,255,0);
uint16_t cyancolor=gfx->color565(39,90,254);  //16 bit  0x07FF 
uint16_t pinkcolor=gfx->color565(203,127,207);

ulong now, lastglow, glowinterval;
ulong lastCPUanim, CPUanim_interval;
ulong lastPoll, PollInterval;
float GPUmeter,CPUmeter;
float GPU_target=0; 
float CPU_target=0;


//------------------------------------------------------
void getConnectedWiFi() {
  WiFi.begin(SSID, PASSWORD);
  
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); 
}
//------------------------------------------------------
//       Get JSON File from server
void getJSONFromServer(const char* url) {

  String payload;

  if (WiFi.status() == WL_CONNECTED) {


    HTTPClient http;
    http.begin(url);

    //Serial.print("Sending HTTP GET request to: ");
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      payload = http.getString();
      //Serial.println(payload);
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
    getConnectedWiFi();
  }

  jstate incomingdata;
  JsonDocument jsnDoc;

  DeserializationError errorcode=deserializeJson(jsnDoc,payload);
  if(errorcode)
  {
    Serial.println("***Deserialization Failure...");
  }
  else
  {
    float gpupcnt=jsnDoc["gpu_percent"];

    String state=jsnDoc["status"];
    

    delay(100);

    JStatus.gpu_pct=jsnDoc["gpu_percent"];
    JStatus.cpu_pct=jsnDoc["cpu_percent"];
    JStatus.mem_pct=jsnDoc["memory_percent"];
    JStatus.status=jsnDoc["status"].as<String>();
    JStatus.display_On=jsnDoc["display_on"];




  }



  
} 
//------------------------------------------------------
void displayBinary(uint16_t num)
{
  for(int index=15;index>=0;index--)
    {
      if((num>>index)& 1)   //shift right by index and then AND with 1 in first position
        Serial.print(1);
          else
            Serial.print(0);
     
    }
  Serial.println();




}
//------------------------------------------------------
uint16_t bitManip(uint16_t basecolor, float scale)
{

  //Serial.print("BaseColor: ");
  //Serial.print(basecolor,HEX);
  //Serial.print("  ");
  //Serial.println(basecolor,BIN);

  //displayBinary(basecolor);

  uint16_t red,green,blue;

  //Serial.println("red");
  red=(basecolor>>11) & (0b11111);   //0x1f  shift first 5 over to right side and mask
  //displayBinary(red);
  //Serial.println(red);

  //Serial.println("green");
  green=(basecolor>>5)& (0b111111); //0x3f    shift green 5 to the right
  //displayBinary(green);
  //Serial.println(green);

  //Serial.println("blue");
  blue=(basecolor & 0b11111);
  //displayBinary(blue);
  //Serial.println(blue);

 // Serial.println("----------SCALE------------");


  red=float(red)*scale;
  green=green*scale;
  blue=blue*scale;

  //Serial.println(red);
 // Serial.println(green);
 // Serial.println(blue);

  //  Serial OR'ing of the componenet colors into their BIT positions in the final scaled color

  uint16_t scaledColor=0;
  scaledColor=(scaledColor | (red <<11)| (green<<5)| (blue));
  

 // Serial.println("New Scaled Color");

 // displayBinary(scaledColor);
  //Serial.println(scaledColor);








  return scaledColor;
}


//------------------------------------------------------
//⁡⁣⁣⁢This was proof of concept and is not actually being used⁡
void vectors()
{
  float v1x,v1y,p1x,p1y,p2x,p2y,p3x,p3y,theta;  ///xy coordinates
  float Cx=360/2.0;   //center x
  float Cy=360/2.0;   //center y
  float r=100;        //radius
  float segwidth;
  

  theta= 20.0;  //45 degree angle
  theta=theta * (PI/180); //convert to radians

  gfx->fillCircle(Cx,Cy,5,RGB565_WHITE);  //draw center
  
  v1x=cos(theta)*r;   //calc and draw vector 1
  v1y=sin(theta)*r;
  gfx->fillCircle(v1x+Cx,Cy-v1y,5,RGB565_WHITE);
  gfx->drawLine(Cx,Cy,v1x+Cx,Cy-v1y,RGB565_WHITE);

  //perp vector
  p1x=-sin(theta)*r/2;
  p1y=cos(theta)*r/2;
  gfx->drawLine(Cx,Cy,p1x+Cx,Cy-p1y,RGB565_WHITE);
  gfx->fillCircle(p1x+Cx,Cy-p1y,5,RGB565_WHITE);

  //perp vector at 180 degrees
  //perp vector
  p3x=(-sin(theta)*r/2)*-1.0;
  p3y=(cos(theta)*r/2)*-1.0;
  gfx->drawLine(Cx,Cy,p3x+Cx,Cy-p3y,RGB565_WHITE);
  gfx->fillCircle(p3x+Cx,Cy-p3y,5,RGB565_WHITE);


  //translated perp vector
  p2x=p1x+cos(theta)*r/2;
  p2y=p1y+sin(theta)*r/2;
  gfx->drawLine((cos(theta)*r/2)+Cx,Cy-(sin(theta)*r/2),p2x+Cx,Cy-p2y,RGB565_WHITE);
  gfx->fillCircle(p2x+Cx,Cy-p2y,5,RGB565_WHITE);



}
//------------------------------------------------------
//------------------------------------------------------
// ⁡⁣⁢⁣Calcualte the 4 points of the segment out on the radius⁡
// ⁡⁣⁢⁣width is span and thickness is inner vs outer radius⁡


void GPU_Graph(float GPU_load)
{
  float v1x,v1y,theta;  ///xy coordinates
  float Cx=360/2.0;   //center x
  float Cy=360/2.0;   //center y
  float n1x,n1y,n2x,n2y,n3x,n3y,n4x,n4y;  //the 4 points of the segment

  const float r=174.0;        //radius
  const float w=10.0;         //segment width
  const float t=20.0;         //thickness

  if(GPU_load > GPU_target)
    GPU_target=GPU_load;
    else
      GPU_target=(0.9*GPU_target)+(0.1*GPU_load);   //ease it down to the lower value over cycles
  
  for(float angle=180;angle>=90;angle-=6)
  {
      //theta= 80.0;  //80 degree angle
      theta=angle * (PI/180); //convert to radians

      
      //  ⁡⁣⁢⁣𝗟𝗲𝗮𝘃𝗲 𝘅,𝘆 𝗮𝘀 𝘂𝗻𝗶𝘁 𝘃𝗲𝗰𝘁𝗼𝗿𝘀 𝗲𝘅𝗽𝗮𝗻𝗱 𝗮𝘁 𝗽𝗹𝗼𝘁 𝘁𝗶𝗺𝗲⁡
      v1x=cos(theta);   //v1x as unit vector
      v1y=sin(theta);   //v1y as unit vector
      //gfx->drawLine(Cx,Cy,v1x*r+Cx,Cy-v1y*r,RGB565_GREEN);

      n1x= -sin(theta)*w/2.0  +  v1x*(r-t);  //form the perpendicular Vector and then translate it out
      n1y=  cos(theta)*w/2.0  +  v1y*(r-t);

      n2x=  (-sin(theta)*w/2.0)*-1.0 +  v1x*(r-t); //change the sign to get the 180 perp point then translate out to edge r-thickness
      n2y=  ( cos(theta)*w/2.0)*-1.0 +  v1y*(r-t);

      n3x= -sin(theta)*w/2.0  +  v1x*(r);  //form the perpendicular Vector and then translate it out
      n3y=  cos(theta)*w/2.0  +  v1y*(r);

      n4x=  (-sin(theta)*w/2.0)*-1.0 +  v1x*(r); //change the sign to get the 180 perp point then translate out to edge r-thickness
      n4y=  ( cos(theta)*w/2.0)*-1.0 +  v1y*(r);

      uint16_t fillcolor;
      float sweep=((180.0-angle)/90.0)*100.0;

      if(sweep<=70)  fillcolor=RGB565_DARKGREEN;
      if(sweep>70 && sweep<85) fillcolor=RGB565_DARKGOLDENROD;
      if(sweep>=85) fillcolor=RGB565_DARKRED;

      if(sweep>GPU_target)  fillcolor=RGB565_BLACK; //this will erase the bars to the top above valid reading level

      gfx->fillTriangle(n1x+Cx,Cy-n1y,n2x+Cx,Cy-n2y,n3x+Cx,Cy-n3y,fillcolor);
      gfx->fillTriangle(n3x+Cx,Cy-n3y,n4x+Cx,Cy-n4y,n2x+Cx,Cy-n2y,fillcolor);
  }
  
  


}
//------------------------------------------------------
//  ⁡⁣⁢⁣𝗖𝗣𝗨 𝗟𝗼𝗮𝗱 𝗚𝗿𝗮𝗽𝗵⁡


void CPU_Graph(float CPU_load)
{
  float v1x,v1y,theta;  ///xy coordinates
  float Cx=360/2.0;   //center x
  float Cy=360/2.0;   //center y
  float n1x,n1y,n2x,n2y,n3x,n3y,n4x,n4y;  //the 4 points of the segment

  const float r=174.0;        //radius
  const float w=10.0;         //segment width
  const float t=20.0;         //thickness

  if(CPU_load > CPU_target)
    CPU_target=CPU_load;
    else
    CPU_target=(0.9*CPU_target)+(0.1*CPU_load);   //ease it down to the lower value over cycles
  
  for(float angle=0;angle<=90;angle+=6)
  {
      //theta= 80.0;  //80 degree angle
      theta=angle * (PI/180); //convert to radians

      
      
      //  ⁡⁣⁢⁣𝗟𝗲𝗮𝘃𝗲 𝘅,𝘆 𝗮𝘀 𝘂𝗻𝗶𝘁 𝘃𝗲𝗰𝘁𝗼𝗿𝘀 𝗲𝘅𝗽𝗮𝗻𝗱 𝗮𝘁 𝗽𝗹𝗼𝘁 𝘁𝗶𝗺𝗲⁡
      v1x=cos(theta);   //v1x as unit vector
      v1y=sin(theta);   //v1y as unit vector
      //gfx->drawLine(Cx,Cy,v1x*r+Cx,Cy-v1y*r,RGB565_GREEN);

      n1x= -sin(theta)*w/2.0  +  v1x*(r-t);  //form the perpendicular Vector and then translate it out
      n1y=  cos(theta)*w/2.0  +  v1y*(r-t);

      n2x=  (-sin(theta)*w/2.0)*-1.0 +  v1x*(r-t); //change the sign to get the 180 perp point then translate out to edge r-thickness
      n2y=  ( cos(theta)*w/2.0)*-1.0 +  v1y*(r-t);

      n3x= -sin(theta)*w/2.0  +  v1x*(r);  //form the perpendicular Vector and then translate it out
      n3y=  cos(theta)*w/2.0  +  v1y*(r);

      n4x=  (-sin(theta)*w/2.0)*-1.0 +  v1x*(r); //change the sign to get the 180 perp point then translate out to edge r-thickness
      n4y=  ( cos(theta)*w/2.0)*-1.0 +  v1y*(r);

      uint16_t fillcolor;
      float sweep=((angle)/90.0)*100.0;

      if(sweep<=70)  fillcolor=RGB565_DARKGREEN;
      if(sweep>70 && sweep<85) fillcolor=RGB565_DARKGOLDENROD;
      if(sweep>=85) fillcolor=RGB565_DARKRED;

      if(sweep>CPU_target)  fillcolor=RGB565_BLACK; //this will erase the bars to the top above valid reading level

      gfx->fillTriangle(n1x+Cx,Cy-n1y,n2x+Cx,Cy-n2y,n3x+Cx,Cy-n3y,fillcolor);
      gfx->fillTriangle(n3x+Cx,Cy-n3y,n4x+Cx,Cy-n4y,n2x+Cx,Cy-n2y,fillcolor);
  }
  
  


}
//------------------------------------------------------
// ⁡⁣⁢⁣Calcualte the 4 points of the segment out on the radius⁡
// ⁡⁣⁢⁣width is span and thickness is inner vs outer radius⁡


void Pulse_Ring(uint16_t ringcolor)
{
  float v1x,v1y,theta;  ///xy coordinates
  float Cx=360/2.0;   //center x
  float Cy=348/2.0;   //center y
  float n1x,n1y,n2x,n2y,n3x,n3y,n4x,n4y;  //the 4 points of the segment

  float r=125;        //radius
  float w=60.0;         //segment width
  float t=5.0;         //thickness
  float k;            //decay constant
  
  


  k=.55 ;
  
float brightcoeff;

if(JStatus.status=="Speaking")    //create more flassh during speaking..widen the coeff bandwidth
{
  brightcoeff=((float(rand()%70)+30.0)/100.0);
}
    else
      brightcoeff=((float(rand()%20)+40.0)/100.0); 



for(float ringlevel=5;ringlevel>0;ringlevel-=1.0) //create the outward gradient
{
  for(float angle=0;angle<=360;angle+=20)
  {
      //theta= 80.0;  //80 degree angle
      theta=angle * (PI/180); //convert to radians

      
      //  ⁡⁣⁢⁣𝗟𝗲𝗮𝘃𝗲 𝘅,𝘆 𝗮𝘀 𝘂𝗻𝗶𝘁 𝘃𝗲𝗰𝘁𝗼𝗿𝘀 𝗲𝘅𝗽𝗮𝗻𝗱 𝗮𝘁 𝗽𝗹𝗼𝘁 𝘁𝗶𝗺𝗲⁡
      v1x=cos(theta);   //v1x as unit vector
      v1y=sin(theta);   //v1y as unit vector
      //gfx->drawLine(Cx,Cy,v1x*r+Cx,Cy-v1y*r,RGB565_GREEN);

      n1x= -sin(theta)*w/2.0  +  v1x*(r-t);  //form the perpendicular Vector and then translate it out
      n1y=  cos(theta)*w/2.0  +  v1y*(r-t);

      n2x=  (-sin(theta)*w/2.0)*-1.0 +  v1x*(r-t); //change the sign to get the 180 perp point then translate out to edge r-thickness
      n2y=  ( cos(theta)*w/2.0)*-1.0 +  v1y*(r-t);

      n3x= -sin(theta)*w/2.0  +  v1x*(r);  //form the perpendicular Vector and then translate it out
      n3y=  cos(theta)*w/2.0  +  v1y*(r);

      n4x=  (-sin(theta)*w/2.0)*-1.0 +  v1x*(r); //change the sign to get the 180 perp point then translate out to edge r-thickness
      n4y=  ( cos(theta)*w/2.0)*-1.0 +  v1y*(r);

      uint16_t fillcolor;
      float sweep=((180.0-angle)/90.0)*100.0;

      float brightness=(ringlevel/5.0);


      fillcolor=bitManip(ringcolor,brightness*brightcoeff);
      


      gfx->fillTriangle(n1x+Cx,Cy-n1y,n2x+Cx,Cy-n2y,n3x+Cx,Cy-n3y,fillcolor);
      gfx->fillTriangle(n3x+Cx,Cy-n3y,n4x+Cx,Cy-n4y,n2x+Cx,Cy-n2y,fillcolor);
  }

  r+=t; //increase the radius by the thickness to grow the shell

  
  
}

}
//-----------------------------------------------------------------------------------

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  Serial.begin(115200);
  Serial.setTxTimeoutMs(0); //prevents the device from hanging if data is not read at serial


  delay(5000);
  Serial.printf("PSRAM total: %u\n", ESP.getPsramSize());
  Serial.printf("PSRAM free:  %u\n", ESP.getFreePsram());


  getConnectedWiFi();

  gfx->begin(40000000);
  gfx->fillScreen(RGB565_BLACK);



  
  
  //gfx->fillCircle(180,180,160,RGB565_GREEN);
  int xpos,ypos;
  int centx=gfx->width()/2;
  int centy=gfx->height()/2;  
  xpos=centx-125;
  ypos=centy-125;

  //gfx->draw16bitRGBBitmap(xpos,ypos,Jarvis_Round250x250,250,250);
  //gfx->fillCircle(180,180,179,gfx->color565(0,128,0));

 // gfx->draw16bitRGBBitmapWithTranColor()


 gfx->draw16bitRGBBitmapWithTranColor(
    xpos,
    ypos,
    Jarvis_Round250x250,
    0xF81F,   // fuscha = transparent
    250,
    250
);

 // for(float i=0;i<10.0;i+=0.5)
 //   gfx->drawCircle(centx, centy, 180-i, RGB565_GREEN);

  bitManip(cyancolor,0.20);

  delay(5000);

  now=millis();
  lastglow=now;   //set thte intial times for all timers
  lastPoll=now;
  lastCPUanim=now;
  



}
//------------------------------------------------------------------------
void loop() {

  glowinterval=25.0;
  PollInterval=200.0;  //poll the cpu 100ms
  CPUanim_interval=10.0; //animate CPU gauge 10ms
  


  
  if((millis()-lastPoll)>PollInterval)    //timing for polling the server for CPU/GPU/Status
  {
    getJSONFromServer("http://powerspec.local:8765/status");
    lastPoll=millis();  //update the last polling event

    if(lastState!=JStatus.status) //only update state when it changes---prevents blink
    {
      gfx->fillRect(100,320,200,63,RGB565_BLACK);
      gfx->setCursor(130,340);
      gfx->setFont(&Lato_Black14pt7b);
      gfx->setTextColor(RGB565_LIGHTSKYBLUE);
      gfx->println(JStatus.status);

      lastState=JStatus.status;

      Serial.println(JStatus.display_On);

      
    }

 }

if(JStatus.display_On)
{
  digitalWrite(TFT_BL, HIGH); //turn on the backlight and draw

  if((millis()-lastglow)>glowinterval)
  {
    if(JStatus.status=="Sleeping")
          Pulse_Ring(pinkcolor);  //sleeping
          else
            Pulse_Ring(cyancolor);  //awake, speaking and thinking


        lastglow=millis();   

        gfx->setCursor(5,210);  //re-write CPU GPU tags so they are not corrupted by the glow
        gfx->setFont(&FreeMonoBold9pt7b);
        gfx->setTextColor(RGB565_GOLD);
        gfx->println("GPU");
        gfx->setCursor(325,210);
        gfx->println("CPU");

  }
  
  
  
  
  if((millis()-lastCPUanim)>CPUanim_interval)  //pass the current status to the gauge animation routine at
    //                                            the correct time interval
    {
      GPU_Graph(JStatus.gpu_pct);
      CPU_Graph(JStatus.cpu_pct);

      lastCPUanim=millis();
    }

}

  else
  {
      digitalWrite(TFT_BL,LOW);


  }
  


  

  

}
