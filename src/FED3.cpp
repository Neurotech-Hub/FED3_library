/*
  Feeding experimentation device 3 (FED3) library
  Code by Lex Kravitz, adapted to Arduino library format by Eric Lin
  alexxai@wustl.edu
  erclin@ucdavis.edu
  December 2020

  The first FED device was developed by Nguyen at al and published in 2016:
  https://www.ncbi.nlm.nih.gov/pubmed/27060385

  FED3 includes hardware and code from:
  *** Adafruit, who made the hardware breakout boards and associated code we used in FED ***

  Cavemoa's excellent examples of datalogging with the Adalogger:
  https://github.com/cavemoa/Feather-M0-Adalogger

  Arduino Time library http://playground.arduino.cc/code/time
  Maintained by Paul Stoffregen https://github.com/PaulStoffregen/Time

  This project is released under the terms of the Creative Commons - Attribution - ShareAlike 3.0 license:
  human readable: https://creativecommons.org/licenses/by-sa/3.0/
  legal wording: https://creativecommons.org/licenses/by-sa/3.0/legalcode
  Copyright (c) 2019, 2020 Lex Kravitz
*/

/**************************************************************************************************************************************************
                                                                                                    Startup stuff
**************************************************************************************************************************************************/
#include "Arduino.h"
#include "FED3.h"

FED3 *FED3::staticFED = nullptr;

#if defined(ESP32)
#define IRAM_ISR_ATTR IRAM_ATTR
#elif defined(__arm__)
#define IRAM_ISR_ATTR // Empty for non-ESP32 platforms
#endif

//  Interrupt handlers
static void IRAM_ISR_ATTR outsidePelletTriggerHandler()
{
  FED3::staticFED->pelletTrigger();
}

static void IRAM_ISR_ATTR outsideLeftTriggerHandler()
{
  FED3::staticFED->leftTrigger();
}

static void IRAM_ISR_ATTR outsideRightTriggerHandler()
{
  FED3::staticFED->rightTrigger();
}

/**************************************************************************************************************************************************
                                                                                                        Main loop
**************************************************************************************************************************************************/
void FED3::run()
{
  // This should be called at least once per loop.  It updates the time, updates display, and controls sleep
  if (digitalRead(PELLET_WELL) == HIGH)
  { // check for pellet
    PelletAvailable = false;
  }
  DateTime now = rtc.now();
  currentHour = now.hour();     // useful for timed feeding sessions
  currentMinute = now.minute(); // useful for timed feeding sessions
  currentSecond = now.second(); // useful for timed feeding sessions
  unixtime = now.unixtime();
  ReadBatteryLevel();
  UpdateDisplay();
  goToSleep();
}

/**************************************************************************************************************************************************
                                                                                                Poke functions
**************************************************************************************************************************************************/
// log left poke
void FED3::logLeftPoke()
{
  if (PelletAvailable == false)
  {
    leftPokeTime = millis();
    LeftCount++;
    leftInterval = 0.0;
    unsigned long startWait = millis();
    while (digitalRead(LEFT_POKE) == LOW)
    {
      if (millis() - startWait > maxPokeTime)
        break; // maxPokeTime timeout
    }
    leftInterval = (millis() - leftPokeTime);
    UpdateDisplay();
    DisplayLeftInt();
    if (leftInterval < minPokeTime)
    {
      Event = "LeftShort";
    }
    else
    {
      Event = "Left";
    }

    logdata();
    Left = false;
  }
}

// log right poke
void FED3::logRightPoke()
{
  if (PelletAvailable == false)
  {
    rightPokeTime = millis();
    RightCount++;
    rightInterval = 0.0;
    unsigned long startWait = millis();
    while (digitalRead(RIGHT_POKE) == LOW)
    {
      if (millis() - startWait > maxPokeTime)
        break; // maxPokeTime timeout
    }
    rightInterval = (millis() - rightPokeTime);
    UpdateDisplay();
    DisplayRightInt();
    if (rightInterval < minPokeTime)
    {
      Event = "RightShort";
    }
    else
    {
      Event = "Right";
    }

    logdata();
    Right = false;
  }
}

void FED3::randomizeActivePoke(int max)
{
  // Store last active side and randomize
  byte lastActive = activePoke;
  activePoke = random(0, 2);

  // Increment consecutive active pokes, or reset consecutive to zero
  if (activePoke == lastActive)
  {
    consecutive++;
  }
  else
  {
    consecutive = 0;
  }

  // if consecutive pokes are too many, swap pokes
  if (consecutive >= max)
  {
    if (activePoke == 0)
    {
      activePoke = 1;
    }
    else if (activePoke == 1)
    {
      activePoke = 0;
    }
    consecutive = 0;
  }
}

/**************************************************************************************************************************************************
                                                                                                Feeding functions
**************************************************************************************************************************************************/
void FED3::Feed(int pulse, bool pixelsoff)
{
  // Run this loop repeatedly until statement below is false
  bool pelletDispensed = false;

  do
  {

    if (pelletDispensed == false)
    {
      pelletDispensed = RotateDisk(-300);
    }

    if (pixelsoff == true)
    {
      pixelsOff();
    }

    // If pellet is detected during or after this motion
    if (pelletDispensed == true)
    {
      ReleaseMotor();
      pelletTime = millis();

      display.fillCircle(25, 99, 5, BLACK);
      display.refresh();
      retInterval = (millis() - pelletTime);
      // while pellet is present and under 60s has elapsed
      while (digitalRead(PELLET_WELL) == LOW and retInterval < 60000)
      { // After pellet is detected, hang here for up to 1 minute to detect when it is removed
        retInterval = (millis() - pelletTime);
        DisplayRetrievalInt();

        // Log pokes while pellet is present
        if (digitalRead(LEFT_POKE) == LOW)
        { // If left poke is triggered
          leftPokeTime = millis();
          if (countAllPokes)
            LeftCount++;
          leftInterval = 0.0;
          while (digitalRead(LEFT_POKE) == LOW)
          {
          } // Hang here until poke is clear
          leftInterval = (millis() - leftPokeTime);
          UpdateDisplay();
          Event = "LeftWithPellet";

          logdata();
        }

        if (digitalRead(RIGHT_POKE) == LOW)
        { // If right poke is triggered
          rightPokeTime = millis();
          RightCount++;
          rightInterval = 0.0;
          while (digitalRead(RIGHT_POKE) == LOW)
          {
          } // Hang here until poke is clear
          rightInterval = (millis() - rightPokeTime);
          UpdateDisplay();
          Event = "RightWithPellet";

          logdata();
        }
      }

      // after 60s has elapsed
      unsigned long pelletWaitStart = millis();
      while (digitalRead(PELLET_WELL) == LOW)
      {
        if (millis() - pelletWaitStart > 300000)
        { // 5 minute timeout
          Event = "PelletStuck";
          logdata();
          break;
        }
        run();
        // Log pokes while pellet is present
        if (digitalRead(LEFT_POKE) == LOW)
        { // If left poke is triggered
          leftPokeTime = millis();
          if (countAllPokes)
            LeftCount++;
          leftInterval = 0.0;
          while (digitalRead(LEFT_POKE) == LOW)
          {
          } // Hang here until poke is clear
          leftInterval = (millis() - leftPokeTime);
          UpdateDisplay();
          Event = "LeftWithPellet";

          logdata();
        }

        if (digitalRead(RIGHT_POKE) == LOW)
        { // If right poke is triggered
          rightPokeTime = millis();
          if (countAllPokes)
            RightCount++;
          rightInterval = 0.0;
          while (digitalRead(RIGHT_POKE) == LOW)
          {
          } // Hang here until poke is clear
          rightInterval = (millis() - rightPokeTime);
          UpdateDisplay();
          Event = "RightWithPellet";

          logdata();
        }
      }

      ReleaseMotor();
      PelletCount++;

      // If pulse duration is specified, send pulse from BNC port
      if (pulse > 0)
      {
        BNC(pulse, 1);
      }

      Left = false;
      Right = false;
      Event = "Pellet";

      // calculate IntetPelletInterval
      DateTime now = rtc.now();
      interPelletInterval = now.unixtime() - lastPellet; // calculate time in seconds since last pellet logged
      lastPellet = now.unixtime();

      logdata();
      numMotorTurns = 0; // reset numMotorTurns
      PelletAvailable = true;
      UpdateDisplay();

      break;
    }

    if (PelletAvailable == false)
    {
      pelletDispensed = dispenseTimer_ms(1500); // delay between pellets that also checks pellet well
      numMotorTurns++;

      // Jam clearing movements
      if (pelletDispensed == false)
      {
        if (numMotorTurns % 5 == 0)
        {
          pelletDispensed = MinorJam();
        }
      }
      if (pelletDispensed == false)
      {
        if (numMotorTurns % 10 == 0 and numMotorTurns % 20 != 0)
        {
          pelletDispensed = VibrateJam();
        }
      }
      if (pelletDispensed == false)
      {
        if (numMotorTurns % 20 == 0)
        {
          pelletDispensed = ClearJam();
        }
      }
    }
  } while (PelletAvailable == false);
}

