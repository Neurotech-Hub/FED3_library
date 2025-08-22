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

#include <Hublink.h>         // Hublink Library
Hublink hublink(cardSelect); // optional (cs, clkFreq) parameters

// Timing variables for HubLink operations
unsigned long lastHublinkUpdate = 0;
const unsigned long HUBLINK_UPDATE_INTERVAL = 10000; // 10 seconds in milliseconds

// Hublink callback function to handle timestamp
void onTimestampReceived(uint32_t timestamp)
{
  Serial.print("Received timestamp: " + String(timestamp));
  fed3.adjustRTC(timestamp);
}

void setup()
{
  // turn on serial debugger
  Serial.begin(9600);
  delay(1000);
  Serial.println("\n\n--- Hello, Hublink FR1 ---\n");

  // Rely on FED to SD.begin()
  fed3.begin(); // Setup the FED3 hardware
  fed3.disableSleep();

  if (hublink.begin())
  {
    Serial.println("✓ Hublink.");
    hublink.setTimestampCallback(onTimestampReceived);
  }
  else
  {
    Serial.println("✗ Failed.");
    while (1)
    {
    }
  }
}

void loop()
{
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

  // Update HubLink variables every 10 seconds
  if (millis() - lastHublinkUpdate >= HUBLINK_UPDATE_INTERVAL)
  {
    if (fed3.pelletIsStuck)
    {
      hublink.setAlert("Pellet Jammed!");
    }
    hublink.setBatteryLevel(fed3.ReadBatteryLevel());
    lastHublinkUpdate = millis();
  }

  // run syn every loop, only blocks when ready
  if (hublink.sync())
  {
    fed3.Event = "HublinkSync";
    fed3.logdata();
  }
}
