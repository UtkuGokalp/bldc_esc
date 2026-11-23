/*
 * bldc_control.c
 *
 *  Created on: Nov 17, 2026
 *      Author: ugklp
 */

#include <utils.h>
#include <stdbool.h>
#include "bldc_control.h"

static bool initialized = false;
static uint8_t torque_percent = 0;
static TIM_HandleTypeDef* tim1_handle = NULL;

#define PHASE_A_TIM_CHANNEL		TIM_CHANNEL_1
#define PHASE_B_TIM_CHANNEL		TIM_CHANNEL_2
#define PHASE_C_TIM_CHANNEL		TIM_CHANNEL_3

#define PWM_MODE1_MASK(bit_pos)	(0x6UL << (bit_pos))

typedef enum RESULT_INTERNAL
{
	RESULT_INTERNAL_SUCCESS = 0,
	RESULT_INTERNAL_INVALID_PARAMETER,
} RESULT_INTERNAL;

typedef enum PHASE_CONNECTION
{
	PHCONN_POS,
	PHCONN_NEG,
	PHCONN_FLOAT,
	PHCONN_COUNT,
} PHASE_CONNECTION;

static RESULT_INTERNAL set_duty_cycle(uint8_t duty, uint32_t timer_channel)
{
	if (duty > 100)
	{
		duty = 100;
	}
	__HAL_TIM_SET_COMPARE(tim1_handle, timer_channel, 100 - duty);
	return RESULT_INTERNAL_SUCCESS;
}

static RESULT_INTERNAL connect_phase_to_dc_bus(uint32_t tim_channel, bool connect_to_pos)
{
	//Restore PWM
	switch (tim_channel)
	{
	case TIM_CHANNEL_1:
		TIM1->CCMR1 &= ~TIM_CCMR1_OC1M; //Clear the bits
		TIM1->CCMR1 |= PWM_MODE1_MASK(TIM_CCMR1_OC1M_Pos); //Enable PWM Mode 1
		TIM1->CCER |= TIM_CCER_CC1NE; //Enable complementary output
		break;
	case TIM_CHANNEL_2:
		TIM1->CCMR1 &= ~TIM_CCMR1_OC2M; //Clear the bits
		TIM1->CCMR1 |= PWM_MODE1_MASK(TIM_CCMR1_OC2M_Pos); //Enable PWM Mode 1
		TIM1->CCER |= TIM_CCER_CC2NE; //Enable complementary output
		break;
	case TIM_CHANNEL_3:
		TIM1->CCMR2 &= ~TIM_CCMR2_OC3M; //Clear the bits
		TIM1->CCMR2 |= PWM_MODE1_MASK(TIM_CCMR2_OC3M_Pos); //Enable PWM Mode 1
		TIM1->CCER |= TIM_CCER_CC3NE; //Enable complementary output
		break;
	default:
		//Abort everything
		return RESULT_INTERNAL_INVALID_PARAMETER;
	}
	TIM1->CNT = 0; //Start PWM from the beginning
	set_duty_cycle(connect_to_pos ? torque_percent : 0, tim_channel);
	return RESULT_INTERNAL_SUCCESS;
}

static RESULT_INTERNAL phase_to_pos(uint32_t tim_channel)
{
	return connect_phase_to_dc_bus(tim_channel, true);
}

static RESULT_INTERNAL phase_to_gnd(uint32_t tim_channel)
{
	return connect_phase_to_dc_bus(tim_channel, false);
}

static RESULT_INTERNAL float_phase(uint32_t tim_channel)
{
	// Force CHx HIGH and CHxN LOW (turns off both transistors)
	switch (tim_channel)
	{
	case TIM_CHANNEL_1:
		// Force OC1REF LOW
		TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
		TIM1->CCMR1 |= TIM_CCMR1_OC1M_2;
		// Enable main output, disable complementary output
		//This means drive main output to forced value (HIGH), drive complementary output to idle value (LOW)
		TIM1->CCER |=  TIM_CCER_CC1E;
		TIM1->CCER &= ~TIM_CCER_CC1NE;
		break;
	case TIM_CHANNEL_2:
		// Force OC2REF LOW
		TIM1->CCMR1 &= ~TIM_CCMR1_OC2M;
		TIM1->CCMR1 |= TIM_CCMR1_OC2M_2;
		// Enable main output, disable complementary output
		//This means drive main output to forced value (HIGH), drive complementary output to idle value (LOW)
		TIM1->CCER |=  TIM_CCER_CC2E;
		TIM1->CCER &= ~TIM_CCER_CC2NE;
		break;
	case TIM_CHANNEL_3:
		// Force OC3REF LOW
		TIM1->CCMR2 &= ~TIM_CCMR2_OC3M;
		TIM1->CCMR2 |= TIM_CCMR2_OC3M_2;
		// Enable main output, disable complementary output
		//This means drive main output to forced value (HIGH), drive complementary output to idle value (LOW)
		TIM1->CCER |=  TIM_CCER_CC3E;
		TIM1->CCER &= ~TIM_CCER_CC3NE;
		break;
	default:
		//Don't do anything
		return RESULT_INTERNAL_INVALID_PARAMETER;
	}
	// Enable off-state output driving (this shouldn't be changed from software so this line shouldn't be needed - the timers should always drive the output or the MOSFET gate drivers can die)
	TIM1->BDTR |= TIM_BDTR_OSSR;
	return RESULT_INTERNAL_SUCCESS;
}

