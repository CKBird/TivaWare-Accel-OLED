//LOCAL PROCESSOR IS CONNECTED TO ACCEL
//SENDS DATA TO REMOTE TO UPDATE BALL

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

#define SLAVE_ADDRESS 0x4C

float p = 3.14159;

//I2C globals (for accelerometer)
volatile int sampTicks = 0;
volatile int16_t drawX = 0;
volatile int16_t drawY = 5;
volatile int16_t drawX1 = 0;
volatile int16_t drawY1 = 69;
volatile int flag = 0;
volatile int ticks = 0;

//Variable Values
volatile uint8_t x = 0;
volatile uint8_t y = 0;
volatile uint8_t z = 0;
volatile int8_t xf = 0;
volatile int8_t yf = 0;
volatile int8_t zf = 0;

//Ball Variables
//WE WILL ASSUME THAT -32 < X < 31
//''        ''        -32 < Y < 31
//If value is < 0, ball rolls left
//If value is > 0, ball rolls right
volatile int BALLX = 64;  //X-Coord
volatile int BALLY = 64;  //Y-Coord
int ballX = 64;
int ballY = 64;
uint8_t ballX2;
uint8_t ballY2;
//Variables for determining screen, paddle, and ball boundaries
#define TOPEDGE 0
#define BOTTOMEDGE 128
#define LEFTEDGE 0
#define RIGHTEDGE 128
#define RADIUS 4
volatile int dx = 0;      //X Direction
volatile int dy = 0;      //Y Direction


void I2CMBusyLoop() {
	while(ROM_I2CMasterBusy(I2C0_BASE)) {}
}

void I2CAcc_Handler() { //KABOOOOOOOOOOOOM
	GPIOIntClear(GPIO_PORTB_BASE, GPIO_INT_PIN_7);
	
	ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
	ROM_I2CMasterDataPut(I2C0_BASE, 0x00); //Mode Register
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	I2CMBusyLoop();
	
	ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, true);
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
	I2CMBusyLoop();
  x = ROM_I2CMasterDataGet(I2C0_BASE);
	//I2CMBusyLoop();
	
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	I2CMBusyLoop();
	y = ROM_I2CMasterDataGet(I2C0_BASE);
	
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
	I2CMBusyLoop();
	z = ROM_I2CMasterDataGet(I2C0_BASE);
	
	//ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
	//ROM_I2CMasterDataPut(I2C0_BASE, 0x01); //Mode Register
	//ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	//I2CMBusyLoop();
	
	//ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, true);
	//ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
	//I2CMBusyLoop();
  //y = ROM_I2CMasterDataGet(I2C0_BASE);
	//I2CMBusyLoop();
	
	//ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
	//ROM_I2CMasterDataPut(I2C0_BASE, 0x02); //Mode Register
	//ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	//I2CMBusyLoop();
	
	//ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, true);
	//ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
	//I2CMBusyLoop();
  //z = ROM_I2CMasterDataGet(I2C0_BASE);
	//I2CMBusyLoop();
	
	flag = 1;
	ticks++;
}

void ConfigureI2C() {
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); 
  ROM_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
  ROM_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
  ROM_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
  ROM_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3); //SDA

  //Set up master and slave
  ROM_I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
  //ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
}

void updateBall (void) {
  //Erase old ball
  fillCircle(ballX, ballY, RADIUS, BLACK);
	
	dx = -(xf/5);
  dy = yf/5;
	
  //Update coordinates and draw new ball
  ballX += dx;
  ballY += dy;
  fillCircle(ballX, ballY, RADIUS, WHITE);

  IntMasterDisable();
  ROM_UARTCharPut(UART1_BASE, 'c');
  ROM_UARTCharPut(UART1_BASE, ballX);
  ROM_UARTCharPut(UART1_BASE, ballY);
  IntMasterEnable();

  //Z Coordinate doesn't matter (is up or down)
	
  if(ballY <= (TOPEDGE+RADIUS) || ballY >= (BOTTOMEDGE-RADIUS))
    dy = 0;     //Instead of bouncing off, just change y-direction to 0
  if(ballX >= (RIGHTEDGE-RADIUS) || ballX <= (LEFTEDGE+RADIUS))
    dx = 0;     //Instead reset, just change x-direction to 0
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

    UARTStdioConfig(0, 9600, 16000000);
}

