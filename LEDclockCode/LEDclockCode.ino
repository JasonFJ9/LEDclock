// Code by Jason FJ
// first "big" project
// Project using DOIT DEVKIT V1

//LED
#include <FastLED.h>
#define NUM_LEDS  66  // 13x5 LEDs + 1
#define LED_PIN   12
CRGB leds[NUM_LEDS];


//Photoresis (light sensor)
#define LIGHT_SENSOR_PIN 32 // ESP32 analog pin
                            // ADC2 pins does not work because of WiFi
int BMode = 1;
bool AutoBright = 0; // default is off
int BrightArr[4] = {10,30,50,70};

//debounce button
#include <ezButton.h>
ezButton button1(19); 
ezButton button2(21);
ezButton button3(22); 
ezButton button4(23);
#define DEBOUNCE_TIME  100

//Wifi & clock
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define UTC_OFFSET_IN_SECONDS 7200         // Offset from greenwich time ( 7200 for Berlin,Germany time)

// SSID and password of Wifi connection:
const char* ssid = "";        // !!! Enter SSID Here !!!
const char* password = "";    // !!! Enter password Here !!!

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_IN_SECONDS);


//Weather
#include <HTTPClient.h>
#include <Arduino_JSON.h>

String openWeatherMapApiKey = "";  //API key from https://openweathermap.org/api

String city = "";            //Country code and City
String countryCode = "DE";

unsigned long lastTime = 0;
unsigned long timerDelay = 3600000; //timer limit for next call 1 hour = 3600 sec

String jsonBuffer;

int WeathTemp,WeathHumid;

//  char Display[] = "00:00";  // initialize Display
int Mode = 0;              // default mode 0 = time
int counter = 0;          // count for next time API get


int DigCoorR[10][9]= {{0,1,2, 26,28, 52,53,54},        // 0 <- coordinates for digits
                      {2, 28, 54},                     // 1   1st, 3rd & 5th rows
                      {0,1,2, 26,27,28, 52,53,54},     // 2
                      {0,1,2, 26,27,28, 52,53,54},     // 3
                      {0,2, 26,27,28, 54},             // 4
                      {0,1,2, 26,27,28, 52,53,54},     // 5
                      {0,1,2, 26,27,28, 52,53,54},     // 6
                      {0,1,2, 28, 54},                 // 7
                      {0,1,2, 26,27,28, 52,53,54},     // 8
                      {0,1,2, 26,27,28, 52,53,54}} ;   // 9

int DigCoorL[10][6]= {{25,23, 51,49},         // 0   2nd & 4th rows
                      {23, 49},               // 1
                      {23, 51},               // 2
                      {23, 49},               // 3
                      {25,23, 49},            // 4
                      {25, 49},               // 5
                      {25, 51,49},            // 6
                      {23, 49},               // 7
                      {25,23, 51,49},         // 8
                      {25,23, 49}} ;          // 9

int sR[10] = {8,3,9,9,6,9,9,5,9,9};
int sL[10] = {4,2,2,2,3,2,3,2,4,3};

int Mid[4][5]= {{19,45},{58},{6},{6}}; // middle section dependant on Mode 0 1 2 3
int sM[4] = {2,1,1,1};

int Jump[4] = {0,3,7,10};

        //Section color( {      0      |      1     |     2     |     3     } )            Modes
int ModeClr[4][4][3] = { {{150,255,0}  ,{0,255,0}   ,{150,255,0},{0,255,0}  } ,        //Time = greenish
                         {{61,69,129}  ,{34,32,108} ,{145,103,6},{155,90,0}}  ,        //Date = light blue.orange
                         {{150,150,0}  ,{155,100,0} ,{0,100,155},{0,0,205}  } ,        //Temp&Humid = yellow.blue   broken 
                         {{255,70,0}   ,{255,0,0} ,{0,100,150},{0,0,200}  } };       //Weather = red.blue

char CharDig[10] = {'0','1','2','3','4','5','6','7','8','9'};





void MainSort(int Mode, char Display[] )  //pick coordinates and color for display
{
  for (int m=0 ; m < sM[Mode] ; m++){   //displays mid section
    Serial.print(Mid[Mode][m]);
    Serial.print(' ');
    leds[Mid[Mode][m]] = CRGB(100,0,150);  
  }
  
 //    Sec = current Section
  for (int Sec = 0; Sec < 4; Sec++){      // displays from section 0 1 2 3   
    int j = Sec;
    if (Sec >= 2){
      ++j;
    }
    int i = 0, Rc = 0, Rl = 0;
    
    while(i<10){
      if (Display[j] == CharDig[i]){
        int *R = DigCoorR[i];
        int *L = DigCoorL[i];
        DisplaySect( R , L , Jump[Sec] , ModeClr[Mode][Sec] , i );  
        i = 10;
      } else{
        i++;
      }
    }
  }
}



void DisplaySect( int CoorR[] , int CoorL[] , int Jump , int C[], int n ) // inputing coordinates to LED display
{ 
  for (int i=0 ; i < sR[n] ; i++){
    Serial.print(CoorR[i] + Jump);
    Serial.print(' ');
    leds[CoorR[i] + Jump] = CRGB(C[0] , C[1], C[2]);
  }
  Serial.println();
  for (int j=0 ; j < sL[n] ; j++){
    Serial.print(CoorL[j] - Jump);
    Serial.print(' ');
    leds[CoorL[j] - Jump] = CRGB(C[0] , C[1], C[2]);
  }
  Serial.println();
 // char clr[] = "colour : ";
  Serial.print(C[0]);Serial.print(' ');Serial.print(C[1]);Serial.print(' ');Serial.print(C[2]);
  Serial.println();
}


