#include "FED3.h"

/******************************************************************************************************************************************************
                                                                                           Classic FED3 functions
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

// write a FEDmode file (this contains the last used FEDmode)
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
