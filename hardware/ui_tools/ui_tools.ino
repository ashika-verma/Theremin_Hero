#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <vector>
#include <string>
#include <list>
#include <WiFi.h> //Connect to WiFi Network
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <mpu9255_esp32.h>
#include "Button.h"
#include "Adafruit_VL6180X.h"
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <math.h>

MPU9255 imu; //imu object called, appropriately, imu
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

const uint16_t PixelCount = 120; // make sure to set this to the number of pixels in your strip
const uint8_t PixelPin = 14;  // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

const uint8_t LOOP_PERIOD = 100; // milliseconds
// the number of loop cycles a single note should take when playing a song
const uint8_t LOOP_COUNT = 4; 

uint32_t primary_timer = 0;

const std::vector<std::string> NOTES = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C" };

// HTTP related vars
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 3000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 3000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

// the number of entries for a menu on a screen
const int ENTRIES_PER_SCREEN = 16;

// Button definitions
const int TOP_PIN = 16;
const int BOT_PIN = 5;
Button topPin(TOP_PIN);
Button bottomPin(BOT_PIN);

// wifi related vars
char host[] = "608dev.net";

const char network[] = "6s08";  //SSID for 6.08 Lab
const char password[] = "iesc6s08"; //Password for 6.08 Lab

//const char network[] = "MIT";  //SSID for 6.08 Lab
//const char password[] = ""; //Password for 6.08 Lab

const char delim[] = ",;";
const int notes_freq[] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523};

// finds the closest frequency index (linearly) given an input frequency
// these frequencies are in the range [C3, C4]s
int find_closest_idx(float freq) {
  int semitone = round(log2(freq / 440.0) * 12.0);  
  semitone += 9;

  if (semitone < 0) {
    return 0;
  } else if (semitone >= NOTES.size()) {
    return NOTES.size() - 1;
  } else {
    return semitone;
  }
}

