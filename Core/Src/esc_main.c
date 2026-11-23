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

void esc_main(void)
{
	bldc_init(&htim1, &htim11);
	bldc_set_torque(40);
	bldc_set_speed(0);
	bldc_set_phase_to_step(1);
	HAL_Delay(2000);

	for	(uint8_t i = 0; i < 101; i++)
	{
		bldc_set_speed(i);
		HAL_Delay(50);
	}

	for	(uint8_t i = 40; i < 101; i++)
	{
		//This does end up drawing a ton of current (>3A), test this a bit more
		//bldc_set_torque(i);
		HAL_Delay(50);
	}

	while (1)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		HAL_Delay(500);
	}
}
