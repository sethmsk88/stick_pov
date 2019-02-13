#include <Adafruit_NeoPixel.h>
#include <IRremote.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
//#include <MemoryFree.h>
#include <EEPROM.h>

#define DATA_PIN 11
#define IR_PIN 5
#define NUM_LEDS 70
#define MAX_LEDS 255

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
#define BTN_REPEAT 0xFFFFFFFF // This IR value is sent when a button is being held down
#define MAX_TIME_VALUE 0xFFFFFFFF

// Each favorite saves two bytes worth of info, so each is allocated two addresses
// First address contains the index of the saved pattern
// Second address contains the speed delay of the saved pattern
// TODO: Give these memory addres variables a suffix like "ADDR"
const uint16_t FAV_0_ADDR = 0;
const uint16_t FAV_1_ADDR = 2;
const uint16_t FAV_2_ADDR = 4;
const uint16_t FAV_3_ADDR = 6;
const uint16_t FAV_4_ADDR = 8;
const uint16_t FAV_5_ADDR = 10;
const uint16_t FAV_6_ADDR = 12;
const uint16_t FAV_7_ADDR = 14;
const uint16_t FAV_8_ADDR = 16;
const uint16_t FAV_9_ADDR = 18;
const uint16_t NUM_LEDS_SAVED_ADDR = 20;
const uint16_t BRIGHTNESS_SAVED_ADDR = 21; // NOT BEING USED - Only needs one byte, so one address
const uint16_t LAST_PATTERN_SAVED_ADDR = 22; // NOT BEING USED - Only needs one byte, so one address

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

uint8_t defaultBrightness = 160;//105; // Default is set to 50% of the brightness range
const uint8_t maxBrightness = 230; // 90% of 255
const uint8_t minBrightness = 20;
const uint8_t brightnessIncrement = 2;

uint32_t patternColumn[NUM_LEDS] = {};
int selectedPattern = 8;
uint8_t numPatterns = 10;
boolean patternChanged = true;
boolean patternComplete = false; // used when a pattern should only show once
uint16_t pat_i_0 = 0; // an index to track progress of a pattern
uint16_t pat_i_1 = 0; // another index to track progress of a pattern
uint8_t speedDelay = 0; // ms of delay between showing columns
uint8_t maxSpeedDelay = 45;
uint8_t speedIncrement = 2;
boolean shortButtonPress = false;
boolean longButtonPress = false;
uint32_t lastButtonPress = 0;
unsigned long buttonPressStartTime = 0;
unsigned long longButtonPressTime = 1000; // 1.5 seconds
unsigned long lastIRSignalReceivedTime = 0;
unsigned long noIRSignalDelay = 150; // if there are no IR signals for this amount of time, then we can say there are no active IR signals
int patDirection = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// Create a receiver object to listen on pin 2
IRrecv myReceiver(IR_PIN);
decode_results IRresults;

//void types(uint16_t var) {Serial.println("Type is uint16_t");}
//void types(int32_t var) {Serial.println("Type is int32_t");}
//void types(uint32_t var) {Serial.println("Type is uint32_t");}

void setup() {
  Serial.begin(9600); // Connect with Serial monitor for testing purposes

  myReceiver.enableIRIn(); // start the receiver
  Serial.println("Ready to receive IR signals");

  applySavedSettings();
  
  strip.begin();
  strip.setBrightness(defaultBrightness);
  strip.show();
}

// NOT USED IN THIS VERSION
// Update the number of LEDs on the strip
void incrementLEDCount() {
  // TODO: Save the new number of LEDs to the EEPROM
  // TODO: Press * to decrement, and # to increment
  if (strip.numPixels() >= MAX_LEDS) {
    strip.updateLength((uint16_t)MAX_LEDS);
  } else {
    strip.updateLength(strip.numPixels() + 1);
    Serial.println(strip.numPixels());
  }
}

// NOT USED IN THIS VERSION
void decrementLEDCount() {
  // TODO: Save the new number of LEDs to the EEPROM
  // TODO: Press * to decrement, and # to increment
  if (strip.numPixels() <= 1) {
    strip.updateLength(0);
  } else {
    strip.updateLength(strip.numPixels() - 1);
  }
}

