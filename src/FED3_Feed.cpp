#include "FED3.h"

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