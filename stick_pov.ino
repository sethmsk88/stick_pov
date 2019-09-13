#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <EEPROM.h>

#define DATA_PIN 11
#define BTN_1_PIN 3
#define BTN_2_PIN 5
#define BTN_3_PIN 9
#define MAX_TIME_VALUE 0xFFFFFFFF
#define COLOR_ORDER GRB
#define MAX_EEPROM_ADDR 1023

// Function prototypes
void colorWipe(uint8_t msDelay);
void solidColor();
void chase(uint8_t groupSize, bool centerOrigin = false);
void rainbow(bool sparkle = false);

//////   BEN, YOU CAN CHANGE THESE TWO VALUES   //////
const uint8_t numLEDs = 53; // The stick has 53 LEDs
int POVSpeedDelay = 16; // Milliseconds of delay between colors

const uint8_t MAX_LEDS = 100; // DO NOT CHANGE THIS
// IMPORTANT NOTE: numLEDs value must also be changed in the applySavedSettings() function if a change to the LED count is made

// Version number should be incremented each time an update is pushed
const uint16_t VERSION = 0;

// EEPROM: 1024 Bytes
// Each favorite saves two bytes worth of info, so each is allocated two addresses
// 1st address contains the index of the saved pattern
// 2nd address contains the speed delay of the saved pattern
// 3rd address contains the color for the saved pattern
// 4th address contains the POV speed delay for the saved pattern
const uint8_t UNSET_EEPROM_VAL = 255; // Initial state for all EEPROM addresses
const uint16_t FAV_0_ADDR = 0;
const uint16_t FAV_1_ADDR = 4;
const uint16_t FAV_2_ADDR = 8;
const uint16_t FAV_3_ADDR = 12;
const uint16_t FAV_4_ADDR = 16;
const uint16_t NUM_LEDS_SAVED_ADDR = 17;
const uint16_t BRIGHTNESS_SAVED_ADDR = 18;
const uint16_t VERSION_ADDR = 1022; // Last 2 bytes of EEPROM

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
  0xFF0000, // 8 - BRIGHT_GREEN
  0xFFFFFF, // 9 - WHITE
};
const uint8_t COLORS_POV[][2] = {{0,8},{0,2},{8,2},{0,4},{6,7},{8,7},{4,6}}; // combinations of color indexes for POV
const uint8_t COLORS_POV3[][3] = {{0,8,2}};

uint8_t defaultBrightness = 25;//105; // Default is set to 50% of the brightness range

uint32_t patternColumn[MAX_LEDS] = {};
int selectedPatternIdx = 0; // default pattern index
int selectedPatternColorIdx = 0; // default color index
uint8_t numPatterns = 12;
boolean patternChanged = true;
boolean patternComplete = false; // used when a pattern should only show once
int pat_i_0 = 0; // an index to track progress of a pattern
int pat_i_1 = 0; // another index to track progress of a pattern
int pat_i_2 = 0; // another index to track progress of a pattern
uint8_t speedDelay = 0; // ms of delay between showing columns
uint8_t maxSpeedDelay = 45;
const int POVSpeedDelayMax = 500;
uint16_t lastButtonPress = 0;
uint16_t pendingButtonPress = 0; // IR value for button press
unsigned long buttonPressStartTime = 0;
uint16_t longButtonPressTime = 1000; // 1 seconds
boolean longButtonPressActionPerformed = false;
unsigned long noIRSignalDelay = 150; // if there are no IR signals for this amount of time, then we can say there are no active IR signals
int patDirection = 0;
boolean patternReverse = false;
uint8_t patternStartingBrightness = 0; // used for patterns that alter brightness (must init to 0)
uint8_t cycleCounter = 0;
bool autoCycle = false;
uint32_t autoCycleTimerStart = 0;

unsigned long buttonHoldStartTime = 0;

uint16_t activeButtonPin = 0;
uint8_t activeBtnsVal = 0;
uint8_t prevBtnsVal = 0;

uint8_t baseHue = 0; // rotating color used by some patterns

// Adafruit_NeoPixel strip;
// Define the array of leds
CRGB leds[numLEDs];

void setup() {
  Serial.begin(9600); // Connect with Serial monitor for testing purposes

  initEEPROM(); // Resets EEPROM saved values when version number changes
  // applySavedSettings();
  
  // strip = Adafruit_NeoPixel(numLEDs, DATA_PIN, NEO_GRB + NEO_KHZ800);
  FastLED.addLeds<WS2812, DATA_PIN, COLOR_ORDER>(leds, numLEDs);

  getFavorite(0); // Set stick to HOME pattern

  // Initialize buttons
  pinMode(BTN_1_PIN, INPUT_PULLUP);
  pinMode(BTN_2_PIN, INPUT_PULLUP);
  pinMode(BTN_3_PIN, INPUT_PULLUP);

  // FastLED.setBrightness(defaultBrightness);
  // FastLED.show();

  randomSeed(analogRead(0)); // seed random number generator for functions that need it
}

