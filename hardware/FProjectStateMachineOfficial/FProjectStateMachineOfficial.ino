#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu9255_esp32.h>
#include<math.h>
#include<string.h>

#include "Adafruit_VL6180X.h"

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int BUTTON_PIN1 = 16;
const int BUTTON_PIN2 = 19;
const int LOOP_PERIOD = 100;

char network[] = "MIT GUEST";  //SSID for 6.08 Lab
char password[] = ""; //Password for 6.08 Lab
char host[] = "608dev.net";
char user[] = "AshikaV";

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request

unsigned long primary_timer;

int old_val;

const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response





//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//Support functions
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


class Button {
  //YOUR BUTTON CLASS IMPLEMENTATION!!!
  public:
  uint32_t t_of_state_2;
  uint32_t t_of_button_change;    
  uint32_t debounce_time;
  uint32_t long_press_time;
  uint8_t pin;
  uint8_t flag;
  bool button_pressed;
  uint8_t state; // This is public for the sake of convenience
  Button(int p) {
  flag = 0;  
    state = 0;
    pin = p;
    t_of_state_2 = millis(); //init
    t_of_button_change = millis(); //init
    debounce_time = 10;
    long_press_time = 1000;
    button_pressed = 0;
  }
  void read() {
    uint8_t button_state = digitalRead(pin);  
    button_pressed = !button_state;
  }
  int update() {
    read();
    flag = 0;
    if (state==0) {
      if (button_pressed) {
        state = 1;
        t_of_button_change = millis();
      }
    } else if (state==1) { 
      if (button_pressed && (millis()-t_of_button_change)>= debounce_time) {
        state = 2;
        t_of_state_2 = millis();
      }
      else if(!button_pressed){
        state = 0;
        t_of_button_change = millis();
      }
    } else if (state==2) {//normally pressed
      
      if (button_pressed && (millis()-t_of_state_2)>= long_press_time) {
        state = 3;
      }
      else if(!button_pressed){
        state = 4;
        t_of_button_change = millis();
      }
    } else if (state==3) {
      if(!button_pressed){
        state = 4;
        t_of_button_change = millis();
      }
    } else if (state==4) {  //unpressed     
      if (!button_pressed && (millis()-t_of_button_change)>= debounce_time) {
        state = 0;
        if(millis()-t_of_state_2<long_press_time){
          flag = 1;
        }
        else{
          flag = 2;
        }
      }
      else if(button_pressed && (millis()-t_of_state_2)<long_press_time){
        state = 2;
        t_of_button_change = millis();
      }
      else if(button_pressed && (millis()-t_of_state_2)>=long_press_time){
        state = 3;
        t_of_button_change = millis();
      }
    }
    return flag;
  }
};

/*----------------------------------
  char_append Function:
  Arguments:
     char* buff: pointer to character array which we will append a
     char c:
     uint16_t buff_size: size of buffer buff

  Return value:
     boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
   do_http_request Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
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

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////







class FProjectStateMachine {
    uint32_t record_timer;
    uint32_t record_time = 20000;
    char song[6000] = {};
    int ind = 0;
    int count = 1;
    int old = 0;
    char temp[15];
    int state;
    long old_mm,older_mm,avg_mm;
    
  public:
    char user[100] = {0};
    FProjectStateMachine() {
      state = -1;
      strcpy(user, "Ashika");
      
    }
    void update(int button1, int button2, long mm, char* output) {
      switch(state){
        case -1:
          strcpy(output, "Welcome to Theremin Hero!\nShort Press for Record Mode\nLong Press for Song Mode");
          state = 0;
          break;
        case 0: //MAIN MENU
          if(button1 == 1 || button2 == 1){ //Short Pressn
            state = 1;
            strcpy(output, "Record\n\nPress button 1 record\nPress button 2 to go back");\
        
          }
          if(button1==2 || button2 ==2){
            state = 2;
            strcpy(output, "Song\n\nPress button 1 record\nPress button 2 to go back");
          }
          break;
        case 1: //RECORD MODE MENU
          if(button1 == 1 || button2 == 2){
            //record button pressed
            record_timer = millis();
            state = 2;
            song[6000] = {};
            ind = 0;
            count = 1;
            old = 0;
            memset(temp, 0, sizeof temp);
            strcpy(output, "Recording...\n\nLong Press to break");
          }
          else if(button2 == 1||button2 == 2){
            state = -1;
          }
          break;
        case 2: //ACTUAL RECORDING. BREAKS AFTER SPECIFIC TIME
          
          older_mm = old_mm;
          old_mm = mm;
        
          avg_mm = ((mm+old_mm+older_mm)/3);
        
          avg_mm = map(avg_mm,0,255,262,523);
        
        
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
          
          if(millis() - record_timer>record_time){
            state = 3;
            strcpy(output, "Upload?\n\nPress button 1 upload\nPress button 2 to go back");
          }
          if(button1 == 2 || button2 == 2){
            state = 1;
            strcpy(output, "Record\n\nPress button 1 record\nPress button 2 to go back");//goes back to record home
          }
          break;
        case 3: //UPLOAD TO THE SERVER? YES OR NAH
          if(button1 == 1 || button1 == 2){
            state = 4;
            strcpy(output, "Uploading to the server");
          }
          else if(button2 == 1 || button2 == 2){ //goes back to record home
            state = 1;
            strcpy(output, "Record\n\nPress button 1 record\nPress button 2 to go back");
          }
          break;
        case 4: //UPLOADS SONG TO THE SERVER
          Serial.println(song);
          char thing[6000];
          sprintf(thing, "songName=%s&musicString=%s", "temp_song", song );
          char request[500];
          sprintf(request,"POST /sandbox/sc/kgarner/project/server.py HTTP/1.1\r\n");
          sprintf(request+strlen(request),"Host: %s\r\n",host);
          strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
          sprintf(request+strlen(request),"Content-Length: %d\r\n\r\n",strlen(thing));
          strcat(request,thing);
//          do_http_request(host,request,response,OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);

          state = 1;
          strcpy(output, "Record\n\nPress button 1 record\nPress button 2 to go back");//goes back to record home
          break;
      }
      
  }
};

FProjectStateMachine wg;
Button button1(BUTTON_PIN1); //button object!
Button button2(BUTTON_PIN2); //button object!
Adafruit_VL6180X sensor;

void setup() {
  Serial.begin(115200); //for debugging if needed.
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
  
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  primary_timer = millis();
  strcpy(wg.user, user);
}

void loop() {
  int bv1 = button1.update(); //get button value
  int bv2 = button2.update(); //get button value
  wg.update(bv1, bv2, sensor.readRange(), response); //input: angle and button, output String to display on this timestep
  if (strcmp(response, old_response) != 0) {//only draw if changed!
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 1);
    tft.println(response);
  }
  memset(old_response, 0, sizeof(old_response));
  strcat(old_response, response);
  while (millis() - primary_timer < 40); //wait for primary timer to increment
  primary_timer = millis();
}
