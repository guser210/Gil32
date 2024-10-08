/*
 * variables.h
 *
 *  Created on: Sep 16, 2024
 *      Author: gilv2
 */

#ifndef INC_VARIABLES_H_
#define INC_VARIABLES_H_



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
volatile uint16_t fcRawThrottle = 0;
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

volatile uint8_t commutated = 0;

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

#define DSHOT_600
#define DSHOT_300

#undef DSHOT_600

#define MOTOR_KV 1800
volatile uint32_t minCommutationDelay = 0;


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
volatile uint8_t memorySettings[32] = {0};
volatile int sizeOfSettings = sizeof(memorySettings);



#endif /* INC_VARIABLES_H_ */
