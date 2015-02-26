#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/rom.h"
#include "glcdfont.c"
#include "math.h" 
#include "Adafruit_GFX.h"

int16_t _width;
int16_t _height; // Display w/h as modified by current rotation
int16_t cursor_x; 
int16_t cursor_y;
uint16_t  textcolor, textbgcolor;
uint8_t textsize, rotation;
bool wrap; // If set, 'wrap' text at right edge of display

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

const uint32_t portA = GPIO_PORTA_BASE;
const uint32_t portB = GPIO_PORTB_BASE;

//Port A pins (SSI)
const uint8_t cl = GPIO_PIN_2;
const uint8_t oc = GPIO_PIN_3;
const uint8_t si = GPIO_PIN_5;

//Port B pins (GPIO)
const uint8_t rst = GPIO_PIN_3; //GPIO_PIN_5;
const uint8_t dc = GPIO_PIN_6;


uint16_t swap1 (uint16_t a, uint16_t b) { 
  uint16_t t = a; 
  a = b; 
  b = t; 
  return t;
}


// Draw a circle outline
void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawPixel(x0  , y0+r, color);
  drawPixel(x0  , y0-r, color);
  drawPixel(x0+r, y0  , color);
  drawPixel(x0-r, y0  , color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 + x, y0 - y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 - y, y0 - x, color);
  }
}

void drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    } 
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}

void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

// Bresenham's algorithm - thx wikpedia
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// Draw a rectangle
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y+h-1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x+w-1, y, h, color);
}

/*void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x, y+h-1, color);
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  // Update in subclasses if desired!
  drawLine(x, y, x+w-1, y, color);
}
*/
/*void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  // Update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++) {
    drawFastVLine(i, y, h, color);
  }
}*/

//void fillScreen(uint16_t color) {
  //fillRect(0, 0, _width, _height, color);
//}

// Draw a rounded rectangle
void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
  // smarter version
  drawFastHLine(x+r  , y    , w-2*r, color); // Top
  drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x    , y+r  , h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// Fill a rounded rectangle
void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
  // smarter version
  fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// Draw a triangle
void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void fillTriangle ( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    drawFastHLine(a, y, b-a+1, color);
  }
}


//Draw XBitMap Files (*.xbm), exported from GIMP,
//Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//C Array can be directly used with this function
/*void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
  
  int16_t i, j, byteWidth = (w + 7) / 8;
  
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (1 << (i % 8))) {
        drawPixel(x+i, y+j, color);
      }
    }
  }
}*/

// Draw a character
void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {

  if((x >= _width)            || // Clip right
     (y >= _height)           || // Clip bottom
     ((x + 6 * size - 1) < 0) || // Clip left
     ((y + 8 * size - 1) < 0))   // Clip top
    return;

  for (int8_t i=0; i<6; i++ ) {
    uint8_t line;
    if (i == 5)
		{ 
      line = 0x0;
    }
    else 
      line = font[(c*5)+i];
    for (int8_t j = 0; j<8; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, color);
        else {  // big size
          fillRect(x+(i*size), y+(j*size), size, size, color);
        } 
      } else if (bg != color) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, bg);
        else {  // big size
          fillRect(x+i*size, y+j*size, size, size, bg);
        }
      }
      line >>= 1;
    }
  }
}

void setCursor(int16_t x, int16_t y) {
  cursor_x = x;
  cursor_y = y;
}

void setTextSize(uint8_t s) {
  textsize = (s > 0) ? s : 1;
}

void setTextColor(uint16_t c) {
  // For 'transparent' background, we'll set the bg 
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}

/*void setTextColor(uint16_t c, uint16_t b) {
  textcolor   = c;
  textbgcolor = b; 
}*/

void setTextWrap(bool w) {
  wrap = w;
}

uint8_t getRotation(void){
  return rotation;
}

void initHW()
{
  _width = WIDTH;
  _height = HEIGHT;
}

void setRotation(uint8_t x) {
  rotation = (x & 3);
  switch(rotation) {
   case 0:
   case 2:
    _width  = WIDTH;
    _height = HEIGHT;
    break;
   case 1:
   case 3:
    _width  = HEIGHT;
    _height = WIDTH;
    break;
  }
}

// Return the size of the display (per current rotation)
int16_t width(void){
  return _width;
}
 
int16_t height(void){
  return _height;
}

//void invertDisplay(boolean i) {
  // Do nothing, must be subclassed if supported
//}


///BETWEEN BOTH MERGED FILES

void writeCommand(uint8_t c) {
  while(SSIBusy(SSI0_BASE))
  {

  }
  //DC low to indicate command, then enable MCU communication
  ROM_GPIOPinWrite (portB, dc, 0x00); 
  //ROM_GPIOPinWrite (portA, oc, 0x00);  

  //Imitate spiwrite
  SSIDataPut(SSI0_BASE, c);

  //Disable MCU communication
  //ROM_GPIOPinWrite (portA, oc, 0x08);
  while(SSIBusy(SSI0_BASE))
  {
    
  }
}


