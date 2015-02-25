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
#define RELOAD_VALUE    16000000 //16,000,000

#define CHARSIZE 1
#define CHARX 6
#define CHARY 0
#define DEFAULTX 0
#define DEFAULTY 5
#define DEFAULTX1 0
#define DEFAULTY1 69

#define TOPEDGE 0
#define BOTTOMEDGE 128
#define LEFTEDGE 0
#define RIGHTEDGE 128
#define RADIUS 4
#define LEFTPADDLE 15
#define RIGHTPADDLE 113
#define PADDLELENGTH 40

float p = 3.14159;
int paddleTopL;
int paddleTopR;
int paddleBotL;
int paddleBotR;
int ballX = 64;
int ballY = 64;
uint8_t ballX2;
uint8_t ballY2;
int dx = 3;
int dy = 2;

volatile int edgeTimes[200]; 
volatile int edgeI = 0;
volatile unsigned char started = false;
volatile int repeat = 0;

//TEXTING globals
volatile int iTicks = 0;
volatile int16_t drawX = 0;
volatile int16_t drawY = 5;
volatile int16_t drawX1 = 0;
volatile int16_t drawY1 = 69;
volatile unsigned char currChar;
volatile int setInd = 0;
volatile char currPress = '-';

//THATS A PADDLIN
int myPaddleX = 15;
int myPaddleY = 44;
int hisPaddleX = 113;
int hisPaddleY = 44;


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

void resetPong(int winner) {
  //Ball goes towards whoever lost
  if (winner == 1)
    dx = 3;
  else
    dx = -3;
  
  //Erase ball and redraw in the center (for both screens)
  fillCircle( ballX, ballY, RADIUS, BLACK);
  ballX = 64;
  ballY = 64;
  ballX2 = 128 - ballX;
  ballY2 = ballY;
  ROM_UARTCharPut(UART1_BASE, 'c');
  ROM_UARTCharPut(UART1_BASE, ballX2);
  ROM_UARTCharPut(UART1_BASE, ballY2);
  fillCircle( ballX, ballY, RADIUS, WHITE);
}

void updateBallP1 (void) {
  //Erase old ball
  fillCircle( ballX, ballY, RADIUS, BLACK);
  
  //Update coordinates and draw new ball
  ballX += dx;
  ballY += dy;
  fillCircle( ballX, ballY, RADIUS, WHITE);
  
  //Flip new coordinates for P2 and send them over
  ballX2 = 128 - ballX;
  ballY2 = ballY;
  ROM_UARTCharPut(UART1_BASE, 'c');
  ROM_UARTCharPut(UART1_BASE, ballX2);
  ROM_UARTCharPut(UART1_BASE, ballY2);
  
  //Refresh paddle locations for collision checking
  paddleTopL = myPaddleY;
  paddleTopR = hisPaddleY;
  paddleBotL = myPaddleY + PADDLELENGTH;
  paddleBotR = hisPaddleY + PADDLELENGTH;
  
  //Check for collisions with...
  //Paddles
  if (((ballX <= LEFTPADDLE+RADIUS) && ((ballY >= paddleTopL) && (ballY <= paddleBotL))) || 
    ((ballX >= RIGHTPADDLE-RADIUS) && ((ballY >= paddleTopR) && (ballY <= paddleBotR)))) 
  {
    //UARTprintf("IM SUPPOSED TO BOUNCE");    
    dx *= -1;
  }
  //Top or bottom walls
  else if ( ballY <= TOPEDGE+RADIUS || ballY >= BOTTOMEDGE-RADIUS )
    dy *= -1;
  //Left or right walls
  else if ( ballX >= RIGHTEDGE-RADIUS )
    resetPong(1); //P1 has won
  else if ( ballX <= LEFTEDGE+RADIUS )
    resetPong(2); //P2 has won
}

