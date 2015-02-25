#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.c"
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/rom.h"

const uint32_t portA = GPIO_PORTA_BASE;
const uint32_t portB = GPIO_PORTB_BASE;

//Port A pins (SSI)
const uint8_t cl = GPIO_PIN_2;
const uint8_t oc = GPIO_PIN_3;
const uint8_t si = GPIO_PIN_5;

//Port B pins (GPIO)
const uint8_t rst = GPIO_PIN_5;
const uint8_t dc = GPIO_PIN_6;

uint16_t swap1 (uint16_t a, uint16_t b) { 
	uint16_t t = a; 
	a = b; 
	b = t; 
	return t;
}


void writeCommand(uint8_t c) {

	//DC low to indicate command, then enable MCU communication
	ROM_GPIOPinWrite (portB, dc, 0x00); 
	ROM_GPIOPinWrite (portA, oc, 0x00);  

	//Imitate spiwrite
	SSIDataPut(SSI0_BASE, c);

	//Disable MCU communication
	ROM_GPIOPinWrite (portA, oc, 0x08);
}


void writeData(uint8_t c) {
    //DC low to indicate command, then enable MCU communication
  ROM_GPIOPinWrite (portB, dc, 0x40); 
  ROM_GPIOPinWrite (portA, oc, 0x00);  

  //Imitate spiwrite
  SSIDataPut(SSI0_BASE, c);

  //Disable MCU communication
  ROM_GPIOPinWrite (portA, oc, 0x08);
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
  ROM_GPIOPinWrite(portB, dc, 1);
  ROM_GPIOPinWrite(portA, oc, 0);

  //spiwrite(color >> 8);    
  //spiwrite(color);
  SSIDataPut(SSI0_BASE, color >> 8);
  SSIDataPut(SSI0_BASE,color);

  //*csport |= cspinmask;
  ROM_GPIOPinWrite(portA, oc, 1);
}

void begin(void) { //FIX THIS

    ROM_GPIOPinWrite(portA, oc, 0x00);                //digitalWrite(_cs, LOW);


    ROM_GPIOPinWrite(portB, rst, 0x20);           //digitalWrite(_rst, HIGH);
    ROM_SysCtlDelay(SysCtlClockGet()/6);
    ROM_GPIOPinWrite(portB, rst, 0x00);           //digitalWrite(_rst, LOW);
    ROM_SysCtlDelay(SysCtlClockGet()/6);
    ROM_GPIOPinWrite(portB, rst, 0x20);           //digitalWrite(_rst, HIGH);
    ROM_SysCtlDelay(SysCtlClockGet()/6);

    // Initialization Sequence
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0x12);  
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0xB1);

    writeCommand(SSD1351_CMD_DISPLAYOFF);  		// 0xAE

    writeCommand(SSD1351_CMD_CLOCKDIV);  		// 0xB3
    writeCommand(0xF1);  						// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    
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

    writeCommand(SSD1351_CMD_STARTLINE); 		// 0xA1
    if (SSD1351HEIGHT == 96) {
      writeData(96);
    } else {
      writeData(0);
    }


    writeCommand(SSD1351_CMD_DISPLAYOFFSET); 	// 0xA2
    writeData(0x0);

    writeCommand(SSD1351_CMD_SETGPIO);
    writeData(0x00);
    
    writeCommand(SSD1351_CMD_FUNCTIONSELECT);
    writeData(0x01); // internal (diode drop)
    //writeData(0x01); // external bias

//    writeCommand(SSSD1351_CMD_SETPHASELENGTH);
//    writeData(0x32);

    writeCommand(SSD1351_CMD_PRECHARGE);  		// 0xB1
    writeCommand(0x32);
 
    writeCommand(SSD1351_CMD_VCOMH);  			// 0xBE
    writeCommand(0x05);

    writeCommand(SSD1351_CMD_NORMALDISPLAY);  	// 0xA6

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
    
    writeCommand(SSD1351_CMD_DISPLAYON);		//--turn on oled panel    
}

void invert(bool v) {
   if (v) {
     writeCommand(SSD1351_CMD_INVERTDISPLAY);
   } else {
     	writeCommand(SSD1351_CMD_NORMALDISPLAY);
   }
 }



