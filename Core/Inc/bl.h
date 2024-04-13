/*
 * bl.h
 *
 *  Created on: Apr 12, 2024
 *      Author: MAAM
 */

#ifndef INC_BL_H_
#define INC_BL_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "crc.h"
#include "usart.h"

/* ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** */

#define BUFFER_MAX_LENGTH			200u
#define BL_RECONNECT_MAX			5u

#define HAL_SUCCESSFUL_ERASE         0xFFFFFFFFU

#define CBL_FLASH_MAX_PAGE_NUMBER    16
#define CBL_FLASH_MASS_ERASE         0xFF

#define BL_FLASH_MAX_PAGE_NUMBER	16u
#define BL_FLASH_MASS_ERASE			0xFFu

#define SRAM_PAGES_NUM				0x14u									//20
#define FLASH_PAGES_NUM				0x40u									//64
#define BOOT_PAGES_NUM				(FLASH_PAGES_NUM / 2u)					//32
#define BANK1_PAGES_NUM				(FLASH_PAGES_NUM - BOOT_PAGES_NUM)		//32

#define STM32F103_SRAM_SIZE         (SRAM_PAGES_NUM * FLASH_PAGE_SIZE)
#define STM32F103_FLASH_SIZE        (FLASH_PAGES_NUM * FLASH_PAGE_SIZE)
#define STM32F103_BOOT_SIZE			(BOOT_PAGES_NUM * FLASH_PAGE_SIZE)
#define STM32F103_BANK1_SIZE		(BANK1_PAGES_NUM * FLASH_PAGE_SIZE)

#define STM32F103_BANK1_BASE		(FLASH_BASE + STM32F103_BOOT_SIZE)

#define STM32F103_SRAM_END          (SRAM_BASE + STM32F103_SRAM_SIZE)
#define STM32F103_FLASH_END         (FLASH_BASE + STM32F103_FLASH_SIZE)

#define BL_VENDOR_ID                100u
#define BL_SW_MAJOR_VERSION         1u
#define BL_SW_MINOR_VERSION         1u
#define BL_SW_PATCH_VERSION         0u

#define BL_H_UART					huart2
#define BL_H_CRC					hcrc

/* ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** */

typedef enum{
	BL_GET_VER_CMD     = 0x10u,
	BL_GET_HELP_CMD    = 0x11u,
	BL_GET_CID_CMD     = 0x12u,
	BL_GO_TO_ADDR_CMD  = 0x14u,
	BL_FLASH_ERASE_CMD = 0x15u,
	BL_MEM_WRITE_CMD   = 0x16u
}BL_tenuCMD;

typedef enum {
	BL_SEND_NACK       = 0xABu,
	BL_SEND_ACK        = 0xCDu
}BL_tenuACK;

typedef enum{
	INVALID_PAGE_NUMBER = 0x00u,
	VALID_PAGE_NUMBER   = 0x01u,
	UNSUCCESSFUL_ERASE  = 0x02u,
	SUCCESSFUL_ERASE    = 0x03u
}BL_tenuFlashState;

typedef void (*tpFunc)(void);

/* ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** */

HAL_StatusTypeDef BL_Fetch_Host_Command(void);
void              BL_Jump_App          (void);

#endif /* INC_BL_H_ */