// Change the number of LEDs on the strip
void changeNumLEDs(int difference) {
  // TODO: There is a problem with this function. The method updateLength causes the stick to freeze
  // TODO: if it's a negative difference, pulse the stick a solid color and delay briefly, since this action
  int minNumLEDs = 5; // setting min number of pixels to 5, because their is a weird bug below 5
  uint16_t currentNumLEDs = numLEDs;
  uint16_t newNumLEDs = currentNumLEDs + difference;

  if (newNumLEDs >= minNumLEDs && newNumLEDs <= MAX_LEDS) {
    // strip.updateLength(newNumLEDs); // ERROR - DISABLED FOR NOW
    showColumn();
    
    // Save number of LEDs to non-volatile memory
    EEPROM.update(NUM_LEDS_SAVED_ADDR, (uint8_t)numLEDs);
  }

  // Serial.print(F("Num LEDs: "));
  // Serial.println((String)numLEDs);
}

// Initialize EEPROM memory addresses if version number has changed
void initEEPROM() {  
  // Check to see if version has changed since last initialization
  uint16_t savedVersion = getSaved16(VERSION_ADDR);
  
  Serial.print(F("Saved version: "));
  Serial.println(savedVersion);

  // If version has changed, save new version number, and re-initialize saved addresses
  if (VERSION != savedVersion) {
    // Save new version number in EEPROM
    setSaved16(VERSION_ADDR, VERSION);

    Serial.print(F("New saved version: "));
    Serial.println(getSaved16(VERSION_ADDR));
    
    // Do NOT re-initialize the version number memory addresses (i.e. the last 2 bytes in EEPROM)
    for (int i=0; i <= MAX_EEPROM_ADDR - 2; i++) {
      EEPROM.update(i, UNSET_EEPROM_VAL);
    }
  }
}

// Set a 16-bit value in EEPROM memory
// NOTE: 16-bit values are saved in big-endian order
void setSaved16(uint16_t addr, uint16_t val) {  
  EEPROM.update(addr, (uint8_t)(val >> 8));
  EEPROM.update(addr + 1, (uint8_t)val);
}

// Get a saved 16-bit value from EEPROM memory
// NOTE: 16-bit values are saved in big-endian order
uint16_t getSaved16(uint16_t addr) {
  if (addr > MAX_EEPROM_ADDR - 1) {
    return 0; // ERROR VALUE
  }
  return ((uint16_t)EEPROM.read(addr) << 8) + EEPROM.read(addr + 1);
}

// TODO - Implement the EEPROM re-initialization function before re-activating this
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
    // TODO: Can't re-assign a constant value. Find a new way to change the number of LEDs
    // numLEDs = 53; // 8 for test device, 53 for stick
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

  // alertUser(COLORS[1], 2, 50, 200); // Flash stick green to alert a save
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
    default:
      // Return HOME mode if invalid favorite mode was requested
      return FAV_0_ADDR + attr_i;
  }
}

void saveBrightness() {
  EEPROM.update(BRIGHTNESS_SAVED_ADDR, FastLED.getBrightness());
}

/**
 * Change the current pattern by modifying the selected pattern index
 * @param difference - the number to add to the pattern index to change it (e.g. -1 or 1)
 **/
void changePattern(int difference) {  
  selectedPatternIdx += difference;

  if (selectedPatternIdx > numPatterns - 1) {
    selectedPatternIdx = 0;
  } else if (selectedPatternIdx < 0) {
    selectedPatternIdx = numPatterns - 1;
  }

  resetIndexesFlags();

  Serial.print(F("patIdx: "));
  Serial.println(selectedPatternIdx);
}

// Check to see if the proposed brightness setting is safe for the color
// This prevents the board from crashing due to power overdraw
// Note: This function is basing its maximum safe values on no more than 53 lights being lit
uint8_t makeSafeBrightness(uint8_t brightness, uint8_t colorIdx, int difference) {
  // TODO - this function needs to be tested, and possibly rewritten
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
  int minBrightness = 5;
  int maxBrightness = 230; // 90% of 255
  uint8_t currentBrightness = FastLED.getBrightness();

  // Toggle patternChanged flag if user is in a pattern that relies on brightness
  // and set brightness to the starting brightness of the pattern
  if (selectedPatternIdx == 4) {
    if (patternStartingBrightness > 0) {
      currentBrightness = patternStartingBrightness;
    }
    resetIndexesFlags();
  }
  
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

  // newBrightness = makeSafeBrightness(newBrightness, selectedPatternColorIdx, difference);

  // Serial.print(F("Brightness: "));
  // Serial.println((String)newBrightness);

  FastLED.setBrightness((uint8_t)newBrightness);
  showColumn();
}

