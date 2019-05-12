
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

const uint16_t PixelCount = 120; // make sure to set this to the number of pixels in your strip
const uint8_t PixelPin = 14;  // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

void setup()
{
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    strip.Begin();
    strip.Show();


    for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
    {
        RgbColor color = RgbColor(random(255), random(255), random(255));
        strip.SetPixelColor(pixel, color);
    }
    strip.Show();
    Serial.println();
    Serial.println("Running...");
}




void clear_led() {
  for (int i = 0; i < PixelCount; i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
}

void loop()
{
   for(int i  = 0; i<PixelCount; i++){
    strip.SetPixelColor(i,RgbColor(255, 0, 255) );
    strip.Show();
    delay(50);
    clear_led();
   }
   
}
