// Utilisation Event UART en emission-reception

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "Driver_SPI.h"
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "GLCD_Config.h"                // Keil.MCB1700::Board Support:Graphic LCD
#include "LPC17xx.h"                    // Device header
#include "cmsis_os.h"                   // CMSIS RTOS header file

extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;
extern ARM_DRIVER_SPI Driver_SPI2;

void mySPI_Thread (void const *argument);                             // thread function
osThreadId tid_mySPI_Thread;                                          // thread id
osThreadDef (mySPI_Thread, osPriorityNormal, 1, 0);                   // thread object



//fonction de CB lancee si Event T ou R
void mySPI_callback(uint32_t event)
{
	switch (event) {
		
		
		case ARM_SPI_EVENT_TRANSFER_COMPLETE  : 	 osSignalSet(tid_mySPI_Thread, 0x01);
																							break;
		
		default : break;
	}
}

void Init_SPI(void){
	Driver_SPI2.Initialize(mySPI_callback);
	Driver_SPI2.PowerControl(ARM_POWER_FULL);
	Driver_SPI2.Control(ARM_SPI_MODE_MASTER | 
											ARM_SPI_CPOL1_CPHA1 | 
//											ARM_SPI_MSB_LSB | 
											ARM_SPI_SS_MASTER_UNUSED |
											ARM_SPI_DATA_BITS(8), 1000000);
	Driver_SPI2.Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
}

int main (void){
	
	osKernelInitialize ();                    // initialize CMSIS-RTOS
	
	// initialize peripherals here
	
	Init_SPI();
	NVIC_SetPriority(SPI_IRQn,2);
	
	GLCD_Initialize();
	GLCD_ClearScreen();
	GLCD_SetFont(&GLCD_Font_16x24);
	GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
//	GLCD_DrawRectangle(x,100,5,5);
	
	tid_mySPI_Thread = osThreadCreate (osThread(mySPI_Thread), NULL);
	
	
	osKernelStart ();                         // start thread execution 
	
	osDelay(osWaitForever);
	
}

void mySPI_Thread (void const *argument) {
	osEvent evt;
	char tab[22+16];
	int i, nb_led;
	
	for (i=0;i<4;i++){
		tab[i] = 0;
	}
	
	// 4 LED bleues
		for (nb_led = 0; nb_led <4;nb_led++){
			tab[4+nb_led*4]=0xff;
			tab[5+nb_led*4]=0xff;
			tab[6+nb_led*4]=0x00;
			tab[7+nb_led*4]=0x00;
			}

		// 4 LED rouges
		for (nb_led = 0; nb_led <4;nb_led++){
			tab[20+nb_led*4]=0xff;
			tab[21+nb_led*4]=0x00;
			tab[22+nb_led*4]=0x00;
			tab[23+nb_led*4]=0xff;
			}
	
		// end
		tab[36] = 0;
		tab[37] = 0;
		
	
  while (1) {
		
		Driver_SPI2.Send(tab,38);
    evt = osSignalWait(0x01, osWaitForever);	// sommeil fin emission
		
		osDelay(1000);
			
			
  }
}



