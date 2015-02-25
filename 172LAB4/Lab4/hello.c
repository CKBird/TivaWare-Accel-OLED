#include "stdint.h"
#include "stdbool.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_nvic.h"

#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/i2c.h"

#include "utils/uartstdio.h"
#include "driverlib/ssi.h"
#include "stdio.h"
#include <string.h>
#include "stdlib.h"
#include "math.h"
#include "Adafruit_GFX.h"
#include "glcdfont.c"

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

//If SysTick needed
#define RELOAD_VALUE    16000000 //16,000,000

#define CHARSIZE 1
#define CHARX 6
#define CHARY 0
#define DEFAULTX 0
#define DEFAULTY 5
#define DEFAULTX1 0
#define DEFAULTY1 69

#define SLAVE_ADDRESS 0x3C

float p = 3.14159;

//I2C globals (for accelerometer)
volatile int8_t x = 0;
volatile int8_t y = 0;
volatile int8_t z = 0;
volatile int sampTicks = 0;
volatile int16_t drawX = 0;
volatile int16_t drawY = 5;
volatile int16_t drawX1 = 0;
volatile int16_t drawY1 = 69;

void I2CAcc_Handler() { //KABOOOOOOOOOOOOM
  //When PB7 (/INT) is asserted
  //Read XYZ from registers
  //Increment sampTicks
  //If sampTicks is a certain amount, set print flags
	writeChar('G', WHITE);
}

void I2CMBusyLoop() {
  while(ROM_I2CMasterBusy(I2C0_BASE)){}
}

void ConfigureI2C() {
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
  //ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); //Done in main()
  ROM_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
  ROM_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
  ROM_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
  ROM_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3); //SDA

  //Set up master and slave
  ROM_I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
  I2CSlaveEnable(I2C0_BASE);

  //Set up slave address
  I2CSlaveInit(I2C0_BASE, SLAVE_ADDRESS);
  ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
}

void updateDraw(void)
{
  drawX += CHARX;
  if(drawX > 118)
  {
    drawX = DEFAULTX;
    drawY += 8;
  }
  if(drawY > 54)
  {
    fillRect(0, 0, 128, 64, BLACK);
    drawX = DEFAULTX;
    drawY = DEFAULTY;
  }
}

void updateDraw1(void)
{
  drawX1 += CHARX;
  if(drawX1 > 118)
  {
    drawX1 = DEFAULTX1;
    drawY1 += 8;
  }
  if(drawY1 > 118)
  {
    fillRect(0, 65, 128, 64, BLACK);
    drawX1 = DEFAULTX1;
    drawY1 = DEFAULTY1;
  }
}

void ConfigureUART0(void)// Same as ConfigureUART()
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //UART Pin A
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    ROM_GPIOPinConfigure(GPIO_PA0_U0RX); //Pin MUX (In/Out)
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    UARTStdioConfig(0, 115200, 16000000);
}

void ConfigureSSI (void) {
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	//ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //Already done in InitConsole()

	//Configure muxs to enable special pin functions
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	GPIOPinConfigure(GPIO_PA4_SSI0RX);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);

	//Configure pins for SSI
	ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_3 | GPIO_PIN_2);

	//Set to master mode (SPI), 8 bit data
	ROM_SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);

	//Enable SSI
	ROM_SSIEnable(SSI0_BASE);

}

void fillpixelbypixel(uint16_t color) 
{
  for (uint8_t x=0; x < width(); x++) 
  	{
    for (uint8_t y=0; y < height(); y++) 
    	{
		drawPixel(x, y, color);
		}
	}
  ROM_SysCtlDelay(SysCtlClockGet()/30); //delay(100);
}


void testlines(uint16_t color) {
   fillScreen(BLACK);
   for (uint16_t x=0; x < width()-1; x+=6) {
     drawLine(0, 0, x, height()-1, color);
   }
   for (uint16_t y=0; y < height()-1; y+=6) {
     drawLine(0, 0, width()-1, y, color);
   }
   
   fillScreen(BLACK);
   for (uint16_t x=0; x < width()-1; x+=6) {
     drawLine(width()-1, 0, x, height()-1, color);
   }
   for (uint16_t y=0; y < height()-1; y+=6) {
     drawLine(width()-1, 0, 0, y, color);
   }
   
   fillScreen(BLACK);
   for (uint16_t x=0; x < width()-1; x+=6) {
     drawLine(0, height()-1, x, 0, color);
   }
   for (uint16_t y=0; y < height()-1; y+=6) {
     drawLine(0, height()-1, width()-1, y, color);
   }

   fillScreen(BLACK);
   for (uint16_t x=0; x < width()-1; x+=6) {
     drawLine(width()-1, height()-1, x, 0, color);
   }
   for (uint16_t y=0; y < height()-1; y+=6) {
     drawLine(width()-1, height()-1, 0, y, color);
   }
   
}

