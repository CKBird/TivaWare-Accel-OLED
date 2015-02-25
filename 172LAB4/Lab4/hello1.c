#include "stdint.h"
#include "stdbool.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_nvic.h"
//#include "inc/hw_ints.h"
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
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "Adafruit_GFX.h"
// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF
#define RELOAD_VALUE 16000000 //16,000,000
//Characters are of size 1 and 5x7(?)
#define CHARSIZE 1
#define CHARX 5
#define CHARY 7


#ifdef DEBEG
void
__error__(char *pcFilename, uint32_t ui32Line)
{

}
#endif

//PULSE globals
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

void SysTick_Handler (void) {
	//If ~1s (.96s) has passed, turn off timeout detection (disable ints), set currChar in BW, move draw cursor, and reset iTicks
	if (iTicks > 2) {
		ROM_SysTickIntDisable();
		drawChar (drawX, drawY, currChar, BLACK, WHITE, CHARSIZE);
    UARTprintf("%c", currChar);
		drawX += CHARX;
		drawY += CHARY;
		iTicks = 0;
		
		setInd = 0;
		currPress = '-'; //Reset to '-' to indicate waiting for key
	}
	else
		iTicks ++;
}

void Edge_Handler (void) {
	//Timeout wait ended with a pulse instead of a timeout
	//ROM_SysTickIntDisable();
	
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

void ConfigureUART(void)
{
	//Configure UART
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
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

int main (void) {
	//Set system clock to 50 Mhz
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
	
	//Enable GPIO port and set it configure it as input 
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  //Enable port
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7 );  //Configure pin as input
	//ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU); //Configure pull up resistor (Only needed for switches)
		
	//Configure chosen pin for interrupts
	ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_BOTH_EDGES); //Interrupt triggered on both edges (rising/falling distinguished in interrupt handler)

	//Enable interrupts (on pin, port, and master)
	GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
	ROM_IntEnable(INT_GPIOB); 
	ROM_IntMasterEnable();
	
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
 	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); 				//B
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);        	//pinMode(_rst, OUTPUT);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
	
	ConfigureUART();
	ConfigureSSI();

	UARTprintf("System Reset\n");
	begin();
  fillScreen(BLACK);
	//Set up SysTick 
	ROM_SysTickPeriodSet(RELOAD_VALUE);  //When SysTick is written to, this is written in as the reload value
	ROM_SysTickIntDisable(); //SysTick_Handler only runs while waiting for a timeout
	ROM_SysTickEnable();    //SysTick will always be running, it gets cleared at the start of the pulse

	int signalArr[90];
	int signalI=0;
	float gap = 0.0;
	float sigNum1 = 0.0;
	int sigNum = 0;
	int timesPressed = 0;
	//ROM_SysTickIntDisable();
	//int repeat = 0;
	
  UARTprintf("I'm on line 170!\n");

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
	
	while(1){
		//ROM_SysCtlSleep();
    	//UARTprintf("I'm on line 188!\n");	
		if ((started == true) && (ROM_SysTickValueGet() < (RELOAD_VALUE - 2000000))) {//If the timer has already started and has counted down past 40ms (2M ticks)
			//repeat = 1;
			//Block all signals while decoding
			GPIOIntDisable(GPIO_PORTB_BASE, GPIO_PIN_7); 
			timesPressed++;
			//We know pulses start with a falling edge, so from edgeTime[0] (falling edge) to edgeTime [1] (rising edge), the signal must be low
			//And from edgeTime[1](rising) to edgeTime[2](falling), the signal must be high, so (edgeTime[odd] - edgeTime[even]) are high values and vice versa
			
			//Calculate the number of ticks in each gap by subtracting
			//Divide that by 11,275 ticks (.2255 ms) to get the number of quarters in each gap
			//We only care about the values of two quarters in each period (which we already know must be low/high for the gap)
			//(Number ticks in gap/11,725)/2 = Number of high/low values to add to signalArr for this gap
			for (int i=0; i < edgeI; i++) {
				gap = edgeTimes[i] - edgeTimes[i+1]; 		//Stores distance between first and second pulse
				sigNum1 = gap/22550; 										//Number of values in gap = (Number ticks in gap/11,725)/2
				float sigNum2 = roundf(sigNum1);
				sigNum = sigNum2;				
				

				for (int j=0; j < sigNum; j++){
					signalArr[signalI+j] = (i%2==0)? 0 : 1; //if it's edgeTime[even] - edgeTime[odd], append low values, and vice versa
				}
				signalI += sigNum;
			}
			signalI--;
      UARTprintf("\n\n");
			int binIndex = 0;
			int signalIndexTwo = 0;
			int dValue = 0;
			int binArray[50];	
			for(int i = 0; i < 50; i++)
				binArray[i] = -2;

      UARTprintf("I'm on line 221!\n");
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
			binIndex = 0;
			for(int i = 0; i < signalB; i++)
        UARTprintf("%d ", binArray[i]);

			dValue = (8 * binArray[signalB - 4]) + (4 * binArray[signalB - 3]) + (2 * binArray[signalB - 2]) + (binArray[signalB - 1]);
			UARTprintf("\nDValue on Line 256: %d\n", dValue);
			//BEGIN TEXTING ------------------------------------------------------------------------------------------------------------------------
			if ((dValue >= 2 && dValue <= 9) || dValue == 0) { //If dValue is one of the acceptable numkeys (otherwise do nothing)
				//Record prevPress before updating currPress
				prevPress = currPress;
				currPress = (char)(((int)'0')+dValue);
				UARTprintf("I'm on line 262!\n");
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
				
				if (prevPress == '-') { //First keypress
					setInd = 0;
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
			//END TEXTING ---------------------------------------------------------------------------------------------------------------------------
	     UARTprintf("I'm on line 334!\n");
			//Delay past redundancy pulses then re-enable interrupts
			ROM_SysCtlDelay (ROM_SysCtlClockGet()*.14); 
		UARTprintf("\nSIGNALI: %d \n", signalI);			

      	//for(int i = 0; i < edgeI; i++)
        //	UARTprintf(" %d ", edgeTimes[i]);

        for(int i = 0; i < signalI; i++)
        	UARTprintf(" %d ", signalArr[i]);

			//Reset global variables (edgeTimes does not need to be reset, gets overwritten)
			edgeI = 0;
			started = false;
			//Reset other values
			signalI =0;
			for(int i = 0; i < 90; i++)
				signalArr[i] = 0;
			for(int i = 0; i < 200; i++)
				edgeTimes[i] = 0;
			
			//If we need to time for a timeout, reenable SysTick ints, reload SysTick, and reset flag (request processed)
			if (timeoutBegin == true){ 
				ROM_SysTickIntEnable();
				NVIC_ST_CURRENT_R = 0x00;
				iTicks = 0; //Redundancy to make sure iTicks is 0
				timeoutBegin = false;
			}
			
			//Everything reset, ready for next pulse
			GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
		}
	}

return 0;
}
