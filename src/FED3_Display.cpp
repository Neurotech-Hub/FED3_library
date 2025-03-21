#include "FED3.h"

/**************************************************************************************************************************************************
                                                                                               Display functions
**************************************************************************************************************************************************/
void FED3::UpdateDisplay()
{
    // Box around data area of screen
    display.drawRect(5, 45, 158, 70, BLACK);

    display.setCursor(5, 15);
    display.print("FED:");
    display.println(FED);
    display.setCursor(6, 15); // this doubling is a way to do bold type
    display.print("FED:");
    display.fillRect(6, 20, 200, 22, WHITE);  // erase text under battery row without clearing the entire screen
    display.fillRect(35, 46, 120, 68, WHITE); // erase the pellet data on screen without clearing the entire screen
    display.setCursor(5, 36);                 // display which sketch is running

    // write the first 8 characters of sessiontype:
    display.print(sessiontype.charAt(0));
    display.print(sessiontype.charAt(1));
    display.print(sessiontype.charAt(2));
    display.print(sessiontype.charAt(3));
    display.print(sessiontype.charAt(4));
    display.print(sessiontype.charAt(5));
    display.print(sessiontype.charAt(6));
    display.print(sessiontype.charAt(7));

    if (DisplayPokes == 1)
    {
        display.setCursor(35, 65);
        display.print("Left: ");
        display.setCursor(95, 65);
        display.print(LeftCount);
        display.setCursor(35, 85);
        display.print("Right:  ");
        display.setCursor(95, 85);
        display.print(RightCount);
    }

    display.setCursor(35, 105);
    display.print("Pellets:");
    display.setCursor(95, 105);
    display.print(PelletCount);

    if (DisplayTimed == true)
    { // If it's a timed Feeding Session
        DisplayTimedFeeding();
    }

    DisplayBattery();
    DisplayDateTime();
    DisplayIndicators();
    display.refresh();
}

void FED3::SetClock()
{

    DateTime now = rtc.now();
    unixtime = now.unixtime();

    /********************************************************
         Display date and time of RTC
       ********************************************************/
    display.setCursor(1, 40);
    display.print("RTC set to:");
    display.setCursor(1, 40);
    display.print("RTC set to:");

    display.fillRoundRect(0, 45, 400, 25, 1, WHITE);
    // display.refresh();
    display.setCursor(1, 60);
    if (now.month() < 10)
        display.print('0'); // Trick to add leading zero for formatting
    display.print(now.month(), DEC);
    display.print("/");
    if (now.day() < 10)
        display.print('0'); // Trick to add leading zero for formatting
    display.print(now.day(), DEC);
    display.print("/");
    display.print(now.year(), DEC);
    display.print(" ");
    display.print(now.hour(), DEC);
    display.print(":");
    if (now.minute() < 10)
        display.print('0'); // Trick to add leading zero for formatting
    display.print(now.minute(), DEC);
    display.print(":");
    if (now.second() < 10)
        display.print('0'); // Trick to add leading zero for formatting
    display.println(now.second(), DEC);
    display.drawFastHLine(30, 80, 100, BLACK);
    display.refresh();

    if (digitalRead(LEFT_POKE) == LOW)
    {
        tone(BUZZER, 800, 1);
        rtc.adjust(DateTime(unixtime - 60));
        EndTime = millis();
    }

    if (digitalRead(RIGHT_POKE) == LOW)
    {
        tone(BUZZER, 800, 1);
        rtc.adjust(DateTime(unixtime + 60));
        EndTime = millis();
    }
}

void FED3::DisplayDateTime()
{
    // Print date and time at bottom of the screen
    DateTime now = rtc.now();

    // Print to display
    display.setCursor(0, 135);
    display.fillRect(0, 123, 200, 60, WHITE);
    display.print(now.month());
    display.print("/");
    display.print(now.day());
    display.print("/");
    display.print(now.year());
    display.print("      ");
    if (now.hour() < 10)
        display.print('0'); // Trick to add leading zero for formatting
    display.print(now.hour());
    display.print(":");
    if (now.minute() < 10)
        display.print('0'); // Trick to add leading zero for formatting
    display.print(now.minute());
}

void FED3::DisplayIndicators()
{
    // Pellet circle
    display.fillCircle(25, 99, 5, WHITE); // pellet
    display.drawCircle(25, 99, 5, BLACK);

    // Poke indicators
    if (DisplayPokes == 1)
    { // only make active poke indicators if DisplayPokes==1
        // Active poke indicator triangles
        if (activePoke == 0)
        {
            display.fillTriangle(20, 55, 26, 59, 20, 63, WHITE);
            display.fillTriangle(20, 75, 26, 79, 20, 83, BLACK);
        }
        if (activePoke == 1)
        {
            display.fillTriangle(20, 75, 26, 79, 20, 83, WHITE);
            display.fillTriangle(20, 55, 26, 59, 20, 63, BLACK);
        }
    }
}