void changeSpeed(int difference) {
  // If currently on colorWipe or sixColorPOV, change POV speed; otherwise, change animation speed
  if (selectedPatternIdx == 1 || selectedPatternIdx == 2) {
    changePOVSpeed(difference);
  } else {
    changeAnimationSpeed(difference);
  }
}

void changePOVSpeed(int difference) {
  int newPOVSpeed = POVSpeedDelay + difference;

  if (newPOVSpeed <= 0) {
    newPOVSpeed = 0;
    alertUser(COLORS[0], 2, 50, 200);
  } else if (newPOVSpeed > POVSpeedDelayMax) {
    newPOVSpeed = POVSpeedDelayMax;
  }

  POVSpeedDelay = newPOVSpeed;

  // Serial.print(F("POV Speed Delay: "));
  // Serial.println((String)POVSpeedDelay);
}

// Change animation speed
void changeAnimationSpeed(int difference) {
  int newAnimationSpeed = speedDelay + difference;

  if (newAnimationSpeed < 0) {
    newAnimationSpeed = 0;
    alertUser(COLORS[0], 2, 50, 200);
  } else if (newAnimationSpeed > maxSpeedDelay) {
    newAnimationSpeed = maxSpeedDelay;
    alertUser(COLORS[0], 2, 50, 200);
  }

  speedDelay = newAnimationSpeed;

  // Serial.print(F("Animation Speed Delay: "));
  // Serial.println((String)speedDelay);
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
  int numColors = getNumColors();
  int numPOVColors = getNumPOVColors();
  int numPOV3Colors = getNumPOV3Colors();
  int totalNumColors = numColors + numPOVColors + numPOV3Colors;
  selectedPatternColorIdx += colorIndexModifier;

  // Wrap around pattern color indexes
  if (selectedPatternColorIdx < 0) {
    selectedPatternColorIdx = totalNumColors - 1;
  } else if (selectedPatternColorIdx >= totalNumColors) {
    selectedPatternColorIdx = 0;
  }

  // Serial.println("Color_i: " + (String)selectedPatternColorIdx);

  changeBrightness(0); // Changing brightness by 0 so that we can make sure the brightness is safe for the new pattern
  resetIndexesFlags();
}

// Returns TRUE if the selected pattern color index is one of the POV color indexes, else returns FALSE
bool isPOVColorIndex(int idx) {
  int numColors = sizeof(COLORS) / sizeof(*COLORS);
  int numPOVColors = sizeof(COLORS_POV) / sizeof(*COLORS_POV);

  if (idx >= numColors && (idx < numColors + numPOVColors)) {
    return true;
  } else {
    return false;
  }
}