void applySavedSettings() {
  int patternIndex_addr;
  int speedDelay_addr;
  int unsetVal = 255; // the number stored at an address that is unset
  
  // if favorites have not yet been saved, set them to 0
  for (int i=0; i <= 9; i++) {
    patternIndex_addr = getFavoriteAddr(i, 0); // Load pattern for this favorite
    speedDelay_addr = getFavoriteAddr(i, 1); // Load speed delay for this favorite

    if (EEPROM.read(patternIndex_addr) == unsetVal) {
      EEPROM.update(patternIndex_addr, 0);
      EEPROM.update(speedDelay_addr, 0);
    }
  }

  // Apply saved brightness setting
  defaultBrightness = EEPROM.read(BRIGHTNESS_SAVED_ADDR);
}

// Save a favorite
void setFavorite(uint8_t i) {
  if (selectedPattern < 0) {
    Serial.println("ERROR: Cannot saved a pattern favorite whose index is negative");
    return;
  }
  
  int patternIndex_addr = getFavoriteAddr(i, 0);
  int speedDelay_addr = getFavoriteAddr(i, 1);
  
  EEPROM.update(patternIndex_addr, selectedPattern);
  EEPROM.update(speedDelay_addr, speedDelay);

  alertUser();

  Serial.println("Favorite " + (String)i + " saved:");
}

// Get a saved favorite, and make it active
void getFavorite(uint8_t i) {
  int patternIndex_addr = getFavoriteAddr(i, 0);
  int speedDelay_addr = getFavoriteAddr(i, 1);
  
  selectedPattern = EEPROM.read(patternIndex_addr);
  speedDelay = EEPROM.read(speedDelay_addr);
  
  resetIndexesFlags();
}

// Get address of favorite in EEPROM
// fav_i - Favorite number (0 - 9)
// attr_i - index of the favorite attribute (0 - 1)
int getFavoriteAddr(int fav_i, int attr_i) {
  switch (fav_i) {
    case 0:
      return FAV_0_ADDR + attr_i;
      break;
    case 1:
      return FAV_1_ADDR + attr_i;
      break;
    case 2:
      return FAV_2_ADDR + attr_i;
      break;
    case 3:
      return FAV_3_ADDR + attr_i;
      break;
    case 4:
      return FAV_4_ADDR + attr_i;
      break;
    case 5:
      return FAV_5_ADDR + attr_i;
      break;
    case 6:
      return FAV_6_ADDR + attr_i;
      break;
    case 7:
      return FAV_7_ADDR + attr_i;
      break;
    case 8:
      return FAV_8_ADDR + attr_i;
      break;
    case 9:
      return FAV_9_ADDR + attr_i;
      break;      
  }
}

void saveBrightness() {
  EEPROM.update(BRIGHTNESS_SAVED_ADDR, strip.getBrightness());
}

