/**
 * Système pilotage LED avec RTOS sur STMF4
 * POL = 1 pour CLK à 1 en idle, PHA = 0 pour lecture sur front montant. de CLK
 * 
  ******************************************************************************
  * @file    Templates/Src/main.c 
  * @author  MCD Application Team + XM
  * @brief   Gestion de LED SPI type SK9822. Ex avec allumage 9 LED RGB
  *          PA7 = MOSI, PA5 = CLK
  *
  * @note    modified by ARM
  *          The modifications allow to use this file as User Code Template
  *          within the Device Family Pack.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX

#include "Driver_SPI.h"                 // ::CMSIS Driver:SPI
#include "Board_LED.h"                  // ::Board Support:LED

#include "stm32f4xx_hal.h" // Keil::Device:STM32Cube HAL:Common
#include "stm32f4xx_hal_adc_ex.h"
#include "stm32f4xx_hal_adc.h"
#include "adc_F4.h"




#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif



#ifdef RTE_CMSIS_RTOS2_RTX5
/**
  * Override default HAL_GetTick function
  */
uint32_t HAL_GetTick (void) {
  static uint32_t ticks = 0U;
         uint32_t i;

  if (osKernelGetState () == osKernelRunning) {
    return ((uint32_t)osKernelGetTickCount ());
  }

  /* If Kernel is not running wait approximately 1 ms then increment 
     and return auxiliary tick counter value */
  for (i = (SystemCoreClock >> 14U); i > 0U; i--) {
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
  }
  return ++ticks;
}

#endif

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern ARM_DRIVER_SPI Driver_SPI1;
static __IO uint32_t TimingDelay;
ADC_HandleTypeDef myADC2Handle;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

void Delay_Init(void);
void Delay(__IO uint32_t nTime);
void TimingDelay_Decrement(void);

void Init_SPI(void);
void mySPI_callback(uint32_t event);

void mySPI_Thread (void const *argument);                             // thread function
osThreadId tid_mySPI_Thread;                                          // thread id
osThreadDef (mySPI_Thread, osPriorityNormal, 1, 0);                   // thread object

void photo_res_env(void const *argument);															// thread function
osThreadId tid_photo_res_env;                                          // thread id
osThreadDef (photo_res_env, osPriorityNormal, 1, 0);                   // thread object


void photo_res_recept(void const *argument);															// thread function
osThreadId tid_photo_res_recept;                                          // thread id
osThreadDef (photo_res_recept, osPriorityNormal, 1, 2000);                   // thread object


osMailQId Id_ma_bal;																											// BAL id
osMailQDef(ma_bal,1,myADC2Handle);																			// avec myADC2Handle contenant la valeur de retour	


/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{

  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, Flash preread and Buffer caches
       - Systick timer is configured by default as source of time base, but user 
             can eventually implement his proper time base source (a general purpose 
             timer for example or other time source), keeping in mind that Time base 
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
             handled in milliseconds basis.
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 168 MHz */
  SystemClock_Config();
  SystemCoreClockUpdate();

  /* Add your application code here
     */
	//#ifdef RTE_CMSIS_RTOS2
  /* Initialize CMSIS-RTOS2 */
  osKernelInitialize ();
	
	Delay_Init();
	Init_SPI();
	LED_Initialize ();
	ADC_Initialize(&myADC2Handle,1); // intialisation de CAN sur le pin PA1
  
	/* Create thread functions that start executing, 
  Example: osThreadNew(app_main, NULL, NULL); */
	tid_mySPI_Thread = osThreadCreate (osThread(mySPI_Thread), NULL);  //création du thread qui s'occupe de l'allumage des leds
	//tid_photo_res_env = osThreadCreate (osThread(photo_res_env), NULL); // création du thread qui s'occupe de l'envoie de la valeur
  //tid_photo_res_recept = osThreadCreate (osThread(photo_res_recept), NULL); //création du thread qui s'occupe de la réception de la valeur
	Id_ma_bal = osMailCreate(osMailQ(ma_bal),NULL); //création de la Mailbox
	
	/* Start thread execution */
  osKernelStart();
	//LED_On (3);

//#endif
	osDelay(osWaitForever);
	
  /* Infinite loop */
//  while (1)
//  {
 // }
}

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initialize SPI Driver
  * @param  None
  * @retval None
  */
void Init_SPI(void){
	/* Initialize the SPI driver */
	Driver_SPI1.Initialize(mySPI_callback);
	 /* Power up the SPI peripheral */
	Driver_SPI1.PowerControl(ARM_POWER_FULL);
	/* Configure the SPI to Master, 8-bit mode @1 MBits/sec */
	Driver_SPI1.Control(ARM_SPI_MODE_MASTER | 
											ARM_SPI_CPOL1_CPHA1 | 
//										ARM_SPI_MSB_LSB | 
											ARM_SPI_SS_MASTER_UNUSED |
											ARM_SPI_DATA_BITS(8), 1000000);
	/* SS line = INACTIVE = HIGH */
	//Driver_SPI1.Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
}


/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Thread SPI Communication
  * @param  None
  * @retval None
  */
