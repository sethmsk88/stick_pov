#include <FastLED.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <EEPROM.h>

#define DATA_PIN 11
#define BTN_1_PIN 3 // Stick: 3, Brain Box: 5
#define BTN_2_PIN 5 // Stick: 5, Brain Box: 8
#define BTN_3_PIN 9 // Stick: 9, Brain Box: 12

#define MAX_TIME_VALUE 0xFFFFFFFF
#define MAX_EEPROM_ADDR 1023

// Function prototypes
void chase(uint8_t groupSize, bool centerOrigin = false);
void rainbow(bool sparkle = false);

//////   BEN, YOU CAN CHANGE THESE TWO VALUES   //////
const uint8_t numLEDs = 53; // The stick has 53 LEDs
int POVSpeedDelay = 16; // Milliseconds of delay between colors
uint8_t POV_2_DELAY = 10;
uint8_t POV_3_DELAY = 5;
uint8_t POV_6_DELAY = 10;
uint16_t AUTO_INTERVAL = 5000; // Milliseconds between pattern changes in auto mode

const uint8_t MAX_LEDS = 100; // DO NOT CHANGE THIS
// IMPORTANT NOTE: numLEDs value must also be changed in the applySavedSettings() function if a change to the LED count is made

// Version number should be incremented each time an update is pushed
const uint16_t VERSION = 0;

// EEPROM: 1024 Bytes
// Each favorite saves four bytes worth of info, so each is allocated four addresses
// 1st address contains the index of the saved pattern
// 2nd address contains the speed delay of the saved pattern
// 3rd address contains the color for the saved pattern
// 4th address contains the POV speed delay for the saved pattern
const uint8_t UNSET_EEPROM_VAL = 255; // Initial state for all EEPROM addresses
const uint16_t HOME_ADDR = 0;
const uint16_t BRIGHTNESS_SAVED_ADDR = 4;
const uint16_t VERSION_ADDR = 1022; // Last 2 bytes of EEPROM

// Colors are in GRB format
const uint32_t COLORS[] = {
  CRGB::Red,        // 0
  CRGB::Green,      // 1
  CRGB::Blue,       // 2
  CRGB::Yellow,     // 3
  CRGB::Cyan,       // 4
  CRGB::Magenta,    // 5
  CRGB::Purple,     // 6
  CRGB::Orange,     // 7
  CRGB::LightGreen, // 8
  CRGB::White       // 9
  // 0x004D99, // Old purple
};
const uint8_t POV_COLOR_SETS_2[][2] = {
  {0,1},  // Red / Green
  {0,2},  // Red / Blue
  {1,2},  // Green / Blue
  {0,4},  // Red / Cyan
  {6,7},  // Purple / Orange
  {1,7},  // Green / Orange
  {4,6}   // Cyan / Purple
}; // combinations of color indexes for POV

const uint8_t POV_COLOR_SETS_3[][3] = {
  {0,1,2} // Red / Green / Blue
};

uint8_t defaultBrightness = 105; // Default is set to 50% of the brightness range

// uint32_t patternColumn[MAX_LEDS] = {};
int selectedPatternIdx = 0; // default pattern index
int selectedColorIdx = 0; // default color index
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
  applySavedSettings();
  
  // strip = Adafruit_NeoPixel(numLEDs, DATA_PIN, NEO_GRB + NEO_KHZ800);
  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, numLEDs);

  // Initialize buttons
  pinMode(BTN_1_PIN, INPUT_PULLUP);
  pinMode(BTN_2_PIN, INPUT_PULLUP);
  pinMode(BTN_3_PIN, INPUT_PULLUP);

  FastLED.setBrightness(defaultBrightness);

  randomSeed(analogRead(0)); // seed random number generator for functions that need it

  // TESTING
  selectedColorIdx = 8; // TESTING
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
  
  // Load HOME pattern if it has previously been saved
  if (EEPROM.read(HOME_ADDR) != UNSET_EEPROM_VAL) {
    selectedPatternIdx = EEPROM.read(HOME_ADDR);
  } else {
    // Else, load the default pattern, and save it as the HOME pattern
    selectedPatternIdx = 0; // default pattern
    EEPROM.update(HOME_ADDR, selectedPatternIdx);
  }

  // Load brightness setting if it has previously been saved
  if (EEPROM.read(BRIGHTNESS_SAVED_ADDR) != UNSET_EEPROM_VAL) {
    defaultBrightness = EEPROM.read(BRIGHTNESS_SAVED_ADDR);
  }

  /**************
  // if favorites have not yet been saved, set them to 0
  int numFavorites = 5;
  for (int i=0; i < numFavorites; i++) {
    patternIndex_addr = getFavoriteAddr(i, 0); // Load pattern for this favorite
    speedDelay_addr = getFavoriteAddr(i, 1); // Load speed delay for this favorite
    patternColorIndex_addr = getFavoriteAddr(i, 2); // Load color index for this favorite
    POVSpeedDelay_addr = getFavoriteAddr(i, 3); // Load color index for this favorite

    if (EEPROM.read(patternIndex_addr) == UNSET_EEPROM_VAL) {
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
  ***************/
}


