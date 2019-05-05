#include<math.h>
#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <vector>
#include <string>
#include <list>
#include <WiFi.h> //Connect to WiFi Network
#include <Wire.h>


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

class Button{
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
    switch (state) {
      case 0:
        if (button_pressed) {
          state = 1;
          t_of_button_change = millis();
        }
        break;
      case 1:
        if (button_pressed) {
          if (millis() - t_of_button_change >= debounce_time) {
            state = 2;
            t_of_state_2 = millis();
          }
        } else {
          t_of_button_change = millis();
          state = 0;
        }
        break;
      case 2:
        if (button_pressed) {
          if (millis() - t_of_state_2 >= long_press_time) {
            state = 3;
          }
        } else {
          t_of_button_change = millis();
          state = 4;
        }
        break;
      case 3:
        if (!button_pressed) {
          t_of_button_change = millis();
          state = 4;
        }
        break;
      case 4:
        if (button_pressed) {
          if (millis() - t_of_state_2 < long_press_time) {
            state = 2;
          } else {
            state = 3;
          }
          t_of_button_change = millis();
        } else {
          if (millis() - t_of_button_change >= debounce_time) {
            state = 0;
            if (millis() - t_of_state_2 < long_press_time) {
              flag = 1;
            } else {
              flag = 2;
            }
          }
        }
    }
    return flag;
  }
};

const int ENTRIES_PER_SCREEN = 16;

const int TOP_PIN = 16;
const int BOT_PIN = 5;
Button topPin(TOP_PIN);
Button bottomPin(BOT_PIN);

// wifi related vars
char host[] = "608dev.net";
const char network[] = "MIT";  //SSID for 6.08 Lab
const char password[] = ""; //Password for 6.08 Lab

char song[] = "261,3;293,1;329,1;349,1;391,1;440,1;493,1;523,1";
const char delim[] = ",;";
const int notes_freq[] = {262,294,330,349,392,440,494,523};

int find_closest(float freq) {
  int min_diff = INT32_MAX;
  int best_idx = 0;

  for (int i = 0; i < sizeof(notes_freq)/sizeof(int); i++) {
    int diff = abs(freq - notes_freq[i]);
    if (diff < min_diff) {
      min_diff = diff;
      best_idx = i;
    }
  }

  return 8 + best_idx * 20;
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
}

class Note {
  private:
    int x;
    int y;

  public:
    Note(float literal) {
      this->y = 15;
      this->x = find_closest(literal);
    }

    void draw() {
      if (y > 15) {
        tft.fillRect(x - 7, y -19, 18, 18, TFT_BLACK);
      }
      if (y < 115) {
        tft.fillRect(x - 7, y + 1, 18, 18, TFT_RED);
      }
      y += 20;
    }
  
    void move_down() {
      this->y += 20;
    }

    int get_y() {
      return y;
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

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(1);
  pinMode(TOP_PIN, INPUT_PULLUP);
  pinMode(BOT_PIN, INPUT_PULLUP);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);

  initializeWifi();

  char request[100];
  sprintf(request, "GET /sandbox/sc/kgarner/project/server.py HTTP/1.1\r\n");
  strcat(request, "Host: 608dev.net\r\n\r\n");
  do_http_request(host, request, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  menu = getEntries(response_buffer);
  songEnd = min(ENTRIES_PER_SCREEN, menu.size());
}

#define MENU 0
#define PLAY 1

uint8_t state = MENU;

void handleNotes() {
  note_idx++;

  if (note_idx < freqs.size()) {
    notes.push_back(Note(freqs[note_idx]));
  }

  for (Note &note : notes) {
    note.draw();
  }

  if (notes.size() > 0) {
    if (notes.front().get_y() > 115) {
      notes.pop_front();
    }
  }

  if (note_idx >= freqs.size() && notes.size() == 0) {
    tft.fillScreen(TFT_BLACK);
    state = MENU;
    note_idx = 0;
  }
}

void handleMenu() {
  tft.setCursor(0, 0);

  for (uint16_t idx = songStart; idx < songEnd; idx++) {
    tft.printf(selectedSong == idx ? "-> ": "   ");
    tft.printf("%s\n", menu[idx].text.c_str());
  }

  if (bottomPin.update() != 0) {
    char request[150];
    sprintf(request, "GET /sandbox/sc/kgarner/project/server.py?id=%s&format=esp HTTP/1.1\r\n", menu[selectedSong].id.c_str());
    strcat(request, "Host: 608dev.net\r\n\r\n");
    do_http_request(host, request, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    freqs = parse_song(response_buffer);
    
    state = PLAY;
    draw_game_screen();
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

  switch(state) {
    case MENU: 
      handleMenu();
      break;
    case PLAY:
      handleNotes();
      break;
  }
  
  while(millis() - primary_timer < LOOP_PERIOD);
}

void draw_game_screen() {
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
}
