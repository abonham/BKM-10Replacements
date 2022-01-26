#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "BKM10Rduino.h"
#define FONT_PX 6
#define BG_PADDING 6
#define BG_SIZE FONT_PX + BG_PADDING
#define HALF_FONT 3

void drawText(Adafruit_SSD1306 *display, int x, int y, const char *text, bool invert, bool bg)
{
    if (bg)
    {
        int w = (strlen(text) * FONT_PX) + 6;
        display->fillRect(x, y, w, BG_SIZE, invert ? WHITE : INVERSE);
    }
    display->setCursor(x + HALF_FONT, y + HALF_FONT);
    display->setTextColor(invert ? INVERSE : WHITE);
    display->print(text);
}

void drawShiftedText(Adafruit_SSD1306 *display, int x, int y, const char *text, const char *alt, bool shifted, bool invert, bool bg)
{
    drawText(display, x, y, shifted ? text : alt, invert, bg);
}

#endif

void updateDisplay(Adafruit_SSD1306 *display, int learnIndex, int screenWidth, int screenHeight)
{

    display->clearDisplay();
    display->setCursor(8, 0);
    String s = String(learnIndex, DEC) + " of " + String(COMMANDS_SIZE, DEC);
    display->print(s);

    display->setCursor(8, FONT_PX * 3);
    display->setTextColor(WHITE);
    display->println("press key:");

    int16_t f_x, f_y;
    uint16_t f_w, f_h;
    int y = FONT_PX * 6;
    display->setTextSize(2);
    String nameStr = String(names[learnIndex]);
    display->getTextBounds(nameStr, 0, y, &f_x, &f_y, &f_w, &f_h);
    display->fillRect(0, y - 4, screenWidth, f_h + 8, WHITE);
    display->setTextColor(INVERSE);
    int x = f_w <= screenWidth ? (screenWidth - f_w) / 2 : 0;
    display->setCursor(x, y);
    display->println(names[learnIndex]);
    display->setTextColor(WHITE);
    display->setTextSize(1);
    display->display();
}

void updateLEDS(Adafruit_SSD1306 *display, LEDStatus *leds)
{
    display->clearDisplay();

    bool shifted = leds->group3LEDMask & LED_SHIFT;

    drawText(display, 0, 0, "Sh", leds->group3LEDMask & LED_SHIFT, shifted);
    drawShiftedText(display, 3 * 8, 0, "16", "Ov", shifted, leds->group3LEDMask & LED_OVERSCAN, leds->group3LEDMask & LED_OVERSCAN);
    drawShiftedText(display, 6 * 8, 0, "Sy", "Hs", shifted, leds->group3LEDMask & LED_H_SYNC, leds->group3LEDMask & LED_H_SYNC);
    drawShiftedText(display, 9 * 8, 0, "B'", "Vs", shifted, leds->group3LEDMask & LED_V_SYNC, leds->group3LEDMask & LED_V_SYNC);
    drawShiftedText(display, 12 * 8, 0, "R", "Mo", shifted, leds->group3LEDMask & LED_MONO, leds->group3LEDMask & LED_MONO);

    drawShiftedText(display, 0 * 8, 24, "G", "Ap", shifted, leds->group4LEDMask & LED_APT, leds->group4LEDMask & LED_APT);
    drawShiftedText(display, 3 * 8, 24, "B", "Cb", shifted, leds->group4LEDMask & LED_COMB, leds->group4LEDMask & LED_COMB);
    drawShiftedText(display, 6 * 8, 24, "F2", "F1", shifted, leds->group4LEDMask & LED_F1, leds->group4LEDMask & LED_F1);
    drawShiftedText(display, 9 * 8, 24, "F4", "F3", shifted, leds->group4LEDMask & LED_F3, leds->group4LEDMask & LED_F3);
    drawShiftedText(display, 12 * 8, 24, "Ad", "Sa", shifted, leds->group4LEDMask & LED_SAFE_AREA, leds->group4LEDMask & LED_SAFE_AREA);

    drawText(display, 0, 48, "Ph", leds->group2LEDMask & LED_PHASE, leds->group2LEDMask & LED_PHASE);
    drawText(display, 3 * 8, 48, "Ch", leds->group2LEDMask & LED_CHROMA, leds->group2LEDMask & LED_CHROMA);
    drawText(display, 6 * 8, 48, "Br", leds->group2LEDMask & LED_BRIGHT, leds->group2LEDMask & LED_BRIGHT);
    drawText(display, 9 * 8, 48, "Co", leds->group2LEDMask & LED_CONTRAST, leds->group2LEDMask & LED_CONTRAST);

    char *enc;
    switch (leds->selectedEncoder)
    {
    case 0:
        enc = (char *)"EnC";
        break;
    case 1:
        enc = (char *)"EnB";
        break;
    case 2:
        enc = (char *)"EnY";
        break;
    case 3:
        enc = (char *)"EnP";
        break;
    default:
        enc = (char *)"En?";
    }
    drawText(display, 12 * 8, 48, enc, false, false);

    display->display();
}
