#include<math.h>
#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <vector>
#include <string>
#include <list>
#include <WiFi.h> //Connect to WiFi Network
#include <Wire.h>
#include "Button.h"
#include "Adafruit_VL6180X.h"


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

const uint8_t LOOP_PERIOD = 100; //milliseconds
const uint8_t LOOP_COUNT = 4;

uint32_t primary_timer = 0;

const char NOTES[10] = "CDEFGABC";

// HTTP related vars
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POSTING_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 3000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 3000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

const int ENTRIES_PER_SCREEN = 16;

const int TOP_PIN = 16;
const int BOT_PIN = 5;
Button topPin(TOP_PIN);
Button bottomPin(BOT_PIN);

// wifi related vars
char host[] = "608dev.net";
//const char network[] = "6s08";  //SSID for 6.08 Lab
//const char password[] = "iesc6s08"; //Password for 6.08 Lab

const char network[] = "MIT";  //SSID for 6.08 Lab
const char password[] = ""; //Password for 6.08 Lab

const char delim[] = ",;";
const int notes_freq[] = {262,294,330,349,392,440,494,523};

int find_closest_idx(float freq) {
  int min_diff = INT32_MAX;
  int best_idx = 0;

  for (int i = 0; i < sizeof(notes_freq)/sizeof(int); i++) {
    int diff = abs(freq - notes_freq[i]);
    if (diff < min_diff) {
      min_diff = diff;
      best_idx = i;
    }
  }

  return best_idx;
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

class Note {
  private:
    int x;
    int y;
    int freq;

  public:
    Note(float freq, int inp_y = 15) {
      this->y = inp_y;
      int idx = find_closest_idx(freq);
      this->x = 20 * idx;
      this->freq = notes_freq[idx];
    }

    void draw(uint16_t inp_color = TFT_RED) {
      if (y > 15) {
        tft.fillRect(x + 1, y -19, 18, 18, TFT_BLACK);
      }
      if (y < 115) {
        tft.fillRect(x + 1, y + 1, 18, 18, inp_color);
      }
      y += 20;
    }
  
    void move_down() {
      this->y += 20;
    }

    int get_y() {
      return y;
    }

    int frequency() {
      return freq;
    }
};


std::vector<float> parse_song(char* input_song) {
  std::vector<float> output;
  char * token = strtok(input_song, delim);

  while (token != NULL) {
    float freq = atof(token);
    token = strtok(NULL, delim);
    
    if (token == NULL) { break; } // idk why this is here...
    
    int duration = atof(token);
    token = strtok(NULL, delim);

    for (int i = 0; i < duration * LOOP_COUNT; i++) {
      output.push_back(freq);
    }
  }

  return output;
}

struct SongEntry {
  std::string id;
  std::string text;
};

const char ENTRY_DELIM[] = ",\n";

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

Adafruit_VL6180X sensor;
long mm,old_mm,older_mm,avg_mm;
int buzzpin = 27;
uint32_t timer = 0;


void setup() {
  Serial.begin(115200); 
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(1);
  pinMode(TOP_PIN, INPUT_PULLUP);
  pinMode(BOT_PIN, INPUT_PULLUP);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);

  ledcSetup(10, 60, 12);
  ledcAttachPin(13, 10);
  ledcWrite(10, 4095);

  initializeWifi();

  char request[100];
  sprintf(request, "GET /sandbox/sc/kgarner/project/server.py HTTP/1.1\r\n");
  strcat(request, "Host: 608dev.net\r\n\r\n");
  do_http_request(host, request, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  menu = getEntries(response_buffer);
  songEnd = min(ENTRIES_PER_SCREEN, menu.size());

  sensor = Adafruit_VL6180X();
  
  Wire.begin();
  sensor.begin();
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

int score = 0;
char score_str[10] = {'0'};
char songId[15] = {};

void playNote() {
  mm = sensor.readRange();
  older_mm = old_mm;
  old_mm = mm;
  
  avg_mm = ((mm+old_mm+older_mm)/3);
  avg_mm = map(avg_mm,10,200,-9,3);

  if (avg_mm < -9) {
    avg_mm = -9;
  } else if (avg_mm > 3) {
    avg_mm = 3;
  }

  avg_mm = 440 * pow(2, avg_mm / 12.0);
  ledcWriteTone(0, avg_mm);
  ledcWrite(0,100);
}

Note note_play = Note(avg_mm, 95);

void handleNotes() {
  note_idx++;

  if (note_idx < freqs.size()) {
    notes.push_back(Note(freqs[note_idx]));
  }

  for (Note &note : notes) {
    note.draw();
  }

  if (notes.size() > 0) {
    if (note_idx > 4 && notes.size() > 1) {
      playNote();
      Serial.printf("expected: %d, actual: %d\n", notes.front().frequency(), avg_mm);
    }
    if (find_closest_idx(notes.front().frequency()) == find_closest_idx(avg_mm)){
        score += 10;
        Serial.printf("score: %d\n", score);
        itoa(score, score_str, 10);
      }
    if (notes.front().get_y() > 115) {
      notes.pop_front();
    }
  }

  note_play.draw(TFT_BLACK);
  note_play = Note(avg_mm, 95);
  note_play.draw(TFT_BLUE);
  tft.drawString(score_str, 0, 0);

  if (note_idx >= freqs.size() && notes.size() == 0) {
    tft.fillScreen(TFT_BLACK);
    state = SHOW_SCORE;
    note_idx = 0;
    ledcWrite(0,0);
  }
}

std::string selectionScreenOptions[5] = {"Free Play Mode", "Get Song and Compete", "Record Song"};
uint8_t selectedOption = 0;
uint16_t recordCount = 0;
bool powerSave = false;

void showScore() {
  tft.setCursor(0, 0);
  tft.printf("Your score: %d, %s", score, songId);
  
  if (bottomPin.update() != 0 || topPin.update() != 0) {
    selectedOption = 0;
    state = MENU;
    tft.fillScreen(TFT_BLACK);

    char thing[1000];
    sprintf(thing, "userName=bananas&songId=%s&score=%d",songId, score);
    char request[1200];
    sprintf(request,"POST /sandbox/sc/kgarner/project/score_server.py HTTP/1.1\r\n");
    sprintf(request+strlen(request),"Host: %s\r\n",host);
    strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request+strlen(request),"Content-Length: %d\r\n\r\n",strlen(thing));
    strcat(request,thing);
    do_http_request(host,request,response_buffer,OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);

    state = MENU;
  }
}

void handleMainMenu() {
  tft.setCursor(0, 0);
  for (int i = 0; i < 3; i++) {
    tft.printf(selectedOption == i ? "-> ": "   ");
    tft.printf("%s\n", selectionScreenOptions[i].c_str());
  }

  int top = topPin.update();
  int bot = bottomPin.update();


  if (top == 2 || bot == 2) {
    powerSave = !powerSave;
    int value = powerSave ? 511: 4095;
    ledcWrite(10, value);
    tft.fillScreen(TFT_BLACK);
  } else if (top != 0) {
    selectedOption = (selectedOption + 1) % 3;
  } else if (bot!= 0) {
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

void drawFreePlayScreen() {
  tft.setCursor(0, 0);
  tft.println("Free Play Mode!");
  tft.println("");
  tft.println("Press Any Button to Exit to Main Screen");

  playNote();
  
  if (bottomPin.update() != 0 || topPin.update() != 0) {
    selectedOption = 0;
    state = MENU;
    tft.fillScreen(TFT_BLACK);
    ledcWrite(0,0);
  }
}

void drawPlayOrReselectScreen() {
  tft.setCursor(0, 0);
  tft.println("Press Upper Button to Start the Game");
  tft.println("Press Lower Button to Go Back to the Song Selection Menu");

  if (topPin.update() != 0) {
    state = PLAY;
    score = 0;
    itoa(score, score_str, 10);
    tft.fillScreen(TFT_BLACK);
    drawGameScreen();
    // other stuff for score -TODO
  } else if (bottomPin.update() != 0) {
    state = GET_SONG;
    tft.fillScreen(TFT_BLACK);
  }
}

void drawRecordScreen() {
  playNote();
  
  if (recordCount < 150) {
    recordCount++;

    tft.setCursor(0, 0);
    float timeLeft = (150 - recordCount) / 10.0;
    
    tft.printf("Time remaining: %f seconds\n", timeLeft);
  
    if (old == avg_mm){
      count += 1;
      old = avg_mm;
    }
    else {
      char output[15]; 
      sprintf(output, "%d,%d;", old, count);
      strcat(song, output);
      old = avg_mm;
      count = 1;
    }
  }


  if (recordCount == 150) {
    ledcWrite(0,0);
  
    char thing[1000];
    sprintf(thing, "songName=%s&musicString=%s", "a_banana", song );
    char request[1200];
    sprintf(request,"POST /sandbox/sc/kgarner/project/server.py HTTP/1.1\r\n");
    sprintf(request+strlen(request),"Host: %s\r\n",host);
    strcat(request,"Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request+strlen(request),"Content-Length: %d\r\n\r\n",strlen(thing));
    strcat(request,thing);
    do_http_request(host,request,response_buffer,OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);

    state = MENU;
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

void handleSongMenu() {
  tft.setCursor(0, 0);

  for (uint16_t idx = songStart; idx < songEnd; idx++) {
    tft.printf(selectedSong == idx ? "-> ": "   ");
    tft.printf("%s\n", menu[idx].text.c_str());
  }

  if (bottomPin.update() != 0) {
    sprintf(songId, menu[selectedSong].id.c_str());
    
    char request[150];
    sprintf(request, "GET /sandbox/sc/kgarner/project/server.py?id=%s&format=esp HTTP/1.1\r\n", menu[selectedSong].id.c_str());
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
  primary_timer = millis();

  handleGameState();
  
  while(millis() - primary_timer < LOOP_PERIOD);
}

void drawGameScreen() {
  tft.fillScreen(TFT_BLACK);
  
  tft.drawLine(0, 95, 160, 95, TFT_GREEN);
  tft.drawLine(0, 115, 160, 115, TFT_GREEN);

  char note[5] = "";
  note[0] = NOTES[0];
  note[1] = '\0';
  tft.drawString(note, 8, 118);

  for (int x = 20; x < 160; x += 20) {
    note[0] = NOTES[x / 20];
    tft.drawLine(x, 0, x, 115, TFT_GREEN);
    tft.drawString(note, x + 8, 118);
  }
  tft.drawString("'", 154, 118);
  tft.drawString(score_str, 0, 0);
}