void checkButtonPress() {
  uint32_t IRVal;
  unsigned long currentTime = millis();
  
  // IR signal received
  if (myReceiver.decode(&IRresults)) {
    IRVal = IRresults.value;

//    Serial.println((String)IRVal);

    lastIRSignalReceivedTime = currentTime;

    // Check for long press
    // If it's not a repeat code, store the IR code, and start the button press timer
    if (IRVal != BTN_REPEAT) {
      lastButtonPress = IRVal;
      buttonPressStartTime = currentTime; // start button press timer

//      Serial.println("lastButtonPress");
//      debugButton(lastButtonPress);
    }
    else { // It is a repeat code
//      Serial.print("longButtonPress = ");
//      longButtonPress ? Serial.println("TRUE") : Serial.println("FALSE");
//      Serial.println("lastButtonPress = " + (String)lastButtonPress);
//      Serial.println("currentTime = " + (String)currentTime);
//      Serial.println("buttonPressStartTime + longButtonPressTime = " + (String)(buttonPressStartTime + longButtonPressTime));
      
      // Check to see if long button press
      if (!longButtonPress && (currentTime >= buttonPressStartTime + longButtonPressTime)) {
        longButtonPress = true;
        Serial.println("Long button press ON");
      }
    }

//    Serial.println("buttonPressStartTime = " + (String)buttonPressStartTime);

    myReceiver.enableIRIn();  // Restart receiver
  }
  else { // No IR signal received

//    Serial.println((String)currentTime);
    
    // Reset lastButtonPress if coming off of a long press
//    if (longButtonPress) {
//      lastButtonPress = 0; // an invalid IR value
//    }
//    longButtonPress = false; // reset longButtonPress

      // TESTING
//      if (longButtonPress) {
//        Serial.print("longButtonPress = ");
//        longButtonPress ? Serial.println("TRUE") : Serial.println("FALSE");
//        Serial.println("lastButtonPress = " + (String)lastButtonPress);
//        Serial.println("currentTime = " + (String)currentTime);
//        Serial.println("buttonPressStartTime + longButtonPressTime = " + (String)(buttonPressStartTime + longButtonPressTime));
//          Serial.println((String)currentTime + " >= " + (String)(lastIRSignalReceivedTime + noIRSignalDelay));
//          Serial.println("buttonPresStartTime+ longButtonPressTime = " + (String)(buttonPressStartTime + longButtonPressTime));
//      }


    if (longButtonPress && (currentTime >= lastIRSignalReceivedTime + noIRSignalDelay)) {
      longButtonPress = false;
      lastButtonPress = 0; // an invalid IR value
      buttonPressStartTime = MAX_TIME_VALUE / 2; // Set to an unreachable time (divide by two to prevent overflow when doing math with buttonPressStartTime)
      Serial.println("Long button press OFF");
    }      
    // If no signal has been received for noIRSignalDelay
    // if (!shortButtonPress && (currentTime >= buttonPressStartTime + noIRSignalDelay)) {
    else if (!shortButtonPress && (currentTime >= lastIRSignalReceivedTime + noIRSignalDelay)) {
      if (isValidIRValue(lastButtonPress)) {
        shortButtonPress = true;
//        Serial.println("Short button press");
      }
    }
  }

  if (shortButtonPress || longButtonPress) {
    switch(lastButtonPress) {
      case BTN_UP:
        shortButtonPress ? increaseSpeed() : increaseBrightness();
        break;
      case BTN_DOWN:
        shortButtonPress ? decreaseSpeed() : decreaseBrightness();
        break;
      case BTN_LEFT:
        prevPattern();
        break;
      case BTN_RIGHT:
        nextPattern();
        break;
      case BTN_OK:
        // Next pattern if short press, otherwise set to OFF pattern
        if (shortButtonPress) {
          nextPattern();
        } else {
          selectedPattern = -1;
        }
        break;
      case BTN_1:
        shortButtonPress ? getFavorite(1) : setFavorite(1);
        break;
      case BTN_2:
        shortButtonPress ? getFavorite(2) : setFavorite(2);
        break;
      case BTN_3:
        shortButtonPress ? getFavorite(3) : setFavorite(3);
        break;
      case BTN_4:
        shortButtonPress ? getFavorite(4) : setFavorite(4);
        break;
      case BTN_5:
        shortButtonPress ? getFavorite(5) : setFavorite(5);
        break;
      case BTN_6:
        shortButtonPress ? getFavorite(6) : setFavorite(6);
        break;
      case BTN_7:
        shortButtonPress ? getFavorite(7) : setFavorite(7);
        break;
      case BTN_8:
        shortButtonPress ? getFavorite(8) : setFavorite(8);
        break;
      case BTN_9:
        shortButtonPress ? getFavorite(9) : setFavorite(9);
        break;
      case BTN_0:
        shortButtonPress ? getFavorite(0) : setFavorite(0);
        break;
      case BTN_ASTERISK:
//        decrementLEDCount();
        break;
      case BTN_POUND:
        if (shortButtonPress) {
          changeDirection();
        }
//        incrementLEDCount();
        break;
    }

//    debugButton(lastButtonPress);

    // Reset shortButtonPress, since it should only perform its action once
    if (shortButtonPress) {
      shortButtonPress = false;
      lastButtonPress = 0; // an invalid IR value
    }
  }
}

// Check to see if IR signal is a valid button code
boolean isValidIRValue(uint32_t irValue) {
  switch (irValue) {
    case BTN_UP:
    case BTN_DOWN:
    case BTN_LEFT:
    case BTN_RIGHT:
    case BTN_OK:
    case BTN_1:
    case BTN_2:
    case BTN_3:
    case BTN_4:
    case BTN_5:
    case BTN_6:
    case BTN_7:
    case BTN_8:
    case BTN_9:
    case BTN_0:
    case BTN_ASTERISK:
    case BTN_POUND:
    case BTN_REPEAT:
//      Serial.println("Valid IR value");
      return true;
  }
//  Serial.println("Invalid IR value");
  return false;
}