// wrapper for handling wifi
void initializeWifi() {
  WiFi.begin(network,password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);

  while (WiFi.status() != WL_CONNECTED && count<12) {
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

  Wire.begin();
}

/**
 * A class corresponding to a note block, with 
 * an x position, x, y, y, and frequency, freq
 */
class Note {
  private:
    int x;
    int y;
    int freq;

  public:
    // constructs a note with a frequency, freq
    // and an initial y position, iny_p
    // the x position is chosen by the closest index
    Note(float freq, int inp_y = 15) {
      this->y = inp_y;
      int idx = find_closest_idx(freq);
      this->x = 12 * idx;
      this->freq = notes_freq[idx];
    }

    // draws this note at its current x and y
    // with the color, inp_color and moves this note
    // down 20 pixels
    void draw(uint16_t inp_color = TFT_RED) {
      if (y > 15) {
        tft.fillRect(x + 1, y -19, 10, 18, TFT_BLACK);
      }
      if (y < 115) {
        tft.fillRect(x + 1, y + 1, 10, 18, inp_color);
      }
      y += 20;
    }

    // returns the y position for this note
    int get_y() {
      return y;
    }

    // returns the frequency for this note
    int frequency() {
      return freq;
    }
};

// this creates a vector of frequencies (as floats) from
// an input song that is a string representing a list of 
// elements, element ::= frequency "," duration
// each element in this float represents a single block,
// thus, if one frequency appears more than once, multiple of
// the same frequency are added. for sanity, every frequency is
// multiplied by LOOP_FREQUENCY 
std::vector<float> parse_song(char* input_song) {
  std::vector<float> output;
  char * token = strtok(input_song, delim);

  while (token != NULL) {
    float freq = atof(token);
    token = strtok(NULL, delim);
    
    if (token == NULL) { break; } // fix a weird issue of premature null
    
    int duration = atof(token);
    token = strtok(NULL, delim);

    // appends duration * LOOP_COUNT frequencies
    for (int i = 0; i < duration * LOOP_COUNT; i++) {
      output.push_back(freq);
    }
  }

  return output;
}

int min(int a, int b) {
  return a < b ? a: b;
}

// a struct for storing a song id and name
struct SongEntry {
  std::string id;
  std::string text;
};

// the delimeters separating a song entry
const char ENTRY_DELIM[] = ",\n";

// gets a vector of song entries from a string in the form
//  entries ::= (entry "\n")*
//  entries ::= id "," name
std::vector<SongEntry> getEntries(char *entries) {
  std::vector<SongEntry> output;
  char *token = strtok(entries, ENTRY_DELIM);

  while(token != NULL) {
    std::string id(token);
    token = strtok(NULL, ENTRY_DELIM);

    if (token == NULL) { break; }

    std::string text(token);

    output.push_back(SongEntry{id, text});
    token = strtok(NULL, ENTRY_DELIM);
  }

  return output;
}


std::vector<float> freqs;
std::list<Note> notes = {};
int note_idx = 0;
std::vector<SongEntry> menu;
uint16_t selectedSong = 0;
uint16_t songStart = 0;
uint16_t songEnd;

// TOF stuff
char song[6000] = {};
int ind = 0;
int count = 1;
int old = 0;

// variables for the TOF sensor
Adafruit_VL6180X sensor;
long mm,old_mm,older_mm,avg_mm;
const int buzzpin = 27;
uint32_t timer = 0;


void setup() {
  Serial.begin(115200); 

  // begin TFT
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);

  // setup buttons
  pinMode(TOP_PIN, INPUT_PULLUP);
  pinMode(BOT_PIN, INPUT_PULLUP);

  // enable PWM for TFT monitor
  ledcSetup(10, 60, 12);
  ledcAttachPin(13, 10);
  ledcWrite(10, 4095);

  initializeWifi();

  // bbegin LEDs
  strip.Begin();
  strip.Show();

  // get the list of possible songs from the server
  char request[100];
  sprintf(request, "GET https://608dev.net/sandbox/sc/kgarner/project/server.py HTTP/1.1\r\n");
  strcat(request, "Host: 608dev.net\r\n\r\n");
  do_http_request(host, request, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  menu = getEntries(response_buffer);
  songEnd = min(ENTRIES_PER_SCREEN, menu.size());

  sensor = Adafruit_VL6180X();
  
  Wire.begin();
  delay(50);
  if (imu.setupIMU(1)){ //try to connect to the IMU
    Serial.println("IMU Connected!");
  } else{
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  
  sensor.begin();

  // attach and enable the buzzer
  ledcSetup(0,2000,8);
  ledcAttachPin(buzzpin,0);
}

#define MENU 0
#define FREE_PLAY 1
#define GET_SONG 2
#define PLAY_OR_RESELECT 3
#define PLAY 4
#define RECORD 5
#define SHOW_SCORE 6

uint8_t state = MENU;

// keeps track of consistent state (score, selected song)
int score = 0;
char score_str[10] = {'0'};
char songId[15] = {};
long old_freq = 0;

//for if the player wants to hear the song
bool listen_song = false;

// plays a note from the sensor and updates the current value
void playNote(int inp_freq = 0) {
  if (listen_song == false){
    mm = sensor.readRange();
    older_mm = old_mm;
    old_mm = mm;
    
    // get an input from TOF and wrap it in the range C3 to C4
    // for all the semitones
    avg_mm = ((mm+old_mm+older_mm)/3);
    avg_mm = map(avg_mm,10,200,-9,3);
  
    // scale the range
    if (avg_mm < -9) {
      avg_mm = -9;
    } else if (avg_mm > 3) {
      avg_mm = 3;
    }

    for (int i = 0; i < PixelCount; i++) {
      strip.SetPixelColor(i, RgbColor(0, 0, 0));
    }
  
    int pixelIdx = (avg_mm + 9) * 2 + 1;
    
    strip.SetPixelColor(pixelIdx,RgbColor(255, 0, 255) );
    strip.SetPixelColor(pixelIdx + 1,RgbColor(255, 0, 255) );
  
    strip.Show();

    // scale to frequency using power and play song
    avg_mm = 440 * pow(2, avg_mm / 12.0);
  }
  
  else{
    avg_mm = inp_freq;
  }
  
  if (old_freq != avg_mm) {
    ledcWriteTone(0, avg_mm);
    ledcWrite(0,100);
    old_freq = avg_mm;
  }
}

// tells you the current note we have
Note note_play = Note(avg_mm, 95);

// handles playing notes for a song and scoring
// this should be called in a loop
void handleNotes() {
  note_idx++;

  // add Notes while we have not gone through the song
  if (note_idx < freqs.size()) {
    notes.push_back(Note(freqs[note_idx]));
  }

  // draw all of the notes
  for (Note &note : notes) {
    note.draw();
    if (note.get_y() == 95 && listen_song == true){
      playNote(note.frequency());

      for (int i = 0; i < PixelCount; i++) {
        strip.SetPixelColor(i, RgbColor(0, 0, 0));
      }

      int expected_note = find_closest_idx(note.frequency());

      int lowPixel = expected_note * 2;
      int highPixel = lowPixel + 3;

      Serial.println(lowPixel);

      
      strip.SetPixelColor(lowPixel,RgbColor(255, 0, 0) );
      strip.SetPixelColor(highPixel,RgbColor(255, 0, 0) );
      strip.Show();
    }
  }

  // while we have notes
  if (notes.size() > 0) {

    // have we started playing a note? if so, play the note
    if (note_idx > 4 && notes.size() > 1 && listen_song == false) {
      playNote();

          // score the current note with what is being played
      int expected_note = find_closest_idx(notes.front().frequency());
      int actual_note = find_closest_idx(avg_mm);

      int lowPixel = expected_note * 2;
      int highPixel = lowPixel + 3;


      strip.SetPixelColor(lowPixel,RgbColor(0, 255, 255) );
      strip.SetPixelColor(highPixel,RgbColor(0, 255, 255) );
      strip.Show();

      Serial.printf("Expected: (%d, %d), Actual: (%d, %d)\n", expected_note, 
        notes.front().frequency(), avg_mm, actual_note);

      
      if (expected_note == actual_note) {
        score += 10;
        itoa(score, score_str, 10);
      }
    }
    
    // remove Notes that have expired
    if (notes.front().get_y() > 115) {
      notes.pop_front();
    }
  }

  if (listen_song == false){
    // move the note to the current location we are playing
    note_play.draw(TFT_BLACK);
    note_play = Note(avg_mm, 95);
    note_play.draw(TFT_BLUE);
    tft.drawString(score_str, 0, 0); 
  }

  // if all the notes are done, go to show score mode
  if (note_idx >= freqs.size() && notes.size() == 0) {
    tft.fillScreen(TFT_BLACK);
    state = SHOW_SCORE;
    note_idx = 0;
    listen_song = false;
    ledcWrite(0,0);
  }
}

// messages for the main string
std::string selectionScreenOptions[5] = {"Free Play Mode", "Get Song and Compete", "Record Song"};

//
uint8_t selectedOption = 0; // main menu option
uint16_t recordCount = 0; // keeps track of how long we have recorded
bool powerSave = false; // should the screen be dark?

// handles showing a score. When a button is pressed,
// POSTs the score to the server and goes to the main menu
void showScore() {
  tft.setCursor(0, 0);
  tft.printf("Your score: %d, %s", score, songId);
  
  // button pressed
  if (bottomPin.update() != 0 || topPin.update() != 0) {
    selectedOption = 0;
    state = MENU;
    tft.fillScreen(TFT_BLACK);

    // post the score to the server
    char thing[200];
    sprintf(thing, "userName=bananas&songId=%s&score=%d",songId, score);
    char request[300];
    sprintf(request,"POST https://608dev.net/sandbox/sc/kgarner/project/score_server.py HTTP/1.1\r\n");
    sprintf(request+strlen(request),"Host: %s\r\n",host);
    strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request+strlen(request),"Content-Length: %d\r\n\r\n",strlen(thing));
    strcat(request,thing);
    do_http_request(host,request,response_buffer,OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);

    state = MENU;
  }
}

// handles the main menu loop. This prints the possible options,
// and a user can toggle between them using the top button. the
// bottom button commits a choice
void handleMainMenu() {
  tft.setCursor(0, 0);

  for (int i = 0; i < 3; i++) {
    tft.printf(selectedOption == i ? "-> ": "   ");
    tft.printf("%s\n", selectionScreenOptions[i].c_str());
  }

  int top = topPin.update();
  int bot = bottomPin.update();

  if (top == 2 || bot == 2) {
    // toggle powersave mode
    powerSave = !powerSave;
    int value = powerSave ? 511: 4095;
    ledcWrite(10, value);
    tft.fillScreen(TFT_BLACK);
  } else if (top != 0) {
    // toggle selected option
    selectedOption = (selectedOption + 1) % 3;
  } else if (bot!= 0) {
    // select the mode
    switch (selectedOption) {
      case 0:
        state = FREE_PLAY;
        tft.fillScreen(TFT_BLACK);
        break;
      case 1: 
        state = GET_SONG;
        tft.fillScreen(TFT_BLACK);
        break;
      case 2:
        memset(&song[0], 0, sizeof(song));
        recordCount = 0;
        state = RECORD;
        tft.fillScreen(TFT_BLACK);
        break;
    }
  }
}

// handles the free play mode. This shows the current
// note that is being played as well as makes the noise.
// can exit by pressing a button
void drawFreePlayScreen() {
  tft.setCursor(0, 0);
  tft.println("Free Play Mode!");
  tft.println("");
  tft.println("Press Any Button to Exit to Main Screen");

  playNote();

  tft.drawLine(0, 95, 160, 95, TFT_GREEN);
  tft.drawLine(0, 115, 160, 115, TFT_GREEN);

  for (int i = 0; i < NOTES.size(); i++) {
    int x = i * 12;
    tft.drawString(NOTES[i].c_str(), x + 1, 118);
    tft.drawLine(x, 95, x, 115, TFT_GREEN);
  }

  note_play.draw(TFT_BLACK);
  note_play = Note(avg_mm, 95);
  note_play.draw(TFT_BLUE);
  
  // exit free play
  if (bottomPin.update() != 0 || topPin.update() != 0) {
    selectedOption = 0;
    state = MENU;
    tft.fillScreen(TFT_BLACK);
    ledcWrite(0,0);
  }
}

// prompts the user to start playing a song, or go back
void drawPlayOrReselectScreen() {
  tft.setCursor(0, 0);
  tft.println("Short Press Upper Button to Start the Game. Long Press Upper Button to Listen to Song");
  tft.println("Press Lower Button to Go Back to the Song Selection Menu");

  // start a song
  int top_press = topPin.update();
  if (top_press != 0) {
    if (top_press == 2){
      listen_song = true;
    }
    state = PLAY;
    score = 0;
    itoa(score, score_str, 10);
    tft.fillScreen(TFT_BLACK);
    drawGameScreen();
  } else if (bottomPin.update() != 0) {
    state = GET_SONG;
    tft.fillScreen(TFT_BLACK);
  }
}

// handles the loop for recording. In this mode,
// the note is recorded and played. This can handle
// up to 15 seconds.
void drawRecordScreen() {
  playNote();

  tft.drawLine(0, 95, 160, 95, TFT_GREEN);
  tft.drawLine(0, 115, 160, 115, TFT_GREEN);

  for (int i = 0; i < NOTES.size(); i++) {
    int x = i * 12;
    tft.drawString(NOTES[i].c_str(), x + 1, 118);
    tft.drawLine(x, 95, x, 115, TFT_GREEN);
  }

  note_play.draw(TFT_BLACK);
  note_play = Note(avg_mm, 95);
  note_play.draw(TFT_BLUE);

  //are we done yet???
  if (recordCount < 150) {
    recordCount++;

    tft.setCursor(0, 0);
    float timeLeft = (150 - recordCount) / 10.0;
    
    tft.printf("Time remaining: %f seconds\n", timeLeft);
  
    // same note previously
    if (old == avg_mm){
      count += 1;
      old = avg_mm;
    }
    else {
      // add a new note
      char output[15]; 
      sprintf(output, "%d,%d;", old, count);
      strcat(song, output);
      old = avg_mm;
      count = 1;
    }
  }

  // when we are done, post
  if (recordCount == 150) {
    ledcWrite(0,0);
  
    char thing[1000];
    sprintf(thing, "songName=%s&musicString=%s", "a_banana", song );
    char request[1200];
    sprintf(request,"POST https://608dev.net/sandbox/sc/kgarner/project/server.py HTTP/1.1\r\n");
    sprintf(request+strlen(request),"Host: %s\r\n",host);
    strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request+strlen(request),"Content-Length: %d\r\n\r\n",strlen(thing));
    strcat(request,thing);
    do_http_request(host,request,response_buffer,OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);

    state = MENU;

    tft.fillScreen(TFT_BLACK);
  }
}

