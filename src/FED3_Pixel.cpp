#include "FED3.h"

// Turn all pixels on to a specific color
void FED3::pixelsOn(int R, int G, int B, int W)
{
    digitalWrite(MOTOR_ENABLE, HIGH); // ENABLE motor driver
    delay(2);                         // let things settle
    for (uint16_t i = 0; i < 8; i++)
    {
        strip.setPixelColor(i, R, G, B, W);
        strip.show();
    }
}

// Turn all pixels off
void FED3::pixelsOff()
{
    digitalWrite(MOTOR_ENABLE, HIGH); // ENABLE motor driver
    delay(2);                         // let things settle
    for (uint16_t i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, 0, 0, 0, 0);
        strip.show();
    }
    digitalWrite(MOTOR_ENABLE, LOW); // disable motor driver and neopixels
}

// colorWipe does a color wipe from left to right
void FED3::colorWipe(uint32_t c, uint8_t wait)
{
    digitalWrite(MOTOR_ENABLE, HIGH); // ENABLE motor driver
    delay(2);                         // let things settle
    for (uint16_t i = 0; i < 8; i++)
    {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
    }
    digitalWrite(MOTOR_ENABLE, LOW); ////disable motor driver and neopixels
    delay(2);                        // let things settle
}

// Visual tracking stimulus - left-most pixel on strip
void FED3::leftPixel(int R, int G, int B, int W)
{
    digitalWrite(MOTOR_ENABLE, HIGH);
    delay(2); // let things settle
    strip.setPixelColor(0, R, G, B, W);
    strip.show();
    //   delay(2); //let things settle
}

// Visual tracking stimulus - left-most pixel on strip
void FED3::rightPixel(int R, int G, int B, int W)
{
    digitalWrite(MOTOR_ENABLE, HIGH);
    delay(2); // let things settle
    strip.setPixelColor(7, R, G, B, W);
    strip.show();
    //   delay(2); //let things settle
}

// Visual tracking stimulus - left poke pixel
void FED3::leftPokePixel(int R, int G, int B, int W)
{
    digitalWrite(MOTOR_ENABLE, HIGH);
    delay(2); // let things settle
    strip.setPixelColor(9, R, G, B, W);
    strip.show();
    //   delay(2); //let things settle
}

// Visual tracking stimulus - right poke pixel
void FED3::rightPokePixel(int R, int G, int B, int W)
{
    digitalWrite(MOTOR_ENABLE, HIGH);
    delay(2); // let things settle
    strip.setPixelColor(8, R, G, B, W);
    strip.show();
    // delay(2); //let things settle
}

// Short helper function for blinking LEDs and BNC out port
void FED3::Blink(byte PIN, byte DELAY_MS, byte loops)
{
    for (byte i = 0; i < loops; i++)
    {
        digitalWrite(PIN, HIGH);
        delay(DELAY_MS);
        digitalWrite(PIN, LOW);
        delay(DELAY_MS);
    }
}