void nextPattern() {
  if (selectedPattern < numPatterns - 1) {
    selectedPattern++;
  } else {
    selectedPattern = 0;
  }
  resetIndexesFlags();
  speedDelay = 0; // reset speed delay
  saveBrightness();
}

void prevPattern() {
  if (selectedPattern > 0) {
    selectedPattern--;
  } else {
    selectedPattern = numPatterns - 1;
  }
  resetIndexesFlags();
  speedDelay = 0; // reset speed delay
  saveBrightness();
}

// Increase brightness of pixels
void increaseBrightness() {  
  if (strip.getBrightness() < maxBrightness) {
    if (strip.getBrightness() + brightnessIncrement >= maxBrightness) {
      strip.setBrightness(maxBrightness);
      Serial.println("Maximum Brightness");
      alertUser();
    } else {
      strip.setBrightness(strip.getBrightness() + brightnessIncrement);
    }
    Serial.println("Brightness = " + (String)strip.getBrightness());
  } else {
    Serial.println("Maximum Brightness");    
    alertUser();
  }
  strip.show();
}

// Decrease brightness of pixels
void decreaseBrightness() {
  if (strip.getBrightness() > minBrightness) {
    if (strip.getBrightness() - brightnessIncrement <= minBrightness) {
      strip.setBrightness(minBrightness);
      Serial.println("Minimum Brightness");
      alertUser();
    } else {
      strip.setBrightness(strip.getBrightness() - brightnessIncrement);
    }
    Serial.println("Brightness = " + (String)strip.getBrightness());
  } else {
    Serial.println("Minimum Brightness");    
    alertUser();
  }
  strip.show();
}

// Increase speed of pattern
void increaseSpeed() {
  if (speedDelay <= 0) {
    speedDelay = 0;
    Serial.println("Maximum speed");
    alertUser();
  } else {
    // Prevent unsigned int math going negative
    if ((int)speedDelay - (int)speedIncrement < 0) {
      speedDelay = 0;
      Serial.println("Maximum speed");
      alertUser();
    } else {
      speedDelay -= speedIncrement;
    }
  }
  Serial.println("Speed Delay: " + (String)speedDelay);
}

// Decrease speed of pattern
void decreaseSpeed() {
  if (speedDelay >= maxSpeedDelay) {
    speedDelay = maxSpeedDelay;
    Serial.println("Minimum speed");
    alertUser();
  } else {
    speedDelay += speedIncrement;
  }
  Serial.println("Speed Delay: " + (String)speedDelay);
}

// Change direction of pattern
void changeDirection() {
  if (patDirection == 0) {
    patDirection = 1;
  } else {
    patDirection = 0;
  }
  resetIndexesFlags();
}

// Set the all pixels on the strip to the values in the patternColumn array
// and then show the pixels
void showColumn() {
  for (int i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, patternColumn[i]);
  }

  if (myReceiver.isIdle()) {
    strip.show();
  }

//  debugPatternColumn();
  
  delay(speedDelay); // tiny bit of flicker
}

void debugPatternColumn() {
  String litPixels = "";
  for (int i=0; i < strip.numPixels(); i++) {
    if (patternColumn[i] == BLACK) {
      litPixels += "0";
    } else {
      litPixels += "1";
    }
  }
  Serial.println(litPixels);
}

// Show the pattern that is currently selected
void showPattern() {
  uint32_t colorSet_0[] = {PURPLE, BLUE, GREEN, RED, PINK, ORANGE, YELLOW};
  uint32_t colorSet_1[] = {RED, GREEN};
  uint32_t colorSet_2[] = {RED, BLUE};
  uint32_t colorSet_3[] = {BLUE, GREEN};
  
  switch (selectedPattern) {
    case -1:
      patternOff();
      break;
    case 0:
      pattern0(colorSet_0, (sizeof(colorSet_0) / sizeof(uint32_t)));
      break;
    case 1:
      pattern1(RED, 10);
      break;
    case 2:
      pattern1(GREEN, 10);
      break;
    case 3:
      pattern0(colorSet_1, (sizeof(colorSet_1) / sizeof(uint32_t)));
      break;
    case 4:
      pattern0(colorSet_2, (sizeof(colorSet_2) / sizeof(uint32_t)));
      break;
    case 5:
      pattern0(colorSet_3, (sizeof(colorSet_3) / sizeof(uint32_t)));
      break;
    case 6:
      pattern4();
      break;
    case 7:
      pattern5(RED);
      break;
    case 8:
      pattern5(GREEN);
      break;
    case 9:
      pattern5(BLUE);
      break;
  }

//  Serial.print("Free memory = ");
//  Serial.println(freeMemory());
}