void FED3::DisplayBattery()
{
    //  Battery graphic showing bars indicating voltage levels
    if (numMotorTurns == 0)
    {
        display.fillRect(117, 2, 40, 16, WHITE);
        display.drawRect(116, 1, 42, 18, BLACK);
        display.drawRect(157, 6, 6, 8, BLACK);
    }
    // 4 bars
    if (measuredvbat > 3.85 & numMotorTurns == 0)
    {
        display.fillRect(120, 4, 7, 12, BLACK);
        display.fillRect(129, 4, 7, 12, BLACK);
        display.fillRect(138, 4, 7, 12, BLACK);
        display.fillRect(147, 4, 7, 12, BLACK);
    }

    // 3 bars
    else if (measuredvbat > 3.7 & numMotorTurns == 0)
    {
        display.fillRect(119, 3, 26, 13, WHITE);
        display.fillRect(120, 4, 7, 12, BLACK);
        display.fillRect(129, 4, 7, 12, BLACK);
        display.fillRect(138, 4, 7, 12, BLACK);
    }

    // 2 bars
    else if (measuredvbat > 3.55 & numMotorTurns == 0)
    {
        display.fillRect(119, 3, 26, 13, WHITE);
        display.fillRect(120, 4, 7, 12, BLACK);
        display.fillRect(129, 4, 7, 12, BLACK);
    }

    // 1 bar
    else if (numMotorTurns == 0)
    {
        display.fillRect(119, 3, 26, 13, WHITE);
        display.fillRect(120, 4, 7, 12, BLACK);
    }

    // display voltage
    display.setTextSize(2);
    display.setFont(&Org_01);

    display.fillRect(86, 0, 28, 12, WHITE);
    display.setCursor(87, 10);
    display.print(measuredvbat, 1);
    display.setFont(&FreeSans9pt7b);
    display.setTextSize(1);

    // display temp/humidity sensor indicator if present
    if (tempSensor == true)
    {
        display.setTextSize(1);
        display.setFont(&Org_01);
        display.setCursor(89, 18);
        display.print("TH");
        display.setFont(&FreeSans9pt7b);
        display.setTextSize(1);
    }
}

// Display "Check SD Card!" if there is a card error
void FED3::DisplaySDError()
{
    display.clearDisplay();
    DisplayText("   Check\n  SD Card!", 20, 40, false); // false = don't clear area
}

// Display text when FED is clearing a jam
void FED3::DisplayJamClear()
{
    DisplayText("Clearing jam", 6, 36);
}

// Display pellet retrieval interval
void FED3::DisplayRetrievalInt()
{
    display.fillRect(85, 22, 70, 15, WHITE);
    display.setCursor(90, 36);
    if (retInterval < 59000)
    {
        display.print(retInterval);
        display.print("ms");
    }
    display.refresh();
}

// Display left poke duration
void FED3::DisplayLeftInt()
{
    display.fillRect(85, 22, 70, 15, WHITE);
    display.setCursor(90, 36);
    if (leftInterval < 10000)
    {
        display.print(leftInterval);
        display.print("ms");
    }
    display.refresh();
}

// Display right poke duration
void FED3::DisplayRightInt()
{
    display.fillRect(85, 22, 70, 15, WHITE);
    display.setCursor(90, 36);
    if (rightInterval < 10000)
    {
        display.print(rightInterval);
        display.print("ms");
    }
    display.refresh();
}

void FED3::StartScreen()
{
    if (ClassicFED3 == false)
    {
        display.setTextSize(3);
        display.setTextColor(BLACK);
        display.clearDisplay();
        display.setCursor(15, 55);
        display.print("FED3");

        // print filename on screen
        display.setTextSize(1);
        display.setCursor(2, 138);
        display.print(filename);

        // Display FED verison number at startup
        display.setCursor(2, 120);
        display.print("v: ");
        display.print(VER);
        display.print("_");
        display.print(sessiontype.charAt(0));
        display.print(sessiontype.charAt(1));
        display.print(sessiontype.charAt(2));
        display.print(sessiontype.charAt(3));
        display.print(sessiontype.charAt(4));
        display.print(sessiontype.charAt(5));
        display.print(sessiontype.charAt(6));
        display.print(sessiontype.charAt(7));
        display.refresh();
        DisplayMouse();
    }
}

void FED3::DisplayTimedFeeding()
{
    display.setCursor(35, 65);
    display.print(timedStart);
    display.print(":00 to ");
    display.print(timedEnd);
    display.print(":00");
}

void FED3::DisplayMinPoke()
{
    display.setCursor(115, 65);
    display.print((minPokeTime / 1000.0), 1);
    display.print("s");
    display.refresh();
}

void FED3::DisplayNoProgram()
{
    display.clearDisplay();
    display.setCursor(15, 45);
    display.print("No program");
    display.setCursor(16, 45);
    display.print("No program");
    display.setCursor(15, 65);
    display.print("resetting FED3...");
    display.refresh();
    for (int i = 0; i < 5; i++)
    {
        colorWipe(strip.Color(5, 0, 0), 25); // RED
        delay(20);
        colorWipe(strip.Color(0, 0, 0), 25); // clear
        delay(40);
    }
    softReset();
}

