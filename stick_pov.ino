#include <Adafruit_NeoPixel.h>
#include <IRremote.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
//#include <MemoryFree.h>
#include <EEPROM.h>

#include "RemoteControlRoku.cpp"

#define DATA_PIN 11
#define IR_PIN 5
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
const uint16_t NUM_LEDS_SAVED_ADDR = 10;
const uint16_t BRIGHTNESS_SAVED_ADDR = 11;
const uint16_t LAST_PATTERN_SAVED_ADDR = 12; // NOT BEING USED - Only needs one byte, so one address

const uint32_t BLACK = 0x000000; // GRB
const uint32_t RED = 0x00FF00;
const uint32_t GREEN = 0x800000;
const uint32_t BLUE = 0x0000FF;
const uint32_t YELLOW = 0xFFFF00;
const uint32_t CYAN = 0x9900FF;
const uint32_t MAGENTA = 0x00FFFF;
const uint32_t PURPLE = 0x008080;
const uint32_t ATTRACTIVE_PURPLE = 0x004d99;
const uint32_t VIOLET = 0x0094D3;
const uint32_t INDIGO = 0x004B82;
const uint32_t PINK = 0xC0FFCB;
const uint32_t ORANGE = 0x90FF00;

const uint32_t COLORS[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, PURPLE, ATTRACTIVE_PURPLE, VIOLET, INDIGO, PINK, ORANGE};
const int COLORS_POV[][2] = {{0,1},{0,2},{1,2}}; // combinations of color indexes for POV

uint8_t defaultBrightness = 160;//105; // Default is set to 50% of the brightness range
const uint8_t maxBrightness = 230; // 90% of 255
const uint8_t minBrightness = 20;
const uint8_t brightnessIncrement = 5;
uint8_t numLEDs = 50;
const uint8_t maxLEDs = 100;

uint32_t patternColumn[maxLEDs] = {};
int selectedPatternIdx = 0; // defaul pattern index
int selectedPatternColorIdx = 0; // default color index
uint8_t numPatterns = 31;
boolean patternChanged = true;
boolean patternComplete = false; // used when a pattern should only show once
uint16_t pat_i_0 = 0; // an index to track progress of a pattern
uint16_t pat_i_1 = 0; // another index to track progress of a pattern
uint16_t pat_i_2 = 0; // another index to track progress of a pattern
uint8_t speedDelay = 0; // ms of delay between showing columns
uint8_t maxSpeedDelay = 45;
const int POVSpeedDelayDefault = 16;
const int POVSpeedDelay = 16;
uint8_t speedIncrement = 1;
uint32_t lastButtonPress = 0;
uint32_t pendingButtonPress = 0; // IR value for button press
unsigned long buttonPressStartTime = 0;
unsigned long noIRSignalDelay = 150; // if there are no IR signals for this amount of time, then we can say there are no active IR signals
int patDirection = 0;
boolean patternReverse = false;
uint8_t tempSavedBrightness = 0; // used for patterns that alter brightness (must init to 0)

unsigned long buttonHoldStartTime = 0;

Adafruit_NeoPixel strip;

// Create a receiver object to listen on pin 2
IRrecv myReceiver(IR_PIN);
decode_results IRresults;

// Using polymorphism to find variable types
//void types(uint16_t var) {Serial.println("Type is uint16_t");}
//void types(int32_t var) {Serial.println("Type is int32_t");}
//void types(uint32_t var) {Serial.println("Type is uint32_t");}

void setup() {
  Serial.begin(9600); // Connect with Serial monitor for testing purposes
  Serial.println("Serial Monitor Ready");
  
  myReceiver.enableIRIn(); // start the receiver

  applySavedSettings();
  strip = Adafruit_NeoPixel(numLEDs, DATA_PIN, NEO_GRB + NEO_KHZ800);

  getFavorite(0); // Set stick to HOME pattern

  strip.begin();
  strip.setBrightness(defaultBrightness);
  strip.show();

  randomSeed(analogRead(0));
}