// minor movement to clear jam
bool FED3::MinorJam()
{
  return RotateDisk(100);
}

// vibration movement to clear jam
bool FED3::VibrateJam()
{
  DisplayJamClear();

  // simple debounce to ensure pellet is out for at least 250ms
  if (dispenseTimer_ms(250))
  {
    display.fillRect(5, 15, 120, 15, WHITE); // erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
    return true;
  }
  for (int i = 0; i < 30; i++)
  {
    if (RotateDisk(120))
    {
      display.fillRect(5, 15, 120, 15, WHITE); // erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
      return true;
    }
    if (RotateDisk(-60))
    {
      display.fillRect(5, 15, 120, 15, WHITE); // erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
      return true;
    }
  }
  return false;
}

// full rotation to clear jam
bool FED3::ClearJam()
{
  DisplayJamClear();

  if (dispenseTimer_ms(250))
  {
    display.fillRect(5, 15, 120, 15, WHITE); // erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
    return true;
  }

  for (int i = 0; i < 21 + random(0, 20); i++)
  {
    if (RotateDisk(-i * 4))
    {
      display.fillRect(5, 15, 120, 15, WHITE); // erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
      return true;
    }
  }

  if (dispenseTimer_ms(250))
  {
    display.fillRect(5, 15, 120, 15, WHITE); // erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
    return true;
  }

  for (int i = 0; i < 21 + random(0, 20); i++)
  {
    if (RotateDisk(i * 4))
    {
      display.fillRect(5, 15, 120, 15, WHITE); // erase the "Jam clear" text without clearing the entire screen by pasting a white box over it
      return true;
    }
  }

  return false;
}

bool FED3::RotateDisk(int steps)
{
  digitalWrite(MOTOR_ENABLE, HIGH); // Enable motor driver
  for (int i = 0; i < (steps > 0 ? steps : -steps); i++)
  {

    if (digitalRead(LEFT_POKE) == LOW)
    { // If left poke is triggered
      leftPokeTime = millis();
      if (countAllPokes)
        LeftCount++;
      leftInterval = 0.0;
      unsigned long startWait = millis();
      while (digitalRead(LEFT_POKE) == LOW)
      {
        if (millis() - startWait > maxPokeTime)
          break; // maxPokeTime timeout
      }
      leftInterval = (millis() - leftPokeTime);
      UpdateDisplay();
      Event = "LeftDuringDispense";

      logdata();
    }

    if (digitalRead(RIGHT_POKE) == LOW)
    { // If right poke is triggered
      rightPokeTime = millis();
      if (countAllPokes)
        RightCount++;
      rightInterval = 0.0;
      unsigned long startWait = millis();
      while (digitalRead(RIGHT_POKE) == LOW)
      {
        if (millis() - startWait > maxPokeTime)
          break; // maxPokeTime timeout
      }
      rightInterval = (millis() - rightPokeTime);
      UpdateDisplay();
      Event = "RightDuringDispense";

      logdata();
    }

    if (steps > 0)
      stepper.step(1);
    else
      stepper.step(-1);
    for (int j = 0; j < 20; j++)
    {
      delayMicroseconds(100);
      if (digitalRead(PELLET_WELL) == LOW)
      {
        delayMicroseconds(100);
        // Debounce
        if (digitalRead(PELLET_WELL) == LOW)
        {
          ReleaseMotor();
          return true;
        }
      }
    }
  }
  ReleaseMotor();
  return false;
}

// Function for delaying between motor movements, but also ending this delay if a pellet is detected
bool FED3::dispenseTimer_ms(int ms)
{
  for (int i = 1; i < ms; i++)
  {
    for (int j = 0; j < 10; j++)
    {
      delayMicroseconds(100);
      if (digitalRead(PELLET_WELL) == LOW)
      {
        delayMicroseconds(100);
        // Debounce
        if (digitalRead(PELLET_WELL) == LOW)
        {
          return true;
        }
      }
    }
  }
  return false;
}

// Timeout function
void FED3::Timeout(int seconds, bool reset, bool whitenoise)
{
  int timeoutStart = millis();
  while ((millis() - timeoutStart) < (seconds * 1000))
  {
    int displayUpdated = millis();
    if (whitenoise)
    {
      int freq = random(50, 250);
      tone(BUZZER, freq, 10);
      delay(10);
    }

    if (digitalRead(LEFT_POKE) == LOW)
    { // If left poke is triggered
      if (reset)
      {
        timeoutStart = millis();
      }
      leftPokeTime = millis();
      if (countAllPokes)
      {
        LeftCount++;
      }
      leftInterval = 0.0;

      unsigned long pokeStart = millis();
      while (digitalRead(LEFT_POKE) == LOW)
      {
        if (millis() - pokeStart > maxPokeTime)
          break; // Using same maxPokeTime as other poke functions
        if (whitenoise)
        {
          int freq = random(50, 250);
          tone(BUZZER, freq, 10);
        }
      }

      leftInterval = (millis() - leftPokeTime);
      Event = "LeftinTimeOut";
      logdata();
    }

    if (digitalRead(RIGHT_POKE) == LOW)
    { // If right poke is triggered
      if (reset)
      {
        timeoutStart = millis();
      }
      if (countAllPokes)
      {
        RightCount++;
      }
      rightPokeTime = millis();
      rightInterval = 0.0;

      unsigned long pokeStart = millis();
      while (digitalRead(RIGHT_POKE) == LOW)
      {
        if (millis() - pokeStart > maxPokeTime)
          break; // Fixed from LEFT_POKE
        if (whitenoise)
        {
          int freq = random(50, 250);
          tone(BUZZER, freq, 10);
        }
      }

      rightInterval = (millis() - rightPokeTime);
      // UpdateDisplay();
      Event = "RightinTimeout";

      logdata();
    }
  }

  display.fillRect(5, 20, 100, 25, WHITE); // erase the data on screen without clearing the entire screen by pasting a white box over it
  UpdateDisplay();
  Left = false;
  Right = false;
}

/**************************************************************************************************************************************************
                                                                                       Audio and neopixel stimuli
**************************************************************************************************************************************************/
void FED3::ConditionedStimulus(int duration)
{
  tone(BUZZER, 4000, duration);
  pixelsOn(0, 0, 10, 0); // blue light for all
}

void FED3::Click()
{
  tone(BUZZER, 800, 8);
}

void FED3::Tone(int freq, int duration)
{
  tone(BUZZER, freq, duration);
}

void FED3::stopTone()
{
  noTone(BUZZER);
}

void FED3::Noise(int duration)
{
  // White noise to signal errors
  for (int i = 0; i < duration / 50; i++)
  {
    tone(BUZZER, random(50, 250), 50);
    delay(duration / 50);
  }
}

// Turn all pixels on to a specific color
void FED3::pixelsOn(int R, int G, int B, int W)
{
  digitalWrite(MOTOR_ENABLE, HIGH); // ENABLE motor driver
  delay(2);                         // let things settle
  for (uint16_t i = 0; i < 8; i++)
  {
    strip.setPixelColor(i, R, G, B, W);
    strip.show();
  }
}

