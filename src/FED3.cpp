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

void FED3::begin()
{
  Serial.begin(9600);
  delay(1000);
  Serial.println("Starting setup...");

  // Reset I2C bus with slower speed
  Wire.end();
  delay(100); // Give devices time to reset
  Wire.begin();
  delay(100); // Give devices time to stabilize

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
  pinMode(BNC_OUT, OUTPUT); // chip select on ESP32

  // Initialize RTC
  Serial.println("Initializing RTC...");
  if (!initializeRTC())
  {
    Serial.println("RTC initialization failed!");
    // do not proceed with setup if RTC initialization fails
    while (true)
    {
      digitalWrite(GREEN_LED, HIGH);
      delay(100);
      digitalWrite(GREEN_LED, LOW);
      delay(100);
    }
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
  // Try reading battery up to 3 times during initialization
  for (int i = 0; i < 3; i++)
  {
    ReadBatteryLevel();
    if (measuredvbat != 0.0)
      break;
    delay(10); // Short delay between attempts
  }
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

// Import Sketch variable from the Arduino script
FED3::FED3(String sketch)
{
  staticFED = this;
  this->sketch = sketch;
  this->sessiontype = sketch;
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
  delay(500); // extra delay

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

// Read battery level
void FED3::ReadBatteryLevel()
{
#if defined(ESP32)
  measuredvbat = maxlipo.cellVoltage();
  batteryPercent = maxlipo.cellPercent();
#elif defined(__arm__)
  analogReadResolution(10);
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  batteryPercent = measuredvbat / 3.3 * 100;
#endif
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
  // Configure RTC GPIO pullups for wakeup pins
  rtc_gpio_pullup_en((gpio_num_t)LEFT_POKE);
  rtc_gpio_pulldown_dis((gpio_num_t)LEFT_POKE);
  rtc_gpio_pullup_en((gpio_num_t)RIGHT_POKE);
  rtc_gpio_pulldown_dis((gpio_num_t)RIGHT_POKE);

  // Set wakeup triggers
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(sleepMs) * 1000);

  uint64_t gpio_mask = (1ULL << LEFT_POKE) | (1ULL << RIGHT_POKE);
  esp_sleep_enable_ext1_wakeup(gpio_mask, ESP_EXT1_WAKEUP_ANY_LOW);

  // Enter light sleep
  esp_light_sleep_start();

  // handle pellet well outside of interrupt context
  if (digitalRead(PELLET_WELL) == HIGH)
  { // check for pellet
    PelletAvailable = false;
  }
#elif defined(__arm__)
  LowPower.sleep(sleepMs);
#endif
}

void FED3::sleepForever()
{
#if defined(ESP32)
  esp_light_sleep_start();
#elif defined(__arm__)
  LowPower.sleep();
#endif
}

void FED3::attachWakeupInterrupts()
{
#if defined(ESP32)
  // Configure pins with pullup and set to input
  pinMode(PELLET_WELL, INPUT_PULLUP);
  pinMode(LEFT_POKE, INPUT_PULLUP);
  pinMode(RIGHT_POKE, INPUT_PULLUP);

  // Attach interrupts for active LOW detection
  // attachInterrupt(digitalPinToInterrupt(PELLET_WELL), outsidePelletTriggerHandler, FALLING);
  attachInterrupt(digitalPinToInterrupt(LEFT_POKE), outsideLeftTriggerHandler, FALLING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_POKE), outsideRightTriggerHandler, FALLING);
#elif defined(__arm__)
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(PELLET_WELL), outsidePelletTriggerHandler, CHANGE);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(LEFT_POKE), outsideLeftTriggerHandler, CHANGE);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(RIGHT_POKE), outsideRightTriggerHandler, CHANGE);
#endif
}

void FED3::detachWakeupInterrupts()
{
#if defined(ESP32)
  detachInterrupt(digitalPinToInterrupt(LEFT_POKE));
  detachInterrupt(digitalPinToInterrupt(RIGHT_POKE));
#elif defined(__arm__)
  // ArduinoLowPower doesn't provide detachInterruptWakeup method
  // The interrupts are automatically managed by the library
  // No action needed for ARM platforms
#endif
}
