#include "FED3.h"

// New RTC initialization function
bool FED3::initializeRTC()
{
    Serial.println("Initializing RTC...");

    if (!rtc.begin())
    {
        Serial.println("ERROR: Couldn't find RTC! Check circuit.");
        return false;
    }
    Serial.println("RTC found successfully");

#if defined(ESP32)
    Serial.println("Starting preferences...");
    // Single preferences session
    if (!preferences.begin(PREFS_NAMESPACE, false))
    {
        Serial.println("ERROR: Failed to initialize preferences");
        return false;
    }

    if (isNewCompilation())
    {
        Serial.println("New compilation detected - updating RTC");
        updateRTC();
        updateCompilationID();
    }
    else
    {
        Serial.println("No new compilation detected - using existing RTC time");
    }

    preferences.end();
    Serial.println("RTC initialization complete");

    // Initialize ESP32's internal RTC
    DateTime now = rtc.now();
    ESP32Time rtc_internal(0); // offset 0
    rtc_internal.setTime(
        now.second(), // seconds
        now.minute(), // minutes
        now.hour(),   // hours
        now.day(),    // day
        now.month(),  // month
        now.year(),   // year
        0             // milliseconds
    );
    Serial.println("ESP32 internal RTC synchronized");
#endif

    return true;
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

// Get current time
DateTime FED3::now()
{
    return rtc.now();
}

// Print RTC time to serial
void FED3::serialPrintRTC()
{
    DateTime current = now();
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d",
                  current.year(), current.month(), current.day(),
                  current.hour(), current.minute(), current.second());
}

// Get compilation date/time
String FED3::getCompileDateTime()
{
    return String(__DATE__) + " " + String(__TIME__);
}

// Check if this is a new compilation
bool FED3::isNewCompilation()
{
#if defined(ESP32)
    // Note: preferences should already be initialized by caller
    const String currentCompileTime = getCompileDateTime();
    const String storedCompileTime = preferences.getString("compileTime", "");

    Serial.println("Checking compilation status:");
    Serial.println(" - Current compile time: " + currentCompileTime);
    Serial.println(" - Stored compile time: " + storedCompileTime);

    return (storedCompileTime != currentCompileTime);
#else
    return false;
#endif
}

// Update the stored compilation ID
void FED3::updateCompilationID()
{
#if defined(ESP32)
    // Note: preferences should already be initialized by caller
    const String currentCompileTime = getCompileDateTime();
    preferences.putString("compileTime", currentCompileTime.c_str());
    Serial.println("Updated stored compilation time to: " + currentCompileTime);
#endif
}

// Update RTC with compilation time
void FED3::updateRTC()
{
    char monthStr[4];
    int month, day, year, hour, minute, second;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);
    month = (strstr(month_names, monthStr) - month_names) / 3 + 1;
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    Serial.printf("Setting RTC to: %04d-%02d-%02d %02d:%02d:%02d\n",
                  year, month, day, hour, minute, second);

    rtc.adjust(DateTime(year, month, day, hour, minute, second));

    // Verify the time was set
    DateTime now = rtc.now();
    Serial.printf("RTC time after setting: %04d-%02d-%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
}