


#include "main.h"

extern "C" COMP_HandleTypeDef hcomp2;

extern "C" TIM_HandleTypeDef htim1; // PWM Drive lines.
extern "C" TIM_HandleTypeDef htim3; // DMA Signal.

extern "C" TIM_HandleTypeDef htim7; // General timer for processing.

extern "C" TIM_HandleTypeDef htim6; // eRPM.
extern "C" TIM_HandleTypeDef htim16; // Startup and commutation.
extern "C" TIM_HandleTypeDef htim17; // Bumps motor at startup.
extern "C" DMA_HandleTypeDef hdma_tim3_ch1;
extern "C" DMA_HandleTypeDef hdma_tim3_ch2;

extern "C" DMA_HandleTypeDef hdma_tim1_ch3;
extern "C" DMA_HandleTypeDef hdma_tim1_ch2;
extern "C" DMA_HandleTypeDef hdma_tim1_ch1;

volatile uint16_t dmaSignal[32] = {0};
volatile uint16_t dshotBits[16] = {0};

volatile uint16_t dmaTelemetry[22] = {80, 80, 0, 0, 80, 0, 0, 80, 80, 0, 80, 0, 0, 0, 80, 0, 80, 0, 80, 0, 0, 0};
volatile uint16_t dmaTelemetry2[22] = {80, 80, 0, 0, 80, 0, 0, 80, 80, 0, 80, 0, 0, 0, 80, 0, 80, 0, 80, 0, 0, 0};
volatile uint8_t bufferReady[2] = {0,0};
volatile uint8_t befferSent = 0;
volatile uint16_t fcThrottle = 0;
volatile uint16_t throttle = 0;
volatile uint8_t allowCommutation = 0;
volatile uint16_t beeping = 0;

volatile uint32_t counterOC4 = 0;
volatile uint32_t counterDMA1 = 0;
volatile uint32_t counterIC2 = 0;

volatile uint16_t dmaFrequency[2] = {0};
volatile uint32_t counterCompTrigger = 0;
volatile uint16_t blanking = 2000;
volatile uint16_t startupDelay = 1000;
volatile uint16_t debugCommutation = 0;
volatile uint16_t debugBump = 0;
volatile uint32_t spinningFast = 0;

volatile uint8_t commutationStep = 0;
volatile uint8_t powerStepCurrent = 0;
volatile uint8_t reverse = 0;
volatile uint8_t rising[2][6] = { {1,0,1,0,1,0}, {0,1,0,1,0,1}};
volatile uint16_t eRPMus = 0;
uint8_t telemetry = 0;
volatile uint8_t masterDirection = 0;

volatile uint32_t noSignalCouter = 0;
volatile uint32_t debug1 = 0;
volatile uint32_t debugSettings = 0;



const void Commutate(void);
#define PWM_A_DIER_H TIM_DIER_CC3DE
#define PWM_B_DIER_H TIM_DIER_CC2DE
#define PWM_C_DIER_H TIM_DIER_CC1DE


#define PWM_A_CCER_H TIM_CCER_CC3E
#define PWM_B_CCER_H TIM_CCER_CC2E
#define PWM_C_CCER_H TIM_CCER_CC1E

#define PWM_A_CCER_LN TIM_CCER_CC3NE
#define PWM_B_CCER_LN TIM_CCER_CC2NE
#define PWM_C_CCER_LN TIM_CCER_CC1NE

#define PWM_A_CCER_L TIM_CCER_CC3E
#define PWM_B_CCER_L TIM_CCER_CC2E
#define PWM_C_CCER_L TIM_CCER_CC1E

#define CCRA  TIM1->CCR3
#define CCRB  TIM1->CCR2
#define CCRC  TIM1->CCR1

#define DIER_OFF ~(PWM_A_DIER_H | PWM_B_DIER_H | PWM_C_DIER_H)
#define CCER_OFF ~(PWM_A_CCER_L | PWM_B_CCER_L | PWM_C_CCER_L | PWM_A_CCER_LN |PWM_B_CCER_LN | PWM_C_CCER_LN)

