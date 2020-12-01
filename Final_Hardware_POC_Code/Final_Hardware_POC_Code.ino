#include <ESP8266WiFi.h>                             // esp8266 library
#include <FirebaseArduino.h>                        // firebase library
#include <strings_en.h>
#include <SPI.h>
#include <Wire.h>
#include "SSD1306Wire.h"

#define FIREBASE_HOST "spotenergy-7e2a0.firebaseio.com"                         // the project name address from firebase id
#define FIREBASE_AUTH "hb0M9ocx1h1zG9P8URK60HfJchJtmdZoxG6y3P6s"                    // the secret key generated from firebase
#define WIFI_SSID "Echo"                                          // input your home or public wifi name 
#define WIFI_PASSWORD "IOT_8266"                                    //password of wifi ssid
#define RELAYCONTROL D3

SSD1306Wire  display(0x3c, D2, D1);

String deviceId = "socketId-Demo";
const int sensorIn = A0;
int mVperAmp = 66;
float Voltage = 0;
float VRMS = 0;
float AmpsRMS = 0;
float Wattage = 0;
float energy = 0;
float NetEnergy = 0;

String fireStatus = "OFF";                                                     // led status received from firebase                                                               
void setup() {
  Serial.begin(9600);
  Serial.println("");
  Serial.println("Initializing OLED Display");
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  OLED_Init();
  delay(1000);
    
  pinMode(A0, INPUT); 
  pinMode(RELAYCONTROL, OUTPUT);
  digitalWrite(RELAYCONTROL, HIGH);                 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                      //try to connect with wifi
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP());                                                      //print local IP address
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);                                       // connect to firebase
}

void loop() {
  fireStatus = Firebase.getString("/"+ deviceId +"/STATUS");                                      // get ld status input from firebase
  if (fireStatus == "ON") {                                                          // compare the input of led status received from firebase
    Serial.println("Led Turned ON");                                                   // make bultin led ON                                                     
    digitalWrite(RELAYCONTROL,LOW);
    long milisec = millis(); // calculate time in milisec
    long time=milisec/1000; // convert time to sec  
    Voltage = getVPP();
    VRMS = (Voltage/2.0) *0.707; // sq root
    AmpsRMS = (VRMS * 1000)/mVperAmp;
    Wattage = (110*AmpsRMS)-8.63; //Observed 18-20 Watt when no load was connected, so substracting offset value to get real consumption.
    energy = ((Wattage*time)/(3600)); //for reading in Wh
    NetEnergy = NetEnergy + energy;
    Serial.print(AmpsRMS);
    Serial.println(" Amps RMS ");
    Serial.print(Wattage);
    Serial.println(" Watt ");
    Serial.print(NetEnergy);
    Serial.println(" Wh ");  
    Firebase.push("/"+ deviceId + "/CURRENT/", NetEnergy);
    OLED_Start(NetEnergy);
    delay(1000); 
  } 
  
  else if (fireStatus == "OFF") {                                                  // compare the input of led status received from firebase
    Serial.println("Led Turned OFF");
    //digitalWrite(LED_BUILTIN, HIGH);                                               // make bultin led OFF
    digitalWrite(RELAYCONTROL,HIGH);
    OLED_End(); 
    delay(1000);                                                       
  }
  else {
    Serial.println("Wrong Credential! Please send ON/OFF");
  }
}



float getVPP()
{
float result;

int readValue; //value read from the sensor
int maxValue = 0; // store max value here
int minValue = 1024; // store min value here

uint32_t start_time = millis();

while((millis()-start_time) < 1000) //sample for 1 Sec
{
readValue = analogRead(sensorIn);
// see if you have a new maxValue
if (readValue > maxValue)
{
/*record the maximum sensor value*/
maxValue = readValue;
}
if (readValue < minValue)
{
/*record the maximum sensor value*/
minValue = readValue;
}
/* Serial.print(readValue);
Serial.println(" readValue ");
Serial.print(maxValue);
Serial.println(" maxValue ");
Serial.print(minValue);
Serial.println(" minValue ");
delay(1000); */
}

// Subtract min from max
result = ((maxValue - minValue) * 5)/1024.0;

return result;
}

void OLED_Init() {
  // clear the display
  display.clear();
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "VOLTROPH");
  // write the buffer to the display
  display.display();
}

void OLED_Start(float Energy) {
  // clear the display
  display.clear();
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Charging ON");
    display.drawString(0, 30, String(Energy) + " Wh");
  // write the buffer to the display
  display.display();
}

void OLED_End() {
  // clear the display
  display.clear();
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Charging OFF");
  // write the buffer to the display
  display.display();
}
