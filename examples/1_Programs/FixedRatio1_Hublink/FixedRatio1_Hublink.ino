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

#include <FED3.h>  //Include the FED3 library

String sketch = "FR1";  //Unique identifier text for each sketch
FED3 fed3(sketch);      //Start the FED3 object

// ======== HUBLINK_HEADER_START ========
#include <HublinkNode_ESP32.h>                       // Hublink Library
HublinkNode_ESP32 hublinkNode(cardSelect, 1000000);  // init Hublink with clkFreq

// business logic
#define ENTER_BLE_EVERY 10           // seconds
#define LEAVE_BLE_AFTER 30           // seconds
unsigned long lastBleEntryTime = 0;  // Tracks the last time we entered the BLE sub-loop

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    hublinkNode.onConnect();
  }

  void onDisconnect(BLEServer* pServer) override {
    hublinkNode.onDisconnect();
  }
};

class FilenameCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    hublinkNode.currentFileName = String(pCharacteristic->getValue().c_str());
    if (hublinkNode.currentFileName != "") {
      hublinkNode.fileTransferInProgress = true;
    }
  }
};
// ======== HUBLINK_HEADER_END ========


void setup() {
  // turn on serial debugger
  Serial.begin(9600);
  delay(1000);
  Serial.println("Entering setup...");

  // 1.
  SPI.begin(SCK, MISO, MOSI, cardSelect);  // init SPI with FED library cardSelect

  // 2.
  fed3.begin();  //Setup the FED3 hardware
  fed3.disableSleep();

  // 3.
  // ======== HUBLINK_SETUP_START ========
  hublinkNode.initBLE("ESP32_BLE_SD");
  hublinkNode.setBLECallbacks(new ServerCallbacks(), new FilenameCallback());
  // ======== HUBLINK_SETUP_END ========

  // 4.
  Serial.println("Setup complete.");
}

// ======== HUBLINK_BLE_ACCESSORY_START ========
void enterBleSubLoop() {
  Serial.println("Entering BLE sub-loop.");
  BLEDevice::getAdvertising()->start();
  unsigned long subLoopStartTime = millis();
  bool connectedInitially = false;

  while ((millis() - subLoopStartTime < LEAVE_BLE_AFTER * 1000 && !connectedInitially) || hublinkNode.deviceConnected) {
    hublinkNode.updateConnectionStatus();  // Update connection and watchdog timeout

    // If the device just connected, mark it as initially connected
    if (hublinkNode.deviceConnected) {
      connectedInitially = true;
    }

    tone(BUZZER, 800, 30);  // Play a short tone to signal ongoing BLE operation
    delay(200);             // Avoid busy waiting
  }

  BLEDevice::getAdvertising()->stop();
  Serial.println("Leaving BLE sub-loop.");
}
// ======== HUBLINK_BLE_ACCESSORY_END ========

void loop() {
  // ======== HUBLINK_LOOP_START ========
  unsigned long currentTime = millis();  // Current time in milliseconds

  // Check if it's time to enter the BLE sub-loop
  if (currentTime - lastBleEntryTime >= ENTER_BLE_EVERY * 1000) {
    enterBleSubLoop();            // Enter and stay in BLE sub-loop for LEAVE_BLE_AFTER seconds
    lastBleEntryTime = millis();  // Reset the last entry time in milliseconds
  }
  // ======== HUBLINK_LOOP_END ========

  fed3.run();                    //Call fed.run at least once per loop
  if (fed3.Left) {               //If left poke is triggered
    fed3.logLeftPoke();          //Log left poke
    fed3.ConditionedStimulus();  //Deliver conditioned stimulus (tone and lights for 200ms)
    // fed3.Feed();                 //Deliver pellet
  }

  if (fed3.Right) {       //If right poke is triggered
    fed3.logRightPoke();  //Log right poke
  }
}