// Returns TRUE if the selected pattern color index is one of the POV color indexes, else returns FALSE
bool isPOV3ColorIndex(int idx) {
  int numColors = sizeof(COLORS) / sizeof(*COLORS);
  int numPOVColors = sizeof(COLORS_POV) / sizeof(*COLORS_POV);
  return (idx >= (numColors + numPOVColors)) ? true : false;
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

// Returns the number of POV3 colors in the COLORS_POV3 array
uint8_t getNumPOV3Colors() {
  return sizeof(COLORS_POV3) / sizeof(*COLORS_POV3);
}

int getNumColorsInPOV(int colorIdx) {
  int numColors = sizeof(COLORS) / sizeof(*COLORS);
  int numPOVColors = sizeof(COLORS_POV) / sizeof(*COLORS_POV);
  int numPOV3Colors = sizeof(COLORS_POV3) / sizeof(*COLORS_POV3);

  if (colorIdx >= numColors) {
    if (colorIdx >= numColors + numPOVColors) {
      return 3;
    } else {
      return 2;
    }
  } else {
    return 1;
  }
}

// Set the all pixels on the strip to the values in the patternColumn array
// and then show the pixels
void showColumn() {
  for (int i=0; i < numLEDs; i++) {
    leds[i] = patternColumn[i];
    // strip.setPixelColor(i, patternColumn[i]);
  }
  
  FastLED.show();
//  debugPatternColumn();
  
  FastLED.delay(speedDelay);
}

/*
void debugPatternColumn() {
  String litPixels = "";
  for (int i=0; i < numLEDs; i++) {
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
      rainbow();
      break;
    case 1:
      speedDelay = 0; // This pattern resets animation delay
      sixColorPOV();
      break;
    case 2:
      colorWipe(5);
      break;
    case 3:
      stackingAnimation();
      break;
    case 4:
      // Skipping this pattern until it is fixed
      // breatheAnimation();
      solidColor();
      break;
    case 5:
      twinkle();
      break;
    case 6:
      colorWipeLoop();
      break;
    case 7:
      colorFade();
      break;
    case 8:
      chase(7);
      break;
    case 9:
      chase(7,true);
      break;
    case 10:
      pong(10);
      break;
    case 11:
      speedDelay = 12; // TODO: Remove This - I was testing with a slower speed
      rainbow(true);
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

  // Make sure brightness is set to a safe value for all colors in this pattern
  // ONLY run this once when changing to the pattern
  uint8_t maxBrightness = 230; // 90% of 255
  uint8_t colorSetMaxBrightness = maxBrightness;
  uint8_t tempBrightness;
  uint8_t currentBrightness = FastLED.getBrightness();
  if (patternChanged) {
    for (int c=0; c < numColors; c++) {
      tempBrightness = makeSafeBrightness(colorSetMaxBrightness, colorIndexes[c], 0);
      if (tempBrightness < colorSetMaxBrightness) {
        colorSetMaxBrightness = tempBrightness;
      }
    }

    if (colorSetMaxBrightness < currentBrightness) {
      FastLED.setBrightness(colorSetMaxBrightness);
      showColumn();
      // Serial.print(F("Brightness changed: "));
      // Serial.println(colorSetMaxBrightness);
    }
  }

  for (uint8_t j=0; j < numLEDs; j++) {
    patternColumn[j] = COLORS[colorIndexes[pat_i_0]];
  }
  showColumn();

  FastLED.delay(POVSpeedDelay);

  // Increment color index (loop index to beginning)
  pat_i_0 = (pat_i_0 < numColors - 1) ? pat_i_0 + 1 : 0;
}

// Display a solid color
void solidColor() {
  int numColors = getNumColors();
  int numPOVColors = getNumPOVColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  bool isPOV3Color = isPOV3ColorIndex(selectedPatternColorIdx);
  int colorIterations = getNumColorsInPOV(selectedPatternColorIdx);
  uint32_t color;

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    if (isPOVColor) {
      color = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
    } else if (isPOV3Color) {
      color = COLORS[ COLORS_POV3[selectedPatternColorIdx - numColors - numPOVColors][c] ];
    } else {
      color = COLORS[selectedPatternColorIdx];
    }

    for (uint8_t i=0; i < numLEDs; i++) {
      patternColumn[i] = color;
    }

    // Insert POV delay if POV color
    if (isPOVColor || isPOV3Color) {
      FastLED.delay(POVSpeedDelay);
    }

    showColumn();    
  }
}

// Color Wipe
void colorWipe(uint8_t msDelay) {
  // TODO: after switching patterns several times, this pattern gets stuck at the beginning of its animation
  int numColors = getNumColors();
  int numPOVColors = getNumPOVColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  bool isPOV3Color = isPOV3ColorIndex(selectedPatternColorIdx);
  int colorIterations = getNumColorsInPOV(selectedPatternColorIdx);
  uint32_t color;

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    if (isPOVColor) {
      color = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
    } else if (isPOV3Color) {
      color = COLORS[ COLORS_POV3[selectedPatternColorIdx - numColors - numPOVColors][c] ];
    } else {
      color = COLORS[selectedPatternColorIdx];
    }

    for (uint8_t i=0; i <= pat_i_0; i++) {
      patternColumn[i] = color;
    }

    // Insert POV delay if POV color
    if (isPOVColor || isPOV3Color) {
      FastLED.delay(POVSpeedDelay);
    }

    showColumn();
  }

  if (!patternComplete) {
    pat_i_0++; // increase pattern index
    
    // Check to see if pattern is complete
    // TODO: shouldn't this be comparing against (numLEDs - 1)?
    if (pat_i_0 == numLEDs) {
      patternComplete = true;
    }
    FastLED.delay(msDelay); // delay to slow down the pattern animation
  }
}

// Loop through all color wipes
void colorWipeLoop() {
  // pat_i_0 is the index of the pixel we are currently filling up to for the animation (aka animation index)
  // pat_i_1 is the color index
  bool isPOVColor = isPOVColorIndex(pat_i_1);
  bool isPOV3Color = isPOV3ColorIndex(pat_i_1);
  uint8_t colorIterations = isPOV3Color ? 3 : 2; // 2 or 3 colors for POV - Also doing solid colors for 2 iterations to keep the animation timing in sync
  uint32_t color;
  int numColors = getNumColors();
  int numPOVColors = getNumPOVColors();
  int numPOV3Colors = getNumPOV3Colors();
  int totalNumColors = numColors + numPOVColors + numPOV3Colors;
  uint16_t numPixels = numLEDs;

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    if (isPOVColor) {
      color = COLORS[ COLORS_POV[pat_i_1 - numColors][c] ];
    } else if (isPOV3Color) {
      color = COLORS[ COLORS_POV3[pat_i_1 - numColors - numPOVColors][c] ];
    } else {
      color = COLORS[pat_i_1];
    }

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

    // Insert POV delay
    FastLED.delay(POVSpeedDelay);
    
    showColumn();
  }

  bool animationComplete = false;
  // Check to see if the pattern animation is now complete
  if (patDirection == 0) {
    if (pat_i_0 == numPixels - 1) {
      animationComplete = true;
      pat_i_0 = 0; // reset animation index
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
  uint16_t numPixels = numLEDs;
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

/* // NOTE: This rainbow method has a weird color bug while changing brightness
void newRainbow() {
  uint8_t deltaHue = 5; // the larger this number is, the smaller the color groupings
  
  EVERY_N_MILLISECONDS(5) { baseHue++; }; // Using FastLED macro function to perform an action at a certain time interval

  // FastLED's built-in rainbow generator
  fill_rainbow(leds, numLEDs, baseHue, deltaHue);
  FastLED.show();
}
*/

// Rainbow
void rainbow(bool sparkle) {  
  uint16_t pat_i_0_max = 256;
  uint16_t pat_i_1_max = numLEDs;

  // Reset pattern indexes if they exceed their max values
  if (pat_i_0 >= pat_i_0_max) {
    pat_i_0 = 0;
  }
  if (pat_i_1 >= pat_i_1_max) {
    pat_i_1 = 0;
  }
  while (pat_i_0 < 256) {
    while (pat_i_1 < pat_i_1_max) {
      patternColumn[pat_i_1] = Wheel(((pat_i_1 * 256 / numLEDs) + pat_i_0) & 255);
      pat_i_1++;
    }
    pat_i_0++;

    // Add a sparkle if flag is set
    if (sparkle) {
      addSparkle(20);
    }

    showColumn();
    return;
  }
}

void addSparkle(fract8 chanceOfSparkle) {
  uint8_t sparklePos;
  if(random8() < chanceOfSparkle) {
    sparklePos = random8(numLEDs - 2);
    // patternColumn[sparklePos - 1] = CRGB::White;
    patternColumn[sparklePos] = CRGB::White;
    patternColumn[sparklePos + 1] = CRGB::White;
  }
}

// Group of pixels bounce off both ends of stick
void pong(uint8_t groupSize) {
  uint32_t color;
  int numPixels = numLEDs;
  int numColors = getNumColors();
  int numPOVColors = getNumPOVColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  bool isPOV3Color = isPOV3ColorIndex(selectedPatternColorIdx);
  int colorIterations = getNumColorsInPOV(selectedPatternColorIdx);

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    if (isPOVColor) {
      color = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
    } else if (isPOV3Color) {
      color = COLORS[ COLORS_POV3[selectedPatternColorIdx - numColors - numPOVColors][c] ];
    } else {
      color = COLORS[selectedPatternColorIdx];
    }

    for (int pixel_i=0; pixel_i < numPixels; pixel_i++) {
      if ((pixel_i >= pat_i_0) && (pixel_i < pat_i_0 + groupSize)) {
        patternColumn[pixel_i] = color;
        // Serial.print(F("1"));
      } else {
        patternColumn[pixel_i] = 0;
        // Serial.print(F("0"));
      }      
    }
    showColumn();

    // Insert POV delay if POV color
    if (isPOVColor || isPOV3Color) {
      FastLED.delay(POVSpeedDelay);
    }
  }

  if (!patternReverse) {
    if (pat_i_0 == numPixels - groupSize) {
      patternReverse = true;
      pat_i_0--;
    } else {
      pat_i_0++;
    }
  } else {
    if (pat_i_0 == 0) {
      patternReverse = false;
      pat_i_0++;
    } else {
      pat_i_0--;
    }
  }
  
}

