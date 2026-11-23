/*
 * esc_main.c
 *
 *  Created on: Nov 23, 2026
 *      Author: ugklp
 */

#include "esc_main.h"
#include "bldc_control.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim11;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim11)
	{
		bldc_update_phase();
	}
}

void esc_main(void)
{
	bldc_init(&htim1);
	bldc_set_torque(50);
	//bldc_set_phase_to_step(1);
	//HAL_Delay(10);
	//bldc_stop_motor();
	//HAL_Delay(1000);
	//HAL_TIM_Base_Start_IT(&htim11);
	while (1)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		HAL_Delay(500);
	}
}
