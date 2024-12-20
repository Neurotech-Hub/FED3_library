#include "FED3.h"

/**************************************************************************************************************************************************
                                                                                       Audio
**************************************************************************************************************************************************/
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