void FED3::DisplayMouse()
{
    // Draw animated mouse...
    for (int i = -50; i < 200; i += 15)
    {
        display.fillRoundRect(i + 25, 82, 15, 10, 6, BLACK); // head
        display.fillRoundRect(i + 22, 80, 8, 5, 3, BLACK);   // ear
        display.fillRoundRect(i + 30, 84, 1, 1, 1, WHITE);   // eye
        // movement of the mouse
        if ((i / 10) % 2 == 0)
        {
            display.fillRoundRect(i, 84, 32, 17, 10, BLACK); // body
            display.drawFastHLine(i - 8, 85, 18, BLACK);     // tail
            display.drawFastHLine(i - 8, 86, 18, BLACK);
            display.drawFastHLine(i - 14, 84, 8, BLACK);
            display.drawFastHLine(i - 14, 85, 8, BLACK);
            display.fillRoundRect(i + 22, 99, 8, 4, 3, BLACK); // front foot
            display.fillRoundRect(i, 97, 8, 6, 3, BLACK);      // back foot
        }
        else
        {
            display.fillRoundRect(i + 2, 82, 30, 17, 10, BLACK); // body
            display.drawFastHLine(i - 6, 91, 18, BLACK);         // tail
            display.drawFastHLine(i - 6, 90, 18, BLACK);
            display.drawFastHLine(i - 12, 92, 8, BLACK);
            display.drawFastHLine(i - 12, 91, 8, BLACK);
            display.fillRoundRect(i + 15, 99, 8, 4, 3, BLACK); // foot
            display.fillRoundRect(i + 8, 97, 8, 6, 3, BLACK);  // back foot
        }
        display.refresh();
        delay(80);
        display.fillRect(i - 25, 73, 95, 33, WHITE);
        previousFEDmode = FEDmode;
        previousFED = FED;

        // If one poke is pushed change mode
        if (FED3Menu == true or ClassicFED3 == true)
        {
            if (digitalRead(LEFT_POKE) == LOW | digitalRead(RIGHT_POKE) == LOW)
                SelectMode();
        }

        // If both pokes are pushed edit device number
        if ((digitalRead(LEFT_POKE) == LOW) && (digitalRead(RIGHT_POKE) == LOW))
        {
            tone(BUZZER, 1000, 200);
            delay(400);
            tone(BUZZER, 1000, 500);
            delay(200);
            tone(BUZZER, 3000, 600);
            colorWipe(strip.Color(2, 2, 2), 40); // Color wipe
            colorWipe(strip.Color(0, 0, 0), 20); // OFF
            EndTime = millis();
            SetFED = true;
            SetDeviceNumber();
        }
    }
}

// Display text when FED is clearing a jam
void FED3::DisplayJammed()
{
    display.clearDisplay();
    display.fillRect(6, 20, 200, 22, WHITE); // erase the data on screen without clearing the entire screen by pasting a white box over it
    display.setCursor(6, 36);
    display.print("JAMMED...");
    display.print("PLEASE CHECK");
    display.refresh();
    ReleaseMotor();
    delay(2); // let things settle
}

/**
 * Displays text on the FED3 screen with support for multiple lines and formatting options
 *
 * @param text The text to display. Use '\n' for line breaks
 * @param x The x-coordinate position to start drawing text
 * @param y The y-coordinate position to start drawing text
 * @param clear_area Whether to clear the display area before drawing (default: true)
 * @param bold Whether to draw the text twice with offset for bold effect (default: false)
 * @param clear_width Width of the area to clear in pixels (default: 200)
 * @param clear_height Height of the area to clear in pixels (default: 22)
 *
 * Example usage:
 * ```cpp
 * // Simple single line
 * DisplayText("Hello", 10, 40);
 *
 * // Multi-line text
 * DisplayText("Line 1\nLine 2", 10, 40);
 *
 * // Bold text without clearing area
 * DisplayText("Important!", 10, 40, false, true);
 *
 * // Custom clear area
 * DisplayText("Small Area", 10, 40, true, false, 100, 30);
 * ```
 */
void FED3::DisplayText(const String &text, int x, int y, bool clear_area, bool bold, int clear_width, int clear_height)
{
    if (clear_area)
    {
        display.clearDisplay();
    }

    // Split text into lines
    int currentY = y;
    int lineHeight = 20;
    String remaining = text;

    while (remaining.length() > 0)
    {
        // Find next line break or end of string
        int lineBreak = remaining.indexOf('\n');
        String line;

        if (lineBreak >= 0)
        {
            line = remaining.substring(0, lineBreak);
            remaining = remaining.substring(lineBreak + 1);
        }
        else
        {
            line = remaining;
            remaining = "";
        }

        // Draw the line
        display.setCursor(x, currentY);
        display.print(line);

        // If bold, draw again with slight offset
        if (bold)
        {
            display.setCursor(x + 1, currentY);
            display.print(line);
        }

        currentY += lineHeight;
    }

    display.refresh();
}