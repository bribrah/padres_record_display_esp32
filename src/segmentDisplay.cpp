#include "segmentDisplay.h"

xSemaphoreHandle segmentDisplayMutex = xSemaphoreCreateMutex();

int numberSegmentLookup[10][7] = {
    {1, 2, 3, 4, 5, 6, -1},     // 0
    {1, 6, -1, -1, -1, -1, -1}, // 1
    {0, 1, 2, 4, 5, -1, -1},    // 2
    {0, 1, 2, 5, 6, -1, -1},    // 3
    {0, 1, 3, 6, -1, -1, -1},   // 4
    {0, 2, 3, 5, 6, -1, -1},    // 5
    {0, 2, 3, 4, 5, 6, -1},     // 6
    {1, 2, 6, -1, -1, -1, -1},  // 7
    {0, 1, 2, 3, 4, 5, 6},      // 8
    {0, 1, 2, 3, 5, 6, -1},     // 9
};

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void changeAllLEDS(int r, int g, int b)
{
    xSemaphoreTake(segmentDisplayMutex, portMAX_DELAY);
    for (int i = 0; i < NUM_LEDS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(r, g, b));
    }
    xSemaphoreGive(segmentDisplayMutex);
}

void _illuminateSegment(int digit, int segment, int r, int g, int b)
{

    int start = (digit * 4 * 7) + segment * 4;
    for (int i = start; i < start + 4; i++)
    {
        pixels.setPixelColor(i, pixels.Color(r, g, b));
    }
}

void illuminateNumber(int number, int r, int g, int b, int digitOffset)
{
    xSemaphoreTake(segmentDisplayMutex, portMAX_DELAY);
    if (number >= 100)
    {
        return;
    }
    else if (number >= 10)
    {
        int secondDigit = number % 10;
        int firstDigit = number / 10;
        for (int i = 0; i < 7; i++)
        {
            if (numberSegmentLookup[firstDigit][i] != -1)
            {
                _illuminateSegment(0 + digitOffset, numberSegmentLookup[firstDigit][i], r, g, b);
            }
            if (numberSegmentLookup[secondDigit][i] != -1)
            {
                _illuminateSegment(1 + digitOffset, numberSegmentLookup[secondDigit][i], r, g, b);
            }
        }
    }
    else
    {
        for (int i = 0; i < 7; i++)
        {
            if (numberSegmentLookup[number][i] == -1)
            {
                break;
            }
            _illuminateSegment(1 + digitOffset, numberSegmentLookup[number][i], r, g, b);
        }
    }
    xSemaphoreGive(segmentDisplayMutex);
}

void showSegmentDisplay()
{
    xSemaphoreTake(segmentDisplayMutex, portMAX_DELAY);
    pixels.show();
    xSemaphoreGive(segmentDisplayMutex);
}

void clearSegmentDisplay()
{
    pixels.clear();
}

void setupSegmentDisplay(uint8_t brightness)
{
    pinMode(LED_PIN, OUTPUT);
    pixels.begin();
    pixels.setBrightness(brightness);
    pixels.clear();
    pixels.show();
}

void setSegmentBrightness(uint8_t brightness)
{
    pixels.setBrightness(brightness);
}