// Turn all pixels off
void FED3::pixelsOff()
{
  digitalWrite(MOTOR_ENABLE, HIGH); // ENABLE motor driver
  delay(2);                         // let things settle
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, 0, 0, 0, 0);
    strip.show();
  }
  digitalWrite(MOTOR_ENABLE, LOW); // disable motor driver and neopixels
}

// colorWipe does a color wipe from left to right
void FED3::colorWipe(uint32_t c, uint8_t wait)
{
  digitalWrite(MOTOR_ENABLE, HIGH); // ENABLE motor driver
  delay(2);                         // let things settle
  for (uint16_t i = 0; i < 8; i++)
  {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
  digitalWrite(MOTOR_ENABLE, LOW); ////disable motor driver and neopixels
  delay(2);                        // let things settle
}

// Visual tracking stimulus - left-most pixel on strip
void FED3::leftPixel(int R, int G, int B, int W)
{
  digitalWrite(MOTOR_ENABLE, HIGH);
  delay(2); // let things settle
  strip.setPixelColor(0, R, G, B, W);
  strip.show();
  //   delay(2); //let things settle
}

// Visual tracking stimulus - left-most pixel on strip
void FED3::rightPixel(int R, int G, int B, int W)
{
  digitalWrite(MOTOR_ENABLE, HIGH);
  delay(2); // let things settle
  strip.setPixelColor(7, R, G, B, W);
  strip.show();
  //   delay(2); //let things settle
}

// Visual tracking stimulus - left poke pixel
void FED3::leftPokePixel(int R, int G, int B, int W)
{
  digitalWrite(MOTOR_ENABLE, HIGH);
  delay(2); // let things settle
  strip.setPixelColor(9, R, G, B, W);
  strip.show();
  //   delay(2); //let things settle
}

// Visual tracking stimulus - right poke pixel
void FED3::rightPokePixel(int R, int G, int B, int W)
{
  digitalWrite(MOTOR_ENABLE, HIGH);
  delay(2); // let things settle
  strip.setPixelColor(8, R, G, B, W);
  strip.show();
  // delay(2); //let things settle
}

// Short helper function for blinking LEDs and BNC out port
void FED3::Blink(byte PIN, byte DELAY_MS, byte loops)
{
  for (byte i = 0; i < loops; i++)
  {
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN, LOW);
    delay(DELAY_MS);
  }
}

// Simple function for sending square wave pulses to the BNC port
void FED3::BNC(int DELAY_MS, int loops)
{
  for (int i = 0; i < loops; i++)
  {
    digitalWrite(BNC_OUT, HIGH);
    digitalWrite(GREEN_LED, HIGH);
    delay(DELAY_MS);
    digitalWrite(BNC_OUT, LOW);
    digitalWrite(GREEN_LED, LOW);
    delay(DELAY_MS);
  }
}

// More advanced function for controlling pulse width and frequency for the BNC port
void FED3::pulseGenerator(int pulse_width, int frequency, int repetitions)
{ // freq in Hz, width in ms, loops in number of times
  for (byte j = 0; j < repetitions; j++)
  {
    digitalWrite(BNC_OUT, HIGH);
    digitalWrite(GREEN_LED, HIGH);
    delay(pulse_width); // pulse high for width
    digitalWrite(BNC_OUT, LOW);
    digitalWrite(GREEN_LED, LOW);
    long temp_delay = (1000 / frequency) - pulse_width;
    if (temp_delay < 0)
      temp_delay = 0;  // if temp delay <0 because parameters are set wrong, set it to 0 so FED3 doesn't crash O_o
    delay(temp_delay); // pin low
  }
}

void FED3::ReadBNC(bool blinkGreen)
{
  pinMode(BNC_OUT, INPUT_PULLDOWN);
  BNCinput = false;
  if (digitalRead(BNC_OUT) == HIGH)
  {
    delay(1);
    if (digitalRead(BNC_OUT) == HIGH)
    {
      if (blinkGreen == true)
      {
        digitalWrite(GREEN_LED, HIGH);
        delay(25);
        digitalWrite(GREEN_LED, LOW);
      }
      BNCinput = true;
    }
  }
}

/**************************************************************************************************************************************************
                                                                                               Display functions
**************************************************************************************************************************************************/
void FED3::UpdateDisplay()
{
  // Box around data area of screen
  display.drawRect(5, 45, 158, 70, BLACK);

  display.setCursor(5, 15);
  display.print("FED:");
  display.println(FED);
  display.setCursor(6, 15); // this doubling is a way to do bold type
  display.print("FED:");
  display.fillRect(6, 20, 200, 22, WHITE);  // erase text under battery row without clearing the entire screen
  display.fillRect(35, 46, 120, 68, WHITE); // erase the pellet data on screen without clearing the entire screen
  display.setCursor(5, 36);                 // display which sketch is running

  // write the first 8 characters of sessiontype:
  display.print(sessiontype.charAt(0));
  display.print(sessiontype.charAt(1));
  display.print(sessiontype.charAt(2));
  display.print(sessiontype.charAt(3));
  display.print(sessiontype.charAt(4));
  display.print(sessiontype.charAt(5));
  display.print(sessiontype.charAt(6));
  display.print(sessiontype.charAt(7));

  if (DisplayPokes == 1)
  {
    display.setCursor(35, 65);
    display.print("Left: ");
    display.setCursor(95, 65);
    display.print(LeftCount);
    display.setCursor(35, 85);
    display.print("Right:  ");
    display.setCursor(95, 85);
    display.print(RightCount);
  }

  display.setCursor(35, 105);
  display.print("Pellets:");
  display.setCursor(95, 105);
  display.print(PelletCount);

  if (DisplayTimed == true)
  { // If it's a timed Feeding Session
    DisplayTimedFeeding();
  }

  DisplayBattery();
  DisplayDateTime();
  DisplayIndicators();
  display.refresh();
}

void FED3::DisplayDateTime()
{
  // Print date and time at bottom of the screen
  DateTime now = rtc.now();

  // Print to display
  display.setCursor(0, 135);
  display.fillRect(0, 123, 200, 60, WHITE);
  display.print(now.month());
  display.print("/");
  display.print(now.day());
  display.print("/");
  display.print(now.year());
  display.print("      ");
  if (now.hour() < 10)
    display.print('0'); // Trick to add leading zero for formatting
  display.print(now.hour());
  display.print(":");
  if (now.minute() < 10)
    display.print('0'); // Trick to add leading zero for formatting
  display.print(now.minute());
}

void FED3::DisplayIndicators()
{
  // Pellet circle
  display.fillCircle(25, 99, 5, WHITE); // pellet
  display.drawCircle(25, 99, 5, BLACK);

  // Poke indicators
  if (DisplayPokes == 1)
  { // only make active poke indicators if DisplayPokes==1
    // Active poke indicator triangles
    if (activePoke == 0)
    {
      display.fillTriangle(20, 55, 26, 59, 20, 63, WHITE);
      display.fillTriangle(20, 75, 26, 79, 20, 83, BLACK);
    }
    if (activePoke == 1)
    {
      display.fillTriangle(20, 75, 26, 79, 20, 83, WHITE);
      display.fillTriangle(20, 55, 26, 59, 20, 63, BLACK);
    }
  }
}