const uint16_t pwmDIER[6] = { //
		uint16_t(PWM_A_DIER_H),//
		uint16_t(PWM_B_DIER_H),//
		uint16_t(PWM_B_DIER_H),//
		uint16_t(PWM_C_DIER_H),//
		uint16_t(PWM_C_DIER_H),//
		uint16_t(PWM_A_DIER_H),//
};
const uint16_t pwmCCER[6] = { //
		uint16_t(PWM_A_CCER_H | PWM_A_CCER_LN | PWM_C_CCER_LN | PWM_C_CCER_H),//
		uint16_t(PWM_B_CCER_H | PWM_B_CCER_LN | PWM_C_CCER_LN | PWM_C_CCER_H),//
		uint16_t(PWM_B_CCER_H | PWM_B_CCER_LN | PWM_A_CCER_LN | PWM_A_CCER_H),//
		uint16_t(PWM_C_CCER_H | PWM_C_CCER_LN | PWM_A_CCER_LN | PWM_A_CCER_H),//
		uint16_t(PWM_C_CCER_H | PWM_C_CCER_LN | PWM_B_CCER_LN | PWM_B_CCER_H),//
		uint16_t(PWM_A_CCER_H | PWM_A_CCER_LN | PWM_B_CCER_LN | PWM_B_CCER_H),//
		};
const uint16_t pwmCCER_Low[6] = { //
		uint16_t(PWM_C_CCER_LN),//
		uint16_t(PWM_C_CCER_LN),//
		uint16_t(PWM_A_CCER_LN),//
		uint16_t(PWM_A_CCER_LN),//
		uint16_t(PWM_B_CCER_LN),//
		uint16_t(PWM_B_CCER_LN),//
};

const uint16_t comp2Off[6] = {// COMP2_CSR INM_SEL
		 0b0110<<4, // B
		 0b0111<<4, // A
		 0b1000<<4, // C
		 0b0110<<4, // B
		 0b0111<<4, // A
		 0b1000<<4, // C
};
//const uint16_t comp2PolarityOff[6] = {//
//		1<<15, // B - Non-Inverting Rising
//		0<<15, // A - Inverting Falling.
//		1<<15, // C - Non-Inverting Rising.
//		0<<15, // B - Inverting Falling.
//		1<<15, // A - Non-Inverting Rising.
//		0<<15, // C - Inverting Falling.
//};

/*\
0110: PB3 -> ZC_B
0111: PB7 -> ZC_A
1000: PA2 -> ZC_C
 Step 0  1   2  3  4  5      0  1  2  3  4  5
 High A  B   B  C  C  A      A  B  B  C  C  A
 Off  B  A   C  B  A  C      B  A  C  B  A  C
 Low  C  C   A  A  B  B      C  C  A  A  B  B
 */


volatile uint8_t memorySettings[32] = {0};
volatile int sizeOfSettings = sizeof(memorySettings);

void readMemory(volatile unsigned char* data, int size, int location){
	location *= 8;

	for( int index = 0; index < size; index++){
		data[index] =  (unsigned int)(*(uint64_t*)(0x0801F800 + location));
		location += 8;
	}

}


void writeMemory(volatile unsigned char* data, int size, int location){
	uint8_t memorySettings[32] = {0};
	readMemory(memorySettings, sizeOfSettings, 0);

	size = size > sizeOfSettings ? sizeOfSettings : size;
	location = location >= sizeOfSettings ? sizeOfSettings : location;

	for( int index = 0; index < size; index++){
		location = location >= sizeOfSettings ? sizeOfSettings : location;
		memorySettings[location++] = data[index];
	}

	FLASH_EraseInitTypeDef epage;
	epage.TypeErase = FLASH_TYPEERASE_PAGES; // FLASH_TYPEERASE_PAGES
	epage.Page = 63;
	epage.NbPages = 1;


	uint32_t error = 0;
//	HAL_StatusTypeDef ret =
	HAL_FLASH_Unlock();

//	ret =
	HAL_FLASHEx_Erase(&epage, &error);
	for( int index = 0; index < sizeOfSettings; index++){
		//ret =
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 0x0801F800 + (index * 8), memorySettings[index]);
	}

	//ret =
	HAL_FLASH_Unlock();

}


const void SetMotorThrottleBeep( uint16_t duty)
{
	static uint16_t remainder = 0;

	duty = duty < 47 ? 0 : duty;

	remainder = duty & 1;
	dmaFrequency[0] = duty>>1 | remainder;
	dmaFrequency[1] = duty>>1;
}


