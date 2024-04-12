/*
 * bl.c
 *
 *  Created on: Apr 12, 2024
 *      Author: MAAM
 */

#include "bl.h"

uint8_t BL_Send(char* format, ...){
	char Buffer[BUFFER_MAX_LENGTH] = {0};

	va_list arg;
	va_start(arg, format);
	vsiprintf(Buffer, format, arg);
	va_end(arg);

	HAL_UART_Transmit(huart2, Buffer, sizeof(Buffer), HAL_MAX_DELAY);

}
