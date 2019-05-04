#include<math.h>
#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <vector>
#include <string>
#include <list>


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

const uint8_t LOOP_PERIOD = 100; //milliseconds
const uint8_t LOOP_COUNT = 5;

uint32_t primary_timer = 0;

const char NOTES[10] = "CDEFGABC";

char network[] = "MIT";

char song[] = "261,3;293,1;329,1;349,1;391,1;440,1;493,1;523,4";
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

std::vector<float> freqs;
std::list<Note> notes = {};
int note_idx = 0;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  draw_game_screen();
  freqs = parse_song(song);
}


void loop() {
  primary_timer = millis();
  note_idx++;

  tft.setCursor(0, 0);

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
