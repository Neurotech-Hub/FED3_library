#include "FED3.h"

/**************************************************************************************************************************************************
                                                                                                Poke functions
**************************************************************************************************************************************************/
// log left poke
void FED3::logLeftPoke()
{
    if (PelletAvailable == false)
    {
        leftPokeTime = millis();
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
        DisplayLeftInt();
        if (leftInterval < minPokeTime)
        {
            Event = "LeftShort";
        }
        else
        {
            Event = "Left";
        }

        logdata();
        Left = false;
    }
}

// log right poke
void FED3::logRightPoke()
{
    if (PelletAvailable == false)
    {
        rightPokeTime = millis();
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
        DisplayRightInt();
        if (rightInterval < minPokeTime)
        {
            Event = "RightShort";
        }
        else
        {
            Event = "Right";
        }

        logdata();
        Right = false;
    }
}

void FED3::randomizeActivePoke(int max)
{
    // Store last active side and randomize
    byte lastActive = activePoke;
    activePoke = random(0, 2);

    // Increment consecutive active pokes, or reset consecutive to zero
    if (activePoke == lastActive)
    {
        consecutive++;
    }
    else
    {
        consecutive = 0;
    }

    // if consecutive pokes are too many, swap pokes
    if (consecutive >= max)
    {
        if (activePoke == 0)
        {
            activePoke = 1;
        }
        else if (activePoke == 1)
        {
            activePoke = 0;
        }
        consecutive = 0;
    }
}