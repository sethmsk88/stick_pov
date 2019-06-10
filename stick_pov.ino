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

// Function prototypes
void colorWipe(uint8_t msDelay, int colorIdx = -1);

// Each favorite saves two bytes worth of info, so each is allocated two addresses
// 1st address contains the index of the saved pattern
// 2nd address contains the speed delay of the saved pattern
// 3rd address contains the color for the saved pattern
// 4th address contains the POV speed delay for the saved pattern
const uint16_t FAV_0_ADDR = 0;
const uint16_t FAV_1_ADDR = 4;
const uint16_t FAV_2_ADDR = 8;
const uint16_t FAV_3_ADDR = 12;
const uint16_t FAV_4_ADDR = 16;
const uint16_t NUM_LEDS_SAVED_ADDR = 17;
const uint16_t BRIGHTNESS_SAVED_ADDR = 18;

// Colors are in GRB format
const uint32_t COLORS[] = {
  0x00FF00, // 0 - RED
  0x800000, // 1 - GREEN
  0x0000FF, // 2 - BLUE
  0xFFFF00, // 3 - YELLOW
  0x9900FF, // 4 - CYAN
  0x00FFFF, // 5 - MAGENTA
  0x004D99, // 6 - PURPLE
  0x90FF00, // 7 - ORANGE
  0xFFFFFF, // 8 - WHITE
};
const uint8_t COLORS_POV[][2] = {{0,1},{0,2},{1,2}}; // combinations of color indexes for POV

uint8_t defaultBrightness = 160;//105; // Default is set to 50% of the brightness range

// IMPORTANT NOTE: numLEDs value must also be changed in the applySavedSettings() function if a change to the LED count is made
uint8_t numLEDs = 53; // 8 for test device, 53 for stick
const uint8_t MAX_LEDS = 60;

uint32_t patternColumn[MAX_LEDS] = {};
int selectedPatternIdx = 0; // default pattern index
int selectedPatternColorIdx = 0; // default color index
uint8_t numPatterns = 8;
boolean patternChanged = true;
boolean patternComplete = false; // used when a pattern should only show once
int pat_i_0 = 0; // an index to track progress of a pattern
int pat_i_1 = 0; // another index to track progress of a pattern
int pat_i_2 = 0; // another index to track progress of a pattern
uint8_t speedDelay = 0; // ms of delay between showing columns
uint8_t maxSpeedDelay = 45;
int POVSpeedDelay = 16;
const int POVSpeedDelayMax = 50;
uint8_t speedIncrement = 1;
int POVSpeedIncrement = 1;
uint16_t lastButtonPress = 0;
uint16_t pendingButtonPress = 0; // IR value for button press
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
  // Serial.println("Serial Monitor Ready");

  // initEEPROM(); // ONLY RUN THIS ONCE - Usually Leave This Commented Out

  myReceiver.enableIRIn(); // start the receiver

  applySavedSettings();
  strip = Adafruit_NeoPixel(numLEDs, DATA_PIN, NEO_GRB + NEO_KHZ800);

  getFavorite(0); // Set stick to HOME pattern

  strip.begin();
  strip.setBrightness(defaultBrightness);
  strip.show();

  randomSeed(analogRead(0));
}

// Change the number of LEDs on the strip
void changeNumLEDs(int difference) {
  // ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR 
  // ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR 
  // ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR 
  // TODO: There is a problem with this function. The method updateLength causes the stick to freeze
  
  // TODO: if it's a negative difference, pulse the stick a solid color and delay briefly, since this action
  // will be on a button hold
  
  int minNumLEDs = 5; // setting min number of pixels to 5, because their is a weird bug below 5
  uint16_t currentNumLEDs = strip.numPixels();
  uint16_t newNumLEDs = currentNumLEDs + difference;

  if (newNumLEDs >= minNumLEDs && newNumLEDs <= MAX_LEDS) {
    // strip.updateLength(newNumLEDs); // ERROR - DISABLED FOR NOW
    showColumn();
    
    // Save number of LEDs to non-volatile memory
    EEPROM.update(NUM_LEDS_SAVED_ADDR, (uint8_t)strip.numPixels());
  }

  Serial.print(F("Num LEDs: "));
  Serial.println((String)strip.numPixels());
}

