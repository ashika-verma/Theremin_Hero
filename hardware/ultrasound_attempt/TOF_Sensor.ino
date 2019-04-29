/* This minimal example shows how to get single-shot range
measurements from the VL6180X.
The range readings are in units of mm. */
// STEP 1: Figure out how to get the duration of a frequency
// WIFI 
// POST/GET
// STEP 2: Concatonate frequencies and durations to a running array (frequency,duration;frequency,duration;frequency,duration;)
// STEP 3: POST to Kendal's server
#include <Wire.h>
#include <VL6180X.h>

VL6180X sensor;


long mm,old_mm,older_mm,avg_mm;
int buzzpin = 27;
int notes[] = {262,294,330,349,392,440,494,523};
char note_alpha[] = {'C','D','E','F','G','A','B','C'};
void setup() 
{
  Serial.begin(115200); 
  Wire.begin();
  sensor.init();
  sensor.configureDefault();
  sensor.setTimeout(500);
  ledcSetup(0,2000,8);
  ledcAttachPin(buzzpin,0);
}
char song[100000] = {};
int ind = 0;
int count = 1;
int old = 0;
char temp[15];
void loop() 
{ 
  mm = sensor.readRangeSingleMillimeters();
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
  else{
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

delay(100);
}
