#include <ESP8266WiFi.h>                    
#include <FirebaseArduino.h>                     
#include <strings_en.h>
#include <TimeLib.h>
#include <SPI.h>
#include <Wire.h>
#include "NTPClient.h"
#include "WiFiUdp.h"
#include "SSD1306Wire.h"

#define FIREBASE_HOST "voltroph-a088d.firebaseio.com"                         
#define FIREBASE_AUTH "I6krREhhiYTel9a4Uiyuv70GJSNkgYIhqYZlvruy"                  
//#define WIFI_SSID "Sourav"                                          
//#define WIFI_PASSWORD "9883224513@"                                    
#define WIFI_SSID "Echo"
#define WIFI_PASSWORD "IOT_8266"
#define RELAYCONTROL D3

SSD1306Wire  display(0x3c, D2, D1);

String deviceId = "FinalSocketId";
const int sensorIn = A0;
float NetEnergy = 0;
float energyRequested = 0;
float timeRequested = 0;
float timeElapsed = 0;
String bookingStartTime;
String bookingEndTime;

const long utcOffsetInSeconds = 19800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

String fireStatus = "OFF"; 

tmElements_t tm;
int Year, Month, Day, Hour, Minute, Second ;
                                                             
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
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                 
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
  Serial.println(WiFi.localIP());                                                  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);                     
}

void loop() { 
  delay(1000);
  pingBackend(0);                                  
  if (fireStatus == "ON") {                                                 
    Serial.println("Led Turned ON");                                                                                     
    digitalWrite(RELAYCONTROL,LOW); 
    getEnergyConsumption();
    updateBooking(NetEnergy, timeElapsed);
    validateBooking();
    OLED_Start(NetEnergy); 
  } 
  
  else if (fireStatus == "OFF") {                                               
    Serial.println("Led Turned OFF");
    digitalWrite(RELAYCONTROL,HIGH);
    resetDevice();
    OLED_End();                                                        
  }
  else {
    Serial.println("Wrong Credential! Please send ON/OFF");
  }
}

void getEnergyConsumption()
{
  int mVperAmp = 66;
  float Voltage = 0;
  float VRMS = 0;
  float AmpsRMS = 0;
  float Wattage = 0;
  float energy = 0;
  long milisec = millis(); // calculate time in milisec
  long time=milisec/1000; // convert time to sec 
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707; // sq root
  AmpsRMS = (VRMS * 1000)/mVperAmp;
  Wattage = (110*AmpsRMS)-8.63; //Observed 18-20 Watt when no load was connected, so substracting offset value to get real consumption.
  energy = ((Wattage*time)/(3600)); //for reading in Wh
  NetEnergy = NetEnergy + energy;
}

void resetDevice()
{
  NetEnergy = 0;
  energyRequested = 0;
  timeRequested = 0;
  timeElapsed = 0;
}

void validateBooking()
{
  fetchBooking();
  String currentTime = returnCurrentTime();
  float timeDiff = compareDates(currentTime, bookingEndTime);
  if (timeDiff < 0){
    fireStatus = "OFF";
    updateBackend();
  }
  currentTime = returnCurrentTime();
  timeDiff = compareDates(bookingStartTime, currentTime);
  Serial.println(currentTime);
  Serial.println(bookingStartTime);
  Serial.println(timeDiff);
  if (timeDiff < 0){
    fireStatus = "OFF";
    updateBackend();
  }
}

void updateBackend()
{
  Firebase.set("/"+ deviceId +"/operationStatus/", "OFF");
}

long returnUTC(String dateTime) {
  createElements(dateTime);
  long converDateTime = makeTime(tm);
  return converDateTime;
}

float compareDates(String Date1, String Date2)
{
  float timeDiff;
  timeDiff = ((returnUTC(Date2) - returnUTC(Date1))/60);
  return timeDiff;
}

void createElements(String dateTime)
{
  sscanf(dateTime.c_str(), "%d-%d-%dT%d:%d:%dZ", &Year, &Month, &Day, &Hour, &Minute, &Second);
  tm.Year = CalendarYrToTm(Year);
  tm.Month = Month;
  tm.Day = Day;
  tm.Hour = Hour;
  tm.Minute = Minute;
  tm.Second = Second;
}

void updateBooking(float energyConsumed, float timeElapsed)
{
  Firebase.set("/"+ deviceId + "/booking/energyConsumed/", energyConsumed);
  Firebase.set("/"+ deviceId + "/booking/timeElapsed/", timeElapsed);
}

void fetchBooking()
{
  energyRequested = Firebase.getFloat("/"+ deviceId +"/booking/energyRequested");
  timeRequested = Firebase.getFloat("/"+ deviceId +"/booking/timeRequested");
  bookingStartTime = Firebase.getString("/"+ deviceId +"/booking/bookingStartTime");
  bookingEndTime = Firebase.getString("/"+ deviceId +"/booking/bookingEndTime");
}

String returnCurrentTime()
{
  timeClient.update();
  String currentTime = timeClient.getFormattedDate();
  return currentTime;
}

void pingBackend(float batteryHealth)
{
  fireStatus = Firebase.getString("/"+ deviceId +"/operationStatus");
  Firebase.set("/"+ deviceId + "/batteryHealth/", batteryHealth);
  Firebase.setString("/"+ deviceId + "/lastPingTime/", returnCurrentTime());
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