void mySPI_Thread (void const *argument){
	osEvent evt;
	unsigned int valeur;
	char tab[24]; // 4 octets de sof +4 leds (4*4= 16 octets) +4 octets eof 
	int i, nb_led;
	
	for (i=0;i<4;i++){ // sof
		tab[i] = 0;
	}
		
		// 4 LED vertes 
		for (nb_led = 0; nb_led <4;nb_led++){
			tab[4+nb_led*4]=0x00; // luminosité
			tab[5+nb_led*4]=0xff ; // couleur bleue
			tab[6+nb_led*4]=0xff; // couleur verte
			tab[7+nb_led*4]=0xff; // couleur rouge
			}
	
			
		for (i=0;i<4;i++){ // eof
		tab[20+i] = 0;
	}		


	
  while (1) {
		HAL_ADC_Start(&myADC2Handle); // start A/D conversion
		while(HAL_ADC_PollForConversion(&myADC2Handle, 15) != HAL_OK);  //Check if conversion is completed with 15 cycles
		valeur=HAL_ADC_GetValue(&myADC2Handle); // lire la valeur une fois que la conversion est fini
		HAL_ADC_Stop(&myADC2Handle); //arrêt la conversion

		if(valeur <2300){
			for (nb_led = 0; nb_led <4;nb_led++){		
			tab[4+nb_led*4]=0xff; // luminosité
			tab[5+nb_led*4]=0xff ; // couleur bleue
			tab[6+nb_led*4]=0xff; // couleur verte
			tab[7+nb_led*4]=0xff; // couleur rouge			
				}
			LED_On(1);
		}
		else{
		for (nb_led = 0; nb_led <4;nb_led++){
			tab[4+nb_led*4]=0xff; // luminosité
			tab[5+nb_led*4]=0x00 ; // couleur bleue
			tab[6+nb_led*4]=0x00; // couleur verte
			tab[7+nb_led*4]=0x00; // couleur rouge		
			}
		LED_Off(1);
  }
		Driver_SPI1.Send(tab,24);
    evt = osSignalWait(0x01, osWaitForever);	// sommeil jusqu'à  fin emission
	osDelay(500);
			

 }
}


/* Private functions ---------------------------------------------------------*/
/**
  * @brief  SPI Callback
  * @param  None
  * @retval None
  */

void mySPI_callback(uint32_t event){
	if (event & ARM_SPI_EVENT_TRANSFER_COMPLETE) {
    /* Transfer or receive is finished */
		osSignalSet(tid_mySPI_Thread, 0x01);
  }
	
}
/* Private functions ---------------------------------------------------------*/
/**
  * @brief   valeur du capteur transmission
  * @param  None
  * @retval None
  */
void photo_res_env(void const *argument){
  unsigned int valeur;
	osEvent evt;
	int *ptr;
	
	
	while(1){
		HAL_ADC_Start(&myADC2Handle); // start A/D conversion
		while(HAL_ADC_PollForConversion(&myADC2Handle, 15) != HAL_OK);  //Check if conversion is completed with 15 cycles
		valeur=HAL_ADC_GetValue(&myADC2Handle); // lire la valeur une fois que la conversion est fini
		HAL_ADC_Stop(&myADC2Handle); //arrêt la conversion
					
		
		ptr=osMailAlloc(Id_ma_bal,osWaitForever);
		*ptr=valeur;                                           // valeur à envoyer
		osMailPut(Id_ma_bal,ptr);                               //Envoi
		osDelay(1000);
		
	
 }
}
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  valeur du capteur reception
  * @param  None
  * @retval None
  */
void photo_res_recept(void const *argument){
	int *recep, valeur_recue;
	osEvent EVretour;
	char tab[24]; // 4 octets de sof +4 leds (4*4= 16 octets) +4 octets eof 
	char tab1[24]; 
	int i, nb_led;
	
	for (i=0;i<4;i++){ // sof
		tab[i] = 0;
	}
		
		// 4 LED bleues 
		for (nb_led = 0; nb_led <4;nb_led++){
			tab[4+nb_led*4]=0xff; // luminosité
			tab[5+nb_led*4]=0xff ; // couleur bleue
			tab[6+nb_led*4]=0x00; // couleur verte
			tab[7+nb_led*4]=0x00; // couleur rouge
			}
	
			
		for (i=0;i<4;i++){ // eof
		tab[20+i] = 0;
	}	
	
	
	
	while(1){
	EVretour = osMailGet(Id_ma_bal, osWaitForever);   // attente mail
	recep=EVretour.value.p;                           // on récupère le pointeur...
	valeur_recue = *recep;                            // ...et la valeur pointé
	
		osMailFree(Id_ma_bal,recep);                     // libération mémoire allouée
	
		if(valeur_recue <2300){
			for (nb_led = 0; nb_led <4;nb_led++){
			tab[4+nb_led*4]=0xff; // luminosité
			tab[5+nb_led*4]=0xff ; // couleur bleue
			}
		}
		else{
		for (nb_led = 0; nb_led <4;nb_led++){
			tab[4+nb_led*4]=0x00; // luminosité
			tab[5+nb_led*4]=0x00 ; // couleur bleue
		}
		Driver_SPI1.Send(tab,24);
    EVretour = osSignalWait(0x01, osWaitForever);	// sommeil jusqu'à  fin emission
	
		
	osDelay(500);	
	
	}	
}
}

/**
  * @brief  Initialize delay time.
  * @param  None
  * @retval None
  */
void Delay_Init(void)
{
  if (SysTick_Config(SystemCoreClock / 1000))
  { 
    /* Capture error */ 
    while (1);
  }
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{ 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}


/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}



/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