const void SetMotorThrottle( uint16_t duty)
{
	static uint16_t remainder = 0;

	duty = duty < 47 ? 0 : duty;
	throttle = duty;
	if( duty == 0)
	{
		allowCommutation = 0;
		startupDelay = 800;
		spinningFast = 0;
	}
	remainder = duty & 1;
	dmaFrequency[0] = duty>>1 | remainder;
	dmaFrequency[1] = duty>>1;
}


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
		{

			hdma_tim3_ch1.Instance->CCR &= ~(1);
			TIM3->DIER &= ~(TIM_DIER_CC1DE); // Disable DMA transfer interrupt for channel 1;
			debugSettings++;
			if( debugSettings == 2)
				debugSettings = 0;
			if( (dmaSignal[31] - dmaSignal[0]) < 900  )
			{

				for( uint8_t i = 0; i < 32; i += 2)
					dshotBits[i>>1] = (dmaSignal[i+1] - dmaSignal[i])>>5;

				volatile uint16_t value = 0;
				for( uint8_t i = 0 ; i < 12; i++)
					value |= (dshotBits[i]<<(11-i));

				//uint16_t crc =         (~(value ^ (value >> 4) ^ (value >> 8))) & 0x0F;
				uint8_t calculatedCRC  = (~(value ^ (value >> 4) ^ (value >> 8))) & 0x0F;
				uint8_t receviedCRC = (dshotBits[12]<<3 | dshotBits[13]<<2 | dshotBits[14]<<1 | dshotBits[15] );
				uint8_t telemetryBit = dshotBits[11];

				value >>= 1; // Shift out telemetry bit and keep throttle value.


				if( receviedCRC == calculatedCRC )
				{

					if( telemetryBit == 1 && fcThrottle == 0 && value < 48)
					{

						switch(value)
						{
						case 1: // Beep 260ms
						case 2:
							beeping = 260;
							break;
						case 3:
						case 4: // Beep 280ms
							beeping = 280;
							break;
						case 5:// Beep 1020ms
							beeping = 1020;
							break;
						case 8: // Spin Direction 1
							masterDirection = 1;
							memorySettings[0] = masterDirection;

							writeMemory(memorySettings, 1, 0);
							reverse = masterDirection;
							break;
						case 7: // Spin Direction 2
							masterDirection = 0;
							memorySettings[0] = masterDirection;
							writeMemory(memorySettings, 1, 0);

							reverse = masterDirection;
							break;
						case 20: // Spin Direction 2
							reverse = masterDirection;
							LED_GPIO_Port->BRR = LED_Pin;
							break;
						case 21: // Spin Direction Reversed.
							reverse = masterDirection == 0 ? 1 : 0;

							break;

						default:
							break;
						}
					}else if( value > 47 && value <= 2047 && beeping == 0)
					{
						noSignalCouter = 0;
						fcThrottle = value;
	//					if( fcThrottle > 1 && fcThrottle < 48)
	//						debugSettings++;
						allowCommutation = 1;
						if( fcThrottle > (throttle+50))
						{
							if( throttle < 47)
								throttle = 47;
							SetMotorThrottle(throttle + 50);
						}
						else
							SetMotorThrottle(fcThrottle);
					}else if( beeping == 0)
					{
						fcThrottle = 0;
						SetMotorThrottle(fcThrottle);
					}
				}
				else if(telemetryBit && receviedCRC == calculatedCRC)
				{
					debug1++;
				}
				else
					eRPMus = 0xffff; // Telemetry RPM = 0;

				if( fcThrottle < 47)
					eRPMus = 0xffff; // Telemetry RPM = 0;
			}

			telemetry = 1;
			TIM3->CNT = 0;
			TIM3->CCR4 = 850;
			TIM3->DIER |= TIM_DIER_CC4IE;
			TIM3->SR = ~TIM_SR_CC4IF;
			//;POUT2_GPIO_Port->BRR = POUT2_Pin;

		}
		else
		{
			debug1++;
		}
	}

}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	if(TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
		{
			// Disable channel 2.
			hdma_tim3_ch2.Instance->CCR &= ~(1);
			TIM3->CCER &= ~TIM_CCER_CC2E;

			bufferReady[befferSent] = 0;

			hdma_tim3_ch1.Instance->CNDTR = 32;

			TIM3->PSC = 3;//1;
			TIM3->ARR = 0xffff;
			TIM3->EGR |= 1;

			hdma_tim3_ch1.Instance->CCR |= (1 | DMA_CCR_TCIE) ; // enable dma and trans complete.
			TIM3->DIER |= (TIM_DIER_CC1DE); // Enable DMA trans interrupt.

		}



	}
}

