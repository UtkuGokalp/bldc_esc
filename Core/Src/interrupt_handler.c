/*
 * interrupt_handler.c
 *
 *  Created on: Nov 23, 2026
 *      Author: ugklp
 */

#include "bldc_control.h"
#include "main.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM11)
	{
		bldc_update_phase();
	}
}
