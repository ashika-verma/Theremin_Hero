/* This minimal example shows how to get single-shot range
measurements from the VL6180X.
The range readings are in units of mm. */
// STEP 1: Figure out how to get the duration of a frequency
// WIFI 
// POST/GET
// STEP 2: Concatonate frequencies and durations to a running array (frequency,duration;frequency,duration;frequency,duration;)
// STEP 3: POST to Kendal's server
#include <Wire.h>
//#include <VL6180X.h>
#include "Adafruit_VL6180X.h"
#include <WiFi.h>

//VL6180X sensor;
Adafruit_VL6180X sensor;

char network[] = "6s08";  //SSID for 6.08 Lab
char password[] = "iesc6s08"; //Password for 6.08 Lab
char host[] = "608dev.net";
char user[] = "AshikaV";

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request

uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

long mm,old_mm,older_mm,avg_mm;
int buzzpin = 27;
int notes[] = {262,294,330,349,392,440,494,523};
char note_alpha[] = {'C','D','E','F','G','A','B','C'};
uint32_t timer = 0;
void setup() 
{
  Serial.begin(115200); 
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  sensor = Adafruit_VL6180X();
  
  Wire.begin();
  sensor.begin();
//  sensor.configureDefault();
//  sensor.setTimeout(500);
  ledcSetup(0,2000,8);
  ledcAttachPin(buzzpin,0);
  timer = millis();
}
char song[6000] = {};
int ind = 0;
int count = 1;
int old = 0;
char temp[15];
int state = 0;

void loop() 

{ 
  if(millis()-timer<5000){
    
  state = 0;
  mm = sensor.readRange();
//  avg_mm = map(avg_mm,0,255,262,523);
  older_mm = old_mm;
  old_mm = mm;

  avg_mm = ((mm+old_mm+older_mm)/3);

  avg_mm = map(avg_mm,0,255,262,523);

//  int poss_notes[8] = {0,0,0,0,0,0,0,0};
//  for (int j = 0; j < 8; j++){
//    poss_notes[j] = abs(avg_mm - notes[j]);
//  }
//  int min_note = poss_notes[0];
//  int note_ind = 0;
//  for (int j = 0; j < 8; j++){
//    if (poss_notes[j] < min_note){
//      min_note = poss_notes[j];
//      note_ind = j;
//    }
//  }

// sprintf(freq, %d,%d;)
// strcat(freq,freq)
  if (old == avg_mm){
    count += 1;
    old = avg_mm;
  }
  else {
    sprintf(temp, "%d,%d;", old, count);
    strcat(song, temp);
    old = avg_mm;
    count = 1;
    
  }

//  Serial.println(mm);
//  Serial.println(avg_mm);
//    if (abs(old_mm - mm) <= 2){
//    int index = avg_mm-33;
//    if (index <0 | index > 7){
//      index = 0;                                                                                                               
//    }
//    tft.fillScreen(TFT_BLACK);
//    tft.setCursor(0,0,1);
//    tft.print(notes[index]);
    ledcWriteTone(0, avg_mm);
    ledcWrite(0,100);
  Serial.println(avg_mm);
  Serial.println(song);
  }else if(state ==0){
    Serial.println("yoooo");


    char thing[6000];
    sprintf(thing, "songName=%s&musicString=%s", "temp_song", song );
    char request[500];
    sprintf(request,"POST /sandbox/sc/kgarner/project/server.py HTTP/1.1\r\n");
    sprintf(request+strlen(request),"Host: %s\r\n",host);
    strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request+strlen(request),"Content-Length: %d\r\n\r\n",strlen(thing));
    strcat(request,thing);
    do_http_request(host,request,response,OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);

    
    state = 1;
//    timer = millis();
  }

delay(100);
}
