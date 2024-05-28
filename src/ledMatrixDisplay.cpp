#include "ledMatrixDisplay.h"

LedMatrixDisplay::LedMatrixDisplay(uint8_t pin)
    : matrix(32, 8, pin,
             NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
             NEO_GRB + NEO_KHZ800)
{
    pinMode(pin, OUTPUT);
    matrixPin = pin;
    currentX = 0;
    currentY = 0;
    currentText = "";
    textLength = 0;
    numTexts = 0;
    currentTextIndex = 0;
    loopsSinceTextChange = 0;
    ledMatrixMutex = xSemaphoreCreateMutex();
}

void LedMatrixDisplay::setupMatrix(uint8_t brightness)
{
    matrix.begin();
    matrix.setTextWrap(false);
    matrix.setBrightness(brightness);
    matrix.setTextColor(matrix.Color(255, 255, 0));
}

void LedMatrixDisplay::showText(ledText text)
{
    if (text.text == currentText)
    {
        return;
    }
    textLength = text.text.length() * 6;
    currentX = matrix.width();
    currentY = 0;
    currentText = text.text;

    matrix.fillScreen(0);
    matrix.setTextColor(matrix.Color(text.r, text.g, text.b));
    matrix.setCursor(currentX, currentY);
    matrix.print(text.text);
    matrix.show();
}

void LedMatrixDisplay::showMultiTexts(ledText *texts, int numTexts)
{
    xSemaphoreTake(ledMatrixMutex, portMAX_DELAY);
    for (int i = 0; i < numTexts; i++)
    {
        textArray[i] = texts[i];
    }

    if (numTexts != this->numTexts)
    {
        currentTextIndex = -1;
    }
    this->numTexts = numTexts;
    loopsSinceTextChange = 0;
    xSemaphoreGive(ledMatrixMutex);
}

void LedMatrixDisplay::showSingleText(ledText text)
{
    xSemaphoreTake(ledMatrixMutex, portMAX_DELAY);
    numTexts = 1;
    textArray[0] = text;
    currentTextIndex = -1 ;
    xSemaphoreGive(ledMatrixMutex);
}

void LedMatrixDisplay::loopMatrix()
{
    if (numTexts == 0)
    {
        return;
    }
    xSemaphoreTake(ledMatrixMutex, portMAX_DELAY);

    matrix.fillScreen(0);
    matrix.setCursor(currentX, currentY);
    matrix.print(currentText);
    matrix.show();
    currentX--;
    if (currentX < -textLength)
    {
        currentX = matrix.width();
        currentTextIndex++;
        if (currentTextIndex >= numTexts)
        {
            loopsSinceTextChange++;
            currentTextIndex = 0;
        }
        showText(textArray[currentTextIndex]);
    }
    xSemaphoreGive(ledMatrixMutex);
}

void LedMatrixDisplay::setBrightness(uint8_t brightness)
{
    matrix.setBrightness(brightness);
}