// Initialize EEPROM memory addresses that we plan to use
// ONLY CALL THIS WHEN SETTING UP A NEW STICK, OR WHEN A CODE CHANGE
// WAS MADE THAT INVOLVES THE EEPROM
void initEEPROM() {
  int numMemAddressesUsed = 19;
  for (int i=0; i < numMemAddressesUsed; i++) {
    EEPROM.update(i, 255);
  }
}

void applySavedSettings() {
  int patternIndex_addr,
    speedDelay_addr,
    patternColorIndex_addr,
    POVSpeedDelay_addr;
  int unsetVal = 255; // the number stored at an address that is unset
  
  // if favorites have not yet been saved, set them to 0
  int numFavorites = 5;
  for (int i=0; i < numFavorites; i++) {
    patternIndex_addr = getFavoriteAddr(i, 0); // Load pattern for this favorite
    speedDelay_addr = getFavoriteAddr(i, 1); // Load speed delay for this favorite
    patternColorIndex_addr = getFavoriteAddr(i, 2); // Load color index for this favorite
    POVSpeedDelay_addr = getFavoriteAddr(i, 3); // Load color index for this favorite

    if (EEPROM.read(patternIndex_addr) == unsetVal) {
      EEPROM.update(patternIndex_addr, 0);
      EEPROM.update(speedDelay_addr, 0);
      EEPROM.update(patternColorIndex_addr, 0);
      EEPROM.update(POVSpeedDelay_addr, 0);
    }
  }

  // Apply saved brightness setting
  if (EEPROM.read(BRIGHTNESS_SAVED_ADDR) != unsetVal) {
    defaultBrightness = EEPROM.read(BRIGHTNESS_SAVED_ADDR);
  }

  // Serial.println("Applying saved number of LEDs " + (String)EEPROM.read(NUM_LEDS_SAVED_ADDR));

  // Apply saved number of LEDs for the stick
  if (EEPROM.read(NUM_LEDS_SAVED_ADDR) != unsetVal) {
    // numLEDs = EEPROM.read(NUM_LEDS_SAVED_ADDR);
    numLEDs = 53; // 8 for test device, 53 for stick
  }
}

// Save a favorite
void setFavorite(uint8_t i) {
  if (selectedPatternIdx < 0) {
    // Serial.println("ERROR: Cannot save a pattern favorite whose index is negative");
    return;
  }
  
  int patternIndex_addr = getFavoriteAddr(i, 0);
  int speedDelay_addr = getFavoriteAddr(i, 1);
  int patternColorIndex_addr = getFavoriteAddr(i, 2);
  int POVSpeedDelay_addr = getFavoriteAddr(i, 3);
  
  EEPROM.update(patternIndex_addr, selectedPatternIdx);
  EEPROM.update(speedDelay_addr, speedDelay);
  EEPROM.update(patternColorIndex_addr, selectedPatternColorIdx);
  EEPROM.update(POVSpeedDelay_addr, POVSpeedDelay);

  alertUser(COLORS[1], 2, 50, 200); // Flash stick green to alert a save

  // Serial.println("Favorite " + (String)i + " saved:");
}

// Get a saved favorite, and make it active
void getFavorite(uint8_t i) {
  int patternIndex_addr = getFavoriteAddr(i, 0);
  int speedDelay_addr = getFavoriteAddr(i, 1);
  int patternColorIndex_addr = getFavoriteAddr(i, 2);
  int POVSpeedDelay_addr = getFavoriteAddr(i, 3);
  
  selectedPatternIdx = EEPROM.read(patternIndex_addr);
  speedDelay = EEPROM.read(speedDelay_addr);
  selectedPatternColorIdx = EEPROM.read(patternColorIndex_addr);
  POVSpeedDelay = EEPROM.read(POVSpeedDelay_addr);
  
  resetIndexesFlags();
}