void ConfigureUART1(void) //Configure UART for Tx/Rx functionality
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
  //UARTprintf(text);
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
    //UARTprintf("%d", i);
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
  UARTprintf("Start of Setup\n");
  begin();

  //UARTprintf("init\n");

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
  
  UARTprintf("Setup Complete\n");
  ROM_SysCtlDelay(SysCtlClockGet()/3); //delay(1000);
}

int main (void) 
{
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); 				
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5); //RESET FOR OLED
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
  ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7);
  ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);    
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
  ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
	
	//Configure chosen pin for interrupts
  ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE); //INT is rising or falling edge? ); 
  //Enable interrupts (on pin, port, and master)
  GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
  ROM_IntEnable(INT_GPIOB); 
  ROM_IntMasterEnable();
  
	ConfigureUART0();
  ConfigureUART1();
	UARTprintf("System Boot...\n");
  ConfigureSSI();
  ConfigureI2C();
  begin(); //Initializes the OLED for use
	//setup(); //Runs the tests
  
	lcdTestPattern();
  ROM_SysCtlDelay(SysCtlClockGet()/6); //delay(500);
  
  initHW();
  setTextSize(1);
  fillScreen(BLACK);

  //Set sampling rate for accelerometer to 64 MHz
  //Begin by sending Sample Rate Register (0x08)
	ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
	ROM_I2CMasterDataPut(I2C0_BASE, 0x08); 
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
  I2CMBusyLoop();
		
	//Then configure it for 64 MHz (001 in the lowest 3 bits)
  ROM_I2CMasterDataPut(I2C0_BASE, 0x61); //0x61 Sampling Rate
  ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	I2CMBusyLoop();
	
	ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
	ROM_I2CMasterDataPut(I2C0_BASE, 0x06); //Int Mode Register
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	I2CMBusyLoop();
	
	ROM_I2CMasterDataPut(I2C0_BASE, 0x10);
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	I2CMBusyLoop();
	
	ROM_I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
	ROM_I2CMasterDataPut(I2C0_BASE, 0x07); //Mode Register
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	I2CMBusyLoop();
	
	ROM_I2CMasterDataPut(I2C0_BASE, 0x01);
	ROM_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	I2CMBusyLoop();
	
	UARTprintf("Configuration Done\n");
	
	//ROM_IntMasterEnable();
  while(1)
	{
		//ROM_SysCtlSleep();
		/*if (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_7) != 0){
			UARTprintf("Interupt Active\n");
		}*/
			if(ticks % 50 == 0) {
				if (!ROM_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1)) 
					ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
				else
					ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
			}
		
			if(flag == 1 && (ticks % 5 == 0)) {
			uint8_t xt = x << 2;
      xt = xt >> 7; //XT holds the sign bit
      uint8_t xq = x << 3;
      xq = xq >> 3; //Shift off the top 3 bits, now contains ONLY the value, not the sign.
 
			if(xt == 1) {
				xf = xq - 1;
				xf = (~xf);
			}
			else
				xf = xq;
			
			uint8_t yt = y << 2;
      yt = yt >> 7; //XT holds the sign bit
      uint8_t yq = y << 3;
      yq = yq >> 3; //Shift off the top 3 bits, now contains ONLY the value, not the sign.
 
			if(yt == 1) {
				yf = yq - 1;
				yf = (~yf);
			}
			else
				yf = yq;
			
			uint8_t zt = z << 2;
      zt = zt >> 7; //XT holds the sign bit
      uint8_t zq = z << 3;
      zq = zq >> 3; //Shift off the top 3 bits, now contains ONLY the value, not the sign.
 
			if(zt == 1) {
				zf = zq - 1;
				zf = (~zf);
			}
			else
				zf = zq;
			
			UARTprintf("Read X Register: %d\n", x); 
			UARTprintf("X's Sign Bit: %d\n", xt);
      UARTprintf("X's Data: %d\n\n", xq);
			UARTprintf("X's Final Value: %d\n", xf);
      UARTprintf("Y's Final Value: %d\n", yf);
			UARTprintf("Z's Final Value: %d\n\n", zf); //These should be in range of -32 < x < 31

      updateBall();
			flag = 0;
		}
  }
}
