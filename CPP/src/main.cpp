/*
 * main.cpp
 *
 *  Created on: Jul 7, 2024
 *      Author: gilv2
 */

#include "main.h"

extern "C" TIM_HandleTypeDef htim3;
extern "C" DMA_HandleTypeDef hdma_tim3_ch1;

uint16_t dshotSignal[32] = {0};
uint16_t dshotTelemetry[22] = {0};
uint16_t dshot[16] = {0};
uint16_t fcThrottle = 0;

volatile uint32_t countePWMCallback = 0;
volatile uint32_t counterOCCallback = 0;
volatile uint32_t counterICCallback = 0;
volatile uint32_t counterDelayPulseCallback = 0;

static void TIM_DMADelayPulseCplt2(DMA_HandleTypeDef *hdma)
{
	counterDelayPulseCallback++;
  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

  if (hdma == htim->hdma[TIM_DMA_ID_CC1])
  {
    htim->Channel = HAL_TIM_ACTIVE_CHANNEL_1;

    if (hdma->Init.Mode == DMA_NORMAL)
    {
      TIM_CHANNEL_STATE_SET(htim, TIM_CHANNEL_1, HAL_TIM_CHANNEL_STATE_READY);
    }
    HAL_TIM_PWM_PulseFinishedCallback(htim);
  }
}
void EnablePWMDMA(void)
{ //22.3.5 Input capture mode


	TIM3->CR1 &= ~(1<<0);
	TIM3->CCER &= ~(1<<0 | 1<<4);
	// 1. Set TI1 as output.
	TIM3->TISEL &= ~(0x0f<<0);  // Set as output.TI1SEL
	// 2. Set output as PWM..
	TIM3->CCMR1 |= (0x06<<4); // OC1M
	TIM3->CCMR1 |= (1<<3); // Preload enable. OC1PE:

	TIM3->CCMR1 &= ~(1); // OUTPUT.
	// 3. Remove filter.
//	TIM3->CCMR1 &= ~(0x0F<<4); // IC1F
//	// 4. Set edge detection to rising/falling.
//	TIM3->CCER |= (1<<1 & 1<<3); // CC1P
//	// 5. No prescaler.
//	TIM3->CCMR1 &= ~(0x03<<2); // IC1PSC


	TIM3->ARR = 80;
	TIM3->CNT = 0;



	TIM3->DIER |= (1<<9); // CCIDE 1 DMA Request enable.

	// Page282 10.3.1
	hdma_tim3_ch1.Instance->CCR &= ~(1<<0); // Disable DMA. EN.
	// 1. Set peripheral register address.
	hdma_tim3_ch1.Instance->CPAR = (uint32_t)&TIM3->CCR1;
	// 2. Set buffer memory address.
	hdma_tim3_ch1.Instance->CMAR = (uint32_t)dshotTelemetry;
	// 3. Set number of bytes to transfer.
	hdma_tim3_ch1.Instance->CNDTR = 32;
	// 4. Configure CCR params.
	hdma_tim3_ch1.Instance->CCR |= (1<<12); // Channel priority PL.
	hdma_tim3_ch1.Instance->CCR |= (1<<4); // Direction Peripheral -> Memory. DIR.
	hdma_tim3_ch1.Instance->CCR |= (1<<5); // Circular mode CIRC.
	hdma_tim3_ch1.Instance->CCR |= (1<<7); // Increment memory. MINC.
	hdma_tim3_ch1.Instance->CCR |= (1<<8); // Peripheral size. 16 bit. PSIZE.
	hdma_tim3_ch1.Instance->CCR |= (1<<10); // Mem size 16 bit. MSIZE.
	hdma_tim3_ch1.Instance->CCR |= (1<<1); // Transfer complete interrupt enable. TCIE.

	DMA1->ISR |= (1<<1 | 1);
	hdma_tim3_ch1.XferCpltCallback = &TIM_DMADelayPulseCplt2;
	hdma_tim3_ch1.Instance->CCR |= (1<<0); // Enable DMA. EN.
	TIM3->CCER |= (1); // Enable.
	//HAL_TIM_IC_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)dshotSignal, 32);
	TIM3->CR1 |= (1<<0);
}
void inline SwitchToOutput(void)
{
//	EnablePWMDMA();
//	return;
	TIM3->CR1 &= ~(1); // Disable timer3;
	TIM3->CCER &= ~(1<<0 | 1<<4);
	hdma_tim3_ch1.Instance->CCR &= ~(1); // Disable DMA.
//
	TIM3->TISEL &= ~(0x0f<<0);  // Set as output.TI1SEL
	// 2. Set output as PWM..
	TIM3->CCMR1 |= (0x06<<4); // OC1M
	TIM3->CCMR1 |= (1<<3); // Preload enable. OC1PE:

	TIM3->CCMR1 &= ~(1); // OUTPUT.

	TIM3->CCER &= ~(1<<0 | 1<<4); // Disable capture on channel 1 and 2.
	TIM3->DIER &= ~(1<<1 | 1<<2); // Disbale Compare interrupt on channels 1 and 2.
	TIM3->DIER &= ~(1<<9); // Disable DMA Request.

	hdma_tim3_ch1.Instance->CCR &= ~(1<<1); // Disable interrupt - transfer complete.

	hdma_tim3_ch1.Instance->CNDTR = 32; // Set telemetry size.

	hdma_tim3_ch1.Instance->CPAR = (uint32_t)&TIM3->CCR1;
	hdma_tim3_ch1.Instance->CMAR = (uint32_t)dshotTelemetry;

	hdma_tim3_ch1.Instance->CCR |= (1<<4); // Direction Mem to Par.
	hdma_tim3_ch1.Instance->CCR |= (1<<1); // Enable Interrupt - transfer complete.


	hdma_tim3_ch1.Instance->CCR |= (1<<12); // Channel priority PL.
	hdma_tim3_ch1.Instance->CCR |= (1<<4); // Direction Peripheral -> Memory. DIR.
	hdma_tim3_ch1.Instance->CCR |= (1<<5); // Circular mode CIRC.
	hdma_tim3_ch1.Instance->CCR |= (1<<7); // Increment memory. MINC.
	hdma_tim3_ch1.Instance->CCR |= (1<<8); // Peripheral size. 16 bit. PSIZE.
	hdma_tim3_ch1.Instance->CCR |= (1<<10); // Mem size 16 bit. MSIZE.
	hdma_tim3_ch1.Instance->CCR |= (1<<1); // Transfer complete interrupt enable. TCIE.


	TIM3->ARR = 80;
	TIM3->CNT = 0;
	TIM3->CCER |= (1<<0); // Enable Capture on channel 1.
	TIM3->DIER |= (1<<9); // Enable DMA Request.

	DMA1->ISR |= (1<<1 | 1);
	hdma_tim3_ch1.XferCpltCallback = &TIM_DMADelayPulseCplt2;

	hdma_tim3_ch1.Instance->CCR |= (1); // launch DMA.
	TIM3->CR1 |= (1); // Enable timer3;
}
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{ // Handle timer relead events.
//	countePWMCallback++;
//}
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{ // Handle Sync events, possibly commutation and other misc timer events.
	counterOCCallback++;
	if( TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
		{
		/*
		 * Synch event queued by IC on channel 2.
		 * Queue DMA for dshot signal.
		 * disable channels 2 and 4...
		 */

			hdma_tim3_ch1.Instance->CCR &= ~(1); // Disable DMA.
			POUT1_GPIO_Port->BSRR = POUT1_Pin;
			TIM3->DIER &= ~(1<<4 | 1<<2);
			POUT1_GPIO_Port->BRR = POUT1_Pin;

			hdma_tim3_ch1.Instance->CNDTR = 32; // reset number of captures.
			hdma_tim3_ch1.Instance->CCR |= (1); // launch DMA.

		}
	}

}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{ // Hanlde dshot signal and sync events.
	counterICCallback++;
	if( TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
		{
			/*
			 * DShot signal processing.
			 */
			POUT2_GPIO_Port->BSRR = POUT2_Pin;

			for( int i = 0; i < 32; i += 2)
				dshot[i>>1] = (dshotSignal[i + 1] - dshotTelemetry[i])>>5;

			uint16_t value = 0;
			for( int i = 0; i < 12; i++)
				value |= dshot[i]<<(11-i);

			fcThrottle = value>>1; // remove telemetry bit from throttle value.

			SwitchToOutput();

//			hdma_tim3_ch1.Instance->CCR &= ~(1); // Disable DMA.
//			hdma_tim3_ch1.Instance->CNDTR = 32; // reset number of captures.
//			hdma_tim3_ch1.Instance->CCR |= (1); // launch DMA.

			POUT2_GPIO_Port->BRR = POUT2_Pin;
		}
		else if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
		{
		/*
		 * Indirect capture of dshot signal. middle of signal, advance CCR4.
		 */

			POUT2_GPIO_Port->ODR ^= POUT2_Pin;

			TIM3->CCR4 = TIM3->CNT + 400;
			TIM3->SR &= ~(1<<4); // CC4IF: Clear interrupt flag to keep from false triggering.

			TIM3->DIER |= (1<<4); // CC4IE: Enable OC interrupt on channel 4.


		}
	}
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{ // Handle Telemetry transfer events.
	countePWMCallback++;
}



void setup(void)
{

	if( HAL_TIM_IC_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)dshotSignal, 32) != HAL_OK)
		Error_Handler();

	/*
	 * Starting DMA and immediately disabling just to have the setup for later use.
	 */
	hdma_tim3_ch1.Instance->CCR &= ~(1<<0); // disable DMA

	// Enable IC on channel 2 to synchronization of dshot signal.
	if( HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2) != HAL_OK)
		Error_Handler();

	/*
	 * Enable OC on channel 4 for sync event.
	 * Disable to keep config for later use.
	 */

	if( HAL_TIM_OC_Start_IT(&htim3, TIM_CHANNEL_4) != HAL_OK)
		Error_Handler();
	TIM3->DIER &= ~(1<<4); // CC4IE: Disable OC interrupt on channel 4.


}

void maincpp(void)
{

	HAL_Delay(2000); // R&D safety timeout.

	for( int i = 0; i < 22; i++)
		dshotTelemetry[i] = 50;

//	EnablePWMDMA();
	setup();
//	EnablePWMDMA();

	while(1)
	{


	}
}