//---   get string from time API   ---------------------------------------------------------------
String getTimeStampString() 
{
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return yearStr + "-" + monthStr + "-" + dayStr + " " +
          hoursStr + ":" + minuteStr + ":" + secondStr;
}


//---   Weather API Stuff   ---------------------------------------------------------------
String httpGETRequest(const char* serverName) 
{
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void getWeather(){
  Serial.println(' ');
  Serial.println('getting Weather');
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";
      
  jsonBuffer = httpGETRequest(serverPath.c_str());
  Serial.println(jsonBuffer);
  JSONVar myObject = JSON.parse(jsonBuffer);
  
  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return;
  }
    
  //Serial.print("JSON object = ");
  //Serial.println(myObject);
  
  Serial.print("Temperature: ");
  Serial.println(myObject["main"]["temp"]);
  WeathTemp = myObject["main"]["temp"];
     
  //Serial.print("Pressure: ");
  //Serial.println(myObject["main"]["pressure"]);
  
  Serial.print("Humidity: ");
  Serial.println(myObject["main"]["humidity"]);
  WeathHumid = myObject["main"]["humidity"];
   
  //Serial.print("Wind Speed: ");
  //Serial.println(myObject["wind"]["speed"]); 

}


void ModeExe(int Mode){
  FastLED.clear();
  
  char Display[] = "00:00";
  switch(Mode){
    case 0:          //mode0 time
      Serial.print("Mode0");
      timeClient.update();
      for (int i=0 ; i < 5 ; i++){
        Display[i] = getTimeStampString()[i+11];
      }
      break;
    case 1:
      Serial.print("Mode1");
      timeClient.update();
      for (int k=0 ; k < 2 ; k++){
        Display[k] = getTimeStampString()[k+8];
      }
      for (int j=3 ; j < 5 ; j++){
        Display[j] = getTimeStampString()[j+2];
      }
      break;
    case 2:
      Serial.println("Mode2");
      Serial.print("removed temp sensor, DHT11 & 22 broke ;-;");   // Mode slot available
      break;
    case 3:
      Serial.print("Mode3");
      char WeathTempArr[2] = {};
      char WeathHumidArr[2] = {};
      itoa(WeathTemp,WeathTempArr,10);
      itoa(WeathHumid,WeathHumidArr,10);
      Display[0] = WeathTempArr[0];
      Display[1] = WeathTempArr[1];
      Display[3] = WeathHumidArr[0];
      Display[4] = WeathHumidArr[1];
      Serial.print(Display[0]);Serial.print(Display[1]);Serial.print(Display[3]);Serial.print(Display[4]);
      break;
  }
  
  Serial.print(Display);
  MainSort(Mode , Display);
  //delay(1000);
  FastLED.show();

}

void setup() {
  Serial.begin(115200);

  // LED Setup
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);  // default brightness , tested:  ~0.13A for 10 brightness(255,0,0)
 // delay(1000);

  //Mode Buttons
  button1.setDebounceTime(DEBOUNCE_TIME); // set debounce time to 50 milliseconds
  button2.setDebounceTime(DEBOUNCE_TIME);
  button3.setDebounceTime(DEBOUNCE_TIME);
  button4.setDebounceTime(DEBOUNCE_TIME);

  //wifi
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    leds[65] = CRGB(150 , 0, 0);
    delay ( 500 );
    Serial.print ( "." );
  }
  
  leds[65] = CRGB(0 , 90, 90);

  timeClient.begin(); //get time API info
  getWeather();  // get first weather info
 // delay(1000);
  ModeExe(Mode);  //display default, mode0 Time
  
}

  

void loop() {

  if ((millis() - lastTime) > timerDelay){   //update weather every hour
    // Check WiFi connection status
    Serial.println("testW1");
    if(WiFi.status()== WL_CONNECTED){
      Serial.println("testW2");
      getWeather();
      lastTime = millis();
    }else {
      Serial.println("WiFi Disconnected");
    }
  }
  
  button1.loop(); // MUST call the loop() function first
  button2.loop();
  button3.loop(); 
  button4.loop();
  
  if (button1.isPressed()){
    Mode++;
    if(Mode > 3){
      Mode = 0;
      }     
    ModeExe(Mode);
 //   delay(1500);
  }
  
  if (button2.isPressed()){
    Mode--;
    if(Mode < 0){
      Mode = 3;
      }     
    ModeExe(Mode);
 //   delay(1500);
  }
  
  if (button3.isPressed()){
    AutoBright = 0;
    BMode++;
    if(BMode > 3){
      BMode = 0;
    } 
    FastLED.setBrightness(BrightArr[BMode]);    
    //   delay(1500);
  }

  if (button4.isPressed()){
    AutoBright = 1;
  }

  

  if ((AutoBright == 1) && (counter % 100 == 0) ){
    int analogValue = analogRead(LIGHT_SENSOR_PIN);
    Serial.print("Analog Value = ");
    Serial.println(analogValue);   // the raw analog reading
    int i;
    if (analogValue < 900) {
      i = 0;
    } else if (analogValue < 1550) {
      i = 1;
    } else if (analogValue < 2300) {
      i = 2;
    } else {
      i = 3;
    }
    FastLED.setBrightness(BrightArr[i]);
    Serial.print("Auto Bright = ");
    Serial.println(BrightArr[i]);   
  }

  if (counter > 20000){
    ModeExe(Mode);
    counter = 0;
    Serial.println("L");
    
  }
  //Serial.print(' ');
  counter++;

}

///////////////////////////////
  