// Set all pixels to black, so very little power is used
void patternOff() {
  setAllPixels(BLACK);
}

// Create vertical columns of the colors listed below
// Note: Loops through 7 showColumn calls before returning
// TODO: Experiment with returning after every call of showColumn()
void pattern0(uint32_t patternColors[], int numPatternColors) {  
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

// Displays diamonds
void pattern2() {
  // Note: this solution prints the whole pattern before returning
  uint32_t color1 = RED;
  uint32_t color2 = GREEN;
  uint8_t row_i = 0;
    
  uint8_t height = 15;
  uint8_t c, k, space = 1;

  // if pattern is at the beginning
//  if (pat_i

  space = height - 1;

  while (row_i < strip.numPixels()) {
    // Top half, if printed
    // Left half, if on stick
    for (k = 1; k <= height; k++) {
      for (c = 1; c <= space; c++) {
        if (row_i >= strip.numPixels())
          break;
        Serial.print("0");
        patternColumn[row_i] = color1;
        row_i++;
      }
      for (c = 1; c <= 2*k-1;c++) {
        if (row_i >= strip.numPixels())
          break;
        Serial.print("1");
        patternColumn[row_i] = color2;
        row_i++;
      }
      for (c = 1; c <= space; c++) {
        if (row_i >= strip.numPixels())
          break;
        Serial.print("0");
        patternColumn[row_i] = color1;
        row_i++;
      }
      showColumn();
      if (row_i >= strip.numPixels())
        break;
          
      Serial.println("");
      space--;
    }
  
    // Bottom half, if printed
    // Right half, if on stick
    for (k = 1; k <= height - 1; k++) {
      for (c= 1; c <= space; c++) {
        if (row_i >= strip.numPixels())
          break;
        Serial.print("0");
        patternColumn[row_i] = color1;
        row_i++;
      }
      for (c = 1; c <= 2*(height - k)-1; c++) {
        if (row_i >= strip.numPixels())
          break;
        Serial.print("*");
        patternColumn[row_i] = color2;
        row_i++;
      }
      for (c= 1; c <= space; c++) {
        if (row_i >= strip.numPixels())
          break;
        Serial.print("0");
        patternColumn[row_i] = color1;
        row_i++;
      }
      if (row_i >= strip.numPixels())
        break;
      Serial.println("");
      space++;
    }
  }
}

// Stretched Diamond
// This function should stretch the pattern in the array pat by the factor stretch, and
// then flip it vertically and print the bottom half.
void pattern3() {
  int pixel_i = 0;
  uint32_t c1 = RED;
  uint32_t c2 = GREEN;
  int height = 7;
  int width = 13;
  int stretch = 3; // factor by which we are stretching the pattern vertically
  int pat[height][width] = {
    {0,0,0,0,0,0,1,0,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,0,0,0,0,0},
    {0,0,0,0,1,1,1,1,1,0,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,0,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1}
  };

  int r; // needs to be in this scope
  for (int c=0; c < width; c++) {
    // top half
    for (r=0; r < height; r++) {
      for (int repeat_i=0; repeat_i < stretch; repeat_i++) {
        if (pixel_i >= strip.numPixels())
          break;
        
        patternColumn[pixel_i] = pat[r][c] ? c1 : c2;
        pixel_i++;
      }
      if (pixel_i >= strip.numPixels())
        break;
    }
    // bottom half
    for (r=r-2; r > 0; r--) {
      for (int repeat_i=0; repeat_i < stretch; repeat_i++) {

        if (pixel_i >= strip.numPixels())
          break;
        
        patternColumn[pixel_i] = pat[r][c] ? c1 : c2;
        pixel_i++;
      }
      if (pixel_i >= strip.numPixels())
        break;
    }
    showColumn();
    
    if (pixel_i >= strip.numPixels())
      break;
  }
}

// Rainbow
void pattern4() {
  uint16_t pat_i_0_max = 256;
  uint16_t pat_i_1_max = strip.numPixels();

  // Reset pattern indexes if they exceed their max values
  if (pat_i_0 >= pat_i_0_max) {
    pat_i_0 = 0;
  }
  if (pat_i_1 >= pat_i_1_max) {
    pat_i_1 = 0;
  }
  while (pat_i_0 < 256) {
    while (pat_i_1 < pat_i_1_max) {
      patternColumn[pat_i_1] = Wheel(((pat_i_1 * 256 / strip.numPixels()) + pat_i_0) & 255);
      pat_i_1++;
    }
    pat_i_0++;
    showColumn();
    return;
  }
}

void pattern5(uint32_t color) {
  if (patDirection == 0) {
    // Reset indexes when needed
    if (pat_i_0 == 0) {
      pat_i_0 = strip.numPixels() - 1;
    }
    if (pat_i_1 == pat_i_0) {
      pat_i_0--;
      pat_i_1 = 0;
    }
  
    // Make a single pixel travel down the stick
    for (int travel_i = 0; travel_i < pat_i_0; travel_i++) {
      if (travel_i == pat_i_1) {
        patternColumn[travel_i] = color;
      } else {
        patternColumn[travel_i] = BLACK;
      }
    }
  
    // Turn on the lit pixels
    for (int lit_i = pat_i_0; lit_i < strip.numPixels(); lit_i++) {
      patternColumn[lit_i] = color;
    }
    
    showColumn();
    pat_i_1++;
  } else {
    // Reset indexes when needed
    if (patternChanged) {
      // Initialize this index when this when the pattern is first changed
      pat_i_1 = strip.numPixels() - 1;
      patternChanged = false;
    }
    if (pat_i_0 == strip.numPixels() - 1) {
      pat_i_0 = 0; // reset to 1 instead of 0 because of base case problem
    }
    if (pat_i_1 == pat_i_0) {
      pat_i_0++;
      pat_i_1 = strip.numPixels() - 1;
    }
  
    // Make a single pixel travel down the stick
    for (int travel_i = strip.numPixels() - 1; travel_i > pat_i_0; travel_i--) {
      if (travel_i == pat_i_1) {
        patternColumn[travel_i] = color;
      } else {
        patternColumn[travel_i] = BLACK;
      }
    }
  
    // Turn on the lit pixels
    for (int lit_i = pat_i_0; lit_i >= 0; lit_i--) {
      patternColumn[lit_i] = color;
    }
    
    showColumn();
    pat_i_1--;
  }
}

void setAllPixels(uint32_t color) {
  for (int i=0; i < strip.numPixels(); i++) {
    patternColumn[i] = color;
  }
  showColumn();
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

// Alert user by flashing stick red
void alertUser() {
  setAllPixels(RED);
  delay(500);
}

void debugButton(uint32_t buttonVal) {
  switch(buttonVal) {
    case BTN_UP:
      Serial.print("Button [UP]");
      break;
    case BTN_DOWN:
      Serial.print("Button [DOWN]");
      break;
    case BTN_LEFT:
      Serial.print("Button [LEFT]");
      break;
    case BTN_RIGHT:
      Serial.print("Button [RIGHT]");
      break;
    case BTN_OK:
      Serial.print("Button [OK]");
      break;
    case BTN_1:
      Serial.print("Button [1]");
      break;
    case BTN_2:
      Serial.print("Button [2]");
      break;
    case BTN_3:
      Serial.print("Button [3]");
      break;
    case BTN_4:
      Serial.print("Button [4]");
      break;
    case BTN_5:
      Serial.print("Button [5]");
      break;
    case BTN_6:
      Serial.print("Button [6]");
      break;
    case BTN_7:
      Serial.print("Button [7]");
      break;
    case BTN_8:
      Serial.print("Button [8]");
      break;
    case BTN_9:
      Serial.print("Button [9]");
      break;
    case BTN_0:
      Serial.print("Button [0]");
      break;
    case BTN_ASTERISK:
      Serial.print("Button [*]");
      break;
    case BTN_POUND:
      Serial.print("Button [#]");
      break;
  }
  if (longButtonPress) {
    Serial.println(" (LONG)");
  } else {
    Serial.println("");
  }
}