void FED3::DisplayBattery()
{
  //  Battery graphic showing bars indicating voltage levels
  if (numMotorTurns == 0)
  {
    display.fillRect(117, 2, 40, 16, WHITE);
    display.drawRect(116, 1, 42, 18, BLACK);
    display.drawRect(157, 6, 6, 8, BLACK);
  }
  // 4 bars
  if (measuredvbat > 3.85 & numMotorTurns == 0)
  {
    display.fillRect(120, 4, 7, 12, BLACK);
    display.fillRect(129, 4, 7, 12, BLACK);
    display.fillRect(138, 4, 7, 12, BLACK);
    display.fillRect(147, 4, 7, 12, BLACK);
  }

  // 3 bars
  else if (measuredvbat > 3.7 & numMotorTurns == 0)
  {
    display.fillRect(119, 3, 26, 13, WHITE);
    display.fillRect(120, 4, 7, 12, BLACK);
    display.fillRect(129, 4, 7, 12, BLACK);
    display.fillRect(138, 4, 7, 12, BLACK);
  }

  // 2 bars
  else if (measuredvbat > 3.55 & numMotorTurns == 0)
  {
    display.fillRect(119, 3, 26, 13, WHITE);
    display.fillRect(120, 4, 7, 12, BLACK);
    display.fillRect(129, 4, 7, 12, BLACK);
  }

  // 1 bar
  else if (numMotorTurns == 0)
  {
    display.fillRect(119, 3, 26, 13, WHITE);
    display.fillRect(120, 4, 7, 12, BLACK);
  }

  // display voltage
  display.setTextSize(2);
  display.setFont(&Org_01);

  display.fillRect(86, 0, 28, 12, WHITE);
  display.setCursor(87, 10);
  display.print(measuredvbat, 1);
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);

  // display temp/humidity sensor indicator if present
  if (tempSensor == true)
  {
    display.setTextSize(1);
    display.setFont(&Org_01);
    display.setCursor(89, 18);
    display.print("TH");
    display.setFont(&FreeSans9pt7b);
    display.setTextSize(1);
  }
}

// Display "Check SD Card!" if there is a card error
void FED3::DisplaySDError()
{
  display.clearDisplay();
  display.setCursor(20, 40);
  display.println("   Check");
  display.setCursor(10, 60);
  display.println("  SD Card!");
  display.refresh();
}

// Display text when FED is clearing a jam
void FED3::DisplayJamClear()
{
  display.fillRect(6, 20, 200, 22, WHITE); // erase the data on screen without clearing the entire screen by pasting a white box over it
  display.setCursor(6, 36);
  display.print("Clearing jam");
  display.refresh();
}

// Display pellet retrieval interval
void FED3::DisplayRetrievalInt()
{
  display.fillRect(85, 22, 70, 15, WHITE);
  display.setCursor(90, 36);
  if (retInterval < 59000)
  {
    display.print(retInterval);
    display.print("ms");
  }
  display.refresh();
}

// Display left poke duration
void FED3::DisplayLeftInt()
{
  display.fillRect(85, 22, 70, 15, WHITE);
  display.setCursor(90, 36);
  if (leftInterval < 10000)
  {
    display.print(leftInterval);
    display.print("ms");
  }
  display.refresh();
}

// Display right poke duration
void FED3::DisplayRightInt()
{
  display.fillRect(85, 22, 70, 15, WHITE);
  display.setCursor(90, 36);
  if (rightInterval < 10000)
  {
    display.print(rightInterval);
    display.print("ms");
  }
  display.refresh();
}

void FED3::StartScreen()
{
  if (ClassicFED3 == false)
  {
    display.setTextSize(3);
    display.setTextColor(BLACK);
    display.clearDisplay();
    display.setCursor(15, 55);
    display.print("FED3");

    // print filename on screen
    display.setTextSize(1);
    display.setCursor(2, 138);
    display.print(filename);

    // Display FED verison number at startup
    display.setCursor(2, 120);
    display.print("v: ");
    display.print(VER);
    display.print("_");
    display.print(sessiontype.charAt(0));
    display.print(sessiontype.charAt(1));
    display.print(sessiontype.charAt(2));
    display.print(sessiontype.charAt(3));
    display.print(sessiontype.charAt(4));
    display.print(sessiontype.charAt(5));
    display.print(sessiontype.charAt(6));
    display.print(sessiontype.charAt(7));
    display.refresh();
    DisplayMouse();
  }
}

void FED3::DisplayTimedFeeding()
{
  display.setCursor(35, 65);
  display.print(timedStart);
  display.print(":00 to ");
  display.print(timedEnd);
  display.print(":00");
}

void FED3::DisplayMinPoke()
{
  display.setCursor(115, 65);
  display.print((minPokeTime / 1000.0), 1);
  display.print("s");
  display.refresh();
}

void FED3::DisplayNoProgram()
{
  display.clearDisplay();
  display.setCursor(15, 45);
  display.print("No program");
  display.setCursor(16, 45);
  display.print("No program");
  display.setCursor(15, 65);
  display.print("resetting FED3...");
  display.refresh();
  for (int i = 0; i < 5; i++)
  {
    colorWipe(strip.Color(5, 0, 0), 25); // RED
    delay(20);
    colorWipe(strip.Color(0, 0, 0), 25); // clear
    delay(40);
  }
  softReset();
}

void FED3::DisplayMouse()
{
  // Draw animated mouse...
  for (int i = -50; i < 200; i += 15)
  {
    display.fillRoundRect(i + 25, 82, 15, 10, 6, BLACK); // head
    display.fillRoundRect(i + 22, 80, 8, 5, 3, BLACK);   // ear
    display.fillRoundRect(i + 30, 84, 1, 1, 1, WHITE);   // eye
    // movement of the mouse
    if ((i / 10) % 2 == 0)
    {
      display.fillRoundRect(i, 84, 32, 17, 10, BLACK); // body
      display.drawFastHLine(i - 8, 85, 18, BLACK);     // tail
      display.drawFastHLine(i - 8, 86, 18, BLACK);
      display.drawFastHLine(i - 14, 84, 8, BLACK);
      display.drawFastHLine(i - 14, 85, 8, BLACK);
      display.fillRoundRect(i + 22, 99, 8, 4, 3, BLACK); // front foot
      display.fillRoundRect(i, 97, 8, 6, 3, BLACK);      // back foot
    }
    else
    {
      display.fillRoundRect(i + 2, 82, 30, 17, 10, BLACK); // body
      display.drawFastHLine(i - 6, 91, 18, BLACK);         // tail
      display.drawFastHLine(i - 6, 90, 18, BLACK);
      display.drawFastHLine(i - 12, 92, 8, BLACK);
      display.drawFastHLine(i - 12, 91, 8, BLACK);
      display.fillRoundRect(i + 15, 99, 8, 4, 3, BLACK); // foot
      display.fillRoundRect(i + 8, 97, 8, 6, 3, BLACK);  // back foot
    }
    display.refresh();
    delay(80);
    display.fillRect(i - 25, 73, 95, 33, WHITE);
    previousFEDmode = FEDmode;
    previousFED = FED;

    // If one poke is pushed change mode
    if (FED3Menu == true or ClassicFED3 == true)
    {
      if (digitalRead(LEFT_POKE) == LOW | digitalRead(RIGHT_POKE) == LOW)
        SelectMode();
    }

    // If both pokes are pushed edit device number
    if ((digitalRead(LEFT_POKE) == LOW) && (digitalRead(RIGHT_POKE) == LOW))
    {
      tone(BUZZER, 1000, 200);
      delay(400);
      tone(BUZZER, 1000, 500);
      delay(200);
      tone(BUZZER, 3000, 600);
      colorWipe(strip.Color(2, 2, 2), 40); // Color wipe
      colorWipe(strip.Color(0, 0, 0), 20); // OFF
      EndTime = millis();
      SetFED = true;
      SetDeviceNumber();
    }
  }
}

/**************************************************************************************************************************************************
                                                                                               SD Logging functions
**************************************************************************************************************************************************/
// Create new files on uSD for FED3 settings
void FED3::CreateFile()
{
  digitalWrite(MOTOR_ENABLE, LOW); // Disable motor driver and neopixel

  // Initialize SD card with SdFat
  if (!fed3SD.begin(cardSelect, SD_SCK_MHZ(SD_CLOCK_SPEED)))
  {
    Serial.println("Failed to begin SD card.");
    error(ERROR_SD_INIT_FAIL);
    return;
  }

  // Open config file for writing, if it doesn't exist, it will be created
  if (!configfile.open("DeviceNumber.csv", O_RDWR | O_CREAT))
  {
    Serial.println("Failed to open DeviceNumber.csv");
    return;
  }
  FED = parseIntFromSdFile(configfile);
  configfile.close();

  // Open ratio file and read FEDmode
  if (!ratiofile.open("FEDmode.csv", O_RDWR | O_CREAT))
  {
    Serial.println("Failed to open FEDmode.csv");
    return;
  }
  FEDmode = parseIntFromSdFile(ratiofile);
  ratiofile.close();

  // Open start file and read timedStart
  if (!startfile.open("start.csv", O_RDWR | O_CREAT))
  {
    Serial.println("Failed to open start.csv");
    return;
  }
  timedStart = parseIntFromSdFile(startfile);
  startfile.close();

  // Open stop file and read timedEnd
  if (!stopfile.open("stop.csv", O_RDWR | O_CREAT))
  {
    Serial.println("Failed to open stop.csv");
    return;
  }
  timedEnd = parseIntFromSdFile(stopfile);
  stopfile.close();

  // Generate a placeholder filename
  strcpy(filename, "FED_____________.CSV");
  getFilename(filename);
}