;
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
		{
			//POUT2_GPIO_Port->BSRR = POUT2_Pin;
			TIM3->DIER &= ~(TIM_DIER_CC4IE); // Diasble OC interrupt for ch4;

			if( bufferReady[0] == 1)
			{
				befferSent = 0;
				bufferReady[0] = 2;
				hdma_tim3_ch2.Instance->CMAR = (uint32_t)dmaTelemetry;

			}
			else if(bufferReady[1] == 1)
			{
				befferSent = 1;
				bufferReady[1] = 2;
				hdma_tim3_ch2.Instance->CMAR = (uint32_t)dmaTelemetry2;
			}
			hdma_tim3_ch2.Instance->CNDTR = 22;

			TIM3->PSC = 0;
			TIM3->ARR = 160;//80;
			TIM3->EGR |= 1;

			hdma_tim3_ch2.Instance->CCR |= (1 | DMA_CCR_TCIE);
			TIM3->CCER |= TIM_CCER_CC2E;
//			POUT2_GPIO_Port->BRR = POUT2_Pin;

		}
		else
		{
			debug1++;
		}
	}
	else if( TIM16 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
		{
			/*
			 * Commutation call, setup from comp2 interrupt.
			 */
			debugCommutation++;// = TIM16->CNT;
			TIM16->DIER &= ~TIM_DIER_CC1IE;
			TIM16->SR = ~TIM_SR_CC1IF;

			Commutate();
		}
	}
	else if( TIM17 == htim->Instance)
	{
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
		{

			TIM17->CNT = 0;
			if (dmaFrequency[0] > 19 && allowCommutation)//initComplete > 0 && motorSpeedCurrent > 0)
			{ // Bump
				debugBump++;
				startupDelay = 800;
				Commutate();
			}
		}
	}
}




void Setup(void)
{

	// Bi-DSHOT
		HAL_TIM_IC_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)dmaSignal, 32);

		HAL_TIM_OC_Start_IT(&htim3, TIM_CHANNEL_4);
		TIM3->DIER &= ~TIM_DIER_CC4IE;

		HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2, (uint32_t*)dmaTelemetry, 22);
		hdma_tim3_ch2.Instance->CCR &= ~(1);
		TIM3->CCER &= ~TIM_CCER_CC2E;

	//

		HAL_TIM_OC_Start_IT(&htim16, TIM_CHANNEL_1);
		TIM16->DIER &= ~TIM_DIER_CC1IE;

	TIM1->CCR5 = 64;



	if (HAL_TIM_Base_Start(&htim7) != HAL_OK)
		Error_Handler();

	if( HAL_TIM_Base_Start(&htim6) != HAL_OK)
		Error_Handler();

	HAL_TIM_Base_Start(&htim1);



	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_5);
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	TIM1->CCR3 = 0;
		HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)dmaFrequency, 2); // A

	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)dmaFrequency, 2); // B

	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)dmaFrequency, 2); // C


	TIM1->DIER &= DIER_OFF;// ~(PWM_A_DIER_H | PWM_B_DIER_H | PWM_C_DIER_H);

	TIM1->CCER &= CCER_OFF;/// ~(PWM_A_CCER_H | PWM_B_CCER_H | PWM_C_CCER_H |					PWM_A_CCER_LN | PWM_B_CCER_LN | PWM_C_CCER_LN);

	hdma_tim1_ch3.Instance->CCR &= ~(1<<1);
	hdma_tim1_ch2.Instance->CCR &= ~(1<<1);
	hdma_tim1_ch1.Instance->CCR &= ~(1<<1);
	//	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)dmaFrequency, 2); // B
//	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)dmaFrequency, 2); // C

	HAL_COMP_Start(&hcomp2);

	TIM17->CCR1 = 100; // Bump timer.

	if(HAL_TIM_OC_Start_IT(&htim17, TIM_CHANNEL_1) != HAL_OK)
		Error_Handler();

}


