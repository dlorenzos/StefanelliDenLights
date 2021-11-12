
/* Christmas Star by Daniel Stefanelli 12/08/2019 */
/* Uses Neopixels, pushbutton rotary encoder and I2C LCD */

//NEOPIXEL library
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

// Libraries for LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Libraries for Rotary Encoder knob
#include <ClickEncoder.h>
#include <TimerOne.h>

// Output PIN for LEDs
#define PIN            6

// Number of LEDs
#define NUMLEDS      472

// Properties of star
#define NUMPOINTS   5
#define LEDSPERPOINT  (NUMLEDS/NUMPOINTS)
#define LEDSPERLEG    (LEDSPERPOINT/2)

// Maximum delay time for animated effects (milliseconds)
#define MINMODEVALUE  10
#define MAXMODEVALUE  300
// Maximum delay time for animated effects (milliseconds)
#define MINBRIGHTNESS  10
#define MAXBRIGHTNESS  250

// Time to stay on one mode when in Automatic cycliing (milliseconds)
#define AUTOTIME  30000

// ENUM List of display modes
enum DisplayMode {
  BRIGHTNESS,
  REDGREENSTATIC,
  REDGREENRANDOM,
  REDGREENDYNAMIC,
  NUMMODES
};

// Character to display for each mode in the Enum list, need to keep these in sync
// Probably a better more elegant way to do this
char DisplayModeString[][16] = {
  "Brightness",
  "Static",
  "Random",
  "Dynamic",
};

// Global variables
// Store last time the mode switched for automatic
static long lastswitchtime = 0;
// Set brightness, Max is 255 but don't go above 20 without additional power supply
static int brightnessValue = 40;
// Array of delaytimes for each mode (Not used for Solid mode)
static uint32_t modeValue[NUMMODES];
// Initialize AutoMode to true
static int AutoMode = 1;
// Array of selected color for each mode (really only used for Solid mode)
uint32_t selectedColor[NUMMODES];

// Include EEPROM I/O
#include <EEPROM.h>

// Create encoder object
ClickEncoder *encoder;

// Create pixels for LEDs
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMLEDS, PIN, NEO_GRB + NEO_KHZ800);

// Create lcd for LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// List of predefined colors
uint32_t red      = pixels.Color(255, 0, 0);
uint32_t green    = pixels.Color(0, 255, 0);
uint32_t blue     = pixels.Color(0, 0, 255);
uint32_t yellow   = pixels.Color(255, 255, 0);
uint32_t magenta  = pixels.Color(255, 0, 255);
uint32_t cyan     = pixels.Color(0, 255, 255);
uint32_t white    = pixels.Color(255, 255, 255);
uint32_t black    = pixels.Color(0, 0, 0);
enum Colors { RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, WHITE, BLACK, NUMCOLORS };
char ColorNames[][16] = {"Red", "Green", "Blue", "Yellow", "Magenta", "Cyan", "White", "Black"};
uint32_t ColorArray[] = {red, green, blue, yellow, magenta, cyan, white, black};

// Displaymode, controlled by pushing rotary encoder button
static int mode = 0;
// Keep track of last mode, initialize to something invalid
static int lastmode = -99;

// Variables used for Rotary encoder
int16_t value;
static int last = 0;
static long lastmillis = 0;
static long startmillis = 0;
uint32_t stepsize = 10;

// Timer routine
void timerIsr() {
  encoder->service();
}

//SETUP (run once)
void setup()
{
  int i;

  // Initialize encoder
  encoder = new ClickEncoder(A1, A0, A2);

  // Not sure what this does
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  // Begin serial output
  Serial.begin(9600);

  randomSeed(analogRead(0));

  // Initialize NeoPixel library
  pixels.begin();
  pixels.setBrightness(brightnessValue);
  pixels.show();

  // Initialize LCD, not sure if backlight does anything
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Den Lights");
  lcd.setCursor(0, 1);
  lcd.print("Version 20211112");
  delay(5000);

  // Read saved settings from EEPROM
//  EEPROM.get(0, mode);
//  EEPROM.get(sizeof(int), AutoMode);
//  EEPROM.get(sizeof(int) * 2, selectedColor);
//  EEPROM.get(sizeof(int) * 2 + sizeof(selectedColor), modeValue);

  for ( i = 0; i < NUMMODES; i++)
    modeValue[i] = 10;
      
  // Range check values
/*
  if (mode < 0) mode = 0;
  else if (mode >= NUMMODES) mode = NUMMODES - 1;

  if (AutoMode < 0) AutoMode = 0;
  else if (AutoMode > 1) AutoMode = 1;

  for ( i = 0; i < NUMMODES; i++) {
    if (modeValue[i] < 0 || modeValue[i] > MAXMODEVALUE)
      modeValue[i] = 100;
  }
*/
  lastmode = -99;
}

// Main Loop
void loop()
{
  // If AutoMode is active then check if it is time to increment the mode
  if (AutoMode) {
    if ((millis() - lastswitchtime) > AUTOTIME) {
      mode++;
      if (mode >= NUMMODES)
        mode = 0;
      lastswitchtime = millis();
    }
  }

  // Periodic check for encoder action
  CheckForClick();

  // Main switch statement to display selected mode
  switch (mode) {
    case BRIGHTNESS:
      if (mode != lastmode) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(DisplayModeString[mode]);
        lastmode = mode;
        printBrightness();
      }
      BrightnessSet();
      break;
    case REDGREENSTATIC:
      if (mode != lastmode) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(DisplayModeString[mode]);
        lastmode = mode;
        printModeValue();
      }
      //      Rainbow1(10);
      break;
    case REDGREENRANDOM:
      if (mode != lastmode) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(DisplayModeString[mode]);
        lastmode = mode;
        printModeValue();
      }
      //      Rainbow2(10);
      break;
    case REDGREENDYNAMIC:
      if (mode != lastmode) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(DisplayModeString[mode]);
        lastmode = mode;
        printModeValue();
      }
      //      Points();
      break;
  }
}