//UART1 receives from other processor and writes it to the OLED
void UART1IntHandler(void) {
	uint32_t ui32Status;

  ui32Status = ROM_UARTIntStatus(UART1_BASE, true);
  ROM_UARTIntClear(UART1_BASE, ui32Status);
  while(ROM_UARTCharsAvail(UART1_BASE)) 
  {
    char input = ROM_UARTCharGet(UART1_BASE);
    if(input == 'y')
    {
      drawLine(hisPaddleX, hisPaddleY, (hisPaddleX), (hisPaddleY + 40), BLACK);
      hisPaddleY = ROM_UARTCharGet(UART1_BASE);
      drawLine(hisPaddleX, hisPaddleY, (hisPaddleX), (hisPaddleY + 40), RED);
      //updateBallP1();
    }
    //If a 'c' has been received, then the next 2 numbers must be an x-coordinate and a y-coordinate
    else if (input == 'c') 
    {
      //Erase old ball
      fillCircle( ballX, ballY, RADIUS, BLACK);
      
      //Then update coords and draw new ball
      ballX = ROM_UARTCharGet(UART1_BASE); //input;
      ballY = ROM_UARTCharGet(UART1_BASE); //y-coordinate always follows right after x-coordinate
      fillCircle( ballX, ballY, RADIUS, WHITE);
    }     
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

    UARTStdioConfig(0, 9600, 16000000);
}

void ConfigureUART1(void)
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); //UART Pin B
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    ROM_GPIOPinConfigure(GPIO_PB0_U1RX); //Pin MUX (In/Out)
    ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);

    ROM_UARTConfigSetExpClk(UART1_BASE, 16000000, 9600,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
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

  fillRect(0, 0, 128, 128, BLACK);
    
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
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); 				//B
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);        	//pinMode(_rst, OUTPUT);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7 );  //Configure pin as input
      
  //Configure chosen pin for interrupts
  ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_BOTH_EDGES); //Interrupt triggered on both edges (rising/falling distinguished in interrupt handler)

  //Enable interrupts (on pin, port, and master)
  GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
  ROM_IntEnable(INT_GPIOB); 
  ROM_IntMasterEnable();
  
	ConfigureUART0();
	UARTprintf("Turning on now\n");
  ConfigureUART1();
	ConfigureSSI();
	ROM_IntEnable(INT_UART1);
  ROM_UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);   

  begin();
  //setup();
  
  //Setup OLED
  lcdTestPattern();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  initHW();
  setTextSize(1);
  fillScreen(BLACK);
  //drawLine(0, 64, 128, 64, GREEN);
    
  //Set up SysTick 
  ROM_SysTickPeriodSet(RELOAD_VALUE);  //When SysTick is written to, this is written in as the reload value
  ROM_SysTickIntDisable(); //Default SysTick interrupt just reloads SysTick
  ROM_SysTickEnable();    //SysTick will always be running, it gets cleared at the start of the pulse
	
  //Pulse variables
  int signalArr[90];
  int signalI=0;
  float gap = 0.0;
  float sigNum1 = 0.0;
  int sigNum = 0;
  int timesPressed = 0;
  ROM_SysTickIntDisable();  

  //TEXTING variables
  char prevPress;
  
  drawLine(myPaddleX, myPaddleY, (myPaddleX), (myPaddleY + 40), BLUE);
  drawLine(hisPaddleX, hisPaddleY, (hisPaddleX), (hisPaddleY + 40), RED);
  updateBallP1();
  int ticks = 0;
  while(1)
	{
		//ROM_SysCtlSleep();
    /*if((ticks % 500000) == 0) 
    {
      updateBallP1();
      ticks = 0;
    }*/
    ticks++;
    if ((started == true) && (ROM_SysTickValueGet() < (RELOAD_VALUE - 2000000))) 
    {
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
        if ((dValue >= 2 && dValue <= 9) || dValue == 0) { //If dValue is one of the acceptable numkeys (otherwise do nothing)
          //Record prevPress before updating currPress
          prevPress = currPress;
          currPress = (char)(((int)'0')+dValue);
          
          if(currPress == '4') //ME UP
          {
            ROM_UARTCharPut(UART1_BASE, 'y'); //Send my move over XBee
            drawLine(myPaddleX, myPaddleY, (myPaddleX), (myPaddleY + 40), BLACK);
            myPaddleY -= 10;
            uint8_t paddleY = myPaddleY;
            ROM_UARTCharPut(UART1_BASE, paddleY);
            drawLine(myPaddleX, myPaddleY, (myPaddleX), (myPaddleY + 40), BLUE);
          }
          else if(currPress == '7') //ME DOWN
          {
            ROM_UARTCharPut(UART1_BASE, 'y'); //Send my move over XBee
            drawLine(myPaddleX, myPaddleY, (myPaddleX), (myPaddleY + 40), BLACK);
            myPaddleY += 10;
            uint8_t paddleY = myPaddleY;
            ROM_UARTCharPut(UART1_BASE, paddleY);
            drawLine(myPaddleX, myPaddleY, (myPaddleX), (myPaddleY + 40), BLUE);
          }
        } //End of receive decode
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
  }
}
