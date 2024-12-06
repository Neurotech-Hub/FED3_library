/*
  Feeding experimentation device 3 (FED3) with hublink.cloud
  Fixed Ratio 1
  alexxai@wustl.edu
  December, 2020

  gaidica@wustl.edu
  November, 2024

  This project is released under the terms of the Creative Commons - Attribution - ShareAlike 3.0 license:
  human readable: https://creativecommons.org/licenses/by-sa/3.0/
  legal wording: https://creativecommons.org/licenses/by-sa/3.0/legalcode
  Copyright (c) 2020 Lex Kravitz
*/

#include <FED3.h> //Include the FED3 library

String sketch = "FR1"; // Unique identifier text for each sketch
FED3 fed3(sketch);     // Start the FED3 object

// ======== HUBLINK_HEADER_START ========
#include <Hublink.h>         // Hublink Library
Hublink hublink(cardSelect); // optional (cs, clkFreq) parameters
unsigned long lastBleEntryTime;
const String advName = "HUBNODE";

class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    hublink.onConnect();
  }

  void onDisconnect(BLEServer *pServer) override
  {
    hublink.onDisconnect();
  }
};

class FilenameCallback : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    hublink.currentFileName = String(pCharacteristic->getValue().c_str());
  }
};

class GatewayCallback : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic) override
  {
    String rtc = hublink.parseGateway(pCharacteristic, "rtc");
    if (rtc.length() > 0)
    {
      Serial.println("Gateway settings received:");
      uint32_t timestamp = rtc.toInt();
      fed3.adjustRTC(timestamp);
      Serial.println("RTC updated to timestamp: " + rtc);
    }
    else
    {
      Serial.println("No valid gateway settings found");
    }
    hublink.sendFilenames = true; // true for any change, triggers sending available filenames
  }
};

// Callback instances
static ServerCallbacks serverCallbacks;
static FilenameCallback filenameCallback;
static GatewayCallback gatewayCallback;

void enterBleSubLoop()
{
  Serial.println("Entering BLE sub-loop.");
  hublink.startAdvertising();

  unsigned long subLoopStartTime = millis();
  bool didConnect = false;

  while ((millis() - subLoopStartTime < hublink.bleConnectFor * 1000 && !didConnect) || hublink.deviceConnected)
  {
    hublink.updateConnectionStatus();
    didConnect |= hublink.deviceConnected;
    delay(100);
  }

  hublink.stopAdvertising();
  Serial.printf("Leaving BLE sub-loop. Free heap: %d\n", ESP.getFreeHeap());
}
// ======== HUBLINK_HEADER_END ========

void setup()
{
  // turn on serial debugger
  Serial.begin(9600);
  delay(1000);
  Serial.println("\n\n--- Hello, Hublink FR1 ---\n");

  // Rely on FED to SD.begin()
  fed3.begin(); // Setup the FED3 hardware
  // fed3.disableSleep();

  // One-time BLE initialization
  hublink.init(advName, true);
  hublink.setBLECallbacks(&serverCallbacks,
                          &filenameCallback,
                          &gatewayCallback);

  lastBleEntryTime = millis();
  Serial.println("Hublink FR1 setup complete.");
}

void loop()
{
  // ======== HUBLINK_LOOP_START ========
  unsigned long currentTime = millis(); // Current time in milliseconds

  // Check if it's time to enter the BLE sub-loop and not disabled
  if (!hublink.disable && currentTime - lastBleEntryTime >= hublink.bleConnectEvery * 1000)
  {
    enterBleSubLoop();
    lastBleEntryTime = millis();
  }
  // ======== HUBLINK_LOOP_END ========

  fed3.run(); // Call fed.run at least once per loop
  if (fed3.Left)
  {                             // If left poke is triggered
    fed3.logLeftPoke();         // Log left poke
    fed3.ConditionedStimulus(); // Deliver conditioned stimulus (tone and lights for 200ms)
    fed3.Feed();                // Deliver pellet
  }

  if (fed3.Right)
  {                      // If right poke is triggered
    fed3.logRightPoke(); // Log right poke
  }
}
