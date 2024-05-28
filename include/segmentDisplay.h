#include "Adafruit_NeoPixel.h"

#define NUM_DIGITS 2
#define NUM_LEDS 4 * 7 * 4
#define LED_PIN 12
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

void changeAllLEDS(int r, int g, int b);
void illuminateNumber(int number, int r, int g, int b, int digitOffset);
void showSegmentDisplay();
void clearSegmentDisplay();
void setupSegmentDisplay(uint8_t brightness);
void setSegmentBrightness(uint8_t brightness);