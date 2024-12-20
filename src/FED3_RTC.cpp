#include "FED3.h"

// New RTC initialization function
bool FED3::initializeRTC()
{
    if (!rtc.begin())
    {
        return false;
    }

#if defined(ESP32)
    // Single preferences session
    if (!preferences.begin(PREFS_NAMESPACE, false))
    {
        return false;
    }

    if (isNewCompilation())
    {
        updateRTC();
        updateCompilationID();
    }

    preferences.end();
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
    const String currentCompileTime = getCompileDateTime();
    const String storedCompileTime = preferences.getString("compileTime", "");
    return (storedCompileTime != currentCompileTime);
#else
    return false;
#endif
}

// Update the stored compilation ID
void FED3::updateCompilationID()
{
#if defined(ESP32)
    const String currentCompileTime = getCompileDateTime();
    preferences.putString("compileTime", currentCompileTime.c_str());
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

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
}