// Update the number of LEDs on the strip
void incrementLEDCount() {
  // Increase number LEDs in strip object
  if (strip.numPixels() >= maxLEDs) {
    strip.updateLength((uint16_t)maxLEDs);
  } else {
    strip.updateLength(strip.numPixels() + 1);
    // Serial.println("Num LEDS: " + (String)strip.numPixels());
  }
  strip.show();

  // Save number of LEDs to non-volatile memory
  EEPROM.update(NUM_LEDS_SAVED_ADDR, (uint8_t)strip.numPixels());
}

void decrementLEDCount() {
  if (strip.numPixels() <= 1) {
    strip.updateLength(1); // Don't ever turn off all LEDs. This could be confusing to the User
  } else {
    // Turn off highest LED
    patternColumn[strip.numPixels() - 1] = BLACK;
    showColumn();
    
    strip.updateLength(strip.numPixels() - 1);
    // Serial.println("Num LEDS: " + (String)strip.numPixels());
  }  
  strip.show();

  // Serial.println("Saving number of LEDs " + (String)strip.numPixels());

  // Save number of LEDs to non-volatile memory
  EEPROM.update(NUM_LEDS_SAVED_ADDR, (uint8_t)strip.numPixels());
}

void applySavedSettings() {
  int patternIndex_addr;
  int speedDelay_addr;
  int unsetVal = 255; // the number stored at an address that is unset
  
  // if favorites have not yet been saved, set them to 0
  int numFavorites = 5;
  for (int i=0; i < numFavorites; i++) {
    patternIndex_addr = getFavoriteAddr(i, 0); // Load pattern for this favorite
    speedDelay_addr = getFavoriteAddr(i, 1); // Load speed delay for this favorite

    if (EEPROM.read(patternIndex_addr) == unsetVal) {
      EEPROM.update(patternIndex_addr, 0);
      EEPROM.update(speedDelay_addr, 0);
    }
  }

  // Reset EEPROM values - ONLY RUN ONCE
  // ONLY activate this if the usage of memory addresses has changed in the EEPROM
  // for (int i=0; i < 22; i++) {
  //   EEPROM.update(i, 255);
  // }

  // Apply saved brightness setting
  defaultBrightness = EEPROM.read(BRIGHTNESS_SAVED_ADDR);

  // Serial.println("Applying saved number of LEDs " + (String)EEPROM.read(NUM_LEDS_SAVED_ADDR));

  // Apply saved number of LEDs for the stick
  if (EEPROM.read(NUM_LEDS_SAVED_ADDR) != unsetVal) {
    numLEDs = EEPROM.read(NUM_LEDS_SAVED_ADDR);
  }
}

// Save a favorite
void setFavorite(uint8_t i) {
  if (selectedPatternIdx < 0) {
    Serial.println("ERROR: Cannot save a pattern favorite whose index is negative");
    return;
  }
  
  int patternIndex_addr = getFavoriteAddr(i, 0);
  int speedDelay_addr = getFavoriteAddr(i, 1);
  
  EEPROM.update(patternIndex_addr, selectedPatternIdx);
  EEPROM.update(speedDelay_addr, speedDelay);

  alertUser();

  Serial.println("Favorite " + (String)i + " saved:");
}

// Get a saved favorite, and make it active
void getFavorite(uint8_t i) {
  int patternIndex_addr = getFavoriteAddr(i, 0);
  int speedDelay_addr = getFavoriteAddr(i, 1);
  
  selectedPatternIdx = EEPROM.read(patternIndex_addr);
  speedDelay = EEPROM.read(speedDelay_addr);
  
  resetIndexesFlags();
}

// Get address of favorite in EEPROM
// fav_i - Favorite number (0 - 4)
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
  }
}

void saveBrightness() {
  EEPROM.update(BRIGHTNESS_SAVED_ADDR, strip.getBrightness());
}