// Get address of favorite in EEPROM
// fav_i - Favorite number (0 - 4)
// attr_i - index of the favorite attribute (0 - 3)
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
  uint16_t IRVal;
  uint16_t buttonActionVal;
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
    IRVal = (uint16_t)IRresults.value;

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
        changeNumLEDs(1);
        break;
      case RemoteControlRoku::BTN_ASTERISK_HOLD:
        changeNumLEDs(-1);
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
        changeBrightness(5);
        break;
      case RemoteControlRoku::BTN_VOL_DOWN:
      case RemoteControlRoku::BTN_VOL_DOWN_HOLD:
        changeBrightness(-5);
        break;
      case RemoteControlRoku::BTN_RETURN:
        changeDirection();
        break;
      case RemoteControlRoku::BTN_FASTFORWARD:
        increaseSpeed();
        break;
      case RemoteControlRoku::BTN_FASTFORWARD_HOLD:
        increasePOVSpeed();
        break;
      case RemoteControlRoku::BTN_REWIND:
        decreaseSpeed();
        break;
      case RemoteControlRoku::BTN_REWIND_HOLD:
        decreasePOVSpeed();
        break;
    }

    pendingButtonPress = 0; // clear value

    Serial.print(F("Button ["));
    Serial.print(RemoteControlRoku::getBtnDescription(buttonActionVal));
    Serial.println(F("]"));
  }
}

void nextPattern() {
  if (selectedPatternIdx < numPatterns - 1) {
    selectedPatternIdx++;
  } else {
    selectedPatternIdx = 0;
  }
  resetIndexesFlags();
  changeBrightness(0); // Changing brightness by 0 so that we can make sure the brightness is safe for the new pattern
  saveBrightness(); // save stick brightness if it has changed

  // Serial.println("selectedPatternIdx = " + (String)selectedPatternIdx); // DEBUG
}

void prevPattern() {
  if (selectedPatternIdx > 0) {
    selectedPatternIdx--;
  } else {
    selectedPatternIdx = numPatterns - 1;
  }
  resetIndexesFlags();
  changeBrightness(0); // Changing brightness by 0 so that we can make sure the brightness is safe for the new pattern
  saveBrightness(); // save stick brightness if it has changed
}

// Check to see if the proposed brightness setting is safe for the color
// This prevents the board from crashing due to power overdraw
// Note: This function is basing its maximum safe values on no more than 53 lights being lit
uint8_t makeSafeBrightness(uint8_t brightness, uint8_t colorIdx, int difference) {
  bool isPOVColor = isPOVColorIndex(colorIdx);
  uint8_t colorIterations = isPOVColor ? 2 : 1; // 2 colors for POV
  int numColors = getNumColors();
  uint8_t maxBrightness = 230; // 90% of 255
  uint16_t maxColorSum = 255 + 127; // One color full brightness, and a second color at half brightness
  uint32_t maxBrightnessProduct = ((uint32_t)maxColorSum) * ((uint32_t)maxBrightness); // Value to represent power draw needed to display this brightness

  // Break the current color into its RGB values
  // NOTE: Colors are in GRB order
  uint8_t g, r, b;
  uint16_t colorSum;
  uint32_t color, brightnessProduct;

  // If POV color, check both colors to make sure they are safe
  for (uint8_t c = 0; c < colorIterations; c++) {
    color = isPOVColor ? COLORS[ COLORS_POV[colorIdx - numColors][c] ] : COLORS[colorIdx];
    g = color >> 16;
    r = color >> 8;
    b = color;
    colorSum = g + r + b;

    brightnessProduct = ((uint32_t)brightness) * ((uint32_t)colorSum);

    // Change brightness to safe value if needed
    if (brightnessProduct > maxBrightnessProduct) {

      // If brightness is greater than the max safe brightness, update the brightness value
      if (brightness > maxBrightnessProduct / colorSum) {
        brightness = maxBrightnessProduct / colorSum;

        // Alert user if the user was actually trying to change the brightness, rather than changing patterns or colors
        if (difference != 0) {
          alertUser(COLORS[0], 2, 50, 200);
        }
      }
    }
  }
  return brightness;
}

