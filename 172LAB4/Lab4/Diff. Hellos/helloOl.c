//Standard Includes
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
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
//Extra Includes for OLED
#include "Adafruit_GFX.h"

//# of bites to send and rec
#define NUM_SSI_DATA            3

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

float p = 3.14159;
//#define WIDTH 128
//#define HEIGHT 128


void InitConsole(void)// Same as ConfigureUART()
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
	ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2);

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

//Own function for first two checkoffs


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

/**************************************************************************/
/*! 
    @brief  Renders a simple test pattern on the LCD
*/
/**************************************************************************/
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

void loop() {
}

volatile int edgeTimes[200]; 
volatile int edgeI = 0;
volatile unsigned char started = false;
volatile int repeat = 0;

//TEXTING globals
volatile int iTicks = 0;
volatile int16_t drawX = 0;
volatile int16_t drawY = 5;
volatile unsigned char currChar;
volatile int setInd = 0;
volatile char currPress = '-';

void SysTick_Handler(void)
{
  //Will make later
}


void Edge_Handler (void) {
  //If SysTick has not already begun, clear it by writing to it to start at reload value
  GPIOIntClear(GPIO_PORTB_BASE, GPIO_PIN_7);
  if (started == false) {
    NVIC_ST_CURRENT_R = 0x00; 
    started = true;
    //repeat = 0;
  }
  //Record time of edge, then increment edgeI
  edgeTimes[edgeI] = ROM_SysTickValueGet(); 
  edgeI++;
}

