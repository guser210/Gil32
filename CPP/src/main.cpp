


#include "main.h"
#include "variables.h"

/*
 * 10/5/2024 - no changes, copied code to notion.
 */

/*\
0110: PB3 -> ZC_B
0111: PB7 -> ZC_A
1000: PA2 -> ZC_C
 Step 0  1   2  3  4  5      0  1  2  3  4  5
 High A  B   B  C  C  A      A  B  B  C  C  A
 Off  B  A   C  B  A  C      B  A  C  B  A  C
 Low  C  C   A  A  B  B      C  C  A  A  B  B
 */
volatile uint16_t throttleHalves = 0;
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

	HAL_FLASH_Unlock();


	HAL_FLASHEx_Erase(&epage, &error);
	for( int index = 0; index < sizeOfSettings; index++){

		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 0x0801F800 + (index * 8), memorySettings[index]);
	}

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

volatile uint8_t motorNotSpinning = 1;
volatile uint16_t debug[4] = {0};
const void SetMotorThrottle( uint16_t duty)
{
	static uint16_t remainder = 0;

	duty = duty < 47 ? 0 : duty;
	throttle = duty;
	if( duty == 0)
	{
		debug[0]++;
		motorNotSpinning = 1;
		allowCommutation = 0;
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


			if( (dmaSignal[31] - dmaSignal[0]) < 900  )
			{

				for( uint8_t i = 0; i < 32; i += 2)
					dshotBits[i>>1] = (dmaSignal[i+1] - dmaSignal[i])>>5;

				volatile uint16_t value = 0;
				for( uint8_t i = 0 ; i < 12; i++)
					value |= (dshotBits[i]<<(11-i));

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

						allowCommutation = 1;

						if( motorNotSpinning && eRPMus < 4500)
						{

							motorNotSpinning = 0;

						}

						if(throttle > 1000 && eRPMus > 4000)
						{
							SetMotorThrottle(throttle>>1); // something wrong? slow down a bit.
							throttleHalves++;
						}
						else
						{
							if( fcThrottle > 300 && motorNotSpinning) //  Line goes with MotorNoSpinning.
								fcThrottle = 300;

							SetMotorThrottle(fcThrottle );
						}
					}else if( beeping == 0)
					{
						fcThrottle = 0;
						SetMotorThrottle(fcThrottle);
					}
				}
				else if(telemetryBit && receviedCRC == calculatedCRC)
				{
					// TODO: nothing.
				}
				else
					eRPMus = 0xffff; // Telemetry RPM = 0;

				if( fcThrottle < 47)
					eRPMus = 0xffff; // Telemetry RPM = 0;
			}

			telemetry = 1;
			TIM3->CNT = 0;
#ifdef DSHOT_600
			TIM3->CCR4 = 700;//850;
#else
			TIM3->CCR4 = 850;
#endif
			TIM3->DIER |= TIM_DIER_CC4IE;
			TIM3->SR = ~TIM_SR_CC4IF;


		}
		else
		{
			// TODO: nothing.
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

#ifdef DSHOT_600
			TIM3->PSC = 1;
#else
			TIM3->PSC = 3;//1;
#endif
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
#ifdef DSHOT_600
			TIM3->ARR = 80;
#else
			TIM3->ARR = 160;//80;
#endif
			TIM3->EGR |= 1;

			hdma_tim3_ch2.Instance->CCR |= (1 | DMA_CCR_TCIE);
			TIM3->CCER |= TIM_CCER_CC2E;

		}
		else
		{
			// TODO: nothing.
		}
	}
	else if( TIM17 == htim->Instance)
	{
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
		{

			TIM17->CNT = 0;
			if (throttle > 5 && allowCommutation)//initComplete > 0 && motorSpeedCurrent > 0)
			{
				/*
				 * Bump motor if not spinning.
				 * this commutates motor if timer expired and throttle > 0.
				 */
				motorNotSpinning = 1;
				debug[1]++;
				Commutate();
			}
		}
	}
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

	TIM1->DIER &= DIER_OFF; // disbale DMA interrupt call on all 3 lines.
	TIM1->CCER &= CCER_OFF; // disable capture compare on all 6 PWM lines.

	CCRA = 0; // need to remove any left over values from DMA transfers.
	CCRB = 0;
	CCRC = 0;

	TIM1->DIER |= (pwmDIER[commutationStep]); // enable DMA on active high side.
	TIM1->CCER |= (pwmCCER[commutationStep]); // enable PWM on ALL ATIVE lines high and low.

	TIM1->EGR |= 1; // force timer1 update.

	COMP2->CSR |= (comp2Off[commutationStep]);// enable comparator line on floating phase.
	if(rising[reverse][commutationStep] )
	{
		COMP2->CSR |= (1<<15); // invert comparator output to always output high.
	}

	COMP2->CSR |= (1); // Enable comp2.

	EXTI->RPR1 |= (1<<18); // Clear pending comp interrupts.
	EXTI->FPR1 |= (1<<18); // Clear pending comp interrupts.


	if(noSignalCouter++ > 1000)
	{ // kill throttle if flight conltroller signal stops.
		SetMotorThrottle(0);
	}

	commutated = 1; // TODO: add better explanation of what this does??
	TIM17->CNT = 0;
	TIM7->CNT = 0; // Reset bump timer
	TIM7->EGR |= 1;
}
volatile uint16_t delayPeriod = 500;
volatile uint16_t debugComp = 0;