void testdrawtext(char *text, uint16_t color) {
  setCursor(0,0);
  setTextColor(color);
  UARTprintf(text);
  int line = 0;
  for (int i=0; i<(strlen(text)); i++) {
    if((i%12) == 0)
    {
      setTextWrap(1);
      line++;
    }
    if(line == 32)
    {
      fillScreen(BLACK);
      setCursor(0,5);
    }
    writeChar(text[i], color);
    setTextWrap(0);
  }
}

void testfastlines(uint16_t color1, uint16_t color2) {
   fillScreen(BLACK);
   for (uint16_t y=0; y < height()-1; y+=5) {
     drawFastHLine(0, y, width()-1, color1);
   }
   for (uint16_t x=0; x < width()-1; x+=5) {
     drawFastVLine(x, 0, height()-1, color2);
   }
}

void testdrawrects(uint16_t color) {
 fillScreen(BLACK);
 for (uint16_t x=0; x < height()-1; x+=6) {
   drawRect((width()-1)/2 -x/2, (height()-1)/2 -x/2 , x, x, color);
 }
}

void testfillrects(uint16_t color1, uint16_t color2) {
 fillScreen(BLACK);
 for (uint16_t x=height()-1; x > 6; x-=6) {
   fillRect((width()-1)/2 -x/2, (height()-1)/2 -x/2 , x, x, color1);
   drawRect((width()-1)/2 -x/2, (height()-1)/2 -x/2 , x, x, color2);
 }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (uint8_t x=radius; x < width()-1; x+=radius*2) {
    for (uint8_t y=radius; y < height()-1; y+=radius*2) {
      fillCircle(x, y, radius, color);
    }
  }  
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (uint8_t x=0; x < width()-1+radius; x+=radius*2) {
    for (uint8_t y=0; y < height()-1+radius; y+=radius*2) {
      drawCircle(x, y, radius, color);
    }
  }  
}