// Print the delay time to the 2nd line of the LCD
void printModeValue() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(modeValue[mode]);
  //  lcd.print(" ms");
}

// Print the brightness to the 2nd line of the LCD
void printBrightness() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(brightnessValue);
  lcd.print(" ms");
}

// Print the Color to the 2nd line of the LCD
void printSelectedColor() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(ColorNames[selectedColor[mode]]);
}

// Routine called to check for Encoder Action
int CheckForClick() {
  ClickEncoder::Button b = encoder->getButton();
  static int HeldCounter = 0;

  if (AutoMode) {
    if ((millis() - lastswitchtime) > AUTOTIME) {
      mode++;
      if (mode >= NUMMODES)
        mode = 0;
      lastswitchtime = millis();
      return 1;
    }
  }

  // Process encoder clicks to cycle through display options
  // Hold encoder button to enter calibrate menu
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Held:
        HeldCounter++;
        if (HeldCounter > 5) {
          lcd.setCursor(0, 0);
          lcd.print("SAVING          ");
          lcd.setCursor(0, 1);
          lcd.print("SETTINGS!!      ");
          // Save settings starting with AutoMode followed by selectedColor and then delayTime
          EEPROM.put(0, mode);
          EEPROM.put(sizeof(int), AutoMode);
          EEPROM.put(sizeof(int) * 2, selectedColor);
          EEPROM.put(sizeof(int) * 2 + sizeof(selectedColor), modeValue);
          delay(1500);
          lastmode = -99;
          return (1);
        }
        break;
      case ClickEncoder::Released:
        break;
      case ClickEncoder::Clicked:
        mode++;
        if (mode >= NUMMODES)
          mode = 0;
        return 1;
        break;
      case ClickEncoder::DoubleClicked:
        if (AutoMode) {
          AutoMode = 0;
          lcd.setCursor(0, 0);
          lcd.print("STOPPING        ");
          lcd.setCursor(0, 1);
          lcd.print("AUTO MODE       ");
          delay(1500);
          lastmode = -99;
          lastswitchtime = millis();
          return (1);
        }
        else {
          AutoMode = 1;
          lcd.setCursor(0, 0);
          lcd.print("STARTING        ");
          lcd.setCursor(0, 1);
          lcd.print("AUTO MODE       ");
          delay(1500);
          lastmode = -99;
          lastswitchtime = millis();
          mode = 0;
          return (1);
        }
        break;
    }
  }

  value += encoder->getValue();
  //  if (mode == 0) {
  //    if (abs(value - last) > 2) {
  //      if (value > last) {
  //        if (selectedColor[mode] < (NUMCOLORS - 1))
  //          selectedColor[mode]++;
  //      }
  //      else {
  //        if (selectedColor[mode] != 0)
  //          selectedColor[mode]--;
  //      }
  //      printSelectedColor();
  //      last = value;
  //      return 1;
  //    }
  //  }
  if (mode == 0) {
    if (abs(value - last) > 2) {
      if (value > last) {
        if (brightnessValue < MAXBRIGHTNESS)
          brightnessValue = brightnessValue + stepsize;
      }
      else {
        if ((brightnessValue > 0) && (brightnessValue >= stepsize))
          brightnessValue = brightnessValue - stepsize;
      }
      if (brightnessValue < MINBRIGHTNESS)
        brightnessValue = MINBRIGHTNESS;

      last = value;
      lastmillis = millis();
      printBrightness();
      return 1;
    }
  }
  else {
    if (abs(value - last) > 2) {
      if (value > last) {
        if (modeValue[mode] < MAXMODEVALUE)
          modeValue[mode] = modeValue[mode] + stepsize;
      }
      else {
        if ((modeValue[mode] > 0) && (modeValue[mode] >= stepsize))
          modeValue[mode] = modeValue[mode] - stepsize;
      }
      if (modeValue[mode] < MINMODEVALUE)
        modeValue[mode] = MINMODEVALUE;

      last = value;
      lastmillis = millis();
      printModeValue();
      return 1;
    }
  }
  return 0;
}

// Routine called to check for Encoder Action
int CheckForClickWithDelay(long tdelay)
{
  ClickEncoder::Button b = encoder->getButton();
  static int HeldCounter = 0;
  int TimeExpired = 0;

  startmillis = millis();

  while (true) {
    if ((millis() - startmillis) >= tdelay)
      return 0;
    else if (CheckForClick())
      return 1;
  }
}

void BrightnessSet() {
  for (int i = 0; i < NUMLEDS; i++ ) {
    if (i % 2)
      pixels.setPixelColor(i, 255, 0, 0);       //  Set pixel's color (in RAM)
    else
      pixels.setPixelColor(i, 0, 255, 0);       //  Set pixel's color (in RAM)
  }

  pixels.setBrightness(brightnessValue);
  pixels.show();

}
