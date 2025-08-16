#include "FED3.h"

/******************************************************************************************************************************************************
                                                                                           Menu System Functions
******************************************************************************************************************************************************/

//  Classic menu display
void FED3::ClassicMenu()
{
    //  0 Free feeding
    //  1 FR1
    //  2 FR3
    //  3 FR5
    //  4 Progressive Ratio
    //  5 Extinction
    //  6 Light tracking FR1 task
    //  7 FR1 (reversed)
    //  8 PR (reversed)
    //  9 self-stim
    //  10 self-stim (reversed)
    //  11 time feeding

    // Set FR based on FEDmode
    if (FEDmode == 0)
        FR = 0; // free feeding
    if (FEDmode == 1)
        FR = 1; // FR1 spatial tracking task
    if (FEDmode == 2)
        FR = 3; // FR3
    if (FEDmode == 3)
        FR = 5; // FR5
    if (FEDmode == 4)
        FR = 99; // Progressive Ratio
    if (FEDmode == 5)
    { // Extinction
        FR = 1;
        ReleaseMotor();
        digitalWrite(MOTOR_ENABLE, LOW); // disable motor driver and neopixels
        delay(2);                        // let things settle
    }
    if (FEDmode == 6)
        FR = 1; // Light tracking
    if (FEDmode == 7)
        FR = 1; // FR1 (reversed)
    if (FEDmode == 8)
        FR = 1; // PR (reversed)
    if (FEDmode == 9)
        FR = 1; // self-stim
    if (FEDmode == 10)
        FR = 1; // self-stim (reversed)

    display.clearDisplay();
    display.setCursor(1, 135);
    display.print(filename);

    display.fillRect(0, 30, 160, 80, WHITE);
    display.setCursor(10, 40);
    display.print("Select Program:");

    display.setCursor(10, 60);
    // Text to display selected FR ratio
    if (FEDmode == 0)
        display.print("Free feeding");
    if (FEDmode == 1)
        display.print("FR1");
    if (FEDmode == 2)
        display.print("FR3");
    if (FEDmode == 3)
        display.print("FR5");
    if (FEDmode == 4)
        display.print("Progressive Ratio");
    if (FEDmode == 5)
        display.print("Extinction");
    if (FEDmode == 6)
        display.print("Light tracking");
    if (FEDmode == 7)
        display.print("FR1 (Reversed)");
    if (FEDmode == 8)
        display.print("Prog Ratio (Rev)");
    if (FEDmode == 9)
        display.print("Self-Stim");
    if (FEDmode == 10)
        display.print("Self-Stim (Rev)");
    if (FEDmode == 11)
        display.print("Timed feeding");

    DisplayMouse();
    display.clearDisplay();
    display.refresh();
}

