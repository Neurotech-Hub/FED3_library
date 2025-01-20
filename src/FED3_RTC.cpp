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
    if (!preferences.begin(PREFS_NAMESPACE, false))
    {
        Serial.println("ERROR: Failed to initialize preferences");
        return false;
    }

    // Only check for new compilation on hard reset
    esp_reset_reason_t reset_reason = esp_reset_reason();
    bool isWakeFromSleep = (reset_reason == ESP_SLEEP_WAKEUP_TIMER);

    if (!isWakeFromSleep && isNewCompilation())
    {
        Serial.println("New compilation detected - updating RTC");
        updateRTC();
        updateCompilationID();
    }
    else if (rtc.lostPower())
    {
        Serial.println("RTC lost power, updating time from compilation");
        updateRTC();
    }
    else
    {
        Serial.println("No new compilation detected - using existing RTC time");
    }

    preferences.end();
    Serial.println("RTC initialization complete");

    // Initialize ESP32's internal RTC
    DateTime now = rtc.now();
    ESP32Time rtc_internal(0);
    rtc_internal.setTime(
        now.second(),
        now.minute(),
        now.hour(),
        now.day(),
        now.month(),
        now.year(),
        0);
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
    // Force format to be consistent and add milliseconds to reduce caching
    char compileDateTime[32];
    snprintf(compileDateTime, sizeof(compileDateTime),
             "%s %s.%lu",
             __DATE__, __TIME__, millis()); // Add millis() to force uniqueness

    Serial.println("Build timestamp: " + String(compileDateTime));
    return String(compileDateTime);
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

    Serial.printf("Compile time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  year, month, day, hour, minute, second);

    // Add upload delay compensation (2 seconds)
    DateTime compileTime(year, month, day, hour, minute, second);
    // add 30 seconds to the compile time to account for upload delay
    DateTime compensatedTime = compileTime + TimeSpan(0, 0, 0, 30);

    Serial.printf("Compensated time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  compensatedTime.year(), compensatedTime.month(), compensatedTime.day(),
                  compensatedTime.hour(), compensatedTime.minute(), compensatedTime.second());

    rtc.adjust(compensatedTime);

    // Verify the time was set
    DateTime now = rtc.now();
    Serial.printf("Verified time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
}