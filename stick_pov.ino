#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <IRLibAll.h>
//#include <IRLibRecvPCI.h>
//#include <IRLibDecodeBase.h>
//#include <IRLib_P01_NEC.h>
//#include <IRLibCombo.h>

#define PIN 11
#define IR_PIN 5
#define NUM_LEDS 58

#define BTN_UP 16718055 // Up (0xFF18E7)
#define BTN_DOWN 16730805 // Down (0xFF4AB5)
#define BTN_LEFT 16716015 // Left (0xFF10EF)
#define BTN_RIGHT 16734885 // Right (0xFF5AA5)
#define BTN_OK 16726215 // OK (0xFF38C7)
#define BTN_ASTERISK 16738455 // * (0xFF6897)
#define BTN_POUND 16756815 // # (0xFFB04F)
#define BTN_0 16750695 // 0 (0xFF9867)
#define BTN_1 16753245 // 1 (0xFFA25D)
#define BTN_2 16736925 // 2 (0xFF629D)
#define BTN_3 16769565 // 3 (0xFFE21D)
#define BTN_4 16720605 // 4 (0xFF22DD)
#define BTN_5 16712445 // 5 (0xFF02FD)
#define BTN_6 16761405 // 6 (0xFFC23D)
#define BTN_7 16769055 // 7 (0xFFE01F)
#define BTN_8 16754775 // 8 (0xFFA857)
#define BTN_9 16748655 // 9 (0xFF906F)

const uint32_t BLACK = 0x000000; // GRB
const uint32_t RED_MEDIUM = 0x008800; // Medium Red
const uint32_t PURPLE = 0x008080;
const uint32_t BLUE = 0x0000FF;
const uint32_t GREEN = 0x800000;
const uint32_t RED = 0x00FF00;
const uint32_t PINK = 0xC0FFCB;
const uint32_t ORANGE = 0xA5FF00;
const uint32_t YELLOW = 0xFFFF00;
const uint32_t VIOLET = 0x0094D3;
const uint32_t INDIGO = 0x004B82;

const uint8_t defaultBrightness = 105; // Default is set to 50% of the brightness range
const uint8_t maxBrightness = 230; // 90% of 255
const uint8_t minBrightness = 20;
const uint8_t brightnessIncrement = 15;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// Create a receiver object to listen on pin 2
IRrecv myReceiver(IR_PIN);

// Create a decoder object
IRdecode myDecoder;

void setup() {
  Serial.begin(9600); // Connect with Serial monitor for testing purposes

  myReceiver.enableIRIn(); // start the receiver
  Serial.println("Ready to receive IR signals");
  
  strip.begin();
  strip.setBrightness(defaultBrightness);
  strip.show(); // Initialize all pixels to 'off'
}

void checkButtonPress() {
  if (myReceiver.getResults()) {
    myDecoder.decode();   // decode it
//    Serial.print("Protocol: ");
//    Serial.println(myDecoder.protocolNum);
//    Serial.print("Value: ");
//    Serial.println(myDecoder.value);

    if (myDecoder.protocolNum == 1) {
      switch(myDecoder.value) {
        case BTN_UP:
          increaseBrightness();
          break;
        case BTN_DOWN:
          decreaseBrightness();
          break;
        case BTN_LEFT:
          break;
        case BTN_RIGHT:
          break;
        case BTN_OK:
          break;
        case BTN_1:
          colorWipe(strip.Color(100, 0, 0), 20); // Red
          break;
        case BTN_2:
          colorWipe(strip.Color(0, 100, 0), 20); // Green
          break;
        case BTN_3:
          colorWipe(strip.Color(0, 0, 100), 20); // Blue
          break;
        case BTN_4:
          break;
        case BTN_5:
          break;
        case BTN_6:
          break;
        case BTN_7:
          break;
        case BTN_8:
          break;
        case BTN_9:
          break;
        case BTN_0:
          break;
        case BTN_ASTERISK:
          break;
        case BTN_POUND:
          break;
      }
    }
//    myDecoder.dumpResults(true);  // Now print results. Use false for less detail
    myReceiver.enableIRIn();  // restart receiver
  }
}

void increaseBrightness() {  
  if (strip.getBrightness() < maxBrightness) {
    if (strip.getBrightness() + brightnessIncrement >= maxBrightness) {
      strip.setBrightness(maxBrightness);
      Serial.println("Maximum Brightness");
    } else {
      strip.setBrightness(strip.getBrightness() + brightnessIncrement);
    }
  } else {
    Serial.println("Maximum Brightness");    
  }
  strip.show();
}

void decreaseBrightness() {
  if (strip.getBrightness() > minBrightness) {
    if (strip.getBrightness() - brightnessIncrement <= minBrightness) {
      strip.setBrightness(minBrightness);
      Serial.println("Minimum Brightness");
    } else {
      strip.setBrightness(strip.getBrightness() - brightnessIncrement);
    }
  } else {
    Serial.println("Minimum Brightness");    
  }
  strip.show();
}

// Set the pixels and show the column
void showColumn(uint32_t img_column[]) {
  for (int i=0; i < (NUM_LEDS); i++) {
    strip.setPixelColor(i, img_column[i]);
  }
  
  strip.show();
  checkButtonPress();

  delay(15);
}

void showPattern() {
  // Pattern creates vertical columns of alternating colors
  uint32_t pattern_column[NUM_LEDS] = {};
  uint32_t pattern_colors[] = {PURPLE, BLUE, GREEN, RED, PINK, ORANGE, YELLOW};
  int num_pattern_colors = (sizeof(pattern_colors) / sizeof(uint32_t));
  
  for (int i=0; i < num_pattern_colors; i++) {
    for (int j=0; j < NUM_LEDS; j++) {
      pattern_column[j] = pattern_colors[i];
    }
    showColumn(pattern_column);
  }
}

void loop() {
  
//  showPattern();
checkButtonPress();
  
  // Some example procedures showing how to display to the pixels:
//  colorWipe(strip.Color(25, 0, 0), 20); // Red
//   colorWipe(strip.Color(50, 50, 20), 200); // White
//  colorWipe(strip.Color(0, 255, 0), 20); // Green
//  colorWipe(strip.Color(0, 0, 255), 50); // Blue
//colorWipe(strip.Color(0, 0, 0, 255), 50); // White RGBW
  // Send a theater pixel chase in...
//  theaterChase(strip.Color(127, 127, 127), 50); // White
//  theaterChase(strip.Color(127, 0, 0), 50); // Red
//  theaterChase(strip.Color(0, 0, 127), 50); // Blue

//  rainbow(20);
//  rainbowCycle(20);
//  theaterChaseRainbow(50);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
