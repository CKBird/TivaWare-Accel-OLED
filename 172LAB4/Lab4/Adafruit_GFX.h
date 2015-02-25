#ifndef _ADAFRUIT_GFX_H
#define _ADAFRUIT_GFX_H
#define swap(a, b) { int16_t t = a; a = b; b = t; }
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define SSD1351WIDTH 128
#define SSD1351HEIGHT 128  // SET THIS TO 96 FOR 1.27"!
#define HEIGHT 128
#define WIDTH 128
// Select one of these defines to set the pixel color order
#define SSD1351_COLORORDER_RGB
// #define SSD1351_COLORORDER_BGR
#if defined SSD1351_COLORORDER_RGB && defined SSD1351_COLORORDER_BGR
  #error "RGB and BGR can not both be defined for SSD1351_COLORODER."
#endif
// Timing Delays
#define SSD1351_DELAYS_HWFILL     (3)
#define SSD1351_DELAYS_HWLINE       (1)
// SSD1351 Commands
#define SSD1351_CMD_SETCOLUMN     0x15
#define SSD1351_CMD_SETROW        0x75
#define SSD1351_CMD_WRITERAM      0x5C
#define SSD1351_CMD_READRAM       0x5D
#define SSD1351_CMD_SETREMAP    0xA0
#define SSD1351_CMD_STARTLINE     0xA1
#define SSD1351_CMD_DISPLAYOFFSET   0xA2
#define SSD1351_CMD_DISPLAYALLOFF   0xA4
#define SSD1351_CMD_DISPLAYALLON    0xA5
#define SSD1351_CMD_NORMALDISPLAY   0xA6
#define SSD1351_CMD_INVERTDISPLAY   0xA7
#define SSD1351_CMD_FUNCTIONSELECT  0xAB
#define SSD1351_CMD_DISPLAYOFF    0xAE
#define SSD1351_CMD_DISPLAYON       0xAF
#define SSD1351_CMD_PRECHARGE     0xB1
#define SSD1351_CMD_DISPLAYENHANCE  0xB2
#define SSD1351_CMD_CLOCKDIV    0xB3
#define SSD1351_CMD_SETVSL    0xB4
#define SSD1351_CMD_SETGPIO     0xB5
#define SSD1351_CMD_PRECHARGE2    0xB6
#define SSD1351_CMD_SETGRAY     0xB8
#define SSD1351_CMD_USELUT    0xB9
#define SSD1351_CMD_PRECHARGELEVEL  0xBB
#define SSD1351_CMD_VCOMH     0xBE
#define SSD1351_CMD_CONTRASTABC   0xC1
#define SSD1351_CMD_CONTRASTMASTER  0xC7
#define SSD1351_CMD_MUXRATIO            0xCA
#define SSD1351_CMD_COMMANDLOCK         0xFD
#define SSD1351_CMD_HORIZSCROLL         0x96
#define SSD1351_CMD_STOPSCROLL          0x9E
#define SSD1351_CMD_STARTSCROLL         0x9F
#include <stdbool.h>

  // This MUST be defined by the subclass:
void drawPixel(int16_t x, int16_t y, uint16_t color);


// These exist only with Adafruit_GFX (no subclass overrides)
void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  //drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg),
//void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap,  int16_t w, int16_t h, uint16_t color);
void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
void setCursor(int16_t x, int16_t y);
void setTextColor(uint16_t c);
//    setTextColor(uint16_t c, uint16_t bg),
void setTextSize(uint8_t s);
void setTextWrap(bool w);
void setRotation(uint8_t r);

int16_t height(void);
int16_t width(void);

uint8_t getRotation(void);

uint16_t swap1 (uint16_t a, uint16_t b);
uint16_t Color565(uint8_t r, uint8_t g, uint8_t b);
// drawing primitives!
void drawPixel(int16_t x, int16_t y, uint16_t color);
void fillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color);
void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void fillScreen(uint16_t fillcolor);
void invert(bool v);
// commands
void begin(void);
void goTo(int x, int y);
void reset(void);
void writeData(uint8_t d);
void writeCommand(uint8_t c);
void writeData_unsafe(uint16_t d);
void setWriteDir(void);
void write8(uint8_t d);
void Adafruit_Configure(uint8_t cs, uint8_t rs, uint8_t sid, uint8_t sclk, uint8_t rst);
//void spiwrite(uint8_t);
void rawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor);
void rawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void rawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void writeChar(uint8_t c, uint16_t color);
void writeChar1(uint8_t c, uint16_t color, uint16_t bg);
void initHW (void);
#endif // _ADAFRUIT_GFX_H