// Save a favorite
// TODO: This function needs to be re-written to use the new memory addresses
void setFavorite(uint8_t i) {
  if (selectedPatternIdx < 0) {
    // Serial.println("ERROR: Cannot save a pattern favorite whose index is negative");
    return;
  }
  
  // int patternIndex_addr = getFavoriteAddr(i, 0);
  // int speedDelay_addr = getFavoriteAddr(i, 1);
  // int patternColorIndex_addr = getFavoriteAddr(i, 2);
  // int POVSpeedDelay_addr = getFavoriteAddr(i, 3);
  
  // EEPROM.update(patternIndex_addr, selectedPatternIdx);
  // EEPROM.update(speedDelay_addr, speedDelay);
  // EEPROM.update(patternColorIndex_addr, selectedColorIdx);
  // EEPROM.update(POVSpeedDelay_addr, POVSpeedDelay);

  // alertUser(COLORS[1], 2, 50, 200); // Flash stick green to alert a save
  // Serial.println("Favorite " + (String)i + " saved:");
}


// Get a saved favorite, and make it active
// TODO: This function needs to be re-written to use the new memory addresses
void getFavorite(uint8_t i) {
  /*
  int patternIndex_addr = getFavoriteAddr(i, 0);
  int speedDelay_addr = getFavoriteAddr(i, 1);
  int patternColorIndex_addr = getFavoriteAddr(i, 2);
  int POVSpeedDelay_addr = getFavoriteAddr(i, 3);
  
  selectedPatternIdx = EEPROM.read(patternIndex_addr);
  speedDelay = EEPROM.read(speedDelay_addr);
  selectedColorIdx = EEPROM.read(patternColorIndex_addr);
  POVSpeedDelay = EEPROM.read(POVSpeedDelay_addr);
  
  resetIndexesFlags();
  */
}

// Get address of favorite in EEPROM
// fav_i - Favorite number (0 - 4)
// attr_i - index of the favorite attribute (0 - 3)
/*
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
*/


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
    alertUser(COLORS[0], 2, 50, 200);
  } else if (newBrightness > maxBrightness) {
    newBrightness = maxBrightness;
    alertUser(COLORS[0], 2, 50, 200);
  }

  FastLED.setBrightness((uint8_t)newBrightness);
  FastLED.show();
  FastLED.delay(speedDelay);
}


// TODO: Rewrite speed changing functions
/*
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
}
*/


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
  int numPOVColors = getNumPov2Sets();
  int numPOV3Colors = getNumPov3Sets();
  int totalNumColors = numColors + numPOVColors + numPOV3Colors;
  selectedColorIdx += colorIndexModifier;

  // Wrap around pattern color indexes
  if (selectedColorIdx < 0) {
    selectedColorIdx = totalNumColors - 1;
  } else if (selectedColorIdx >= totalNumColors) {
    selectedColorIdx = 0;
  }

  // Serial.println("Color_i: " + (String)selectedColorIdx);

  changeBrightness(0); // Changing brightness by 0 so that we can make sure the brightness is safe for the new pattern
  resetIndexesFlags();
}


// Returns the number of colors in the COLORS array
uint8_t getNumColors() {
  return sizeof(COLORS) / sizeof(*COLORS);
}


// Returns the number of POV2 color sets in the POV_COLOR_SETS_2 array
uint8_t getNumPov2Sets() {
  return sizeof(POV_COLOR_SETS_2) / sizeof(*POV_COLOR_SETS_2);
}


// Returns the number of POV3 color sets in the POV_COLOR_SETS_3 array
uint8_t getNumPov3Sets() {
  return sizeof(POV_COLOR_SETS_3) / sizeof(*POV_COLOR_SETS_3);
}


