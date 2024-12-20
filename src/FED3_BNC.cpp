#include "FED3.h"

// Simple function for sending square wave pulses to the BNC port
void FED3::BNC(int DELAY_MS, int loops)
{
    for (int i = 0; i < loops; i++)
    {
        digitalWrite(BNC_OUT, HIGH);
        digitalWrite(GREEN_LED, HIGH);
        delay(DELAY_MS);
        digitalWrite(BNC_OUT, LOW);
        digitalWrite(GREEN_LED, LOW);
        delay(DELAY_MS);
    }
}

// More advanced function for controlling pulse width and frequency for the BNC port
void FED3::pulseGenerator(int pulse_width, int frequency, int repetitions)
{ // freq in Hz, width in ms, loops in number of times
    for (byte j = 0; j < repetitions; j++)
    {
        digitalWrite(BNC_OUT, HIGH);
        digitalWrite(GREEN_LED, HIGH);
        delay(pulse_width); // pulse high for width
        digitalWrite(BNC_OUT, LOW);
        digitalWrite(GREEN_LED, LOW);
        long temp_delay = (1000 / frequency) - pulse_width;
        if (temp_delay < 0)
            temp_delay = 0; // if temp delay <0 because parameters are set wrong, set it to 0 so FED3 doesn't crash O_o
        delay(temp_delay);  // pin low
    }
}

void FED3::ReadBNC(bool blinkGreen)
{
    pinMode(BNC_OUT, INPUT_PULLDOWN);
    BNCinput = false;
    if (digitalRead(BNC_OUT) == HIGH)
    {
        delay(1);
        if (digitalRead(BNC_OUT) == HIGH)
        {
            if (blinkGreen == true)
            {
                digitalWrite(GREEN_LED, HIGH);
                delay(25);
                digitalWrite(GREEN_LED, LOW);
            }
            BNCinput = true;
        }
    }
}