void writeData(uint8_t c) {
    //DC low to indicate command, then enable MCU communication
  ROM_GPIOPinWrite (portB, dc, 0x40); 
  //ROM_GPIOPinWrite (portA, oc, 0x00);  

  //Imitate spiwrite
  SSIDataPut(SSI0_BASE, c);

  //Disable MCU communication
  //ROM_GPIOPinWrite (portA, oc, 0x08);
} 

/***********************************/

void goTo(int x, int y) {
  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT)) return;
  
  // set x and y coordinate
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(SSD1351WIDTH-1);

  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(SSD1351HEIGHT-1);

  writeCommand(SSD1351_CMD_WRITERAM);  
}

uint16_t Color565(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c;
  c = r >> 3;
  c <<= 6;
  c |= g >> 2;
  c <<= 5;
  c |= b >> 3;

  return c;
}

// Draw a filled rectangle with no rotation.
void rawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor) {
  // Bounds check
  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    return;

  // Y bounds check
  if (y+h > SSD1351HEIGHT)
  {
    h = SSD1351HEIGHT - y - 1;
  }

  // X bounds check
  if (x+w > SSD1351WIDTH)
  {
    w = SSD1351WIDTH - x - 1;
  }
  
  // set location
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(x+w-1);
  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(y+h-1);
  // fill!
  writeCommand(SSD1351_CMD_WRITERAM);  

  for (uint16_t i=0; i < w*h; i++) {
    writeData(fillcolor >> 8);
    writeData(fillcolor);
  }
}

/**************************************************************************/
/*!
    @brief  Draws a filled rectangle using HW acceleration
*/
/**************************************************************************/
void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fillcolor) { //Remove from GFX
  // Transform x and y based on current rotation.
  switch (getRotation()) {
  case 0:  // No rotation
    rawFillRect(x, y, w, h, fillcolor);
    break;
  case 1:  // Rotated 90 degrees clockwise.
    swap1(x, y);
    x = WIDTH - x - h;
    rawFillRect(x, y, h, w, fillcolor);
    break;
  case 2:  // Rotated 180 degrees clockwise.
    x = WIDTH - x - w;
    y = HEIGHT - y - h;
    rawFillRect(x, y, w, h, fillcolor);
    break;
  case 3:  // Rotated 270 degrees clockwise.
    swap1(x, y);
    y = HEIGHT - y - w;
    rawFillRect(x, y, h, w, fillcolor);
    break;
  }
}

void fillScreen(uint16_t fillcolor) {
  fillRect(0, 0, SSD1351WIDTH, SSD1351HEIGHT, fillcolor);
}

// Draw a horizontal line ignoring any screen rotation.
void rawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  // Bounds check
  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
    return;

  // X bounds check
  if (x+w > SSD1351WIDTH)
  {
    w = SSD1351WIDTH - x - 1;
  }

  if (w < 0) return;

  // set location
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(x+w-1);
  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(y);
  // fill!
  writeCommand(SSD1351_CMD_WRITERAM);  

  for (uint16_t i=0; i < w; i++) {
    writeData(color >> 8);
    writeData(color);
  }
}

// Draw a vertical line ignoring any screen rotation.
void rawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  // Bounds check
  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT))
  return;

  // X bounds check
  if (y+h > SSD1351HEIGHT)
  {
    h = SSD1351HEIGHT - y - 1;
  }

  if (h < 0) return;

  // set location
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(x);
  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(y+h-1);
  // fill!
  writeCommand(SSD1351_CMD_WRITERAM);  

  for (uint16_t i=0; i < h; i++) {
    writeData(color >> 8);
    writeData(color);
  }
}

void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  // Transform x and y based on current rotation.
  switch (getRotation()) {
  case 0:  // No rotation
    rawFastVLine(x, y, h, color);
    break;
  case 1:  // Rotated 90 degrees clockwise.
    swap1(x, y);
    x = WIDTH - x - h;
    rawFastHLine(x, y, h, color);
    break;
  case 2:  // Rotated 180 degrees clockwise.
    x = WIDTH - x - 1;
    y = HEIGHT - y - h;
    rawFastVLine(x, y, h, color);
    break;
  case 3:  // Rotated 270 degrees clockwise.
    swap1(x, y);
    y = HEIGHT - y - 1;
    rawFastHLine(x, y, h, color);
    break;
  }
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  // Transform x and y based on current rotation.
  switch (getRotation()) {
  case 0:  // No rotation.
    rawFastHLine(x, y, w, color);
    break;
  case 1:  // Rotated 90 degrees clockwise.
    swap1(x, y);
    x = WIDTH - x - 1;
    rawFastVLine(x, y, w, color);
    break;
  case 2:  // Rotated 180 degrees clockwise.
    x = WIDTH - x - w;
    y = HEIGHT - y - 1;
    rawFastHLine(x, y, w, color);
    break;
  case 3:  // Rotated 270 degrees clockwise.
    swap1(x, y);
    y = HEIGHT - y - w;
    rawFastVLine(x, y, w, color);
    break;
  }
}