void chase(uint8_t groupSize, bool centerOrigin) {
  uint32_t color;
  int numPixels = numLEDs;
  int numColors = getNumColors();
  int numPOVColors = getNumPOVColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  bool isPOV3Color = isPOV3ColorIndex(selectedPatternColorIdx);
  int colorIterations = getNumColorsInPOV(selectedPatternColorIdx);
  int offsetIdx;

  // If pattern direction is reversed, initialize iterator to last pixel index
  if (patternChanged && (patDirection == 1)) {
    patternChanged = false;
    pat_i_0 = numPixels;
  }

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    if (isPOVColor) {
      color = COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ];
    } else if (isPOV3Color) {
      color = COLORS[ COLORS_POV3[selectedPatternColorIdx - numColors - numPOVColors][c] ];
    } else {
      color = COLORS[selectedPatternColorIdx];
    }

    offsetIdx = pat_i_0;

    // Serial.println(F("pixel_i  pat_i_0  offsetIdx"));
    // Serial.println("offset_" + (String)offsetIdx + " ");

    for (int pixel_i=0; pixel_i < numPixels; pixel_i++) {
      
      // This causes the chasing animation to originate from the center of the stick
      if (centerOrigin) {
        // Reverse direction of animation half way down the light strip
        if (pixel_i > (numPixels / 2 - 1)) {
          offsetIdx = pat_i_0 * -1;
        }
      }

      // Serial.print((String)pixel_i + "       ");
      // Serial.print((String)pat_i_0 + "       ");
      // Serial.println((String)offsetIdx);
    
      // Serial.print((String)((pixel_i + offsetIdx) % (groupSize * 2)) + "  ");
      if (abs((pixel_i + offsetIdx) % (groupSize * 2)) < groupSize) {
        patternColumn[pixel_i] = color;
        // Serial.print(F("1 "));
      } else {
        patternColumn[pixel_i] = 0;
        // Serial.print(F("0 "));
      }
    }
    showColumn();

    // Insert POV delay if POV color
    if (isPOVColor || isPOV3Color) {
      FastLED.delay(POVSpeedDelay);
    }
    // Serial.println("");
  }

  if (!isPOVColor && !isPOV3Color) {
    FastLED.delay(10);
  }

  // Increment offset iterator and wraparound
  if (patDirection == 0) {
    pat_i_0 = (pat_i_0 <= numPixels) ? (pat_i_0 + 1) : 0;
  } else {
    pat_i_0 = (pat_i_0 >= 0) ? (pat_i_0 - 1) : (numPixels);
  }
}