const void Commutate(void)
{

	powerStepCurrent++;
	powerStepCurrent %= 6;


	if( reverse)
		commutationStep = 5 - powerStepCurrent;
	else
		commutationStep = powerStepCurrent; // forward.


	/*
	 0110: PB3 -> ZC_B
	 0111: PB7 -> ZC_A
	 1000: PA2 -> ZC_C
	 Step 0  1   2  3  4  5      0  1  2  3  4  5
	 High A  B   B  C  C  A      A  B  B  C  C  A
	 Off  B  A   C  B  A  C      B  A  C  B  A  C
	 Low  C  C   A  A  B  B      C  C  A  A  B  B
	 */

	COMP2->CSR &= ~((1) | (0b1111<<4) | (1<<15)); // Disable comp2.

	TIM1->DIER &= DIER_OFF;
	TIM1->CCER &= CCER_OFF;

	CCRA = 0;
	CCRB = 0;
	CCRC = 0;

	TIM1->DIER |= (pwmDIER[commutationStep]);
	TIM1->CCER |= (pwmCCER[commutationStep]);

	TIM1->EGR |= 1;

	COMP2->CSR |= (comp2Off[commutationStep]);// | comp2PolarityOff[commutationStep]);
	if(rising[reverse][commutationStep] )
	{
		COMP2->CSR |= (1<<15); // Reverse direction. ->comp2PolarityOff[commutationStep]
	}

	COMP2->CSR |= (1); // Enable comp2.

	TIM17->CNT = 0;

	EXTI->RPR1 |= (1<<18); // Clear pending comp interrupts.
	EXTI->FPR1 |= (1<<18); // Clear pending comp interrupts.
	TEL_GPIO_Port->BRR = TEL_Pin;
	if(noSignalCouter++ > 200)
	{
		SetMotorThrottle(0);
	}
}
volatile uint16_t delayPeriod = 500;
volatile uint16_t debugComp = 0;


const void biDShotDMAFormat(const uint32_t rawValue)
{
	static uint32_t value = 0
					,crc = 0
					,gcr =  0
					,finalValue = 0;

	static const uint8_t nibs[16] = //
	{//
			0x19	,0x1B	,0x12	,0x13
			,0x1D	,0x15	,0x16	,0x17
			,0x1A	,0x09	,0x0A	,0x0B
			,0x1E	,0x0D	,0x0E	,0x0F
	};

	crc = (~(rawValue ^ (rawValue >> 4) ^ (rawValue >> 8))) & 0x0F;
	value = rawValue<<4 | crc;

	gcr =	nibs[(value>>12 	& 0x0f)]<<15
			| nibs[(value>>8 	& 0x0f)]<<10
			| nibs[(value>>4 	& 0x0f)]<<5
			| nibs[(value		& 0x0f)];

	finalValue = 0;
	for(uint8_t i = 19; i != 0xff ; i-- )
	{
		finalValue |= ( finalValue &  1<<(i+1) )>>1; // Get previous bit.

		if( gcr & 1<<i )
			finalValue ^=  (1<<i); // Flip if new bit 1.
	}
	if(bufferReady[0] == 0)
	{
		dmaTelemetry[21] = 0;
		for( uint8_t i = 20; i != 0xff; i--)
		{
			dmaTelemetry[20-i] = ( finalValue & 1<<i) ? 0 : 160;//80;
		}
		bufferReady[0] = 1;
	}
	else if( bufferReady[1] == 0)
	{
		dmaTelemetry2[21] = 0;
		for( uint8_t i = 20; i != 0xff; i--)
		{
			dmaTelemetry2[20-i] = ( finalValue & 1<<i) ? 0 : 160;//80;
		}
		bufferReady[1] = 1;
	}

}




const uint32_t packBiDishotFrame(const uint32_t value)
{

	/*
	 * Frame is form by shifting the value until all bits after 9 are zero.
	 * for every shift expo is incremented by 1.
	 * then expo is added after bit 9.
	 */
	uint32_t expo = 0;
	uint32_t v = 0;
	v = value;
	if ((v>>9)  > 0)
	{
		while (v > 0b111111111)
		{
			v >>=1;
			expo++;
		}

		v |= (expo << 9);
	}

	return v;
}

void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
	if( allowCommutation == 0)
		return;