void drawPixel(int16_t x, int16_t y, uint16_t color)
{
  // Transform x and y based on current rotation.
  switch (getRotation()) {
  // Case 0: No rotation
  case 1:  // Rotated 90 degrees clockwise.
    swap1(x, y);
    x = WIDTH - x - 1;
    break;
  case 2:  // Rotated 180 degrees clockwise.
    x = WIDTH - x - 1;
    y = HEIGHT - y - 1;
    break;
  case 3:  // Rotated 270 degrees clockwise.
    swap1(x, y);
    y = HEIGHT - y - 1;
    break;
  }

  // Bounds check.
  if ((x >= SSD1351WIDTH) || (y >= SSD1351HEIGHT)) return;
  if ((x < 0) || (y < 0)) return;

  goTo(x, y);
  
  // setup for data
  //*rsport |= rspinmask;
  //*csport &= ~ cspinmask; //FIx THIS
  //ROM_GPIOPinWrite(portB, dc, 1);
  //ROM_GPIOPinWrite(portA, oc, 0);

  writeData(color >> 8);    
  writeData(color);
  //SSIDataPut(SSI0_BASE, color >> 8);
  //SSIDataPut(SSI0_BASE,color);

  //*csport |= cspinmask;
  //ROM_GPIOPinWrite(portA, oc, 1);
}

void begin(void) { //FIX THIS

  //  ROM_GPIOPinWrite(portA, oc, 0x00);                //digitalWrite(_cs, LOW);


    ROM_GPIOPinWrite(portB, rst, rst/*0x20*/);           //digitalWrite(_rst, HIGH);
    ROM_SysCtlDelay(SysCtlClockGet()/6);
    ROM_GPIOPinWrite(portB, rst, 0x00);           //digitalWrite(_rst, LOW);
    ROM_SysCtlDelay(SysCtlClockGet()/6);
    ROM_GPIOPinWrite(portB, rst, rst/*0x20*/);           //digitalWrite(_rst, HIGH);
    ROM_SysCtlDelay(SysCtlClockGet()/6);

    // Initialization Sequence
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0x12);  
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0xB1);

    writeCommand(SSD1351_CMD_DISPLAYOFF);     // 0xAE

    writeCommand(SSD1351_CMD_CLOCKDIV);     // 0xB3
    writeCommand(0xF1);             // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    
    writeCommand(SSD1351_CMD_MUXRATIO);
    writeData(127);
    
    writeCommand(SSD1351_CMD_SETREMAP);
    writeData(0x74);
  
    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData(0x00);
    writeData(0x7F);
    writeCommand(SSD1351_CMD_SETROW);
    writeData(0x00);
    writeData(0x7F);

    writeCommand(SSD1351_CMD_STARTLINE);    // 0xA1
    if (SSD1351HEIGHT == 96) {
      writeData(96);
    } else {
      writeData(0);
    }


    writeCommand(SSD1351_CMD_DISPLAYOFFSET);  // 0xA2
    writeData(0x0);

    writeCommand(SSD1351_CMD_SETGPIO);
    writeData(0x00);
    
    writeCommand(SSD1351_CMD_FUNCTIONSELECT);
    writeData(0x01); // internal (diode drop)
    //writeData(0x01); // external bias

//    writeCommand(SSSD1351_CMD_SETPHASELENGTH);
//    writeData(0x32);

    writeCommand(SSD1351_CMD_PRECHARGE);      // 0xB1
    writeCommand(0x32);
 
    writeCommand(SSD1351_CMD_VCOMH);        // 0xBE
    writeCommand(0x05);

    writeCommand(SSD1351_CMD_NORMALDISPLAY);    // 0xA6

    writeCommand(SSD1351_CMD_CONTRASTABC);
    writeData(0xC8);
    writeData(0x80);
    writeData(0xC8);

    writeCommand(SSD1351_CMD_CONTRASTMASTER);
    writeData(0x0F);

    writeCommand(SSD1351_CMD_SETVSL );
    writeData(0xA0);
    writeData(0xB5);
    writeData(0x55);
    
    writeCommand(SSD1351_CMD_PRECHARGE2);
    writeData(0x01);
    
    writeCommand(SSD1351_CMD_DISPLAYON);    //--turn on oled panel    
}

void invert(bool v) {
   if (v) {
     writeCommand(SSD1351_CMD_INVERTDISPLAY);
   } else {
      writeCommand(SSD1351_CMD_NORMALDISPLAY);
   }
 }


void writeChar(uint8_t c, uint16_t color) {
  if (c == '\n') {
    cursor_y += textsize*8;
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else {
    drawChar(cursor_x, cursor_y, c, color, color, textsize);
    cursor_x += textsize*6;
    if (wrap && (cursor_x > (_width - textsize*6))) {
      cursor_y += textsize*8;
      cursor_x = 0;
    }
  }
}

void writeChar1(uint8_t c, uint16_t color, uint16_t bg) {
  if (c == '\n') {
    cursor_y += textsize*8;
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else {
    drawChar(cursor_x, cursor_y, c, color, bg, textsize);
    cursor_x += textsize*6;
    if (wrap && (cursor_x > (_width - textsize*6))) {
      cursor_y += textsize*8;
      cursor_x = 0;
    }
  }
}