// Stacking animation
void stackingAnimation() {
  // TODO: after switching patterns several times, this pattern gets stuck at the beginning of its animation
  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  int colorIterations = isPOVColor ? 2 : 1; // 2 colors for POV
  int numPixels = numLEDs;
  uint32_t color;
  uint8_t travelGroupSize = 3;

  // pat_i_0 is the last pixel in the traveling range
  // pat_i_1 is

  // Serial.println("i_0: " + (String)pat_i_0);
  // Serial.println("i_1: " + (String)pat_i_1);

  if (patDirection == 0) {
    // Reset indexes when needed
    if (pat_i_0 == 0) {
      pat_i_0 = numPixels - 1;
    }
    if (pat_i_1 == pat_i_0) {
      pat_i_0 = (pat_i_0 - travelGroupSize > 0) ? pat_i_0 - travelGroupSize : 0;
      pat_i_1 = 0;
    }

    for (int c=0; c < colorIterations; c++) {
      // Get the color
      color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];

      // Serial.println(F("travel_i: "));

      // Make 3 pixels travel down the stick
      for (int travel_i = 2; travel_i < pat_i_0; travel_i++) {
        if (travel_i == pat_i_1 || travel_i == pat_i_1 - 1 || travel_i == pat_i_1 - 2) {
          patternColumn[travel_i - 2] = color;
          patternColumn[travel_i - 1] = color;
          patternColumn[travel_i] = color;
          // Serial.print(F("1"));
        } else {
          patternColumn[travel_i] = 0; // BLACK
          // Serial.print(F("0"));
        }
      }
    
      // Turn on the lit pixels
      for (int lit_i = pat_i_0; lit_i < numPixels; lit_i++) {
        patternColumn[lit_i] = color;
        // Serial.print(F("1"));
      }

      // Serial.println("");
      
      // Insert POV delay if POV color
      if (isPOVColor) {
        FastLED.delay(POVSpeedDelay);
      }

      showColumn();
    }

    pat_i_1++;
  } else {
    // Reset indexes when needed
    if (patternChanged) {
      // Initialize this index when this when the pattern is first changed
      pat_i_1 = numPixels - 1;
      patternChanged = false;
    }
    if (pat_i_0 == numPixels - 1) {
      pat_i_0 = 0; // reset to 1 instead of 0 because of base case problem
    }
    if (pat_i_1 == pat_i_0) {
      // pat_i_0++;
      pat_i_0 = (pat_i_0 + travelGroupSize <= numPixels - 1) ? pat_i_0 + travelGroupSize : numPixels - 1;
      pat_i_1 = numPixels - 1;
    }
  
    for (int c=0; c < colorIterations; c++) {
      // Get the color
      color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];

      // Serial.println(F("travel_i: "));

      // Make a single pixel travel down the stick
      for (int travel_i = numPixels - 1; travel_i > pat_i_0 + 2; travel_i--) {
        if (travel_i == pat_i_1 || travel_i == pat_i_1 + 1 || travel_i == pat_i_1 + 2) {
          patternColumn[travel_i + 2] = color;
          patternColumn[travel_i + 1] = color;
          patternColumn[travel_i] = color;

          // Serial.print(F("1"));
        } else {
          patternColumn[travel_i] = 0; // BLACK

          // Serial.print(F("0"));
        }
      }
    
      // Turn on the lit pixels
      for (int lit_i = pat_i_0; lit_i >= 0; lit_i--) {
        patternColumn[lit_i] = color;

        // Serial.print(F("1"));
      }

      // Serial.println("");
      
      // Insert POV delay if POV color
      if (isPOVColor) {
        FastLED.delay(POVSpeedDelay);
      }

      showColumn();
    }

    pat_i_1--;
  }
}