//return;
	/*
	 * Hysterises affects initial spin speed. lower spins at lower throttle.
	 *
	 */

	TIM1->CCR5 = 64;//dmaFrequency[0] < 300 ? 100 : (300 - dmaFrequency[0]);

	//TIM7->CNT = 0;
	EXTI->RPR1 |= (1<<18); // Clear pending comp interrupts.
	EXTI->FPR1 |= (1<<18); // Clear pending comp interrupts.



	if( (COMP2->CSR & COMP_CSR_VALUE) == 0)
	{
		return;
	}

	COMP2->CSR &= ~((1) | (0b1111<<4) | (1<<15));
	EXTI->RPR1 |= (1<<18); // Clear pending comp interrupts.
	EXTI->FPR1 |= (1<<18); // Clear pending comp interrupts.

	if( commutationStep == 2)
		TEL_GPIO_Port->BSRR = TEL_Pin;

	if( commutationStep == 0)
	{
		eRPMus = TIM6->CNT;
		TIM6->CNT = 0;
	}

	TIM16->CNT = 0; // Commutation timer.

	TIM16->CCR1 = startupDelay;
	startupDelay -= 1;
	if(startupDelay <= 400)
	{
		if(rising[reverse][commutationStep] )
			TIM1->CCR5 = 800;//512; // 256;
		else
			TIM1->CCR5 = 800;//512; // 256;
		startupDelay = 200;


		if( eRPMus < 3000)
		{
			if( spinningFast++ > 2000)
			{
				spinningFast = 1000000;

				TIM1->CCR5 = 500;
				Commutate();
				return;
			}
		}else
			spinningFast = 0;

	}
	else
		TIM1->CCR5 = 200;//800;

	TIM16->DIER |= TIM_DIER_CC1IE;
	TIM16->SR = ~TIM_SR_CC1IF; // OC commutate.




}


const void Beep(const uint16_t freq, const uint16_t delay)
{
	if( throttle > 0)
		return;
	LED_GPIO_Port->BSRR = LED_Pin;
	allowCommutation = 0;
	uint16_t psc = TIM1->PSC;
	uint16_t arr = TIM1->ARR;
	TIM1->PSC = 1;
	TIM1->ARR = 64000000/freq;
	TIM1->CNT = 0;
	TIM1->EGR |= 1;
	//Commutate();
	SetMotorThrottleBeep(TIM1->ARR>>5);
	Commutate();
	HAL_Delay(delay>>1);
	LED_GPIO_Port->BRR = LED_Pin;
	HAL_Delay(delay>>1);

	SetMotorThrottleBeep(0);
	TIM1->PSC = psc;
	TIM1->ARR = arr;
	TIM1->EGR |= 1;


}

void maincpp(void)
{

 	LED_GPIO_Port->BSRR = LED_Pin;
 	HAL_Delay(1000);
	SetMotorThrottle(0);
	Setup();
	LED_GPIO_Port->BRR = LED_Pin;
	blanking = 256;


	readMemory(memorySettings, sizeOfSettings, 0);
	if( memorySettings[0] != 1)
		memorySettings[0] = 0;

	masterDirection = memorySettings[0];
	reverse = masterDirection;
	/*
	 Step 0  1   2  3  4  5      0  1  2  3  4  5
	 High A  B   B  C  C  A      A  B  B  C  C  A
	 Off  B  A   C  B  A  C      B  A  C  B  A  C
	 Low  C  C   A  A  B  B      C  C  A  A  B  B
	 */
	commutationStep = 0;

	commutationStep = 5;
	Commutate();
	uint16_t beeps[10][2] = {
			{1244,260},
			{1000,280},
			{980,700},
			{1861,200},
			{1561,120},
			{1000,300},
			{1000,300},
			{1261,200},
			{1000,500},
			{1261,200}
	};

	for( int i = 0; i < 6; i++)
	{
		beeping = beeps[i][1];
		Beep(beeps[i][0],beeping);

	}

	HAL_Delay(500);
	beeping = 0;

	fcThrottle = 0;
	uint16_t localeRPM = 0;

	while(1)
	{
		if(beeping)
		{
			Beep(1244, beeping);
			beeping = 0;
		}

		//if( localeRPM != eRPMus)
		{
			localeRPM = eRPMus;
	 		//v = packBiDishotFrame((const uint32_t)localeRPM);
	 		//if( (hdma_tim3_ch2.Instance->CCR & 1) != 1)
	 		biDShotDMAFormat( packBiDishotFrame((const uint32_t)localeRPM) );
		}
//
		if( reverse == masterDirection)
			LED_GPIO_Port->BRR = LED_Pin;
		else
			LED_GPIO_Port->BSRR = LED_Pin;

	}
}
