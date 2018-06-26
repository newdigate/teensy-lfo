// Waveform Modulation Example - Create waveforms with 
// modulated frequency
//
// This example is meant to be used with 3 buttons (pin 0,
// 1, 2) and 2 knobs (pins 16/A2, 17/A3), which are present
// on the audio tutorial kit.
//   https://www.pjrc.com/store/audio_tutorial_kit.html
//
// Use an oscilloscope to view the 2 waveforms.
//
// Button0 changes the waveform shape
//
// Knob A2 changes the amount of frequency modulation
//
// Knob A3 varies the shape (only for Pulse & Variable Triangle)
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <Encoder.h>
#include "BAGuitar.h"
using namespace BAGuitar;

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#define TFT_SCLK 14  // SCLK can also use pin 14
#define TFT_MOSI 7  // MOSI can also use pin 7
#define TFT_CS   6  // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
#define TFT_DC    12  //  but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define TFT_RST   17  // RST can use any pin
#define SD_CS     4  // CS for SD card, can use any pin

#include <Adafruit_GFX.h>    // Core graphics library
#include <ST7735_t3.h> // Hardware-specific library
#include <SPI.h>

// Option 1: use any pins but a little slower
ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=842,814
AudioSynthWaveformModulated waveformMod1;   //xy=1063.5714797973633,785.9999866485596
AudioOutputI2S           i2s1;           //xy=1331,871
AudioRecordQueue         queue1;         //xy=1337.142765045166,953.1428050994873
AudioConnection          patchCord1(sine1, 0, waveformMod1, 0);
AudioConnection          patchCord2(waveformMod1, 0, i2s1, 0);
AudioConnection          patchCord3(waveformMod1, queue1);
AudioConnection          patchCord4(waveformMod1, 0, i2s1, 1);
// GUItool: end automatically generated code

BAAudioControlWM8731      codecControl;

Bounce button0 = Bounce(4, 15);
Bounce button1 = Bounce(24, 15);
Bounce button2 = Bounce(27, 15);

Encoder knobLeft(2, 3);
Encoder knobCenter(10, 16);
Encoder knobRight(25, 26);

int current_waveform=0;

long positionLeft  = -999;
long positionCenter  = -999;
long positionRight = -999;

extern const int16_t myWaveform[256];  // defined in myWaveform.ino

void setup() {
  Serial.begin(9600);
  pinMode(4, INPUT_PULLUP);
  pinMode(24, INPUT_PULLUP);
  pinMode(27, INPUT_PULLUP);

  delay(300);
  Serial.println("Waveform Modulation Test");
  
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Comment these out if not using the audio adaptor board.
  // If the codec was already powered up (due to reboot) power itd own first
  codecControl.disable();
  delay(100);
  AudioMemory(24);

  Serial.println("Enabling codec...\n");
  codecControl.enable();
  delay(100);
  // Confirgure both to use "myWaveform" for WAVEFORM_ARBITRARY
  waveformMod1.arbitraryWaveform(myWaveform, 172.0);

  // Configure for middle C note without modulation
  waveformMod1.frequency(261.63);
  waveformMod1.amplitude(1.0);
  sine1.frequency(1); // Sine waves are low frequency oscillators (LFO)

  current_waveform = WAVEFORM_TRIANGLE_VARIABLE;
  waveformMod1.begin(current_waveform);


  tft.initR(INITR_144GREENTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(2);
  tft.println("LFO-01");
  // uncomment to try modulating phase instead of frequency
  //waveformMod1.phaseModulation(720.0);
  queue1.begin();
}

long last_millis_scope =0;
void loop() {
  long current_millis = millis();
  
  if (queue1.available() && current_millis > last_millis_scope + 20) {
    last_millis_scope = current_millis;
    tft.fillScreen(ST7735_BLACK);
    int16_t buffer[128];
    memcpy(buffer, queue1.readBuffer(), 128);

    for (byte b = 1; b < 63; b++)
      tft.drawLine((b-1)*2, 64 - (buffer[b-1] >> 9),b*2, 64-(buffer[b] >> 9), ST7735_RED);
    
    queue1.freeBuffer();  
  }
  
  long newLeft, newRight, newCenter;
  newLeft = knobLeft.read();
  newRight = knobRight.read();
  newCenter = knobCenter.read();
  if (newLeft != positionLeft || newRight != positionRight || newCenter != positionCenter) {
    Serial.print("Left = ");
    Serial.print(newLeft);

    
    Serial.print(", Center = ");
    Serial.print(newCenter);
    
    Serial.print(", Right = ");
    Serial.print(newRight);
    Serial.println();
    positionLeft = newLeft;
    positionRight = newRight;
    positionCenter = newCenter;
  }
  
  // Read the buttons and knobs, scale knobs to 0-1.0
  button0.update();
  button1.update();
  button2.update();
  float knob_A2 = (float)positionLeft / 1023.0;
  float knob_A3 = (float)positionRight / 1023.0;

  // use Knobsto adjust the amount of modulation
  sine1.amplitude(knob_A2);
  sine1.frequency(knob_A3);

  // Button 0 or 2 changes the waveform type
  if (button0.fallingEdge() || button2.fallingEdge()) {
    switch (current_waveform) {
      case WAVEFORM_SINE:
        current_waveform = WAVEFORM_SAWTOOTH;
        Serial.println("Sawtooth");
        break;
      case WAVEFORM_SAWTOOTH:
        current_waveform = WAVEFORM_SAWTOOTH_REVERSE;
        Serial.println("Reverse Sawtooth");
        break;
      case WAVEFORM_SAWTOOTH_REVERSE:
        current_waveform = WAVEFORM_SQUARE;
        Serial.println("Square");
        break;
      case WAVEFORM_SQUARE:
        current_waveform = WAVEFORM_TRIANGLE;
        Serial.println("Triangle");
        break;
      case WAVEFORM_TRIANGLE:
        current_waveform = WAVEFORM_TRIANGLE_VARIABLE;
        Serial.println("Variable Triangle");
        break;
      case WAVEFORM_TRIANGLE_VARIABLE:
        current_waveform = WAVEFORM_ARBITRARY;
        Serial.println("Arbitary Waveform");
        break;
      case WAVEFORM_ARBITRARY:
        current_waveform = WAVEFORM_PULSE;
        Serial.println("Pulse");
        break;
      case WAVEFORM_PULSE:
        current_waveform = WAVEFORM_SAMPLE_HOLD;
        Serial.println("Sample & Hold");
        break;
      case WAVEFORM_SAMPLE_HOLD:
        current_waveform = WAVEFORM_SINE;
        Serial.println("Sine");
        break;
    }
    waveformMod1.begin(current_waveform);
  }
  
}