void checkButtonPress() { 
  uint32_t IRVal;
  uint32_t buttonActionVal;
  unsigned long currentTime = millis();
  bool buttonAction = false; // whether or not to perform button action
  bool buttonHold = false; // whether or not a button is being held down

  // Serial.println((String)currentTime); // DEBUG

  // Number of milliseconds that must pass after a button press without receiving a button hold code,
  // before it is considered a button press instead of a button hold.
  int buttonPressThreshold = 200;

  // Number of milliseconds that the user must hold a button before the button hold action is performed
  int buttonHoldThreshold = 1000;
  
  // IR signal received
  if (myReceiver.decode(&IRresults)) {
    IRVal = IRresults.value;


    // Serial.println("IR Received [" + RemoteControlRoku::getBtnDescription(IRVal) + "]"); // DEBUG
    // Serial.println((String)IRVal);

    if (RemoteControlRoku::isButtonPress(IRVal)) {
      // Button presses must wait for a short amount of time (i.e. buttonPressThreshold) before action is performed. This is
      // so we can check to see if the user is holding the button
      lastButtonPress = IRVal;
      pendingButtonPress = IRVal;
      buttonPressStartTime = currentTime;

    } else if (RemoteControlRoku::isButtonHold(IRVal)) {
      // Serial.println("lastButtonPress = " + RemoteControlRoku::getBtnDescription(lastButtonPress)); // DEBUG
      // if the lastButtonPress was NOT a button hold, set the buttonHoldStartTime
      if (RemoteControlRoku::isButtonPress(lastButtonPress)) {
        buttonHoldStartTime = buttonPressStartTime;
      }
      lastButtonPress = IRVal;

      // if buttonHoldThreshold has passed, trigger button hold action
      if (currentTime - buttonHoldStartTime >= buttonHoldThreshold) {
        buttonHold = true;
        buttonAction = true;
      }
    } else {
      // Invalid IR value
    }

    myReceiver.enableIRIn();  // Restart receiver
  }
  else {
    // No IR signal received

    // BEGIN DEBUG
    // if (RemoteControlRoku::isButtonPress(pendingButtonPress)) {
      // Serial.println((String)currentTime + " - " + (String)buttonPressStartTime + " = " + (String)(currentTime - buttonPressStartTime));
    // }
    // END DEBUG

    // If there is a pending button press, but a button hold code was received, reset the buttonPressStartTime value
    if (RemoteControlRoku::isButtonPress(pendingButtonPress) && RemoteControlRoku::isButtonHold(lastButtonPress)) {
        buttonPressStartTime = currentTime;
    }

    // if the last IR code was a button press, check to see if the threshold time has passed
    if (RemoteControlRoku::isButtonPress(pendingButtonPress)
      && (currentTime - buttonPressStartTime >= buttonPressThreshold)) {
        buttonAction = true;
    }
  }  

  // If a button action is ready to be performed (i.e. if the wait threshold has passed)
  if (buttonAction) {
    
    // Use buttonHold to determine which stored button code to use for the action (e.g. the press, or the hold)
    buttonActionVal = buttonHold ? lastButtonPress : pendingButtonPress;

    switch(buttonActionVal) {
      case RemoteControlRoku::BTN_UP:
        nextPattern();
        break;
      case RemoteControlRoku::BTN_DOWN:
        prevPattern();
        break;
      case RemoteControlRoku::BTN_LEFT:
      case RemoteControlRoku::BTN_LEFT_HOLD:
        changeColor(-1); // Change pattern to prev color
        break;
      case RemoteControlRoku::BTN_RIGHT:
      case RemoteControlRoku::BTN_RIGHT_HOLD:
        changeColor(1); // Change pattern to next color
        break;
      case RemoteControlRoku::BTN_POWER:
        // Power button OFF turns all pixels off
        // Power button ON set stick to HOME favorite mode
        if (selectedPatternIdx > -1) {
          selectedPatternIdx = -1;
        } else {
          getFavorite(0); // Set to HOME
        }
        break;
      case RemoteControlRoku::BTN_ASTERISK:
        incrementLEDCount();
        break;
      case RemoteControlRoku::BTN_ASTERISK_HOLD:
        decrementLEDCount();
        break;
      case RemoteControlRoku::BTN_HOME:
        getFavorite(0);
        break;
      case RemoteControlRoku::BTN_HOME_HOLD:
        setFavorite(0);
        break;
      case RemoteControlRoku::BTN_MEDIA_0:
        getFavorite(1);
        break;
      case RemoteControlRoku::BTN_MEDIA_0_HOLD:
        setFavorite(1);
        break;
      case RemoteControlRoku::BTN_MEDIA_1:
        getFavorite(2);
        break;
      case RemoteControlRoku::BTN_MEDIA_1_HOLD:
        setFavorite(2);
        break;
      case RemoteControlRoku::BTN_MEDIA_2:
        getFavorite(3);
        break;
      case RemoteControlRoku::BTN_MEDIA_2_HOLD:
        setFavorite(3);
        break;
      case RemoteControlRoku::BTN_MEDIA_3:
        getFavorite(4);
        break;
      case RemoteControlRoku::BTN_MEDIA_3_HOLD:
        setFavorite(4);
        break;
      case RemoteControlRoku::BTN_VOL_UP:
      case RemoteControlRoku::BTN_VOL_UP_HOLD:
        increaseBrightness();
        break;
      case RemoteControlRoku::BTN_VOL_DOWN:
      case RemoteControlRoku::BTN_VOL_DOWN_HOLD:
        decreaseBrightness();
        break;
      case RemoteControlRoku::BTN_RETURN:
        changeDirection();
        break;
      case RemoteControlRoku::BTN_FASTFORWARD:
      case RemoteControlRoku::BTN_FASTFORWARD_HOLD:
        increaseSpeed();
        break;
      case RemoteControlRoku::BTN_REWIND:
      case RemoteControlRoku::BTN_REWIND_HOLD:
        decreaseSpeed();
        break;
    }

    pendingButtonPress = 0; // clear value

    Serial.println("Button [" + RemoteControlRoku::getBtnDescription(buttonActionVal) + "]"); // DEBUG
  }
}

