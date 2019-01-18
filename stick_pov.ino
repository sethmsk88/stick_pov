#include <Adafruit_NeoPixel.h>
#include <IRremote.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

//#include <IRLibAll.h>


#define DATA_PIN 11
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

const uint8_t defaultBrightness = 60;//105; // Default is set to 50% of the brightness range
const uint8_t maxBrightness = 230; // 90% of 255
const uint8_t minBrightness = 20;
const uint8_t brightnessIncrement = 15;

uint32_t patternColumn[NUM_LEDS] = {};
uint8_t selectedPattern = 0;
boolean patternChanged = true;
boolean patternComplete = false; // used when a pattern should only show once
uint8_t pat_i_0 = 0; // an index to track progress of a pattern
uint8_t pat_i_1 = 0; // another index to track progress of a pattern

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// Create a receiver object to listen on pin 2
IRrecv myReceiver(IR_PIN);
decode_results IRresults;

// Create a decoder object
//IRdecode myDecoder;

void setup() {
  Serial.begin(9600); // Connect with Serial monitor for testing purposes

  myReceiver.enableIRIn(); // start the receiver
  Serial.println("Ready to receive IR signals");
  
  strip.begin();
  strip.setBrightness(defaultBrightness);
  strip.show();
}

// Lets check to see if this s
void checkButtonPress() {
  // If an IR signal was received
  if (myReceiver.decode(&IRresults)) {
    Serial.println("decode_type: " + (String)IRresults.decode_type);
    Serial.print("Value: ");
    Serial.println(IRresults.value, HEX);

    // If the IR signal is using the correct protocol
    switch(IRresults.value) {
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
        selectedPattern = 0;
        resetIndexesFlags();
        break;
      case BTN_2:
        selectedPattern = 1;
        resetIndexesFlags();
        break;
      case BTN_3:
        selectedPattern = 2;
        resetIndexesFlags();
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
    debugButton(IRresults.value);
//    clearPixels();
    
    myReceiver.enableIRIn();  // Restart receiver
  }
}

// Increase brightness of pixels
void increaseBrightness() {  
  if (strip.getBrightness() < maxBrightness) {
    if (strip.getBrightness() + brightnessIncrement >= maxBrightness) {
      strip.setBrightness(maxBrightness);
//      Serial.println("Maximum Brightness");
    } else {
      strip.setBrightness(strip.getBrightness() + brightnessIncrement);
    }
  } else {
//    Serial.println("Maximum Brightness");    
  }
  strip.show();
}

// Decrease brightness of pixels
void decreaseBrightness() {
  if (strip.getBrightness() > minBrightness) {
    if (strip.getBrightness() - brightnessIncrement <= minBrightness) {
      strip.setBrightness(minBrightness);
//      Serial.println("Minimum Brightness");
    } else {
      strip.setBrightness(strip.getBrightness() - brightnessIncrement);
    }
  } else {
//    Serial.println("Minimum Brightness");    
  }
  strip.show();
}

// Increase speed of pattern
void increaseSpeed() {
}

// Decrease speed of pattern
void decreaseSpeed() {
}

// Set the all pixels on the strip to the values in the patternColumn array
// and then show the pixels
void showColumn() {
  for (int i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, patternColumn[i]);
  }

  if (myReceiver.isIdle()) {
    strip.show();
  } else {
    Serial.println("Skipped show()");
  }
  
//  delay(2); // tiny bit of flicker
}

// Show the pattern that is currently selected
void showPattern() {
  switch (selectedPattern) {
    case 0:
      pattern0();
      break;
    case 1:
      pattern1(RED, 20);
      break;
    case 2:
      pattern1(GREEN, 10);
      break;
  }
}

// Create vertical columns of the colors listed below
// Note: Loops through 7 showColumn calls before returning
// TODO: Experiment with returning after every call of showColumn()
void pattern0() {  
  uint32_t patternColors[] = {PURPLE, BLUE, GREEN, RED, PINK, ORANGE, YELLOW};
  int numPatternColors = (sizeof(patternColors) / sizeof(uint32_t));
  
  for (uint8_t i=0; i < numPatternColors; i++) {
    for (uint8_t j=0; j < strip.numPixels(); j++) {
      patternColumn[j] = patternColors[i];
    }
    showColumn();
  }
}

// Color Wipe
void pattern1(uint32_t color, uint8_t msDelay) {  
  // If a pattern animation should only run once (e.g. Color Wipe)
  if (patternChanged || !patternComplete) {
    patternChanged = false;
    
    // fill up to the pattern index
    for (uint8_t i=0; i <= pat_i_0; i++) {
      patternColumn[i] = color;
    }
    pat_i_0++; // increase pattern index

    showColumn();
    delay(msDelay);    

    // Check to see if pattern is complete
    if (pat_i_0 == strip.numPixels())
      patternComplete = true;
  }
}

// Set all pixels to black
void clearPixels() {
  for (int i=0; i < strip.numPixels(); i++) {
    patternColumn[i] = BLACK;
    showColumn();
  }
}

// Reset indexes and flags
void resetIndexesFlags() {
  pat_i_0 = 0;
  pat_i_1 = 0;
  patternChanged = true;
  patternComplete = false;
}

void loop() {
  
  showPattern();
  checkButtonPress();

  // DEBUGGING
//  if (millis() % 50 == 0) {
//    Serial.println("Selected Pattern: " + (String)selectedPattern);
//  }

  
//checkButtonPress();
  
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

void debugButton(uint32_t buttonVal) {
  switch(buttonVal) {
    case BTN_UP:
      Serial.println("Button [UP]");
      break;
    case BTN_DOWN:
      Serial.println("Button [DOWN]");
      break;
    case BTN_LEFT:
      Serial.println("Button [LEFT]");
      break;
    case BTN_RIGHT:
      Serial.println("Button [RIGHT]");
      break;
    case BTN_OK:
      Serial.println("Button [OK]");
      break;
    case BTN_1:
      Serial.println("Button [1]");
      break;
    case BTN_2:
      Serial.println("Button [2]");
      break;
    case BTN_3:
      Serial.println("Button [3]");
      break;
    case BTN_4:
      Serial.println("Button [4]");
      break;
    case BTN_5:
      Serial.println("Button [5]");
      break;
    case BTN_6:
      Serial.println("Button [6]");
      break;
    case BTN_7:
      Serial.println("Button [7]");
      break;
    case BTN_8:
      Serial.println("Button [8]");
      break;
    case BTN_9:
      Serial.println("Button [9]");
      break;
    case BTN_0:
      Serial.println("Button [0]");
      break;
    case BTN_ASTERISK:
      Serial.println("Button [*]");
      break;
    case BTN_POUND:
      Serial.println("Button [#]");
      break;
  }
}