void testtriangles() {
  fillScreen(BLACK);
  int color = 0xF800;
  int t;
  int w = width()/2;
  int x = height();
  int y = 0;
  int z = width();
  for(t = 0 ; t <= 15; t+=1) {
    drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  fillScreen(BLACK);
  int color = 100;
  
  int x = 0;
  int y = 0;
  int w = width();
  int h = height();
  for(int i = 0 ; i <= 24; i++) {
    drawRoundRect(x, y, w, h, 5, color);
    x+=2;
    y+=3;
    w-=4;
    h-=6;
    color+=1100;
    UARTprintf("%d", i);
  }
}

void tftPrintTest() {
  //Print out all characters in font[]
  int line = 0;
  fillScreen(BLACK);
  for (int i=0; i<1275; i++) 
  {
    if((i%12) == 0)
    {
      setTextWrap(1);
      line++;
    }
    if(line == 32)
    {
      fillScreen(BLACK);
      setCursor(0,5);
      line = 0;
    }
    writeChar(font[i], WHITE); //writeChar(font[i]);
    setTextWrap(0);
  }
  
  //Print out Hello World!
  fillScreen(BLACK);
  setTextSize(2);
  char* hw = "Hello world!";
  testdrawtext(hw, GREEN);
}

void mediabuttons() {
 // play
  fillScreen(BLACK);
  fillRoundRect(25, 10, 78, 60, 8, WHITE);
  fillTriangle(42, 20, 42, 60, 90, 40, RED);
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  // pause
  fillRoundRect(25, 90, 78, 60, 8, WHITE);
  fillRoundRect(39, 98, 20, 45, 5, GREEN);
  fillRoundRect(69, 98, 20, 45, 5, GREEN);
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  // play color
  fillTriangle(42, 20, 42, 60, 90, 40, BLUE);
  ROM_SysCtlDelay(SysCtlClockGet()/60); //delay(50);
  // pause color
  fillRoundRect(39, 98, 20, 45, 5, RED);
  fillRoundRect(69, 98, 20, 45, 5, RED);
  // play color
  fillTriangle(42, 20, 42, 60, 90, 40, GREEN);
}

void lcdTestPattern(void)
{
  uint32_t i,j;
  goTo(0, 0);
  
  for(i=0;i<128;i++)
  {
    for(j=0;j<128;j++)
    {
      if(i<16){
        writeData(RED>>8); writeData(RED);
      }
      else if(i<32) {
        writeData(YELLOW>>8);writeData(YELLOW);
      }
      else if(i<48){writeData(GREEN>>8);writeData(GREEN);}
      else if(i<64){writeData(CYAN>>8);writeData(CYAN);}
      else if(i<80){writeData(BLUE>>8);writeData(BLUE);}
      else if(i<96){writeData(MAGENTA>>8);writeData(MAGENTA);}
      else if(i<112){writeData(BLACK>>8);writeData(BLACK);}
      else {
        writeData(WHITE>>8);      
        writeData(WHITE);
       }
    }
  }
}

void lcdTestPatternR(void)
{
  uint32_t i,j;
  goTo(0, 0);
  
  for(j=0;j<128;j++)
  {
    for(i=0;i<128;i++)
    {
      if(i<16){
        writeData(RED>>8); writeData(RED);
      }
      else if(i<32) {
        writeData(YELLOW>>8);writeData(YELLOW);
      }
      else if(i<48){writeData(GREEN>>8);writeData(GREEN);}
      else if(i<64){writeData(CYAN>>8);writeData(CYAN);}
      else if(i<80){writeData(BLUE>>8);writeData(BLUE);}
      else if(i<96){writeData(MAGENTA>>8);writeData(MAGENTA);}
      else if(i<112){writeData(BLACK>>8);writeData(BLACK);}
      else {
        writeData(WHITE>>8);      
        writeData(WHITE);
       }
    }
  }
}

void setup(void) {
  UARTprintf("hello!\n");
  begin();

  UARTprintf("init\n");

  //uint16_t time = 111;
  fillRect(0, 0, 128, 128, BLACK);
  //time = millis() - time;
  
  //UARTprintf(time, %d);
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  
  lcdTestPattern();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  
	lcdTestPatternR();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  	
  invert(true);
  ROM_SysCtlDelay(SysCtlClockGet()/30); //delay(100);
  invert(false);
  ROM_SysCtlDelay(SysCtlClockGet()/30); //delay(100);

  initHW();
  setTextSize(1);
  fillScreen(BLACK);
  testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", WHITE);
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);

  setCursor(0,5);
  setTextSize(1);
  // tft print function!
  tftPrintTest();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  
  //a single pixel
  drawPixel(width()/2, height()/2, GREEN);
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);

  // line draw test
  testlines(YELLOW);
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);    
 
  // optimized lines
  testfastlines(RED, BLUE);
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);    

  testdrawrects(GREEN);
  ROM_SysCtlDelay(SysCtlClockGet()/3); //delay(1000);

  testfillrects(YELLOW, MAGENTA);
  ROM_SysCtlDelay(SysCtlClockGet()/3); //delay(1000);

  fillScreen(BLACK);
  testfillcircles(10, BLUE);
  testdrawcircles(10, WHITE);
  ROM_SysCtlDelay(SysCtlClockGet()/3); //delay(1000);
   
  testroundrects();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  
  testtriangles();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  
  UARTprintf("done\n");
  ROM_SysCtlDelay(SysCtlClockGet()/3); //delay(1000);
}

int main (void) 
{
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); 				
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4); //RESET FOR OLED
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7 );
      
  //Configure chosen pin for interrupts
  ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_RISING_EDGE); //INT is rising or falling edge? ); 

  //Enable interrupts (on pin, port, and master)
  GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
  ROM_IntEnable(INT_GPIOB); 
  ROM_IntMasterEnable();
  
	ConfigureUART0();
	UARTprintf("Turning on now\n");
  ConfigureSSI();
  ConfigureI2C();
	
  begin(); //Initializes the OLED for use
  setup(); //Runs the tests
  lcdTestPattern();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  
  initHW();
  setTextSize(1);
  fillScreen(BLACK);

  //Set sampling rate for accelerometer to 64 MHz
  //Begin by sending Sample Rate Register (0x08)
  ROM_I2CMasterDataPut(I2C0_BASE, 0x08); 
  ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
  I2CMBusyLoop();
  //Then configure it for 64 MHz (001 in the lowest 3 bits)
  ROM_I2CMasterDataPut(I2C0_BASE, 0x01);
  ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
  I2CMBusyLoop();


  while(1)
	{
		ROM_SysCtlSleep();
    // REMEMBER THIS --- GPIOIntDisable(GPIO_PORTB_BASE, GPIO_PIN_7); 

  }
}