// difference is the amount by which you would like to change the brightness
// (e.g. -5 = darker, 5 = brighter)
void changeBrightness(int difference) {
  int minBrightness = 20;
  int maxBrightness = 230; // 90% of 255
  uint8_t currentBrightness = strip.getBrightness();
  int newBrightness = currentBrightness + difference;

  // change the brightness level
  if (newBrightness < minBrightness) {
    newBrightness = minBrightness;
    // Serial.println("Here");
    alertUser(COLORS[0], 2, 50, 200);
  } else if (newBrightness > maxBrightness) {
    newBrightness = maxBrightness;
    // Serial.println("There");
    alertUser(COLORS[0], 2, 50, 200);
  }

  newBrightness = makeSafeBrightness(newBrightness, selectedPatternColorIdx, difference);

  // Serial.print(F("Brightness: "));
  // Serial.println((String)newBrightness);

  strip.setBrightness((uint8_t)newBrightness);
  showColumn();
}

// Increase speed of pattern
void increaseSpeed() {
  if (speedDelay <= 0) {
    speedDelay = 0;
    // Serial.println("Maximum speed");
    alertUser(COLORS[0], 2, 50, 200);
  } else {
    // Prevent unsigned int math going negative
    if ((int)speedDelay - (int)speedIncrement < 0) {
      speedDelay = 0;
      // Serial.println("Maximum speed");
      alertUser(COLORS[0], 2, 50, 200);
    } else {
      speedDelay -= speedIncrement;
    }
  }
  Serial.print(F("Speed Delay: "));
  Serial.println((String)speedDelay);
}

// Decrease speed of pattern
void decreaseSpeed() {
  if (speedDelay >= maxSpeedDelay) {
    speedDelay = maxSpeedDelay;
    // Serial.println("Minimum speed");
    alertUser(COLORS[0], 2, 50, 200);
  } else {
    speedDelay += speedIncrement;
  }
  Serial.print(F("Speed Delay: "));
  Serial.println((String)speedDelay);
}

// Increase speed of POV effect
void increasePOVSpeed() {
  if (POVSpeedDelay <= 0) {
    POVSpeedDelay = 0;
    // Serial.println("Maximum POV speed");
    alertUser(COLORS[0], 2, 50, 200);
  } else {
    if (POVSpeedDelay - POVSpeedIncrement < 0) {
      POVSpeedDelay = 0;
      // Serial.println("Maximum POV speed");
    } else {
      POVSpeedDelay -= POVSpeedIncrement;
    }
    alertUser(0, 1, 0, 400); // Flash stick black for half a second
  }
  Serial.print(F("POV Speed Delay: "));
  Serial.println((String)POVSpeedDelay);
}

// Decrease speed of POV effect
void decreasePOVSpeed() {
  if (POVSpeedDelay >= POVSpeedDelayMax) {
    POVSpeedDelay = POVSpeedDelayMax;
    // Serial.println("Minimum POV speed");
    alertUser(COLORS[0], 2, 50, 200);
  } else {
    POVSpeedDelay += POVSpeedIncrement;
    alertUser(0, 1, 0, 500); // Flash stick black for half a second
  }
  Serial.print(F("POV Speed Delay: "));
  Serial.println((String)POVSpeedDelay);
}

// Change direction of pattern
void changeDirection() {
  if (patDirection == 0) {
    patDirection = 1;
  } else {
    patDirection = 0;
  }
  resetIndexesFlags();

  // Serial.println(F("Pattern direction changed"));
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

  changeBrightness(0); // Changing brightness by 0 so that we can make sure the brightness is safe for the new pattern
  resetIndexesFlags();
}

// Returns TRUE if the selected pattern color index is one of the POV color indexes, else returns FALSE
bool isPOVColorIndex(int idx) {
  int numColors = sizeof(COLORS) / sizeof(*COLORS);
  return idx >= numColors ? true : false;
}