static RESULT_INTERNAL config_tim_registers(uint32_t timer_channel, PHASE_CONNECTION conn)
{
	switch (conn)
	{
	case PHCONN_POS:
		phase_to_pos(timer_channel);
		break;
	case PHCONN_NEG:
		phase_to_gnd(timer_channel);
		break;
	case PHCONN_FLOAT:
		float_phase(timer_channel);
		break;
	default:
		return RESULT_INTERNAL_INVALID_PARAMETER;
	}
	return RESULT_INTERNAL_SUCCESS;
}

static RESULT_INTERNAL set_phase_connection(uint32_t timer_channel, PHASE_CONNECTION conn)
{
	return config_tim_registers(timer_channel, conn);
}

uint8_t bldc_get_speed(void)
{
	return torque_percent;
}

void bldc_set_torque(uint8_t percent)
{
	if (percent > 100)
	{
		percent = 100;
	}
	torque_percent = percent;
}

BLDC_CONTROL_RESULT bldc_init(TIM_HandleTypeDef* htim1)
{
	if (initialized)
	{
		return BLDC_CONTROL_RESULT_ALREADY_INITIALIZED;
	}
	if (htim1 == NULL)
	{
		return BLDC_CONTROL_RESULT_NULLPTR;
	}
	tim1_handle = htim1;

	torque_percent = 0;
	set_duty_cycle(torque_percent, PHASE_A_TIM_CHANNEL);
	set_duty_cycle(torque_percent, PHASE_B_TIM_CHANNEL);
	set_duty_cycle(torque_percent, PHASE_C_TIM_CHANNEL);

	HAL_TIM_PWM_Start(htim1, PHASE_A_TIM_CHANNEL);
	HAL_TIMEx_PWMN_Start(htim1, PHASE_A_TIM_CHANNEL);

	HAL_TIM_PWM_Start(htim1, PHASE_B_TIM_CHANNEL);
	HAL_TIMEx_PWMN_Start(htim1, PHASE_B_TIM_CHANNEL);

	HAL_TIM_PWM_Start(htim1, PHASE_C_TIM_CHANNEL);
	HAL_TIMEx_PWMN_Start(htim1, PHASE_C_TIM_CHANNEL);

	initialized = true;
	return BLDC_CONTROL_RESULT_SUCCESS;
}

BLDC_CONTROL_RESULT bldc_stop_motor(void)
{
	set_phase_connection(PHASE_A_TIM_CHANNEL, PHCONN_NEG);
	set_phase_connection(PHASE_B_TIM_CHANNEL, PHCONN_NEG);
	set_phase_connection(PHASE_C_TIM_CHANNEL, PHCONN_NEG);
	return BLDC_CONTROL_RESULT_SUCCESS;
}

BLDC_CONTROL_RESULT bldc_set_phase_to_step(uint8_t step)
{
	if (!initialized)
	{
		return BLDC_CONTROL_RESULT_NOT_INITIALIZED;
	}
	switch (step)
	{
	case 1:
		//Step 1
		set_phase_connection(PHASE_A_TIM_CHANNEL, PHCONN_POS);
		set_phase_connection(PHASE_B_TIM_CHANNEL, PHCONN_NEG);
		set_phase_connection(PHASE_C_TIM_CHANNEL, PHCONN_FLOAT);
		break;
	case 2:
		//Step 2
		set_phase_connection(PHASE_A_TIM_CHANNEL, PHCONN_POS);
		set_phase_connection(PHASE_B_TIM_CHANNEL, PHCONN_FLOAT);
		set_phase_connection(PHASE_C_TIM_CHANNEL, PHCONN_NEG);
		break;
	case 3:
		//Step 3
		set_phase_connection(PHASE_A_TIM_CHANNEL, PHCONN_FLOAT);
		set_phase_connection(PHASE_B_TIM_CHANNEL, PHCONN_POS);
		set_phase_connection(PHASE_C_TIM_CHANNEL, PHCONN_NEG);
		break;
	case 4:
		//Step 4
		set_phase_connection(PHASE_A_TIM_CHANNEL, PHCONN_NEG);
		set_phase_connection(PHASE_B_TIM_CHANNEL, PHCONN_POS);
		set_phase_connection(PHASE_C_TIM_CHANNEL, PHCONN_FLOAT);
		break;
	case 5:
		//Step 5
		set_phase_connection(PHASE_A_TIM_CHANNEL, PHCONN_NEG);
		set_phase_connection(PHASE_B_TIM_CHANNEL, PHCONN_FLOAT);
		set_phase_connection(PHASE_C_TIM_CHANNEL, PHCONN_POS);
		break;
	case 6:
		//Step 6
		set_phase_connection(PHASE_A_TIM_CHANNEL, PHCONN_FLOAT);
		set_phase_connection(PHASE_B_TIM_CHANNEL, PHCONN_NEG);
		set_phase_connection(PHASE_C_TIM_CHANNEL, PHCONN_POS);
		break;
	default:
		return BLDC_CONTROL_INVALID_ARGUMENT;
	}
	return BLDC_CONTROL_RESULT_SUCCESS;
}

BLDC_CONTROL_RESULT bldc_update_phase(void)
{
	if (!initialized)
	{
		return BLDC_CONTROL_RESULT_NOT_INITIALIZED;
	}

	static uint8_t current_step = 1;
	bldc_set_torque(torque_percent);
	BLDC_CONTROL_RESULT result = bldc_set_phase_to_step(current_step);

	if (result != BLDC_CONTROL_RESULT_SUCCESS)
	{
		return result;
	}

	current_step++;
	if (current_step > 6)
	{
		current_step = 1;
	}
	return BLDC_CONTROL_RESULT_SUCCESS;
}
