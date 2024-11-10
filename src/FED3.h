/*
Feeding experimentation device 3 (FED3) library
Code by Lex Kravitz, adapted to Arduino library format by Eric Lin
alexxai@wustl.edu
May 2021

The original FED device was developed by Katrina Nguyen at al in 2016:
https://www.ncbi.nlm.nih.gov/pubmed/27060385

This device includes hardware and code from:
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

#define VER "1.17.0"

#ifndef FED3_H
#define FED3_H

// include these libraries
#include <Arduino.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>
#include <Wire.h>
#include <SPI.h>
#include <Stepper.h>

#if defined(ESP32)
#include <esp_sleep.h>
#elif defined(__arm__)
#include <ArduinoLowPower.h>
#endif

#include "RTClib.h"
#include <SdFat.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Org_01.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_AHTX0.h>

// Feather M0: https://github.com/adafruit/Adafruit-Feather-M0-Adalogger-PCB/blob/master/Adafruit%20Feather%20M0%20Adalogger%20Pinout.pdf
// Feather ESP32-S3: https://github.com/adafruit/Adafruit-Feather-ESP32-S3-PCB/blob/main/Adafruit%20Feather%20ESP32-S3%20Pinout.pdf
// Pin definitions
#if defined(ESP32)
#define NEOPIXEL 33
#define NEOPIXEL_POWER 21
#define MOTOR_ENABLE 13
#define GREEN_LED LED_BUILTIN
#define PELLET_WELL 39
#define LEFT_POKE 6
#define RIGHT_POKE 5
#define BUZZER 38
// #define VBATPIN         A7 // replaced by MAX17048 Battery Monitor
#define cardSelect A0
#define BNC_OUT 1 // BNC_OUT send to NC GPIO for ESP32
#define SHARP_SCK 12
#define SHARP_MOSI 11
#define SHARP_SS 10
#elif defined(__arm__)
#define NEOPIXEL A1
#define MOTOR_ENABLE 13
#define GREEN_LED 8
#define PELLET_WELL 1
#define LEFT_POKE 6
#define RIGHT_POKE 5
#define BUZZER 0
#define VBATPIN A7
#define cardSelect 4
#define BNC_OUT A0
#define SHARP_SCK 12
#define SHARP_MOSI 11
#define SHARP_SS 10
#endif

#define BLACK 0
#define WHITE 1
#define STEPS 2038

extern bool Left;

enum ErrorCode
{
    ERROR_SD_INIT_FAIL = 1, // Explicitly assigned to 1
    ERROR_FILE_NOT_FOUND,   // Automatically assigned to 2
    ERROR_WRITE_FAIL,       // Automatically assigned to 3
    ERROR_READ_FAIL         // Automatically assigned to 4
};

class FED3
{
    // Members
public:
    FED3(void);
    FED3(String sketch);
    String sketch = "undef";
    String sessiontype = "undef";

    void classInterruptHandler(void);
    void begin();
    void run();

    // SD logging
    // encapsulate SdFat Library for conflicts
    SdFat fed3SD;
    SdFile logfile;
    SdFile ratiofile;
    SdFile configfile;
    SdFile startfile;
    SdFile stopfile;

    char filename[21]; // Array for file name data logged to named in setup
    void logdata();
    void CreateFile();
    void CreateDataFile();
    void writeHeader();
    void writeConfigFile();
    void writeFEDmode();
    void error(ErrorCode errorCode);
    void getFilename(char *filename);
    bool suppressSDerrors = false; // set to true to suppress SD card errors at startup

    // Battery
    float measuredvbat = 1.0;
    void ReadBatteryLevel();

    // Neopixel
    void pixelsOn(int R, int G, int B, int W);
    void pixelsOff();
    void Blink(byte PIN, byte DELAY_MS, byte loops);
    void colorWipe(uint32_t c, uint8_t wait);
    void leftPixel(int R, int G, int B, int W);
    void rightPixel(int R, int G, int B, int W);
    void leftPokePixel(int R, int G, int B, int W);
    void rightPokePixel(int R, int G, int B, int W);

    // Display functions
    void UpdateDisplay();
    void DisplaySDError();
    void DisplayJamClear();
    void DisplayRetrievalInt();
    void DisplayLeftInt();
    void DisplayRightInt();
    void DisplayBattery();
    void DisplayDateTime();
    void DisplayIndicators();
    void DisplayTimedFeeding();
    void DisplayNoProgram();
    void DisplayMinPoke();
    void DisplayMouse();

    // Startup menu function
    void ClassicMenu();
    void StartScreen();
    void FED3MenuScreen();
    void SetClock();

    // BNC input/output
    void ReadBNC(bool blinkGreen);
    bool BNCinput = false;

    // Motor
    void ReleaseMotor();
    int numMotorTurns = 0;

    // Set FED
    void SelectMode();
    void SetDeviceNumber();

    // Stimuli
    void ConditionedStimulus(int duration = 200);
    void Click();
    void Noise(int duration = 200);
    void BNC(int DELAY_MS, int loops);
    void pulseGenerator(int pulse_width, int frequency, int repetitions);

    void Tone(int freq, int duration);
    void stopTone();

    // Pelet and poke functions
    void CheckRatio();
    void logLeftPoke();
    void logRightPoke();
    void Feed(int pulse = 0, bool pixelsoff = true);
    bool dispenseTimer_ms(int ms);
    void pelletTrigger();
    void leftTrigger();
    void rightTrigger();
    void goToSleep();
    void Timeout(int timeout, bool reset = false, bool whitenoise = false);
    int minPokeTime = 0;
    int maxPokeTime = 20000;
    void randomizeActivePoke(int max);
    int consecutive = 0;

    // jam movements
    bool RotateDisk(int steps);
    bool ClearJam();
    bool VibrateJam();
    bool MinorJam();

    // timed feeding variables
    int timedStart; // hour to start the timed Feeding session, out of 24 hour clock
    int timedEnd;   // hour to start the timed Feeding session, out of 24 hour clock

    // mode variables
    int FED;
    int FR = 1;
    bool DisplayPokes = true;
    bool DisplayTimed = false;
    byte FEDmode = 1;
    byte previousFEDmode = FEDmode;

    // event counters
    int LeftCount = 0;
    int RightCount = 0;
    int PelletCount = 0;
    int BlockPelletCount = 0;
    int timeout = 0;

    bool countAllPokes = true;

    // state variables
    bool activePoke = 1; // 0 for right, 1 for left, defaults to left poke active
    bool Left = false;
    bool Right = false;
    bool PelletAvailable = false;
    unsigned long currentHour;
    unsigned long currentMinute;
    unsigned long currentSecond;
    unsigned long displayupdate;
    String Event = "None"; // What kind of event just happened?

    // Bandit variables
    int prob_left = 0;
    int prob_right = 0;
    int pelletsToSwitch = 0;
    bool allowBlockRepeat = false;

    // timing variables
    int retInterval = 0;
    int leftInterval = 0;
    int rightInterval = 0;
    int leftPokeTime = 0.0;
    int rightPokeTime = 0.0;
    unsigned long pelletTime = 0;
    unsigned long lastPellet = 0;
    unsigned long unixtime = 0;
    int interPelletInterval = 0;

    // flags
    bool Ratio_Met = false;
    bool EnableSleep = true;
    void disableSleep();
    void enableSleep();
    bool ClassicFED3 = false;
    bool FED3Menu = false;
    bool tempSensor = false;

    int EndTime = 0;
    int ratio = 1;
    int previousFR = FR;
    int previousFED = FED;

    bool SetFED = false;
    bool setTimed = false;

    // Neopixel strip
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(10, NEOPIXEL, NEO_GRBW + NEO_KHZ800);
    // Display
    Adafruit_SharpMem display = Adafruit_SharpMem(SHARP_SCK, SHARP_MOSI, SHARP_SS, 144, 168);
    // Stepper
    Stepper stepper = Stepper(STEPS, A2, A3, A4, A5);
    // Temp/Humidity Sensor
    Adafruit_AHTX0 aht;

    // Multiplatform
    void softReset();
    void lowPowerSleep(int sleepMs);
    void attachWakeupInterrupts();
    int parseIntFromSdFile(SdFile &file);

private:
    static FED3 *staticFED;
    static void updatePelletTriggerISR();
    static void updateLeftTriggerISR();
    static void updateRightTriggerISR();
};

#endif