int main (void) {
	uint32_t pui32DataTx[NUM_SSI_DATA];
	uint32_t pui32DataRx[NUM_SSI_DATA];
	uint32_t ui32Index;

	ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
 	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); 				//B
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);        	//pinMode(_rst, OUTPUT);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);

	InitConsole();
	ConfigureSSI();
	begin();
  UARTprintf("Turning on now\n");
  setup();

    //Set up SysTick 
  ROM_SysTickPeriodSet(RELOAD_VALUE);  //When SysTick is written to, this is written in as the reload value
  ROM_SysTickIntDisable(); //Default SysTick interrupt just reloads SysTick
  ROM_SysTickEnable();    //SysTick will always be running, it gets cleared at the start of the pulse

  int signalArr[90];
  int signalI=0;
  float gap = 0.0;
  float sigNum1 = 0.0;
  int sigNum = 0;
  int timesPressed = 0;
  ROM_SysTickIntDisable();
  //int repeat = 0;
  

    //TEXTING variables
  unsigned char timeoutBegin = false; //boolean
  char prevPress;
  char* currSet;
  char set2[] = "abc";
  char set3[] = "def";
  char set4[] = "ghi";
  char set5[] = "jkl";
  char set6[] = "mno";
  char set7[] = "pqrs";
  char set8[] = "tuv";
  char set9[] = "wxyz";
  char set0[] = " ";

  UARTprintf("Everything Loaded: should be displaying patterns.\n");
	//float p = 3.1415926;
	while(1)
	{
		ROM_SysCtlSleep();
  
    if ((started == true) && (ROM_SysTickValueGet() < (RELOAD_VALUE - 2000000))) {
      GPIOIntDisable(GPIO_PORTB_BASE, GPIO_PIN_7); 
      timesPressed++;
      
      for (int i=0; i < edgeI; i++) {
        gap = edgeTimes[i] - edgeTimes[i+1];    
        sigNum1 = gap/22550;                    
        float sigNum2 = roundf(sigNum1);
        sigNum = sigNum2;       
        
        for (int j=0; j < sigNum; j++){
          signalArr[signalI+j] = (i%2==0)? 0 : 1; //if it's edgeTime[even] - edgeTime[odd], append low values, and vice versa
        }
        signalI += sigNum;
      }

      signalI--;      

  int binIndex = 0;
  int signalIndexTwo = 0;
  
  int dValue = 0;
  int binArray[50];
    
      for(int i = 0; i < 50; i++)
        binArray[i] = -2;

      while(signalIndexTwo < 84)
      {
        if((signalArr[signalIndexTwo] - signalArr[signalIndexTwo + 1]) == -1) //Rising edge, 0 to 128
        {
          binArray[binIndex] = 1;
          signalIndexTwo = signalIndexTwo + 2;
          binIndex++;
        }
        else if(signalArr[signalIndexTwo] - signalArr[signalIndexTwo + 1] == 1) //Falling edge, 128 to 0
        {
          binArray[binIndex] = 0;
          signalIndexTwo = signalIndexTwo + 2;
          binIndex++;
        }
        else
        {
          binArray[binIndex] = -1; //-1 if no change (no edge in period)
          signalIndexTwo = signalIndexTwo + 2;
          binIndex++;
        }
      } //Above loop will fill binArray with the values 1, 0, and -1 based on rising, falling, or neither


      binIndex = 0;
      int signalB = 0;
      for(int i = 0; i < 50; i++)
      {
        if(binArray[i] == -2)
          i = 50;
        else
          signalB++; //Holds # of elements, not address at last element
      }
      
      dValue = (8 * binArray[signalB - 4]) + (4 * binArray[signalB - 3]) + (2 * binArray[signalB - 2]) + (binArray[signalB - 1]);
      UARTprintf("%d ", dValue);
      //DVALUE HAS INTEGER NUMBER OF WHAT THEY PRESSED

      if ((dValue >= 2 && dValue <= 9) || dValue == 0) { //If dValue is one of the acceptable numkeys (otherwise do nothing)
        //Record prevPress before updating currPress
        prevPress = currPress;
        currPress = (char)(((int)'0')+dValue);
        //UARTprintf("CURRPRESS: %c\n", currPress);
        //Select the correct char array for currSet
        switch (currPress) {
          case '2':
            currSet = set2;
            break;
          case '3':
            currSet = set3;
            break;
          case '4':
            currSet = set4;
            break;
          case '5':
            currSet = set5;
            break;
          case '6':
            currSet = set6;
            break;
          case '7':
            currSet = set7;
            break;
          case '8':
            currSet = set8;
            break;
          case '9':
            currSet = set9;
            break;
          case '0':
            currSet = set0;
            break;
        }
        char* death = "Death Does not Scare me";
        UARTprintf("PREVPRESS: %c\n", prevPress);
        testdrawtext(death, WHITE);
        if (prevPress == '-') { //First keypress
          setInd = 0;
          UARTprintf("Im the best\n");
          currChar = currSet[setInd];
          drawChar (drawX, drawY, currChar, WHITE, WHITE, CHARSIZE);
                UARTprintf("%c", currChar);
          //timeoutBegin = true;
        }
        
        else if (currPress == prevPress) { //If the same key was pressed again, cycle to next char in the set
          setInd++;
          
          //Wraparound setInd depending on the current set
          if ((currPress == '2' || currPress == '3' || currPress == '4' || currPress == '5' || currPress == '6' || currPress == '8') && setInd > 2)
            setInd = 0;
          if ((currPress == '7' || currPress == '9') && setInd > 3)
            setInd = 0;
          if (currPress == 0 && setInd > 0)
            setInd = 0;
          
          //Update currChar and update it to the screen in WB
          currChar = currSet[setInd];
          drawChar (drawX, drawY, currChar, WHITE, WHITE, CHARSIZE);
          UARTprintf("%c", currChar);
          //Set timeoutBegin flag to begin timing for timeout
          //timeoutBegin = true;
        }
        else { //Otherwise, a different key was pressed, so set currChar in BW, and move the draw cursor
          drawChar (drawX, drawY, currChar, BLACK, WHITE, CHARSIZE);
          UARTprintf("%c", currChar);
          drawX += CHARX;
          drawY += CHARY;
          
          //Then draw newest keypress
          setInd = 0;
          currChar = currSet[setInd];
          drawChar(drawX, drawY, currChar, WHITE, WHITE, CHARSIZE);
          UARTprintf("%c", currChar);
        } 
      }


      ROM_SysCtlDelay (ROM_SysCtlClockGet()*.14); 
      GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
      
      //Reset global variables (edgeTimes does not need to be reset, gets overwritten)
      edgeI = 0;
      started = false;
      //Reset other values
      signalI =0;
      for(int i = 0; i < 90; i++)
        signalArr[i] = 0;
      
      for(int i = 0; i < 200; i++)
        edgeTimes[i] = 0;
    }
    //UARTprintf("After If Loop \n");
  }

return 0;
   
	}
}