// Returns the number of colors in the COLORS array
// TODO: change return type to uint8_t
int getNumColors() {
  return sizeof(COLORS) / sizeof(*COLORS);
}

// Returns the number of POV colors in the COLORS_POV array
uint8_t getNumPOVColors() {
  return sizeof(COLORS_POV) / sizeof(*COLORS_POV);
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
  
  delay(speedDelay);
}

/*
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
*/

// Show the pattern that is currently selected
void showPattern() {  
  switch (selectedPatternIdx) {
    case -1:
      patternOff();
      break;
    case 0:
      pattern4();
      break;
    case 1:
      sixColorPOV();
      break;
    case 2:
      colorWipe(5);
      break;
    case 3:
      pattern5();
      break;
    case 4:
      pattern8();
      break;
    case 5:
      pattern9();
      break;
    case 6:
      colorWipeLoop();
      break;
    case 7:
      colorFade();
      break;
  }
}

// Set all pixels to black, so very little power is used
void patternOff() {
  setAllPixels(0); // Set to BLACK
}

// 6-color POV
void sixColorPOV() {  
  int colorIndexes[] = {2,1,0,6,7,3};
  int numColors = sizeof(colorIndexes) / sizeof(*colorIndexes);

  for (uint8_t j=0; j < strip.numPixels(); j++) {
    patternColumn[j] = COLORS[colorIndexes[pat_i_0]];
  }
  showColumn();
  delay(POVSpeedDelay);

  // Increment color index (loop index to beginning)
  pat_i_0 = (pat_i_0 < numColors - 1) ? pat_i_0 + 1 : 0;
}

// Color Wipe
void colorWipe(uint8_t msDelay, int colorIdx) {
  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  int colorIterations = isPOVColor ? 2 : 1; // 2 colors for POV
  uint32_t color;

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];

    for (uint8_t i=0; i <= pat_i_0; i++) {
      patternColumn[i] = color;
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
    // TODO: shouldn't this be comparing against (strip.numPixels() - 1)?
    if (pat_i_0 == strip.numPixels()) {
      patternComplete = true;
    }
    delay(msDelay); // delay to slow down the pattern animation
  }
}

// Loop through all color wipes
void colorWipeLoop() {
  // pat_i_0 is the index of the pixel we are currently filling up to for the animation (aka animation index)
  // pat_i_1 is the color index
  bool isPOVColor = isPOVColorIndex(pat_i_1);
  uint8_t colorIterations = 2; // 2 colors for POV - Also doing solid colors for 2 iterations to keep the animation timing in sync
  uint32_t color;
  int numColors = getNumColors();
  uint8_t totalNumColors = ((uint8_t)numColors) + getNumPOVColors();
  uint16_t numPixels = strip.numPixels();

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    color = isPOVColor ? COLORS[ COLORS_POV[pat_i_1 - numColors][c] ] : COLORS[pat_i_1];

    // Light the pixels
    if (patDirection == 0) {
      for (uint16_t pixel_i=0; pixel_i <= pat_i_0; pixel_i++) {
        patternColumn[pixel_i] = color;
      }
    } else {
      for (int pixel_i = numPixels - 1; pixel_i >= pat_i_0; pixel_i--) {
        patternColumn[pixel_i] = color;
      }
    }

    // Insert POV delay if POV color
    delay(POVSpeedDelay);
    
    showColumn();
  }

  bool animationComplete = false;
  // Check to see if the pattern animation is now complete
  if (patDirection == 0) {
    if (pat_i_0 == numPixels - 1) {
      animationComplete = true;
      pat_i_0 = 0; // reset animation index
      
      // Advance the color index
      // Loop back to the first color if we reached the last color, else, increment color index
      pat_i_1 = (pat_i_1 == totalNumColors - 1) ? 0 : pat_i_1 + 1;
    } else {
      pat_i_0++;
    }
  } else {
    if (pat_i_0 == 0) {
      animationComplete = true;
      pat_i_0 = numPixels - 1; // reset animation index
    } else {
      pat_i_0--;
    }
  }

  if (animationComplete) {
    // Advance the color index
    // Loop back to the first color if we reached the last color, else, increment color index
    pat_i_1 = (pat_i_1 == totalNumColors - 1) ? 0 : pat_i_1 + 1;
  }
}

