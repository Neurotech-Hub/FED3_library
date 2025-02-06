#include "FED3.h"

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

    DateTime now = rtc.now();

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
    // !! temporary debugging
#if defined(ESP32)
    logfile.print(ESP.getFreeHeap());
#else
    logfile.print(VER);
#endif
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

    // new option to create a new file for each day
    if (createDailyFile == true)
    {
        // copied from below for consistency
        if (filename[9] != now.day() / 10 + '0' || filename[10] != now.day() % 10 + '0')
        {
            CreateDataFile();
            writeHeader();
            LeftCount = 0;
            RightCount = 0;
            PelletCount = 0;
        }
    }
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

/**
 * Retrieves a value from the meta.json configuration file
 *
 * Example usage:
 *   String program = getMetaValue("fed", "program");     // returns "Classic"
 *   String mouseId = getMetaValue("subject", "id");      // returns "mouse001"
 */
String FED3::getMetaValue(const char *rootKey, const char *subKey)
{
    FatFile metaFile;
    if (!metaFile.open(META_JSON_PATH, O_READ))
    {
        Serial.println("Failed to open meta.json");
        return "";
    }

    const size_t capacity = JSON_OBJECT_SIZE(3) + 120; // Increased for nested objects
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, metaFile);
    metaFile.close();

    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return "";
    }

    // Get the root object first
    JsonObject rootObj = doc[rootKey];
    if (!rootObj.isNull())
    {
        // Then get the nested value
        const char *value = rootObj[subKey];
        if (value)
        {
            return String(value);
        }
    }

    Serial.printf("Value not found for %s > %s\n", rootKey, subKey);
    return "";
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