// Returns the total number of color sets
uint8_t getNumColorSets() {
  return getNumColors() + getNumPov2Sets() + getNumPov3Sets();
}


// Get the number of colors in the selected color set
// NOTE: if more than 1 color, it is a POV color set
uint8_t getNumColorsInSet(uint8_t colorIdx) {
  uint8_t numColors = getNumColors();
  uint8_t numPov2Sets = getNumPov2Sets();

  if (colorIdx >= numColors + numPov2Sets) {
    return 3;
  } else if (colorIdx >= numColors) {
    return 2;
  } else {
    return 1;
  }
}


// Get the index of a selected POV color within its own color set array
uint8_t getPovColorIdx(uint8_t colorIdx, uint8_t numColorsInSet) {
  uint8_t numColors = getNumColors();
  uint8_t numPov2Sets = getNumPov2Sets();

  if (numColorsInSet == 3) {
    return colorIdx - numPov2Sets - numColors;
  } else if (numColorsInSet == 2) {
    return colorIdx - numColors;
  }
}


// Delay to be inserted between colors in POV sets
void delayPov(uint8_t numColorsInSet) {
  if (numColorsInSet == 2) {
    FastLED.delay(POV_2_DELAY);
  } else if (numColorsInSet == 3) {
    FastLED.delay(POV_3_DELAY);
  } else if (numColorsInSet == 6) {
    FastLED.delay(POV_6_DELAY);
  }
}


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
      sixColorPOV();
      break;
    case 2:
      colorWipe();
      break;
    case 3:
      stackingAnimation();
      break;
    case 4:
      // Skipping this pattern until it is fixed
      // breatheAnimation();
      colorWipe();
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
      rainbow(true);
      break;
  }
}


// Set all pixels to black, so very little power is used
void patternOff() {
  setAllPixels(0); // Set to BLACK
}


// 6-color POV
// NOTE: This pattern returns after each color is flashed, so we don't spend
// too much time in this function due to POV delays.
void sixColorPOV() {  
  int colorIndexes[] = {2,1,0,6,7,3};
  int numColorsInSet = sizeof(colorIndexes) / sizeof(*colorIndexes);

  for (uint8_t i=0; i < numLEDs; i++) {
    leds[i] = COLORS[colorIndexes[pat_i_0]];
  }
  FastLED.show();
  delayPov(numColorsInSet);

  // Increment color index (loop index to beginning)
  pat_i_0 = (pat_i_0 < numColorsInSet - 1) ? pat_i_0 + 1 : 0;
}


// Display a solid color
/*
void solidColor() {
  int numColors = getNumColors();
  int numPOVColors = getNumPov2Sets();
  bool isPOVColor = isPOVColorIndex(selectedColorIdx);
  bool isPOV3Color = isPOV3ColorIndex(selectedColorIdx);
  int colorIterations = getNumColorsInPOV(selectedColorIdx);
  uint32_t color;

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    if (isPOVColor) {
      color = COLORS[ POV_COLOR_SETS_2[selectedColorIdx - numColors][c] ];
    } else if (isPOV3Color) {
      color = COLORS[ POV_COLOR_SETS_3[selectedColorIdx - numColors - numPOVColors][c] ];
    } else {
      color = COLORS[selectedColorIdx];
    }

    for (uint8_t i=0; i < numLEDs; i++) {
      leds[i] = color;
    }

    // Insert POV delay if POV color
    if (isPOVColor || isPOV3Color) {
      FastLED.delay(POVSpeedDelay);
    }

    FastLED.show();
    FastLED.delay(speedDelay);
  }
}
*/