// Create a new datafile
void FED3::CreateDataFile()
{
  digitalWrite(MOTOR_ENABLE, LOW); // Disable motor driver and neopixel
  getFilename(filename);           // Generate the filename

  // Open logfile for writing
  if (!logfile.open(filename, O_WRITE | O_CREAT | O_TRUNC))
  {
    error(ERROR_WRITE_FAIL); // Call error handler if the file cannot be opened
    return;
  }
}

// Write the header to the datafile
void FED3::writeHeader()
{
  digitalWrite(MOTOR_ENABLE, LOW); // Disable motor driver and neopixel
  // Write data header to file of microSD card

  if (sessiontype == "Bandit")
  {
    if (tempSensor == false)
    {
      logfile.println("MM:DD:YYYY hh:mm:ss,Library_Version,Session_type,Device_Number,Battery_Voltage,Motor_Turns,PelletsToSwitch,Prob_left,Prob_right,Event,High_prob_poke,Left_Poke_Count,Right_Poke_Count,Pellet_Count,Block_Pellet_Count,Retrieval_Time,InterPelletInterval,Poke_Time");
    }
    else if (tempSensor == true)
    {
      logfile.println("MM:DD:YYYY hh:mm:ss,Temp,Humidity,Library_Version,Session_type,Device_Number,Battery_Voltage,Motor_Turns,PelletsToSwitch,Prob_left,Prob_right,Event,High_prob_poke,Left_Poke_Count,Right_Poke_Count,Pellet_Count,Block_Pellet_Count,Retrieval_Time,InterPelletInterval,Poke_Time");
    }
  }

  else if (sessiontype != "Bandit")
  {
    if (tempSensor == false)
    {
      logfile.println("MM:DD:YYYY hh:mm:ss,Library_Version,Session_type,Device_Number,Battery_Voltage,Motor_Turns,FR,Event,Active_Poke,Left_Poke_Count,Right_Poke_Count,Pellet_Count,Block_Pellet_Count,Retrieval_Time,InterPelletInterval,Poke_Time");
    }
    if (tempSensor == true)
    {
      logfile.println("MM:DD:YYYY hh:mm:ss,Temp,Humidity,Library_Version,Session_type,Device_Number,Battery_Voltage,Motor_Turns,FR,Event,Active_Poke,Left_Poke_Count,Right_Poke_Count,Pellet_Count,Block_Pellet_Count,Retrieval_Time,InterPelletInterval,Poke_Time");
    }
  }

  logfile.close();
}

// write a configfile (this contains the FED device number)
void FED3::writeConfigFile()
{
  digitalWrite(MOTOR_ENABLE, LOW); // Disable motor driver and neopixel

  // Open configfile directly for writing
  if (!configfile.open("DeviceNumber.csv", O_WRITE | O_CREAT | O_TRUNC))
  {
    Serial.println("Failed to open DeviceNumber.csv for writing");
    return;
  }

  configfile.rewind();     // Go to the beginning of the file
  configfile.println(FED); // Write the FED value
  configfile.sync();       // Use sync() to ensure data is written
  configfile.close();      // Close the file
}

void FED3::logdata()
{
  if (EnableSleep == true)
  {
    digitalWrite(MOTOR_ENABLE, LOW); // Disable motor driver and neopixel
  }

  // Initialize SD card if not already initialized
  if (!fed3SD.begin(cardSelect, SD_SCK_MHZ(SD_CLOCK_SPEED)))
  {
    error(ERROR_SD_INIT_FAIL); // Handle SD card initialization failure
    return;
  }

  // Fix filename (the .CSV extension can become corrupted) and open file
  filename[16] = '.';
  filename[17] = 'C';
  filename[18] = 'S';
  filename[19] = 'V';

  // Open logfile directly for appending data
  if (!logfile.open(filename, O_WRITE | O_CREAT | O_APPEND))
  {
    display.fillRect(68, 1, 15, 22, WHITE); // Clear a space on the display

    // Draw SD card icon to indicate error
    display.drawRect(70, 2, 11, 14, BLACK);
    display.drawRect(69, 6, 2, 10, BLACK);
    display.fillRect(70, 7, 4, 8, WHITE);
    display.drawRect(72, 4, 1, 3, BLACK);
    display.drawRect(74, 4, 1, 3, BLACK);
    display.drawRect(76, 4, 1, 3, BLACK);
    display.drawRect(78, 4, 1, 3, BLACK);

    // Draw exclamation point
    display.fillRect(72, 6, 6, 16, WHITE);
    display.setCursor(74, 16);
    display.setTextSize(2);
    display.setFont(&Org_01);
    display.print("!");
    display.setFont(&FreeSans9pt7b);
    display.setTextSize(1);
    return;
  }

  // Log data to the file
  DateTime now = rtc.now();
  logfile.print(now.month());
  logfile.print("/");
  logfile.print(now.day());
  logfile.print("/");
  logfile.print(now.year());
  logfile.print(" ");
  logfile.print(now.hour());
  logfile.print(":");
  if (now.minute() < 10)
    logfile.print('0');
  logfile.print(now.minute());
  logfile.print(":");
  if (now.second() < 10)
    logfile.print('0');
  logfile.print(now.second());
  logfile.print(",");

  // Log temp and humidity if tempSensor is true
  if (tempSensor)
  {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data
    logfile.print(temp.temperature);
    logfile.print(",");
    logfile.print(humidity.relative_humidity);
    logfile.print(",");
  }

  // Log library version, session type, device number, and battery voltage
  logfile.print(VER);
  logfile.print(",");
  logfile.print(sessiontype);
  logfile.print(",");
  logfile.print(FED);
  logfile.print(",");
  ReadBatteryLevel();
  logfile.print(measuredvbat);
  logfile.print(",");

  // Log motor turns and FR ratio
  if (Event != "Pellet")
  {
    logfile.print(sqrt(-1)); // Print NaN if not a pellet Event
  }
  else
  {
    logfile.print(numMotorTurns + 1); // Print number of attempts to dispense a pellet
  }
  logfile.print(",");
  if (sessiontype == "Bandit")
  {
    logfile.print(pelletsToSwitch);
    logfile.print(",");
    logfile.print(prob_left);
    logfile.print(",");
    logfile.print(prob_right);
    logfile.print(",");
  }
  else
  {
    logfile.print(FR);
    logfile.print(",");
  }

  // Log event type and active poke side
  logfile.print(Event);
  logfile.print(",");
  if (sessiontype == "Bandit")
  {
    if (prob_left > prob_right)
      logfile.print("Left");
    else if (prob_left < prob_right)
      logfile.print("Right");
    else
      logfile.print("nan");
  }
  else
  {
    logfile.print(activePoke == 0 ? "Right" : "Left");
  }
  logfile.print(",");

  // Log left and right counts, pellet counts, block pellet count
  logfile.print(LeftCount);
  logfile.print(",");
  logfile.print(RightCount);
  logfile.print(",");
  logfile.print(PelletCount);
  logfile.print(",");
  logfile.print(BlockPelletCount);
  logfile.print(",");

  // Log pellet retrieval interval
  if (Event != "Pellet")
  {
    logfile.print(sqrt(-1)); // Print NaN if not a pellet Event
  }
  else if (retInterval < 60000)
  {
    logfile.print(retInterval / 1000.0); // Log interval below 1 min
  }
  else
  {
    logfile.print("Timed_out");
  }
  logfile.print(",");

  // Log inter-pellet interval
  if (Event != "Pellet" || PelletCount < 2)
  {
    logfile.print(sqrt(-1));
  }
  else
  {
    logfile.print(interPelletInterval);
  }
  logfile.print(",");

  // Log poke duration
  if (Event == "Pellet")
  {
    logfile.println(sqrt(-1));
  }
  else if (Event.startsWith("Left"))
  {
    logfile.println(leftInterval / 1000.0);
  }
  else if (Event.startsWith("Right"))
  {
    logfile.println(rightInterval / 1000.0);
  }
  else
  {
    logfile.println(sqrt(-1));
  }

  // Commit data to SD card and close file
  Blink(GREEN_LED, 25, 2);
  logfile.sync();  // Use sync() instead of flush() to write data
  logfile.close(); // Close the file
}

