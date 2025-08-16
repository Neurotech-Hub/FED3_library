#include "FED3.h"

void FED3::Timeout(int seconds, bool reset, bool whitenoise)
{
    unsigned long timeoutStart = millis();

    while ((millis() - timeoutStart) < (seconds * 1000UL))
    {
        // Generate white noise if enabled
        if (whitenoise)
        {
            int freq = random(50, 250);
            tone(BUZZER, freq, 10);
            delay(10);
        }

        // Handle left poke during timeout
        if (digitalRead(LEFT_POKE) == LOW)
        {
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
            while (digitalRead(LEFT_POKE) == LOW)
            {
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

        // Handle right poke during timeout
        if (digitalRead(RIGHT_POKE) == LOW)
        {
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
            while (digitalRead(RIGHT_POKE) == LOW)
            { // Fixed: was checking LEFT_POKE
                if (whitenoise)
                {
                    int freq = random(50, 250);
                    tone(BUZZER, freq, 10);
                }
            }
            rightInterval = (millis() - rightPokeTime);
            UpdateDisplay();
            Event = "RightinTimeout";
            logdata();
        }
    }

    // Clear timeout display and reset states
    display.fillRect(5, 20, 100, 25, WHITE);
    UpdateDisplay();
    Left = false;
    Right = false;
}

void FED3::ConditionedStimulus(int duration)
{
    tone(BUZZER, 4000, duration);
    pixelsOn(0, 0, 10, 0); // blue light for all
}

void FED3::Click()
{
    tone(BUZZER, 800, 8);
}

void FED3::Tone(int freq, int duration)
{
    tone(BUZZER, freq, duration);
}

void FED3::stopTone()
{
    noTone(BUZZER);
}

void FED3::Noise(int duration)
{
    // White noise to signal errors
    for (int i = 0; i < duration / 50; i++)
    {
        tone(BUZZER, random(50, 250), 50);
        delay(duration / 50);
    }
}