void handleGameState() {
  switch (state) {
    case MENU:
      handleMainMenu();
      break;
    case FREE_PLAY: 
      drawFreePlayScreen();
      // handle free play mode here! -TODO
      break;
    case GET_SONG:
      handleSongMenu();
      break;
    case PLAY_OR_RESELECT:
      drawPlayOrReselectScreen();
      break;
    case PLAY:
      handleNotes();
      break;
    case RECORD:
      drawRecordScreen();
      if (bottomPin.update() != 0) {
        selectedOption = 0;
        state = MENU;
        tft.fillScreen(TFT_BLACK);
      }
      break;
    case SHOW_SCORE:
      showScore();
      break;
  }
}

// handles displaying a song menu
void handleSongMenu() {
  tft.setCursor(0, 0);

  for (uint16_t idx = songStart; idx < songEnd; idx++) {
    tft.printf(selectedSong == idx ? "-> ": "   ");
    tft.printf("%s\n", menu[idx].text.c_str());
  }

  if (bottomPin.update() != 0) {
    sprintf(songId, menu[selectedSong].id.c_str());
    
    char request[150];
    sprintf(request, "GET https://608dev.net/sandbox/sc/kgarner/project/server.py?id=%s&format=esp HTTP/1.1\r\n", menu[selectedSong].id.c_str());
    strcat(request, "Host: 608dev.net\r\n\r\n");
    do_http_request(host, request, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    freqs = parse_song(response_buffer);
    
    state = PLAY_OR_RESELECT;
    tft.fillScreen(TFT_BLACK);
  } else if (topPin.update() != 0) {
    selectedSong++;

    if (selectedSong == menu.size()) {
      songStart = 0;
      songEnd = min(ENTRIES_PER_SCREEN, menu.size());
      selectedSong = 0;
    } else if (selectedSong % 16 == 0) {
      tft.fillScreen(TFT_BLACK);

      if (songStart + 16 > menu.size()) {
        songStart = 0;
        songEnd = min(ENTRIES_PER_SCREEN, menu.size());
      } else {
        songStart += 16;
        songEnd = min(songEnd + 16, menu.size());
      }
    }
  }
}

void loop() {
  imu.readAccelData(imu.accelCount);
  float x = imu.accelCount[0]*imu.aRes;
  primary_timer = millis();

  handleGameState();
  
  while(millis() - primary_timer < LOOP_PERIOD);
}

void drawGameScreen() {
  tft.fillScreen(TFT_BLACK);
  
  tft.drawLine(0, 95, 160, 95, TFT_GREEN);
  tft.drawLine(0, 115, 160, 115, TFT_GREEN);

  for (int i = 0; i < NOTES.size(); i++) {
    int x = i * 12;
    tft.drawString(NOTES[i].c_str(), x + 1, 118);
    tft.drawLine(x, 0, x, 115, TFT_GREEN);
  }

  tft.drawString("'", 154, 118);
  tft.drawString(score_str, 0, 0);
}