// If any errors are detected with the SD card upon boot this function
// will blink both LEDs on the Feather M0, turn the NeoPixel into red wipe pattern,
// and display "Check SD Card" on the screen
void FED3::error(ErrorCode errorCode)
{
  Serial.print("ERROR: ");
  Serial.println(errorCode);

  if (suppressSDerrors == false)
  {
    DisplaySDError();
    while (1)
    {
      uint8_t i;
      for (i = 0; i < errno; i++)
      {
        Blink(GREEN_LED, 25, 2);
        colorWipe(strip.Color(5, 0, 0), 25); // RED
      }
      for (i = errno; i < 10; i++)
      {
        colorWipe(strip.Color(0, 0, 0), 25); // clear
      }
    }
  }
}

// This function creates a unique filename for each file that
// starts with the letters: "FED_"
// then the date in MMDDYY followed by "_"
// then an incrementing number for each new file created on the same date
void FED3::getFilename(char *filename)
{
  if (!fed3SD.begin(cardSelect, SD_SCK_MHZ(SD_CLOCK_SPEED)))
  {
    Serial.println("Failed to begin SD card.");
    error(ERROR_SD_INIT_FAIL);
    return;
  }

  DateTime now = rtc.now();

  filename[3] = FED / 100 + '0';
  filename[4] = FED / 10 + '0';
  filename[5] = FED % 10 + '0';
  filename[7] = now.month() / 10 + '0';
  filename[8] = now.month() % 10 + '0';
  filename[9] = now.day() / 10 + '0';
  filename[10] = now.day() % 10 + '0';
  filename[11] = (now.year() - 2000) / 10 + '0';
  filename[12] = (now.year() - 2000) % 10 + '0';
  filename[16] = '.';
  filename[17] = 'C';
  filename[18] = 'S';
  filename[19] = 'V';
  for (uint8_t i = 0; i < 100; i++)
  {
    filename[14] = '0' + i / 10;
    filename[15] = '0' + i % 10;

    if (!fed3SD.exists(filename))
    {
      break;
    }
  }
  return;
}

/**************************************************************************************************************************************************
                                                                                               Change device number
**************************************************************************************************************************************************/
// Change device number
void FED3::SetDeviceNumber()
{
  // This code is activated when both pokes are pressed simultaneously from the
  // start screen, allowing the user to set the device # of the FED on the device
  while (SetFED == true)
  {
    // adjust FED device number
    display.fillRect(0, 0, 200, 80, WHITE);
    display.setCursor(5, 46);
    display.println("Set Device Number");
    display.fillRect(36, 122, 180, 28, WHITE);
    delay(100);
    display.refresh();

    display.setCursor(38, 138);
    if (FED < 100 & FED >= 10)
    {
      display.print("0");
    }
    if (FED < 10)
    {
      display.print("00");
    }
    display.print(FED);

    delay(100);
    display.refresh();

    if (digitalRead(RIGHT_POKE) == LOW)
    {
      FED += 1;
      Click();
      EndTime = millis();
      if (FED > 700)
      {
        FED = 700;
      }
    }

    if (digitalRead(LEFT_POKE) == LOW)
    {
      FED -= 1;
      Click();
      EndTime = millis();
      if (FED < 1)
      {
        FED = 0;
      }
    }
    if (millis() - EndTime > 3000)
    { // if 3 seconds passes confirm device #
      SetFED = false;
      display.setCursor(5, 70);
      display.println("...Set!");
      display.refresh();
      delay(1000);
      EndTime = millis();
      display.clearDisplay();
      display.refresh();

      ///////////////////////////////////
      //////////  ADJUST CLOCK //////////
      while (millis() - EndTime < 3000)
      {
        SetClock();
        delay(10);
      }

      display.setCursor(5, 105);
      display.println("...Clock is set!");
      display.refresh();
      delay(1000);

      ///////////////////////////////////

      while (setTimed == true)
      {
        // set timed feeding start and stop
        display.fillRect(5, 56, 120, 18, WHITE);
        delay(200);
        display.refresh();

        display.fillRect(0, 0, 200, 80, WHITE);
        display.setCursor(5, 46);
        display.println("Set Timed Feeding");
        display.setCursor(15, 70);
        display.print(timedStart);
        display.print(":00 - ");
        display.print(timedEnd);
        display.print(":00");
        delay(50);
        display.refresh();

        if (digitalRead(LEFT_POKE) == LOW)
        {
          timedStart += 1;
          EndTime = millis();
          if (timedStart > 24)
          {
            timedStart = 0;
          }
          if (timedStart > timedEnd)
          {
            timedEnd = timedStart + 1;
          }
        }

        if (digitalRead(RIGHT_POKE) == LOW)
        {
          timedEnd += 1;
          EndTime = millis();
          if (timedEnd > 24)
          {
            timedEnd = 0;
          }
          if (timedStart > timedEnd)
          {
            timedStart = timedEnd - 1;
          }
        }
        if (millis() - EndTime > 3000)
        { // if 3 seconds passes confirm time settings
          setTimed = false;
          display.setCursor(5, 95);
          display.println("...Timing set!");
          delay(1000);
          display.refresh();
        }
      }
      writeFEDmode();
      writeConfigFile();
      softReset(); // processor software reset
    }
  }
}

// set clock
void FED3::SetClock()
{

  DateTime now = rtc.now();
  unixtime = now.unixtime();

  /********************************************************
       Display date and time of RTC
     ********************************************************/
  display.setCursor(1, 40);
  display.print("RTC set to:");
  display.setCursor(1, 40);
  display.print("RTC set to:");

  display.fillRoundRect(0, 45, 400, 25, 1, WHITE);
  // display.refresh();
  display.setCursor(1, 60);
  if (now.month() < 10)
    display.print('0'); // Trick to add leading zero for formatting
  display.print(now.month(), DEC);
  display.print("/");
  if (now.day() < 10)
    display.print('0'); // Trick to add leading zero for formatting
  display.print(now.day(), DEC);
  display.print("/");
  display.print(now.year(), DEC);
  display.print(" ");
  display.print(now.hour(), DEC);
  display.print(":");
  if (now.minute() < 10)
    display.print('0'); // Trick to add leading zero for formatting
  display.print(now.minute(), DEC);
  display.print(":");
  if (now.second() < 10)
    display.print('0'); // Trick to add leading zero for formatting
  display.println(now.second(), DEC);
  display.drawFastHLine(30, 80, 100, BLACK);
  display.refresh();

  if (digitalRead(LEFT_POKE) == LOW)
  {
    tone(BUZZER, 800, 1);
    rtc.adjust(DateTime(unixtime - 60));
    EndTime = millis();
  }

  if (digitalRead(RIGHT_POKE) == LOW)
  {
    tone(BUZZER, 800, 1);
    rtc.adjust(DateTime(unixtime + 60));
    EndTime = millis();
  }
}

// Read battery level
void FED3::ReadBatteryLevel()
{
#if defined(ESP32)
  measuredvbat = maxlipo.cellVoltage();
#elif defined(__arm__)
  analogReadResolution(10);
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
#endif
}