// Mode selection handling
void FED3::SelectMode()
{
    // Mode select on startup screen
    // If both pokes are activated
    if ((digitalRead(LEFT_POKE) == LOW) && (digitalRead(RIGHT_POKE) == LOW))
    {
        tone(BUZZER, 3000, 500);
        colorWipe(strip.Color(2, 2, 2), 40); // Color wipe
        colorWipe(strip.Color(0, 0, 0), 20); // OFF
        EndTime = millis();
        SetFED = true;
        setTimed = true;
        SetDeviceNumber();
    }

    // If Left Poke is activated
    else if (digitalRead(LEFT_POKE) == LOW)
    {
        EndTime = millis();
        FEDmode -= 1;
        tone(BUZZER, 2500, 200);
        colorWipe(strip.Color(2, 0, 2), 40); // Color wipe
        colorWipe(strip.Color(0, 0, 0), 20); // OFF

        if (psygene)
        {
            if (FEDmode == -1)
                FEDmode = 3;
        }
        else
        {
            if (FEDmode == -1)
                FEDmode = 11;
        }
    }

    // If Right Poke is activated
    else if (digitalRead(RIGHT_POKE) == LOW)
    {
        EndTime = millis();
        FEDmode += 1;
        tone(BUZZER, 2500, 200);
        colorWipe(strip.Color(2, 2, 0), 40); // Color wipe
        colorWipe(strip.Color(0, 0, 0), 20); // OFF

        if (psygene)
        {
            if (FEDmode == 4)
                FEDmode = 0;
        }
        else
        {
            if (FEDmode == 12)
                FEDmode = 0;
        }
    }

    // Double check that modes never go over bounds
    if (psygene)
    {
        if (FEDmode < 0)
            FEDmode = 0;
        if (FEDmode > 3)
            FEDmode = 3;
    }
    else
    {
        if (FEDmode < 0)
            FEDmode = 0;
        if (FEDmode > 11)
            FEDmode = 11;
    }

    display.fillRect(10, 48, 200, 50, WHITE); // erase the selected program text
    display.setCursor(10, 60);                // Display selected program

    // Display mode names based on menu type
    if (ClassicFED3 == true)
    {
        if (FEDmode == 0)
            display.print("Free feeding");
        if (FEDmode == 1)
            display.print("FR1");
        if (FEDmode == 2)
            display.print("FR3");
        if (FEDmode == 3)
            display.print("FR5");
        if (FEDmode == 4)
            display.print("Progressive Ratio");
        if (FEDmode == 5)
            display.print("Extinction");
        if (FEDmode == 6)
            display.print("Light tracking");
        if (FEDmode == 7)
            display.print("FR1 (Reversed)");
        if (FEDmode == 8)
            display.print("Prog Ratio (Rev)");
        if (FEDmode == 9)
            display.print("Self-Stim");
        if (FEDmode == 10)
            display.print("Self-Stim (Rev)");
        if (FEDmode == 11)
            display.print("Timed feeding");
        display.refresh();
    }
    else if (psygene)
    {
        if (FEDmode == 0)
            display.print("Bandit_100_0");
        if (FEDmode == 1)
            display.print("FR1");
        if (FEDmode == 2)
            display.print("Bandit_80_20");
        if (FEDmode == 3)
            display.print("PR1");
        display.refresh();
    }
    else
    {
        if (FEDmode == 0)
            display.print("Mode 1");
        if (FEDmode == 1)
            display.print("Mode 2");
        if (FEDmode == 2)
            display.print("Mode 3");
        if (FEDmode == 3)
            display.print("Mode 4");
        if (FEDmode == 4)
            display.print("Mode 5");
        if (FEDmode == 5)
            display.print("Mode 6");
        if (FEDmode == 6)
            display.print("Mode 7");
        if (FEDmode == 7)
            display.print("Mode 8");
        if (FEDmode == 8)
            display.print("Mode 9");
        if (FEDmode == 9)
            display.print("Mode 10");
        if (FEDmode == 10)
            display.print("Mode 11");
        if (FEDmode == 11)
            display.print("Mode 12");
        display.refresh();
    }

    while (millis() - EndTime < 1500)
    {
        SelectMode();
    }
    display.setCursor(10, 100);
    display.println("...Selected!");
    display.refresh();
    delay(500);
    writeFEDmode();
    delay(200);
    softReset(); // processor software reset
}

// Psygene menu implementation
void FED3::psygeneMenu()
{
    display.clearDisplay();
    display.setCursor(1, 135);
    display.print(filename);
    display.setCursor(10, 20);
    display.println("Psygene Menu");
    display.setCursor(11, 20);
    display.println("Psygene Menu");
    display.fillRect(0, 30, 160, 80, WHITE);
    display.setCursor(10, 40);
    display.print("Select Mode:");

    display.setCursor(10, 60);
    if (FEDmode == 0)
        display.print("Bandit_100_0");
    if (FEDmode == 1)
        display.print("FR1");
    if (FEDmode == 2)
        display.print("Bandit_80_20");
    if (FEDmode == 3)
        display.print("PR1");

    DisplayMouse();
    display.clearDisplay();
    display.refresh();
}