// Color Wipe
void colorWipe() {
  uint8_t numColorsInSet = getNumColorsInSet(selectedColorIdx);
  uint32_t color;

  for (uint8_t color_i = 0; color_i < numColorsInSet; color_i++) {
    if (numColorsInSet == 3) {
      color = COLORS[ POV_COLOR_SETS_3[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else if (numColorsInSet == 2) {
      color = COLORS[ POV_COLOR_SETS_2[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else {
      color = COLORS[selectedColorIdx];
    }

    for (uint8_t i=0; i <= pat_i_0; i++) {
      leds[i] = color;
    }
    FastLED.show();
    delayPov(numColorsInSet);
  }

  if (!patternComplete) {
    pat_i_0++; // increase pattern index
    
    // Check to see if pattern is complete
    if (pat_i_0 == numLEDs - 1) {
      patternComplete = true;
    }
    FastLED.delay(speedDelay);
  }
}


// Loop through all color wipes
void colorWipeLoop() {
  // pat_i_0 is the index of the pixel we are currently filling up to for the animation (aka animation index)
  // pat_i_1 is the color index
  uint8_t colorIdx = pat_i_1; // Copying to descriptive variable name for clarity
  uint8_t numColorsInSet = getNumColorsInSet(colorIdx);
  uint32_t color;
  
  for (uint8_t color_i = 0; color_i < numColorsInSet; color_i++) {
    if (numColorsInSet == 3) {
      color = COLORS[ POV_COLOR_SETS_3[getPovColorIdx(colorIdx, numColorsInSet)][color_i] ];
    } else if (numColorsInSet == 2) {
      color = COLORS[ POV_COLOR_SETS_2[getPovColorIdx(colorIdx, numColorsInSet)][color_i] ];
    } else {
      color = COLORS[selectedColorIdx];
    }

    for (uint8_t i=0; i <= pat_i_0; i++) {
      leds[i] = color;
    }

    FastLED.show();
    delayPov(numColorsInSet);
  }

  FastLED.delay(speedDelay);
  
  bool animationComplete = false;
  // Check to see if the pattern animation is now complete
  if (pat_i_0 == numLEDs - 1) {
    animationComplete = true;
    pat_i_0 = 0; // reset animation index
  } else {
    pat_i_0++;
  }

  if (animationComplete) {
    // Advance the color index
    // Loop back to the first color if we reached the last color, else, increment color index
    pat_i_1 = (pat_i_1 == getNumColorSets() - 1) ? 0 : pat_i_1 + 1;
  }

  /* DISABLED PATTERN DIRECTION FEATURE - OLD CODE
  // Light the pixels
  if (patDirection == 0) {
    for (uint16_t pixel_i=0; pixel_i <= pat_i_0; pixel_i++) {
      leds[pixel_i] = color;
    }
  } else {
    for (int pixel_i = numPixels - 1; pixel_i >= pat_i_0; pixel_i--) {
      leds[pixel_i] = color;
    }
  }

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
  */
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

  for (int pixel_i=0; pixel_i < numPixels; pixel_i++) {
    leds[pixel_i] = gapColor;
  }
  FastLED.show();
  FastLED.delay(speedDelay);

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
      leds[pat_i_1] = Wheel(((pat_i_1 * 256 / numLEDs) + pat_i_0) & 255);
      pat_i_1++;
    }
    pat_i_0++;

    // Add a sparkle if flag is set
    if (sparkle) {
      addSparkle(20);
    }

    FastLED.show();
    FastLED.delay(speedDelay);
    return;
  }
}


// Set a random LED in the pattern to white
void addSparkle(fract8 chanceOfSparkle) {
  uint8_t sparklePos;
  if(random8() < chanceOfSparkle) {
    sparklePos = random8(numLEDs - 2);
    leds[sparklePos] = CRGB::White;
    leds[sparklePos + 1] = CRGB::White;
  }
}


// Group of pixels bounce off both ends of stick
void pong(uint8_t groupSize) {
  uint8_t numColorsInSet = getNumColorsInSet(selectedColorIdx);
  uint32_t color;

  for (uint8_t color_i = 0; color_i < numColorsInSet; color_i++) {
    if (numColorsInSet == 3) {
      color = COLORS[ POV_COLOR_SETS_3[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else if (numColorsInSet == 2) {
      color = COLORS[ POV_COLOR_SETS_2[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else {
      color = COLORS[selectedColorIdx];
    }

    for (uint8_t pixel_i=0; pixel_i < numLEDs; pixel_i++) {
      if ((pixel_i >= pat_i_0) && (pixel_i < pat_i_0 + groupSize)) {
        leds[pixel_i] = color;
      } else {
        leds[pixel_i] = 0;
      }      
    }
    FastLED.show();
    delayPov(numColorsInSet); // Inserts delay if selected color set is POV
  }

  // Reverse direction of traveling pixel group when group gets to the end of the LED strip
  if (!patternReverse) {
    if (pat_i_0 == numLEDs - groupSize) {
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
  uint8_t numColorsInSet = getNumColorsInSet(selectedColorIdx);
  uint32_t color;
  int offsetIdx;  // must be signed int
  int numLEDs_int = numLEDs; // must be signed int

  // TODO: Re-activate pattern direction change code below
  /*****
  // Pattern direction change code
  // If pattern direction is reversed, initialize iterator to last pixel index
  if (patternChanged && (patDirection == 1)) {
    patternChanged = false;
    pat_i_0 = numPixels;
  }
  *****/

  for (uint8_t color_i = 0; color_i < numColorsInSet; color_i++) {
    if (numColorsInSet == 3) {
      color = COLORS[ POV_COLOR_SETS_3[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else if (numColorsInSet == 2) {
      color = COLORS[ POV_COLOR_SETS_2[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else {
      color = COLORS[selectedColorIdx];
    }

    offsetIdx = pat_i_0;

    for (int pixel_i=0; pixel_i < numLEDs_int; pixel_i++) {
      // Reverse direction of animation half way down the light strip
      if (centerOrigin) {
        if (pixel_i > (numLEDs_int / 2 - 1)) {
          offsetIdx = pat_i_0 * -1;
        }
      }

      if (abs((pixel_i + offsetIdx) % (groupSize * 2)) < groupSize) {
        leds[pixel_i] = color;
      } else {
        leds[pixel_i] = 0;
      }
    }

    FastLED.show();
    delayPov(numColorsInSet); // Inserts delay if selected color set is POV
  }

  // FastLED.delay(speedDelay);

  // Increment offset iterator and wraparound
  pat_i_0 = (pat_i_0 < numLEDs_int - 1) ? (pat_i_0 + 1) : 0;

  // TODO: Re-activate pattern direction change code below
  /****
  // Pattern direction change code
  if (patDirection == 0) {
    pat_i_0 = (pat_i_0 < numLEDs_int - 1) ? (pat_i_0 + 1) : 0;
  } else {
    pat_i_0 = (pat_i_0 >= 0) ? (pat_i_0 - 1) : (numLEDs_int);
  }
  ****/
}


// Stacking animation
void stackingAnimation() {
  uint8_t numColorsInSet = getNumColorsInSet(selectedColorIdx);
  uint32_t color;
  uint8_t travelGroupSize = 3;

  // Reset indexes when needed
  if (pat_i_0 == 0) {
    pat_i_0 = numLEDs - 1;
  }
  if (pat_i_1 == pat_i_0) {
    pat_i_0 = (pat_i_0 - travelGroupSize > 0) ? pat_i_0 - travelGroupSize : 0;
    pat_i_1 = 0;
  }

  for (uint8_t color_i = 0; color_i < numColorsInSet; color_i++) {
    if (numColorsInSet == 3) {
      color = COLORS[ POV_COLOR_SETS_3[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else if (numColorsInSet == 2) {
      color = COLORS[ POV_COLOR_SETS_2[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else {
      color = COLORS[selectedColorIdx];
    }

    // Make 3 pixels travel down the stick
    for (int travel_i = 2; travel_i < pat_i_0; travel_i++) {
      if (travel_i == pat_i_1 || travel_i == pat_i_1 - 1 || travel_i == pat_i_1 - 2) {
        leds[travel_i - 2] = color;
        leds[travel_i - 1] = color;
        leds[travel_i] = color;
      } else {
        leds[travel_i] = 0; // OFF
      }
    }

    // Turn on the lit pixels
    for (int lit_i = pat_i_0; lit_i < numLEDs; lit_i++) {
      leds[lit_i] = color;
    }

    FastLED.show();
    delayPov(numColorsInSet); // Inserts delay if selected color set is POV
  }

  pat_i_1++;

  // TODO: Maybe re-activate this reversed pattern direction
/*******
  // START Reversed pattern direction
  // Reset indexes when needed
  if (patternChanged) {
    // Initialize this index when this when the pattern is first changed
    pat_i_1 = numLEDs - 1;
    patternChanged = false;
  }
  if (pat_i_0 == numLEDs - 1) {
    pat_i_0 = 0; // reset to 1 instead of 0 because of base case problem
  }
  if (pat_i_1 == pat_i_0) {
    pat_i_0 = (pat_i_0 + travelGroupSize <= numLEDs - 1) ? pat_i_0 + travelGroupSize : numLEDs - 1;
    pat_i_1 = numLEDs - 1;
  }

  for (int c=0; c < colorIterations; c++) {
    // Get the color
    color = isPOVColor ? COLORS[ POV_COLOR_SETS_2[selectedColorIdx - numColors][c] ] : COLORS[selectedColorIdx];

    // Make a single pixel travel down the stick
    for (int travel_i = numLEDs - 1; travel_i > pat_i_0 + 2; travel_i--) {
      if (travel_i == pat_i_1 || travel_i == pat_i_1 + 1 || travel_i == pat_i_1 + 2) {
        leds[travel_i + 2] = color;
        leds[travel_i + 1] = color;
        leds[travel_i] = color;
      } else {
        leds[travel_i] = 0; // OFF
      }
    }
    // Turn on the lit pixels
    for (int lit_i = pat_i_0; lit_i >= 0; lit_i--) {
      leds[lit_i] = color;
    }
    FastLED.show();
    FastLED.delay(speedDelay);
  }

  pat_i_1--;
  // END Reversed pattern direction
******/
}


// Twinkle Animation
void twinkle() {
  uint8_t numColorsInSet = getNumColorsInSet(selectedColorIdx);
  uint32_t color;
  // group the lights into groups of 15
  int groupSize = 15;
  int pixel_i = 0;
  int group_i = 0;
  int rand_i_0, rand_i_1, rand_i_2;

  for (uint8_t color_i = 0; color_i < numColorsInSet; color_i++) {
    if (numColorsInSet == 3) {
      color = COLORS[ POV_COLOR_SETS_3[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else if (numColorsInSet == 2) {
      color = COLORS[ POV_COLOR_SETS_2[getPovColorIdx(selectedColorIdx, numColorsInSet)][color_i] ];
    } else {
      color = COLORS[selectedColorIdx];
    }

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
        leds[pixel_i] = color;
      } else {
        leds[pixel_i] = 0; // OFF
      }
      pixel_i++;
    }
    
    FastLED.show();
    delayPov(numColorsInSet); // Inserts delay if selected color set is POV
  }

  // FastLED.delay(speedDelay);
}


void setAllPixels(uint32_t color) {
  for (int i=0; i < numLEDs; i++) {
    leds[i] = color;
  }
  FastLED.show();
  FastLED.delay(speedDelay);
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
  FastLED.show();
  FastLED.delay(speedDelay);
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
      if (longPress) {
        Serial.println("Set to HOME");
        selectedPatternIdx = EEPROM.read(HOME_ADDR); // Set to HOME pattern
      }
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
      // Set stick to OFF mode
      // If already in OFF mode, break
      if (selectedPatternIdx == -1) {
        break;
      }

      // Save the current pattern as the HOME pattern and save the brightness
      EEPROM.update(HOME_ADDR, selectedPatternIdx);
      saveBrightness();

      Serial.print(F("Saved HOME: "));
      Serial.println(EEPROM.read(HOME_ADDR));
      Serial.print(F("Saved Brightness: "));
      Serial.println(EEPROM.read(BRIGHTNESS_SAVED_ADDR));

      // Set pattern to the OFF pattern
      selectedPatternIdx = -1;
      
      break;    
  }
}


// Check for button input
void checkButtonPress() {  
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


void loop() {
  showPattern();

  checkButtonPress();

  // If auto cycle mode is on, change to next pattern 
  // if (autoCycle && autoCycleTimeHasElapsed()) {
  if (autoCycle) {
    EVERY_N_MILLISECONDS(AUTO_INTERVAL){ changePattern(1); };
  }

  cycleCounter++;
}


// Breathe Animation
void breatheAnimation() {
  // TODO: Rewrite this pattern
  // TODO: Fix this pattern. It is causing the stick to get stuck sometimes, and also messing up the brightness when you go to another pattern
  // NOTE: In order to change the top-end brightness of this pattern, user
  // must switch to a different pattern, change the brightness, and then switch back.
/*******
  int numColors = getNumColors();
  bool isPOVColor = isPOVColorIndex(selectedColorIdx);
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
    color = isPOVColor ? COLORS[ POV_COLOR_SETS_2[selectedColorIdx - numColors][c] ] : COLORS[selectedColorIdx];
    
    // Light the pixels
    for (int i = 0; i < numLEDs; i++) {
      leds[i] = color;
    }
    FastLED.setBrightness(pat_i_1);

    // Insert POV delay if POV color
    if (isPOVColor) {
      FastLED.delay(POVSpeedDelay);
    }

    FastLED.show();
    FastLED.delay(speedDelay);
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
*********/
}