/**************************************************************************************************************************************************
                                                                                               Interrupts and sleep
**************************************************************************************************************************************************/
void FED3::disableSleep()
{
  EnableSleep = false;
}

void FED3::enableSleep()
{
  EnableSleep = true;
}

// What happens when pellet is detected
void FED3::pelletTrigger()
{
  if (digitalRead(PELLET_WELL) == HIGH)
  {
    PelletAvailable = false;
  }
}

// What happens when left poke is poked
void FED3::leftTrigger()
{
  if (digitalRead(PELLET_WELL) == HIGH)
  {
    if (digitalRead(LEFT_POKE) == LOW)
    {
      Left = true;
    }
  }
}

// What happens when right poke is poked
void FED3::rightTrigger()
{
  if (digitalRead(PELLET_WELL) == HIGH)
  {
    if (digitalRead(RIGHT_POKE) == LOW)
    {
      Right = true;
    }
  }
}

// Sleep function
void FED3::goToSleep()
{
  if (EnableSleep == true)
  {
    ReleaseMotor();
    delay(2);            // let things settle
    lowPowerSleep(5000); // Wake up every 5 sec to check the pellet well
  }
  pelletTrigger(); // check pellet well to make sure it's not stuck thinking there's a pellet when there's not
}

// Pull all motor pins low to de-energize stepper and save power, also disable motor driver with the EN pin
void FED3::ReleaseMotor()
{
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);
  if (EnableSleep == true)
  {
    digitalWrite(MOTOR_ENABLE, LOW); // disable motor driver and neopixels
  }
}

/**************************************************************************************************************************************************
                                                                                               Startup Functions
**************************************************************************************************************************************************/
// Constructor

// Import Sketch variable from the Arduino script
FED3::FED3(String sketch)
{
  staticFED = this;
  this->sketch = sketch;
  this->sessiontype = sketch;
}