const void biDShotDMAFormat(const uint32_t rawValue)
{
	static uint32_t value = 0
					,crc = 0
					,gcr =  0
					,finalValue = 0;

	static const uint8_t nibs[16] = // TODO: get source and give credit for this code.
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
#ifdef DSHOT_600
			dmaTelemetry[20-i] = ( finalValue & 1<<i) ? 0 : 80;
#else
			dmaTelemetry[20-i] = ( finalValue & 1<<i) ? 0 : 160;//80;
#endif

		}
		bufferReady[0] = 1;
	}
	else if( bufferReady[1] == 0)
	{
		dmaTelemetry2[21] = 0;
		for( uint8_t i = 20; i != 0xff; i--)
		{
#ifdef DSHOT_600
			dmaTelemetry2[20-i] = ( finalValue & 1<<i) ? 0 : 80;

#else
			dmaTelemetry2[20-i] = ( finalValue & 1<<i) ? 0 : 160;//80;
#endif

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
	 *
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
	SetMotorThrottleBeep(TIM1->ARR>>6);
	Commutate();
	HAL_Delay(delay>>1);
	LED_GPIO_Port->BRR = LED_Pin;
	HAL_Delay(delay>>1);

	SetMotorThrottleBeep(0);
	TIM1->PSC = psc;
	TIM1->ARR = arr;
	TIM1->EGR |= 1;

}


volatile uint16_t eRPMFF = 0;

void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
	if( allowCommutation == 0)
		return;

	TIM1->CCR5 = 96;//256;//throttle < 196 ? throttle : 196;
	EXTI->RPR1 |= (1<<18); // Clear pending comp interrupts.

	if(EXTI->RPR1 & 1<<18)// TODO: Gil new 10/22/2024
		return;

	if( eRPMus == 0xffff)
	{
		eRPMFF++; // For debug only.
		commutationDelay = 20000;
	}

	if( commutated && TIM7->CNT < (commutationDelay > 0 ? commutationDelay>>2 : 10000))
	{
		return;
	}

	/*
	 * Hysterises affects initial spin speed. lower spins at lower throttle.
	 *
	 */

	/* Loop below should be a setting for user adjustment.
	 *
	 */
	for( int i = 0; i < 12; i++)
	{ // Abort commutation if Comparator triggers or Comparator value changes to zero.
		if(EXTI->RPR1 & 1<<18)
			return;
		if( (COMP2->CSR & COMP_CSR_VALUE) == 0)
			return;
	}

	commutated = 0;

	commutationDelay = TIM7->CNT;


	COMP2->CSR &= ~((1) | (0b1111<<4) | (1<<15));
	EXTI->RPR1 |= (1<<18); // Clear pending comp interrupts.


	if( commutationStep == 0)
	{
		eRPMus = TIM6->CNT;
		TIM6->CNT = 0;
	}


	Commutate();


}


void Setup(void)
{

	/*
	 * Future params.
	 * Motor KV.
	 * Motor Poles.
	 * RPM percent from KV.
	 */
	// minCommutationDelay no longer needed.
	minCommutationDelay = ( MOTOR_KV * 26) * 0.94;// 0.7;
	minCommutationDelay = ( minCommutationDelay * 7 / 60);
	minCommutationDelay = (1000000 / minCommutationDelay);
	minCommutationDelay /= 6;
	minCommutationDelay *= 32;
	minCommutationDelay = uint32_t( (float)minCommutationDelay * 1.0F);//1.25f);

	// Bi-DSHOT
	HAL_TIM_IC_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)dmaSignal, 32);

	HAL_TIM_OC_Start_IT(&htim3, TIM_CHANNEL_4);
	TIM3->DIER &= ~TIM_DIER_CC4IE;

	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2, (uint32_t*)dmaTelemetry, 22);
	hdma_tim3_ch2.Instance->CCR &= ~(1);
	TIM3->CCER &= ~TIM_CCER_CC2E;

	//

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

	TIM1->CCER &= CCER_OFF;/// ~(PWM_A_CCER_H | PWM_B_CCER_H | PWM_C_CCER_H |	PWM_A_CCER_LN | PWM_B_CCER_LN | PWM_C_CCER_LN);

	hdma_tim1_ch3.Instance->CCR &= ~(1<<1);
	hdma_tim1_ch2.Instance->CCR &= ~(1<<1);
	hdma_tim1_ch1.Instance->CCR &= ~(1<<1);


	HAL_COMP_Start(&hcomp2);

	TIM17->CCR1 = 25; // Bump timer.

	if(HAL_TIM_OC_Start_IT(&htim17, TIM_CHANNEL_1) != HAL_OK)
		Error_Handler();


}

void maincpp(void)
{

 	LED_GPIO_Port->BSRR = LED_Pin;
	HAL_Delay(1000);
	SetMotorThrottle(0);
	Setup();
	LED_GPIO_Port->BRR = LED_Pin;



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
			{1244,200},
			{1000,220},
			{980,400},
			{1861,180},
			{1561,160},
			{1000,210},
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
	volatile uint16_t localeRPM = 0;

	while(1)
	{
		if(beeping)
		{
			Beep(1244, beeping);
			beeping = 0;
		}


		{
			localeRPM = eRPMus;

	 		biDShotDMAFormat( packBiDishotFrame((const uint32_t)localeRPM) );
		}
//
		if( reverse == masterDirection)
			LED_GPIO_Port->BRR = LED_Pin;
		else
			LED_GPIO_Port->BSRR = LED_Pin;

	}
}