// Breathe Animation
void breatheAnimation() {
  // TODO: Fix this pattern. It is causing the stick to get stuck sometimes, and also messing up the brightness when you go to another pattern
  // NOTE: In order to change the top-end brightness of this pattern, user
  // must switch to a different pattern, change the brightness, and then switch back.

  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedPatternColorIdx);
  int colorIterations = isPOVColor ? 2 : 1; // 2 colors for POV
  uint32_t color;

  uint8_t minPatternBrightness = 10;
  int numRepeats = 4;
  
  // Loop the brightness from minBrightness up to the brightness
  // setting the user has set.
  // Initialize brightness
  if (patternChanged) {
    pat_i_0 = FastLED.getBrightness(); // pat_i_0 is the max brightness
    pat_i_1 = pat_i_0;
    patternChanged = false;
    patternStartingBrightness = pat_i_0;

    // Serial.print(F("Max breathe brightness: "));
    // Serial.println(pat_i_0);
  }

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];
    
    // Light the pixels
    for (int i = 0; i < numLEDs; i++) {
      patternColumn[i] = color;
    }
    FastLED.setBrightness(pat_i_1);

    // Insert POV delay if POV color
    if (isPOVColor) {
      FastLED.delay(POVSpeedDelay);
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

// Twinkle Animation
void twinkle() {
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

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    color = isPOVColor ? COLORS[ COLORS_POV[selectedPatternColorIdx - numColors][c] ] : COLORS[selectedPatternColorIdx];

    // Light 3 random pixels out of every group of 15
    while (pixel_i < numLEDs) {
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
      FastLED.delay(POVSpeedDelay);
    }

    showColumn();
  }
}