// write a FEDmode file (this contains the last used FEDmode)
// Change device number through menu interface
void FED3::SetDeviceNumber()
{
    // This code is activated when both pokes are pressed simultaneously from the
    // start screen, allowing the user to set the device # of the FED on the device
    while (SetFED == true)
    {
        // adjust FED device number
        display.fillRect(0, 0, 200, 80, WHITE);
        display.setCursor(5, 46);
        display.println("Set Device Number");
        display.fillRect(36, 122, 180, 28, WHITE);
        delay(100);
        display.refresh();

        display.setCursor(38, 138);
        if (FED < 100 & FED >= 10)
        {
            display.print("0");
        }
        if (FED < 10)
        {
            display.print("00");
        }
        display.print(FED);

        delay(100);
        display.refresh();

        if (digitalRead(RIGHT_POKE) == LOW)
        {
            FED += 1;
            Click();
            EndTime = millis();
            if (FED > 700)
            {
                FED = 700;
            }
        }

        if (digitalRead(LEFT_POKE) == LOW)
        {
            FED -= 1;
            Click();
            EndTime = millis();
            if (FED < 1)
            {
                FED = 0;
            }
        }
        if (millis() - EndTime > 3000)
        { // if 3 seconds passes confirm device #
            SetFED = false;
            display.setCursor(5, 70);
            display.println("...Set!");
            display.refresh();
            delay(1000);
            EndTime = millis();
            display.clearDisplay();
            display.refresh();

            ///////////////////////////////////
            //////////  ADJUST CLOCK //////////
            while (millis() - EndTime < 3000)
            {
                SetClock();
                delay(10);
            }

            display.setCursor(5, 105);
            display.println("...Clock is set!");
            display.refresh();
            delay(1000);

            ///////////////////////////////////

            while (setTimed == true)
            {
                // set timed feeding start and stop
                display.fillRect(5, 56, 120, 18, WHITE);
                delay(200);
                display.refresh();

                display.fillRect(0, 0, 200, 80, WHITE);
                display.setCursor(5, 46);
                display.println("Set Timed Feeding");
                display.setCursor(15, 70);
                display.print(timedStart);
                display.print(":00 - ");
                display.print(timedEnd);
                display.print(":00");
                delay(50);
                display.refresh();

                if (digitalRead(LEFT_POKE) == LOW)
                {
                    timedStart += 1;
                    EndTime = millis();
                    if (timedStart > 24)
                    {
                        timedStart = 0;
                    }
                    if (timedStart > timedEnd)
                    {
                        timedEnd = timedStart + 1;
                    }
                }

                if (digitalRead(RIGHT_POKE) == LOW)
                {
                    timedEnd += 1;
                    EndTime = millis();
                    if (timedEnd > 24)
                    {
                        timedEnd = 0;
                    }
                    if (timedStart > timedEnd)
                    {
                        timedStart = timedEnd - 1;
                    }
                }
                if (millis() - EndTime > 3000)
                { // if 3 seconds passes confirm time settings
                    setTimed = false;
                    display.setCursor(5, 95);
                    display.println("...Timing set!");
                    delay(1000);
                    display.refresh();
                }
            }
            writeFEDmode();
            writeConfigFile();
            softReset(); // processor software reset
        }
    }
}

void FED3::FED3MenuScreen()
{
    display.clearDisplay();
    display.setCursor(1, 135);
    display.print(filename);
    display.setCursor(10, 20);
    display.println("FED3 Menu");
    display.setCursor(11, 20);
    display.println("FED3 Menu");
    display.fillRect(0, 30, 160, 80, WHITE);
    display.setCursor(10, 40);
    display.print("Select Mode:");

    display.setCursor(10, 60);
    // Text to display selected FR ratio
    if (FEDmode == 0)
        display.print("Mode 1");
    if (FEDmode == 1)
        display.print("Mode 2");
    if (FEDmode == 2)
        display.print("Mode 3");
    if (FEDmode == 3)
        display.print("Mode 4");
    if (FEDmode == 4)
        display.print("Mode 5");
    if (FEDmode == 5)
        display.print("Mode 6");
    if (FEDmode == 6)
        display.print("Mode 7");
    if (FEDmode == 7)
        display.print("Mode 8");
    if (FEDmode == 8)
        display.print("Mode 9");
    if (FEDmode == 9)
        display.print("Mode 10");
    if (FEDmode == 10)
        display.print("Mode 11");
    if (FEDmode == 11)
        display.print("Mode 12");
    DisplayMouse();
    display.clearDisplay();
    display.refresh();
}

void FED3::writeFEDmode()
{
    // Open ratiofile and write FEDmode, then sync and close
    if (ratiofile.open("FEDmode.csv", O_WRITE | O_CREAT))
    {
        ratiofile.seekSet(0); // Move to the beginning of the file
        ratiofile.println(FEDmode);
        ratiofile.sync();  // Commit changes to the SD card
        ratiofile.close(); // Close the file
    }
    else
    {
        error(ERROR_WRITE_FAIL); // Handle error if file fails to open
    }

    // Open startfile and write timedStart, then sync and close
    if (startfile.open("start.csv", O_WRITE | O_CREAT))
    {
        startfile.seekSet(0);
        startfile.println(timedStart);
        startfile.sync();
        startfile.close();
    }
    else
    {
        error(ERROR_WRITE_FAIL);
    }

    // Open stopfile and write timedEnd, then sync and close
    if (stopfile.open("stop.csv", O_WRITE | O_CREAT))
    {
        stopfile.seekSet(0);
        stopfile.println(timedEnd);
        stopfile.sync();
        stopfile.close();
    }
    else
    {
        error(ERROR_WRITE_FAIL);
    }
}