void nextPattern() {
  if (selectedPatternIdx < numPatterns - 1) {
    selectedPatternIdx++;
  } else {
    selectedPatternIdx = 0;
  }
  resetIndexesFlags();
  saveBrightness(); // save stick brightness if it has changed

  Serial.println("selectedPatternIdx = " + (String)selectedPatternIdx); // DEBUG
}

void prevPattern() {
  if (selectedPatternIdx > 0) {
    selectedPatternIdx--;
  } else {
    selectedPatternIdx = numPatterns - 1;
  }
  resetIndexesFlags();
  saveBrightness(); // save stick brightness if it has changed
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
    showColumn();
  } else {
    Serial.println("Maximum Brightness");    
    alertUser();
  }
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
    showColumn();
  } else {
    Serial.println("Minimum Brightness");    
    alertUser();
  }
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

// Change color of pattern if it can be manually changed
void changeColor(int colorIndexModifier) {
  int numColors = sizeof(COLORS) / sizeof(*COLORS);
  int numPOVColors = sizeof(COLORS_POV) / sizeof(*COLORS_POV);
  selectedPatternColorIdx += colorIndexModifier;

  // Wrap around pattern color indexes
  if (selectedPatternColorIdx < 0) {
    selectedPatternColorIdx = numColors + numPOVColors - 1;
  } else if (selectedPatternColorIdx >= numColors + numPOVColors) {
    selectedPatternColorIdx = 0;
  }

  resetIndexesFlags();
}

// Returns TRUE if the selected pattern color index is one of the POV color indexes, else returns FALSE
bool isPOVColorIndex(int idx) {
  int numColors = sizeof(COLORS) / sizeof(*COLORS);
  return idx >= numColors ? true : false;
}

