


#include "main.h"

extern "C" TIM_HandleTypeDef htim3;
extern "C" DMA_HandleTypeDef hdma_tim3_ch1;
extern "C" DMA_HandleTypeDef hdma_tim3_ch2;

volatile uint16_t dmaSignal[32] = {0};
volatile uint16_t dshotBits[16] = {0};

volatile uint16_t dmaTelemetry[22] = {80, 80, 0, 0, 80, 0, 0, 80, 80, 0, 80, 0, 0, 0, 80, 0, 80, 0, 80, 0, 0, 0};

volatile uint16_t fcThrottle = 0;

volatile uint32_t counterOC4 = 0;
volatile uint32_t counterDMA1 = 0;
volatile uint32_t counterIC2 = 0;

uint8_t telemetry = 0;
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
		{

			hdma_tim3_ch1.Instance->CCR &= ~(1);
			TIM3->DIER &= ~(TIM_DIER_CC1DE); // Disable DMA transfer interrupt for channel 1;


			for( uint8_t i = 0; i < 32; i += 2)
				dshotBits[i>>1] = (dmaSignal[i+1] - dmaSignal[i])>>5;

			uint16_t value = 0;
			for( uint8_t i = 0 ; i < 12; i++)
				value |= (dshotBits[i]<<(11-i));

			fcThrottle = value>>1;


			telemetry = 1;
			TIM3->CNT = 0;
			TIM3->CCR4 = 700;
			TIM3->DIER |= TIM_DIER_CC4IE;
			TIM3->SR = ~TIM_SR_CC4IF;
			POUT2_GPIO_Port->BRR = POUT2_Pin;

		}
	}

}
volatile uint32_t debug1 = 0;
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	if(TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
		{
			// Disable channel 2.
			hdma_tim3_ch2.Instance->CCR &= ~(1);
			TIM3->CCER &= ~TIM_CCER_CC2E;

			hdma_tim3_ch1.Instance->CNDTR = 32;

			TIM3->PSC = 1;
			TIM3->ARR = 0xffff;
			TIM3->EGR |= 1;

			hdma_tim3_ch1.Instance->CCR |= (1 | DMA_CCR_TCIE) ; // enable dma and trans complete.
			TIM3->DIER |= (TIM_DIER_CC1DE); // Enable DMA trans interrupt.

		}

	}
}
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(TIM3 == htim->Instance)
	{
		if( htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
		{
			POUT2_GPIO_Port->BSRR = POUT2_Pin;
			TIM3->DIER &= ~(TIM_DIER_CC4IE); // Diasble OC interrupt for ch4;

			hdma_tim3_ch2.Instance->CNDTR = 22;

			TIM3->PSC = 0;
			TIM3->ARR = 80;
			TIM3->EGR |= 1;

			hdma_tim3_ch2.Instance->CCR |= (1 | DMA_CCR_TCIE);
			TIM3->CCER |= TIM_CCER_CC2E;
			POUT2_GPIO_Port->BRR = POUT2_Pin;

		}

	}

}




void Setup(void)
{

	HAL_TIM_IC_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)dmaSignal, 32);

	HAL_TIM_OC_Start_IT(&htim3, TIM_CHANNEL_4);
	TIM3->DIER &= ~TIM_DIER_CC4IE;

	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2, (uint32_t*)dmaTelemetry, 22);
	hdma_tim3_ch2.Instance->CCR &= ~(1);
	TIM3->CCER &= ~TIM_CCER_CC2E;

}


void maincpp(void)
{

	Setup();
//		uint32_t v = packBiDishotFrame((const uint32_t)500);
//		biDShotDMAFormat( v );
	while(1)
	{

	}
}