//  dateTime function
void FED3::dateTime(uint16_t *date, uint16_t *time)
{
  if (!staticFED)
    return; // Safety check
  DateTime now = staticFED->rtc.now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

void FED3::begin()
{
  Serial.begin(9600);
  Serial.println("Starting setup...");

  // Initialize pins
  Serial.println("Initializing pins...");
  pinMode(PELLET_WELL, INPUT_PULLUP); // protects NC at startup
  pinMode(LEFT_POKE, INPUT_PULLUP);   // protects NC at startup
  pinMode(RIGHT_POKE, INPUT_PULLUP);  // protects NC at startup
#if defined(__arm__)
  pinMode(VBATPIN, INPUT);
#endif
  pinMode(MOTOR_ENABLE, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(BNC_OUT, OUTPUT);

  // Initialize RTC
  Serial.println("Initializing RTC...");
  if (!rtc.begin())
  {
    Serial.println("RTC initialization failed!");
  }
  else
  {
    Serial.println("RTC initialized.");
  }

  // Initialize Neopixels
  Serial.println("Initializing Neopixels...");
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  Serial.println("Neopixels initialized.");

  // Initialize stepper
  Serial.println("Initializing stepper motor...");
  stepper.setSpeed(250);
  Serial.println("Stepper motor initialized.");

  // Initialize display
  Serial.println("Initializing display...");
  display.begin();
  display.setFont(&FreeSans9pt7b);
  display.setRotation(3);
  display.setTextColor(BLACK);
  display.setTextSize(1);
  Serial.println("Display initialized.");

  // Check if AHT20 temp humidity sensor is present
  Serial.println("Checking temperature/humidity sensor...");
  if (aht.begin())
  {
    tempSensor = true;
    Serial.println("AHT20 sensor detected.");
  }
  else
  {
    Serial.println("AHT20 sensor not detected.");
  }

  // Initialize SD card and create the datafile
  Serial.println("Initializing SD card...");
  SdFile::dateTimeCallback(dateTime);
  CreateFile();
  Serial.println("SD card initialized and file created.");

  // Initialize interrupts
  Serial.println("Attaching interrupts...");
  staticFED = this;
  attachWakeupInterrupts();
  Serial.println("Interrupts attached.");

  // Create data file for current session
  Serial.println("Creating data file for current session...");
  CreateDataFile();
  writeHeader();
  EndTime = 0;
  Serial.println("Data file created and header written.");

  // Read battery level
  Serial.println("Reading battery level...");
#if defined(ESP32)
  maxlipo.begin();
#endif
  ReadBatteryLevel();
  Serial.print("Battery level read: ");
  Serial.println(measuredvbat);

  // Startup display
  Serial.println("Displaying startup screen...");
  if (ClassicFED3 == true)
  {
    ClassicMenu();
  }
  else if (FED3Menu == true)
  {
    FED3MenuScreen();
  }
  else
  {
    StartScreen();
  }
  display.clearDisplay();
  display.refresh();
  Serial.println("Setup complete.");
}

void FED3::FED3MenuScreen()
{
  display.clearDisplay();
  display.setCursor(1, 135);
  display.print(filename);
  display.setCursor(10, 20);
  display.println("FED3 Menu");
  display.setCursor(11, 20);
  display.println("FED3 Menu");
  display.fillRect(0, 30, 160, 80, WHITE);
  display.setCursor(10, 40);
  display.print("Select Mode:");

  display.setCursor(10, 60);
  // Text to display selected FR ratio
  if (FEDmode == 0)
    display.print("Mode 1");
  if (FEDmode == 1)
    display.print("Mode 2");
  if (FEDmode == 2)
    display.print("Mode 3");
  if (FEDmode == 3)
    display.print("Mode 4");
  if (FEDmode == 4)
    display.print("Mode 5");
  if (FEDmode == 5)
    display.print("Mode 6");
  if (FEDmode == 6)
    display.print("Mode 7");
  if (FEDmode == 7)
    display.print("Mode 8");
  if (FEDmode == 8)
    display.print("Mode 9");
  if (FEDmode == 9)
    display.print("Mode 10");
  if (FEDmode == 10)
    display.print("Mode 11");
  if (FEDmode == 11)
    display.print("Mode 12");
  DisplayMouse();
  display.clearDisplay();
  display.refresh();
}

// Set FEDMode
void FED3::SelectMode()
{
  // Mode select on startup screen
  // If both pokes are activated
  if ((digitalRead(LEFT_POKE) == LOW) && (digitalRead(RIGHT_POKE) == LOW))
  {
    tone(BUZZER, 3000, 500);
    colorWipe(strip.Color(2, 2, 2), 40); // Color wipe
    colorWipe(strip.Color(0, 0, 0), 20); // OFF
    EndTime = millis();
    SetFED = true;
    setTimed = true;
    SetDeviceNumber();
  }

  // If Left Poke is activated
  else if (digitalRead(LEFT_POKE) == LOW)
  {
    EndTime = millis();
    FEDmode -= 1;
    tone(BUZZER, 2500, 200);
    colorWipe(strip.Color(2, 0, 2), 40); // Color wipe
    colorWipe(strip.Color(0, 0, 0), 20); // OFF
    if (FEDmode == -1)
      FEDmode = 11;
  }

  // If Right Poke is activated
  else if (digitalRead(RIGHT_POKE) == LOW)
  {
    EndTime = millis();
    FEDmode += 1;
    tone(BUZZER, 2500, 200);
    colorWipe(strip.Color(2, 2, 0), 40); // Color wipe
    colorWipe(strip.Color(0, 0, 0), 20); // OFF
    if (FEDmode == 12)
      FEDmode = 0;
  }

  if (FEDmode < 0)
    FEDmode = 0;
  if (FEDmode > 11)
    FEDmode = 11;

  display.fillRect(10, 48, 200, 50, WHITE); // erase the selected program text
  display.setCursor(10, 60);                // Display selected program

  // In classic mode we pre-specify these programs names
  if (ClassicFED3 == true)
  {
    if (FEDmode == 0)
      display.print("Free feeding");
    if (FEDmode == 1)
      display.print("FR1");
    if (FEDmode == 2)
      display.print("FR3");
    if (FEDmode == 3)
      display.print("FR5");
    if (FEDmode == 4)
      display.print("Progressive Ratio");
    if (FEDmode == 5)
      display.print("Extinction");
    if (FEDmode == 6)
      display.print("Light tracking");
    if (FEDmode == 7)
      display.print("FR1 (Reversed)");
    if (FEDmode == 8)
      display.print("Prog Ratio (Rev)");
    if (FEDmode == 9)
      display.print("Self-Stim");
    if (FEDmode == 10)
      display.print("Self-Stim (Rev)");
    if (FEDmode == 11)
      display.print("Timed feeding");
    display.refresh();
  }

  // Otherwise we don't know them and just use Mode 1 through Mode 4
  else
  {
    if (FEDmode == 0)
      display.print("Mode 1");
    if (FEDmode == 1)
      display.print("Mode 2");
    if (FEDmode == 2)
      display.print("Mode 3");
    if (FEDmode == 3)
      display.print("Mode 4");
    if (FEDmode == 4)
      display.print("Mode 5");
    if (FEDmode == 5)
      display.print("Mode 6");
    if (FEDmode == 6)
      display.print("Mode 7");
    if (FEDmode == 7)
      display.print("Mode 8");
    if (FEDmode == 8)
      display.print("Mode 9");
    if (FEDmode == 9)
      display.print("Mode 10");
    if (FEDmode == 10)
      display.print("Mode 11");
    if (FEDmode == 11)
      display.print("Mode 12");
    display.refresh();
  }

  while (millis() - EndTime < 1500)
  {
    SelectMode();
  }
  display.setCursor(10, 100);
  display.println("...Selected!");
  display.refresh();
  delay(500);
  writeFEDmode();
  delay(200);
  softReset(); // processor software reset
}

/******************************************************************************************************************************************************
                                                                                           Classic FED3 functions
******************************************************************************************************************************************************/

//  Classic menu display
void FED3::ClassicMenu()
{
  //  0 Free feeding
  //  1 FR1
  //  2 FR3
  //  3 FR5
  //  4 Progressive Ratio
  //  5 Extinction
  //  6 Light tracking FR1 task
  //  7 FR1 (reversed)
  //  8 PR (reversed)
  //  9 self-stim
  //  10 self-stim (reversed)
  //  11 time feeding

  // Set FR based on FEDmode
  if (FEDmode == 0)
    FR = 0; // free feeding
  if (FEDmode == 1)
    FR = 1; // FR1 spatial tracking task
  if (FEDmode == 2)
    FR = 3; // FR3
  if (FEDmode == 3)
    FR = 5; // FR5
  if (FEDmode == 4)
    FR = 99; // Progressive Ratio
  if (FEDmode == 5)
  { // Extinction
    FR = 1;
    ReleaseMotor();
    digitalWrite(MOTOR_ENABLE, LOW); // disable motor driver and neopixels
    delay(2);                        // let things settle
  }
  if (FEDmode == 6)
    FR = 1; // Light tracking
  if (FEDmode == 7)
    FR = 1; // FR1 (reversed)
  if (FEDmode == 8)
    FR = 1; // PR (reversed)
  if (FEDmode == 9)
    FR = 1; // self-stim
  if (FEDmode == 10)
    FR = 1; // self-stim (reversed)

  display.clearDisplay();
  display.setCursor(1, 135);
  display.print(filename);

  display.fillRect(0, 30, 160, 80, WHITE);
  display.setCursor(10, 40);
  display.print("Select Program:");

  display.setCursor(10, 60);
  // Text to display selected FR ratio
  if (FEDmode == 0)
    display.print("Free feeding");
  if (FEDmode == 1)
    display.print("FR1");
  if (FEDmode == 2)
    display.print("FR3");
  if (FEDmode == 3)
    display.print("FR5");
  if (FEDmode == 4)
    display.print("Progressive Ratio");
  if (FEDmode == 5)
    display.print("Extinction");
  if (FEDmode == 6)
    display.print("Light tracking");
  if (FEDmode == 7)
    display.print("FR1 (Reversed)");
  if (FEDmode == 8)
    display.print("Prog Ratio (Rev)");
  if (FEDmode == 9)
    display.print("Self-Stim");
  if (FEDmode == 10)
    display.print("Self-Stim (Rev)");
  if (FEDmode == 11)
    display.print("Timed feeding");

  DisplayMouse();
  display.clearDisplay();
  display.refresh();
}

// write a FEDmode file (this contains the last used FEDmode)
void FED3::writeFEDmode()
{
  // Open ratiofile and write FEDmode, then sync and close
  if (ratiofile.open("FEDmode.csv", O_WRITE | O_CREAT))
  {
    ratiofile.seekSet(0); // Move to the beginning of the file
    ratiofile.println(FEDmode);
    ratiofile.sync();  // Commit changes to the SD card
    ratiofile.close(); // Close the file
  }
  else
  {
    error(ERROR_WRITE_FAIL); // Handle error if file fails to open
  }

  // Open startfile and write timedStart, then sync and close
  if (startfile.open("start.csv", O_WRITE | O_CREAT))
  {
    startfile.seekSet(0);
    startfile.println(timedStart);
    startfile.sync();
    startfile.close();
  }
  else
  {
    error(ERROR_WRITE_FAIL);
  }

  // Open stopfile and write timedEnd, then sync and close
  if (stopfile.open("stop.csv", O_WRITE | O_CREAT))
  {
    stopfile.seekSet(0);
    stopfile.println(timedEnd);
    stopfile.sync();
    stopfile.close();
  }
  else
  {
    error(ERROR_WRITE_FAIL);
  }
}

/******************************************************************************************************************************************************
                                                                                           Mutliplatform FED Updates
******************************************************************************************************************************************************/

// software reset for multiple architectures
void FED3::softReset()
{
#if defined(ESP32)
  esp_restart();
#elif defined(__arm__)
  NVIC_SystemReset();
#endif
}

void FED3::lowPowerSleep(int sleepMs)
{
#if defined(ESP32)
  // Enable timer wake-up and enter deep sleep
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(sleepMs) * 1000);
  esp_light_sleep_start();
#elif defined(__arm__)
  // ARM-based boards using Arduino Low Power library
  LowPower.sleep(sleepMs); // Use the sleep duration directly in milliseconds
#endif
}

void FED3::attachWakeupInterrupts()
{
#if defined(ESP32)
  // ESP32: Attach interrupts for each pin
  attachInterrupt(digitalPinToInterrupt(PELLET_WELL), outsidePelletTriggerHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LEFT_POKE), outsideLeftTriggerHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RIGHT_POKE), outsideRightTriggerHandler, CHANGE);
#elif defined(__arm__)
  // ARM-based boards: Use Arduino Low Power library to attach wake-up interrupts
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(PELLET_WELL), outsidePelletTriggerHandler, CHANGE);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(LEFT_POKE), outsideLeftTriggerHandler, CHANGE);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(RIGHT_POKE), outsideRightTriggerHandler, CHANGE);
#endif
}

// Helper function to parse an integer from an SdFile
int FED3::parseIntFromSdFile(SdFile &file)
{
  char buffer[10];
  int index = 0;
  while (file.available() && index < sizeof(buffer) - 1)
  {
    char c = file.read();
    if (isdigit(c))
    {
      buffer[index++] = c;
    }
    else if (index > 0)
    {
      break;
    }
  }
  buffer[index] = '\0';
  return atoi(buffer);
}

void FED3::adjustRTC(uint32_t timestamp)
{
  rtc.adjust(DateTime(timestamp));

  DateTime now = rtc.now();
  // Print same format to serial
  Serial.print(now.month());
  Serial.print("/");
  Serial.print(now.day());
  Serial.print("/");
  Serial.print(now.year());
  Serial.print(" ");
  if (now.hour() < 10)
    Serial.print('0');
  Serial.print(now.hour());
  Serial.print(":");
  if (now.minute() < 10)
    Serial.print('0');
  Serial.println(now.minute());
}

void FED3::DisplayBLE(String advName)
{
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(BLACK);
  display.setCursor(40, 60);
  display.println("BLE");

  display.setTextSize(1);
  display.setCursor(20, 100);
  display.println(advName);

  display.refresh();
}