void setAllPixels(uint32_t color) {
  for (int i=0; i < numLEDs; i++) {
    patternColumn[i] = color;
  }
  showColumn();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  uint32_t newColor;

  if (WheelPos < 85) {
    newColor = ((uint32_t)(255 - WheelPos * 3) << 16) + 0 + (WheelPos * 3);
    // return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    newColor = 0 + ((uint32_t)(WheelPos * 3) << 8) + (255 - WheelPos * 3);
    // return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    newColor = ((uint32_t)(WheelPos * 3) << 16) + ((uint32_t)(255 - WheelPos * 3) << 8) + 0;
    // return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  return newColor;
}

// Alert user by flashing stick
void alertUser(uint32_t color, uint8_t numFlashes, uint16_t midDelay, uint16_t endDelay) {    
  for (uint8_t i=0; i < numFlashes; i++) {
    setAllPixels(0); // BLACK
    FastLED.delay(midDelay);
    setAllPixels(color);
    FastLED.delay(midDelay);
    // Serial.println("flash " + (String)i);
  }
  FastLED.delay(endDelay);

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
  patternReverse = false;

  // Set brightness back to what it was if it was changed for a pattern
  if (patternStartingBrightness > 0) {
    FastLED.setBrightness(patternStartingBrightness);
    patternStartingBrightness = 0;
  }
}

/**
 * Get the current state of a button
 * @param btnPin
 * @return true if button is pressed, otherwise false
 **/
bool getButtonState(uint16_t btnPin) {
  return digitalRead(btnPin) == LOW ? true : false;
}

/**
 * Perform the action for a single or multi-button press
 * @param btnsVal - An 8-bit value indicating which button or combination of buttons are pressed
 * @param longPress - A boolean value indicating whether it is a short press or long press. Default is false, for short press
 * Example of btnsVal: Pressing btn2 and btn3 = 110(base2) = 6(base10))
 **/
void btnAction(uint8_t btnsVal, bool longPress = false) {
  uint8_t brightnessDiff = 1;

  // Action Interval is the interval at which actions should be performed. If the interval is 1,
  // actions will be performed every cycle. If the interval is 5, actions will be performed every 5 cycles.
  uint8_t actionInterval = 1; // this value MUST be greater than 0
  
  // if (longPress) Serial.print("Long ");
  // Serial.print("Press ");

  switch (btnsVal) {
    // Button 1
    case 1:
      // Serial.println("1");
      changePattern(1);
      setAutoCycle(false);
      break;

    // Button 2
    case 2:
      toggleAutoCycle();
      break;

    // Buttons 1, 2
    case 3:
      // Serial.println("1 2");
      // Slow down the long press action using modulus on the cycle counter
      actionInterval = 1; // NOT USING SLOWDOWN RIGHT NOW
      if (longPress && (cycleCounter % actionInterval == 0)) {
        changeBrightness(brightnessDiff);
      }
      break;

    // Button 3
    case 4:
      // Serial.println("3");
      changePattern(-1);
      setAutoCycle(false);
      break;

    // Buttons 1, 3
    case 5:
      // Serial.println("1 3");
      if (longPress) getFavorite(0); // Set to HOME pattern
      break;
      
    // Buttons 2, 3
    case 6:
      // Serial.println("2 3");
      actionInterval = 1; // NOT USING SLOWDOWN RIGHT NOW
      if (longPress && (cycleCounter % actionInterval == 0)) {
        changeBrightness(-brightnessDiff);
      }
      break;

    // Buttons 1, 2, 3
    case 7:
      // Serial.println("1 2 3");
      // Save the current pattern as the HOME pattern
      setFavorite(0);

      // Set pattern to the OFF pattern
      selectedPatternIdx = -1;
      
      break;    
  }
}

// Check for button input
void checkButtonPressNew() {  
  // Get button states
  bool btn1 = getButtonState(BTN_1_PIN);
  bool btn2 = getButtonState(BTN_2_PIN);
  bool btn3 = getButtonState(BTN_3_PIN);
  uint8_t currentBtnsVal = 0;
  bool btnStateChanged = false;
  bool btnReleased = false;

  // Check to see if a button state has changed
  if (btn1) currentBtnsVal += 1;
  if (btn2) currentBtnsVal += 2;
  if (btn3) currentBtnsVal += 4;
  if (currentBtnsVal != prevBtnsVal) {
    btnStateChanged = true;

    // Use these print statements when figuring out multi-button short presses
    // Serial.print(prevBtnsVal);
    // Serial.print(" - ");
    // Serial.println(currentBtnsVal);
  }

  if (btnStateChanged) {
    // if at least one button is pressed, restart the timer
    if (currentBtnsVal > 0) {
      buttonPressStartTime = millis();
    }
    // else, a button was released
    else {
      btnReleased = true;
    }
  }
  // if the button state did NOT change, and there was a button pressed in the previous cycle
  else if (prevBtnsVal > 0) {
    // Check for long press
    if (millis() - buttonPressStartTime >= longButtonPressTime) {
      // perform long button press action
      btnAction(prevBtnsVal, true);
      longButtonPressActionPerformed = true;
    }
  }

  // when buttons are released
  if (btnReleased) {
    // If long press action was not performed, perform button press action
    // Long press actions are performed before the buttons are released, so they should not be repeated here
    if (!longButtonPressActionPerformed) {
      // perform button press action
      btnAction(prevBtnsVal);
    }

    // reset flag indicating that a long button press action has been performed
    longButtonPressActionPerformed = false;
  }

  // update the previous buttons value
  prevBtnsVal = currentBtnsVal;
}

// Toggle auto cycle mode ON/OFF 
void toggleAutoCycle() {
  autoCycle = !autoCycle;
  
  Serial.print(F("Auto:"));
  if (autoCycle) {
    autoCycleTimerStart = millis();
    Serial.println(F("ON"));
  } else {
    Serial.println(F("OFF"));
  }
}

// Set auto cycle mode ON/OFF
void setAutoCycle(bool state) {
  autoCycle = state;
}

// Check to see if the auto cycle time interval has elapsed
// return true if time has elapsed, else return false
bool autoCycleTimeHasElapsed() {
  uint16_t autoCycleInterval = 3000; // 3 seconds

  if (millis() - autoCycleTimerStart > autoCycleInterval) {
    autoCycleTimerStart = millis();
    return true;
  } else {
    return false;
  }

  return (millis() - autoCycleTimerStart) > autoCycleInterval ? true : false;
}

void loop() {
  
  // autoCycleTimerStart

  showPattern();

  checkButtonPressNew();

  // If auto cycle mode is on, change to next pattern 
  if (autoCycle && autoCycleTimeHasElapsed()) {
    changePattern(1);
  }

  cycleCounter++;
}
