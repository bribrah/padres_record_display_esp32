#include "utils.h"
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define LED_MATRIX_PIN 32
#define MAX_TEXTS 10
struct ledText
{
    String text;
    int r;
    int g;
    int b;
};

class LedMatrixDisplay
{
public:
    int textLoopCounter = 0;

    LedMatrixDisplay(uint8_t pin);
    void setupMatrix(uint8_t brightness);
    void showMultiTexts(ledText *texts, int numTexts);
    void showSingleText(ledText text);
    void loopMatrix();
    void setBrightness(uint8_t brightness);
    SemaphoreHandle_t ledMatrixMutex;

private:
    Adafruit_NeoMatrix matrix;
    int currentX;
    int currentY;
    String currentText;
    int textLength;
    uint8_t matrixPin;
    ledText textArray[MAX_TEXTS];
    int numTexts;
    int currentTextIndex;
    int loopsSinceTextChange;

    void showText(ledText text);
};
