/*
 * bldc_control.h
 *
 *  Created on: Nov 17, 2026
 *      Author: ugklp
 */

#ifndef INC_BLDC_CONTROL_H_
#define INC_BLDC_CONTROL_H_

#include "main.h"

typedef enum BLDC_CONTROL_RESULT
{
	BLDC_CONTROL_RESULT_SUCCESS = 0,
	BLDC_CONTROL_RESULT_ALREADY_INITIALIZED,
	BLDC_CONTROL_RESULT_NOT_INITIALIZED,
	BLDC_CONTROL_RESULT_NULLPTR,
} BLDC_CONTROL_RESULT;

BLDC_CONTROL_RESULT bldc_init(TIM_HandleTypeDef* htim1);
BLDC_CONTROL_RESULT bldc_update_phase(void);
uint8_t bldc_get_speed(void);
void bldc_set_speed(uint8_t percent);
#endif /* INC_BLDC_CONTROL_H_ */