// Fade full stick through all colors (one solid color at a time)
void colorFade() {
  // pat_i_0 is the current color index
  // pat_i_1 is the color fade step we are currently on

  uint8_t colorSet[] = {0, 1, 2}; // Fading through R, G, B

  int numColors = 3;
  uint16_t numPixels = strip.numPixels();
  uint8_t steps = 64; // how many steps to take to fade from one color to another

  uint32_t currentColor = COLORS[colorSet[pat_i_0]];
  uint32_t gapColor; // the calculated color that will be shown
  
  uint32_t nextColor = (pat_i_0 < numColors - 1) ? COLORS[colorSet[pat_i_0] + 1] : COLORS[colorSet[0]];

  uint8_t currentColor_G = currentColor >> 16;
  uint8_t currentColor_R = currentColor >> 8;
  uint8_t currentColor_B = currentColor;
  uint8_t nextColor_G = nextColor >> 16;
  uint8_t nextColor_R = nextColor >> 8;
  uint8_t nextColor_B = nextColor;
  uint8_t gapColor_G, gapColor_R, gapColor_B;

  int G_gap = nextColor_G - currentColor_G;
  int R_gap = nextColor_R - currentColor_R;
  int B_gap = nextColor_B - currentColor_B;

  gapColor_G = currentColor_G + (G_gap / steps * pat_i_1);
  gapColor_R = currentColor_R + (R_gap / steps * pat_i_1);
  gapColor_B = currentColor_B + (B_gap / steps * pat_i_1);

  gapColor = 0;
  gapColor = gapColor | (((uint32_t)(gapColor_G)) << 16);
  gapColor = gapColor | (((uint32_t)(gapColor_R)) << 8);
  gapColor = gapColor | ((uint32_t)(gapColor_B));

  for (int pixel_i=0; pixel_i <= numPixels; pixel_i++) {
    patternColumn[pixel_i] = gapColor;
  }
  showColumn();

  if (pat_i_1 < steps - 1) {
    pat_i_1++;
  } else {
    pat_i_1 = 0;
    pat_i_0 = (pat_i_0 < numColors - 1) ? pat_i_0 + 1 : 0;
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
  int colorIterations = isPOVColor ? 2 : 1; // 2 colors for POV
  uint32_t color;

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
      // Get the color
      color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];

      // Make a single pixel travel down the stick
      for (int travel_i = 0; travel_i < pat_i_0; travel_i++) {
        if (travel_i == pat_i_1) {
          patternColumn[travel_i] = color;
        } else {
          patternColumn[travel_i] = 0; // BLACK
        }
      }
    
      // Turn on the lit pixels
      for (int lit_i = pat_i_0; lit_i < strip.numPixels(); lit_i++) {
        patternColumn[lit_i] = color;
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
          patternColumn[travel_i] = 0; // BLACK
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

// Pulsating glow effect
void pattern8() {
  // NOTE: In order to change the top-end brightness of this pattern, user
  // must switch to a different pattern, change the brightness, and then switch back.

  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  int colorIterations = isPOVColor ? 2 : 1; // 2 colors for POV
  uint32_t color;

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

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];
    
    // Light the pixels
    for (int i = 0; i < strip.numPixels(); i++) {
      patternColumn[i] = color;
    }
    strip.setBrightness(pat_i_1);

    // Insert POV delay if POV color
    if (isPOVColor) {
      delay(POVSpeedDelay);
    }

    showColumn();
  }

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
void pattern9() {
  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  int colorIterations = isPOVColor ? 2 : 1; // 2 colors for POV
  uint32_t color;
  
  // group the lights into groups of 15
  int groupSize = 15;
  int pixel_i = 0;
  int group_i = 0;
  int rand_i_0,
    rand_i_1,
    rand_i_2;
  int slowDownCycles = 5;

/*
  if (pat_i_0 < slowDownCycles) {
    pat_i_0++;
    showColumn();
    return;
  } else {
    pat_i_0 = 0;
  }  
*/

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];

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
        patternColumn[pixel_i] = 0; // BLACK
      }
      pixel_i++;
    }

    // Insert POV delay if POV color
    if (isPOVColor) {
      delay(POVSpeedDelay);
    }

    showColumn();
  }
}