// Returns the number of colors in the COLORS array
int getNumColors() {
  return sizeof(COLORS) / sizeof(*COLORS);
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

  int colorIndexes[] = {0, 1};
  
  switch (selectedPatternIdx) {
    case -1:
      patternOff();
      break;
    case 0:
      pattern4();
      break;
    case 1:
      pattern0(colorSet_0, (sizeof(colorSet_0) / sizeof(uint32_t)));
      break;
    case 2:
      pattern1(5);
      break;
    case 3:
      break;
    case 4:
      break;
    case 5:
      break;
    case 6:
      break;
    case 7:
      break;
    case 8:
      break;
    case 9:
      break;
    case 10:
      break;
    case 11:
      pattern5();
      break;
    case 12:
      break;
    case 13:
      break;
    case 14:
      break;
    case 15:
      break;
    case 16:
      break;
    case 17:
      pattern6(ORANGE);
      break;
    case 18:
      pattern7(ORANGE);
      break;
    case 19:
      pattern8(RED);
      break;
    case 20:
      pattern8(GREEN);
      break;
    case 21:
      pattern8(BLUE);
      break;
    case 22:
      pattern8(ATTRACTIVE_PURPLE);
      break;
    case 23:
      pattern8(CYAN);
      break;
    case 24:
      pattern8(YELLOW);
      break;
    case 25:
      pattern9(RED);
      break;
    case 26:
      pattern9(GREEN);
      break;
    case 27:
      pattern9(BLUE);
      break;
    case 28:
      pattern9(ATTRACTIVE_PURPLE);
      break;
    case 29:
      pattern9(CYAN);
      break;
    case 30:
      pattern9(YELLOW);
      break;
  }
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
void pattern1(uint8_t msDelay) {  
  // When we first enter the pattern, turn the stick black
  if (patternChanged) {
    // NOTE: I turned this off because it was happening on every iteration for some reason
    // setAllPixels(BLACK);
  }
  
  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  int colorIterations = 1;

  // If we have a POV color, we need to flash the pattern twice to show each color  
  if (isPOVColor) {
    colorIterations = 2;
  }

  for (int c=0; c < colorIterations; c++) {
    for (uint8_t i=0; i <= pat_i_0; i++) {
      if (isPOVColor) {
        // get the first then second color in the POV color
        patternColumn[i] = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
      } else {
        patternColumn[i] = COLORS[selectedPatternColorIdx];
      }
    }

    // Insert POV delay if POV color
    if (isPOVColor) {
      delay(POVSpeedDelay);
    }

    showColumn();
  }

  if (!patternComplete) {
    pat_i_0++; // increase pattern index
    
    // Check to see if pattern is complete
    if (pat_i_0 == strip.numPixels()) {
      patternComplete = true;
    }
    delay(msDelay); // delay to slow down the pattern animation
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

void pattern5() {
  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  int colorIterations = 1;

  // If we have a POV color, we need to flash the pattern twice to show each color  
  if (isPOVColor) {
    colorIterations = 2;
  }

  if (patDirection == 0) {
    // Reset indexes when needed
    if (pat_i_0 == 0) {
      pat_i_0 = strip.numPixels() - 1;
    }
    if (pat_i_1 == pat_i_0) {
      pat_i_0--;
      pat_i_1 = 0;
    }
  
    for (int c=0; c < colorIterations; c++) {
      // Make a single pixel travel down the stick
      for (int travel_i = 0; travel_i < pat_i_0; travel_i++) {
        if (travel_i == pat_i_1) {
          if (isPOVColor) {
            // get the first then second color in the POV color
            patternColumn[travel_i] = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
          } else {
            patternColumn[travel_i] = COLORS[selectedPatternColorIdx];
          }
        } else {
          patternColumn[travel_i] = BLACK;
        }
      }
    
      // Turn on the lit pixels
      for (int lit_i = pat_i_0; lit_i < strip.numPixels(); lit_i++) {
        if (isPOVColor) {
          // get the first then second color in the POV color
          patternColumn[lit_i] = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
        } else {
          patternColumn[lit_i] = COLORS[selectedPatternColorIdx];
        }
      }
      
      // Insert POV delay if POV color
      if (isPOVColor) {
        delay(POVSpeedDelay);
      }

      showColumn();
    }

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
  
    for (int c=0; c < colorIterations; c++) {
      // Make a single pixel travel down the stick
      for (int travel_i = strip.numPixels() - 1; travel_i > pat_i_0; travel_i--) {
        if (travel_i == pat_i_1) {
          if (isPOVColor) {
            // get the first then second color in the POV color
            patternColumn[travel_i] = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
          } else {
            patternColumn[travel_i] = COLORS[selectedPatternColorIdx];
          }
        } else {
          patternColumn[travel_i] = BLACK;
        }
      }
    
      // Turn on the lit pixels
      for (int lit_i = pat_i_0; lit_i >= 0; lit_i--) {
        if (isPOVColor) {
          // get the first then second color in the POV color
          patternColumn[lit_i] = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
        } else {
          patternColumn[lit_i] = COLORS[selectedPatternColorIdx];
        }
      }
      
      // Insert POV delay if POV color
      if (isPOVColor) {
        delay(POVSpeedDelay);
      }

      showColumn();
    }

    pat_i_1--;
  }
}

void pattern6(uint32_t color) {
  // select a new random amount number to build up to
  if (patternChanged || patternComplete) {
    pat_i_0 = random(5, strip.numPixels());
    patternChanged = false;
    patternComplete = false;
    patternReverse = false;
  }

  // Light the pixels one at a time so it looks like they are building up
  // Turn on the lit pixels
  for (int i = 0; i < strip.numPixels(); i++) {
    if (i <= pat_i_1) {
      patternColumn[i] = color;
    } else {
      patternColumn[i] = BLACK;
    }
  }
  showColumn();
  
  if (!patternReverse) {
    pat_i_1++;
  } else {
    pat_i_1--;
  }

  if (pat_i_1 == pat_i_0) {
    pat_i_1 = 0;
    patternComplete = true;
  }

  if (pat_i_1 == pat_i_0) {
    if (!patternReverse) {
      pat_i_0 = 0;
      patternReverse = true;
    } else {
      pat_i_1 = 0;
      patternComplete = true;
    }
  }
}

void pattern7(uint32_t color) {
  int minPixel_i = 4;
  int patIncrement = 2;
  
  // select a new random amount number to build up to
  if (patternChanged || patternComplete) {
    pat_i_0 = strip.numPixels();
    pat_i_1 = minPixel_i;
    patternChanged = false;
    patternComplete = false;
    patternReverse = false;
  }

  // Light the pixels one at a time so it looks like they are building up
  // Turn on the lit pixels
  for (int i = 0; i < strip.numPixels(); i += patIncrement) {
    if (i <= pat_i_1) {
      patternColumn[i] = color;

      if (i+1 < strip.numPixels()) {
        patternColumn[i+1] = color;
      }
    } else {
      patternColumn[i] = BLACK;
      if (i+1 < strip.numPixels()) {
        patternColumn[i+1] = BLACK;
      }
    }
  }
  showColumn();
  
  if (!patternReverse) {
    if (pat_i_1 + patIncrement < strip.numPixels()) {
      pat_i_1 += patIncrement;
    } else {
      pat_i_1 = strip.numPixels();
    }
  } else {
    if (pat_i_1 - patIncrement >= 0) {
      pat_i_1 -= patIncrement;
    } else {
      pat_i_1 = 0;
    }
  }

  if (pat_i_1 == pat_i_0) {
    if (!patternReverse) {
      pat_i_0 = 0;
      patternReverse = true;
    } else {
      pat_i_1 = minPixel_i;
      patternComplete = true;
    }
  }
}

// Pulsating glow effect
void pattern8(uint32_t color) {
  // NOTE: In order to change the top-end brightness of this pattern, user
  // must switch to a different pattern, change the brightness, and then switch back.

  uint8_t minPatternBrightness = 10;
  int numRepeats = 8;
  
  // Loop the brightness from minBrightness up to the brightness
  // setting the user has set.
  // Initialize brightness
  if (patternChanged) {
    pat_i_0 = strip.getBrightness(); // pat_i_0 is the max brightness
    pat_i_1 = pat_i_0;
    patternChanged = false;
    tempSavedBrightness = pat_i_0;
  }

  // Light the pixels
  for (int i = 0; i < strip.numPixels(); i++) {
    patternColumn[i] = color;
  }
  strip.setBrightness(pat_i_1);
  showColumn();

  // Oscillate brightness from min to max
  if (!patternReverse) {
    if (pat_i_1 > minPatternBrightness) {
      // Repeat brightness levels that are the lower half of the pulsing range so it slows down
      if ((pat_i_1 < pat_i_0 / 2) && (pat_i_1 >= pat_i_0 / 3)) {
        if (pat_i_2 == (numRepeats / 2) - 1) {
          pat_i_1--;
          pat_i_2 = 0;
        } else {
          pat_i_2++;
        }
      } else if (pat_i_1 < pat_i_0 / 3) {
        if (pat_i_2 == numRepeats - 1) {
          pat_i_1--;
          pat_i_2 = 0;
        } else {
          pat_i_2++;
        }
      } else {
        pat_i_1--;
      }
    } else {
      patternReverse = true;
    }
  } else {
    if (pat_i_1 < pat_i_0) { // pat_i_0 is max brightness for this pattern
      // Repeat brightness levels that are the lower half of the pulsing range
      if ((pat_i_1 < pat_i_0 / 2) && (pat_i_1 >= pat_i_0 / 3)) {
        if (pat_i_2 == (numRepeats / 2) - 1) {
          pat_i_1++;
          pat_i_2 = 0;
        } else {
          pat_i_2++;
        }
      } else if (pat_i_1 < pat_i_0 / 3) {
        if (pat_i_2 == numRepeats - 1) {
          pat_i_1++;
          pat_i_2 = 0;
        } else {
          pat_i_2++;
        }
      } else {
        pat_i_1++;
      }
    } else {
      patternReverse = false;
    }
  }
}

// Twinkle effect
void pattern9(uint32_t color) {
  // group the lights into groups of 15
  int groupSize = 15;
  int pixel_i = 0;
  int group_i = 0;
  int rand_i_0,
    rand_i_1,
    rand_i_2;
  int slowDownCycles = 5;

  if (pat_i_0 < slowDownCycles) {
    pat_i_0++;
    showColumn();
    return;
  } else {
    pat_i_0 = 0;
  }

  // Light 3 random pixels out of every group of 15
  while (pixel_i < strip.numPixels()) {
    // Get the indexes of the randomly chosen pixels in the group
    if (pixel_i % groupSize == 0) {
      rand_i_0 = random(pixel_i, pixel_i + groupSize);
      rand_i_1 = random(pixel_i, pixel_i + groupSize);
      rand_i_2 = random(pixel_i, pixel_i + groupSize);
      
      group_i++;
    }

    if (pixel_i == rand_i_0 || pixel_i == rand_i_1 || pixel_i == rand_i_2) {
      patternColumn[pixel_i] = color;
    } else {
      patternColumn[pixel_i] = BLACK;
    }
    pixel_i++;
  }
  showColumn();
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
  pat_i_2 = 0;
  patternChanged = true;
  patternComplete = false;

  // Set brightness back to what it was if it was changed for a pattern
  if (tempSavedBrightness > 0) {
    strip.setBrightness(tempSavedBrightness);
    tempSavedBrightness = 0;
  }
}

void loop() {  
  showPattern();
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

// Alert user by flashing stick red
void alertUser() {    
  setAllPixels(BLACK);
  delay(50);
  setAllPixels(RED);
  delay(50);
  setAllPixels(BLACK);
  delay(50);
  setAllPixels(RED);
  delay(200);

  patternChanged = true; // Trigger a pattern restart for patterns that are unchanging
  showColumn();
}

void debugButton(uint32_t buttonVal) {
  

   
}
