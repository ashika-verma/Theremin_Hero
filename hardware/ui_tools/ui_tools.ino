#include<math.h>
#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <map>
#include <vector>
#include <string>


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

const uint8_t LOOP_PERIOD = 100; //milliseconds
uint32_t primary_timer = 0;

const char NOTES[10] = "CDEFGABC";

char network[] = "MIT";


void setup() {
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  
  draw_game_screen();
}

void loop() {
  primary_timer = millis();
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
}

class Note {
  private:
    static std::map<char, int> x_positions = {{'A', 8}, {'B', 28}, {'C',48}, {'D',68}, 
                                              {'E', 88}, {'F', 108}, {'G', 128}, {'H', 148}};
    static std::map<char, int> frequencies = {{'A', 8}, {'B', 28}, {'C',48}, {'D',68}, 
                                              {'E', 88}, {'F', 108}, {'G', 128}, {'H', 148}};
    int x;
    int y;
    int freq;

  public:
    Note(char literal) {
      y = 0;
      x = Note::x_positions.at(literal);
      freq = Note::frequencies.at(literal);
    }
  
    void move_down() {
      y += 25;
    }

    int get_x() {
      return x;
    }

    int get_y() {
      return y;
    }

    int get_freq() {
      return freq;
    }
}

void draw_current_notes(std::vector<Note> notes) {
  for (Note note : notes) {
    tft.drawString("$", note.get_x(), note.get_y());
  }
}