void setAllPixels(uint32_t color) {
  for (int i=0; i < strip.numPixels(); i++) {
    patternColumn[i] = color;
  }
  showColumn();
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
void alertUser(uint32_t color, uint8_t numFlashes, uint16_t midDelay, uint16_t endDelay) {    
  for (uint8_t i=0; i < numFlashes; i++) {
    setAllPixels(0); // BLACK
    delay(midDelay);
    setAllPixels(color);
    delay(midDelay);
    // Serial.println("flash " + (String)i);
  }
  delay(endDelay);

  patternChanged = true; // Trigger a pattern restart for patterns that are unchanging
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
  /*
  oldColorWipe(strip.Color(25, 0, 0), 20); // Red
  oldColorWipe(strip.Color(50, 50, 20), 200); // White
  oldColorWipe(strip.Color(0, 255, 0), 20); // Green
  oldColorWipe(strip.Color(0, 0, 255), 50); // Blue
  oldColorWipe(strip.Color(0, 0, 0, 255), 50); // White RGBW
  Send a theater pixel chase in...
  theaterChase(strip.Color(127, 127, 127), 50); // White
  theaterChase(strip.Color(127, 0, 0), 50); // Red
  theaterChase(strip.Color(0, 0, 127), 50); // Blue

  rainbow(20);
  rainbowCycle(20);
  theaterChaseRainbow(50);
  */
}

// Fill the pixels one after another with a color
/*
void oldColorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
*/
/*
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
*/

// Slightly different, this makes the rainbow equally distributed throughout
/*
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
*/

//Theatre-style crawling lights.
/*
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
*/

//Theatre-style crawling lights with rainbow effect
/*
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
*/

// Displays diamonds
/*
void pattern2() {
  // Note: this solution prints the whole pattern before returning
  uint32_t color1 = COLORS[0]; // red
  uint32_t color2 = COLORS[1]; // green
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
        patternColumn[row_i] = color1;
        row_i++;
      }
      for (c = 1; c <= 2*k-1;c++) {
        if (row_i >= strip.numPixels())
          break;
        patternColumn[row_i] = color2;
        row_i++;
      }
      for (c = 1; c <= space; c++) {
        if (row_i >= strip.numPixels())
          break;
        patternColumn[row_i] = color1;
        row_i++;
      }
      showColumn();
      if (row_i >= strip.numPixels())
        break;
          
      space--;
    }
  
    // Bottom half, if printed
    // Right half, if on stick
    for (k = 1; k <= height - 1; k++) {
      for (c= 1; c <= space; c++) {
        if (row_i >= strip.numPixels())
          break;
        patternColumn[row_i] = color1;
        row_i++;
      }
      for (c = 1; c <= 2*(height - k)-1; c++) {
        if (row_i >= strip.numPixels())
          break;
        patternColumn[row_i] = color2;
        row_i++;
      }
      for (c= 1; c <= space; c++) {
        if (row_i >= strip.numPixels())
          break;
        patternColumn[row_i] = color1;
        row_i++;
      }
      if (row_i >= strip.numPixels())
        break;
      space++;
    }
  }
}
*/

// Stretched Diamond
// This function should stretch the pattern in the array pat by the factor stretch, and
// then flip it vertically and print the bottom half.
/*
void pattern3() {
  int pixel_i = 0;
  uint32_t c1 = COLORS[0]; // red
  uint32_t c2 = COLORS[1]; // green
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
*/