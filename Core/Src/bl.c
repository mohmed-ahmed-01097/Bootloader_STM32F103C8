/*
 * bl.c
 *
 *  Created on: Apr 12, 2024
 *      Author: MAAM
 */

#include "bl.h"

#define MCU_REG_ADD			*((volatile uint32_t*) STM32F103_BANK1_BASE      )
#define RESET_HANDLER_ADD	*((volatile uint32_t*)(STM32F103_BANK1_BASE) + 4u)


#define SET_VERSION(_v)	_v##_VENDOR_ID, _v##_SW_MAJOR_VERSION, _v##_SW_MINOR_VERSION, _v##_SW_PATCH_VERSION
#define SEND_VERSION(version)	_Send_UART("Version ID %03d:%02d.%02d.%02d", SET_VERSION(version))

extern UART_HandleTypeDef BL_H_UART;
extern CRC_HandleTypeDef BL_H_CRC;
static FLASH_EraseInitTypeDef hflash;

static uint8_t pMsgBuffer[BUFFER_MAX_LENGTH] = {0};

/* ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** */

static HAL_StatusTypeDef _Verify_Crc(uint32_t* pData, uint8_t data_size, uint32_t crc);
static HAL_StatusTypeDef _Reciece_UART(uint8_t* const pBuffer);
static HAL_StatusTypeDef _Send_UART(char* format, ...);

static HAL_StatusTypeDef _Send_CMD_UART(uint8_t* pData, uint8_t data_size);
static HAL_StatusTypeDef _Send_ACK(uint8_t data_size);
static HAL_StatusTypeDef _Send_NACK(void);

static HAL_StatusTypeDef _Get_Version(void);
static HAL_StatusTypeDef _Get_Help(void);
static HAL_StatusTypeDef _Get_Chip_ID(void);
static HAL_StatusTypeDef _Erase_Flash(uint8_t* const pBuffer);
static HAL_StatusTypeDef _Write_Flash(uint8_t* const pBuffer);

static BL_tenuFlashState _Perform_Flash_Erase(uint32_t PageAddress, uint8_t page_Number);
static HAL_StatusTypeDef _Address_Varification(uint32_t Address);
static HAL_StatusTypeDef _Paylaod_Write(uint16_t *pdata, uint32_t StartAddress, uint8_t Payloadlen);

/* ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** */

static HAL_StatusTypeDef _Verify_Crc(uint32_t* pData, uint8_t data_size, uint32_t crc){
	HAL_StatusTypeDef Ret_State = HAL_ERROR;
	uint32_t dataBuffer = 0;
	uint32_t new_Crc = 0;

	for(uint8_t i = 0 ; i < data_size ; i++){
		dataBuffer = (uint32_t)pData[i];
		new_Crc = HAL_CRC_Accumulate(&BL_H_CRC, &dataBuffer, 1u);
	}
	__HAL_CRC_DR_RESET(&hcrc);

	if(new_Crc == crc){
		Ret_State = HAL_OK;
	}

	return Ret_State;
}

static HAL_StatusTypeDef _Reciece_UART(uint8_t* const pBuffer){
	HAL_StatusTypeDef Ret_State = HAL_ERROR;

	Ret_State = HAL_UART_Receive(&BL_H_UART, (uint8_t*)pBuffer, 1u, HAL_MAX_DELAY);

	if(Ret_State == HAL_OK){

		Ret_State = HAL_UART_Receive(&BL_H_UART, (uint8_t*)(pBuffer + 1), (uint16_t)(*pBuffer), HAL_MAX_DELAY);

		if(Ret_State == HAL_OK){
			uint16_t data_size = ((uint16_t)*pBuffer) - 4 + 1;
			uint32_t crc = *(uint32_t*)(pBuffer + data_size);
			Ret_State = _Verify_Crc((uint32_t*)pBuffer, data_size, crc);
		}
	}

	return Ret_State;
}

static HAL_StatusTypeDef _Send_UART(char* format, ...){
	char pMsg[BUFFER_MAX_LENGTH] = {0};
	//uint16_t data_size = 0;

	va_list args;
	va_start(args, format);
	vsiprintf(pMsg, format, args);	/* TODO: get the return of the function */
	//data_size = (uint16_t)vsiprintf(pMsg, format, arg);
	va_end(args);

	return HAL_UART_Transmit(&BL_H_UART, (uint8_t*)pMsg, sizeof(pMsg), HAL_MAX_DELAY);
	//return HAL_UART_Transmit(&BL_H_UART, (uint8_t*)pMsg, data_size, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef _Send_CMD_UART(uint8_t* pData, uint8_t data_size){
	HAL_StatusTypeDef Ret_State = HAL_ERROR;

	Ret_State = _Send_ACK(data_size);
	if(Ret_State == HAL_OK){
		Ret_State = HAL_UART_Transmit(&BL_H_UART, (uint8_t*)pData, (uint16_t)data_size, HAL_MAX_DELAY);
	}else{
		_Send_NACK();
	}
	return Ret_State;
}

static HAL_StatusTypeDef _Send_ACK(uint8_t data_size){
	uint8_t ACK_value[]={BL_SEND_ACK, data_size};
	return HAL_UART_Transmit(&BL_H_UART, (uint8_t*)ACK_value, 2u, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef _Send_NACK(void){
	uint8_t ACK_value = BL_SEND_NACK;
	return HAL_UART_Transmit(&BL_H_UART, (uint8_t*)&ACK_value, 1u, HAL_MAX_DELAY);
}

/* ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** */

static HAL_StatusTypeDef _Get_Version(void){
	uint8_t Veraion[] = {SET_VERSION(BL)};
	//SEND_VERSION(BL);
	return _Send_CMD_UART(Veraion, sizeof(Veraion));
}

static HAL_StatusTypeDef _Get_Help(void){
	uint8_t Commands[] = {
			BL_GET_VER_CMD,
			BL_GET_HELP_CMD,
			BL_GET_CID_CMD,
			BL_GO_TO_ADDR_CMD,
			BL_FLASH_ERASE_CMD,
			BL_MEM_WRITE_CMD
	};
	return _Send_CMD_UART(Commands, sizeof(Commands));
}

static HAL_StatusTypeDef _Get_Chip_ID(void){
	uint16_t Chip_ID = (uint16_t)(DBGMCU->IDCODE & 0x00000FFF);
	return _Send_CMD_UART((uint8_t*)&Chip_ID, sizeof(Chip_ID));
}

static HAL_StatusTypeDef _Erase_Flash(uint8_t* const pBuffer){
	uint32_t PageAddress = *((uint32_t*)&pBuffer[2]);
	uint8_t page_Number = pBuffer[6];

	uint8_t data = (uint8_t)_Perform_Flash_Erase(PageAddress, page_Number);
	return _Send_CMD_UART(&data, sizeof(data));
}

static HAL_StatusTypeDef _Write_Flash(uint8_t* const pBuffer){
	static uint16_t index = 0;
	HAL_StatusTypeDef Ret_State = HAL_ERROR;
	uint16_t data_size = 0;
	uint32_t Address_Host = 0;

	Address_Host = *((uint32_t*)&pBuffer[2]) + 64 * index++;
	data_size = pBuffer[6];
	Ret_State = _Address_Varification(Address_Host);

	if(Ret_State == HAL_OK){
		Ret_State = _Paylaod_Write((uint16_t*)&pBuffer[7], Address_Host, data_size);
	}
	uint8_t data = (uint8_t)Ret_State;
	_Send_CMD_UART(&data, sizeof(data));
	return Ret_State;
}

static HAL_StatusTypeDef _Address_Varification(uint32_t Address){
	HAL_StatusTypeDef Ret_State = HAL_ERROR;

	switch(Address){
		case SRAM_BASE ... STM32F103_SRAM_END:
		case FLASH_BASE ... STM32F103_FLASH_END:
			Ret_State = HAL_OK;
			break;
		default:
			Ret_State = HAL_ERROR;
			break;
	}

	return Ret_State;
}

static HAL_StatusTypeDef _Paylaod_Write(uint16_t *pdata, uint32_t StartAddress, uint8_t Payloadlen){
	HAL_StatusTypeDef Ret_State=HAL_ERROR;
	uint32_t Address = 0;

	HAL_FLASH_Unlock();
	Payloadlen /= 2;
	for(uint8_t i = 0, UpdataAdress = 0 ; i<Payloadlen ; i++, UpdataAdress+=2){
		Address =  StartAddress + UpdataAdress;
		Ret_State = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address, pdata[i]);
		if(Ret_State != HAL_OK){
			break;
		}
	}
	HAL_FLASH_Lock();

	return Ret_State;
}

static BL_tenuFlashState _Perform_Flash_Erase(uint32_t PageAddress, uint8_t page_Number){
	BL_tenuFlashState PageStatus = INVALID_PAGE_NUMBER;
	uint32_t PageError = 0;

	if(page_Number > FLASH_PAGES_NUM || PageAddress != BL_FLASH_MASS_ERASE){
		PageStatus=INVALID_PAGE_NUMBER;
	}else{
		PageStatus=VALID_PAGE_NUMBER;

		hflash.TypeErase = FLASH_TYPEERASE_PAGES;
		hflash.Banks = FLASH_BANK_1;
		if(PageAddress == BL_FLASH_MASS_ERASE ){
			hflash.PageAddress = STM32F103_BANK1_BASE;
			hflash.NbPages = 12;	/* TODO: Page Number*/
		}else{
			hflash.PageAddress = PageAddress;
			hflash.NbPages = page_Number;
		}

		HAL_FLASH_Unlock();
		HAL_FLASHEx_Erase(&hflash, &PageError);
		HAL_FLASH_Lock();

		if(PageError == HAL_SUCCESSFUL_ERASE){
			PageStatus = SUCCESSFUL_ERASE;
		}else{
			PageStatus = UNSUCCESSFUL_ERASE;
		}
	}
	return PageStatus;
}

/* ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** *
 * ************************************************************************************************************************************** */

HAL_StatusTypeDef BL_Fetch_Host_Command(void){
	HAL_StatusTypeDef Ret_State = HAL_OK;

	memset(pMsgBuffer, 0, BUFFER_MAX_LENGTH);
	Ret_State = _Reciece_UART((uint8_t*) pMsgBuffer);

	if(Ret_State == HAL_OK){
		switch((BL_tenuCMD)pMsgBuffer[1]){	// State Machine
			case BL_GET_VER_CMD:			Ret_State = _Get_Version();					break;
			case BL_GET_HELP_CMD:			Ret_State = _Get_Help();					break;
			case BL_GET_CID_CMD:			Ret_State = _Get_Chip_ID();					break;
			case BL_GO_TO_ADDR_CMD:			Ret_State = _Send_UART("jump to address");	break;
			case BL_FLASH_ERASE_CMD:		Ret_State = _Erase_Flash(pMsgBuffer);		break;
			case BL_MEM_WRITE_CMD:			Ret_State = _Write_Flash(pMsgBuffer);		break;
			default:
				Ret_State = _Send_UART("Hello Bootloader");
				break;
		}
	}else{
		_Send_NACK();
	}

	return Ret_State;
}

void BL_Jump_App(void){
	tpFunc Reset_Function = (tpFunc) (RESET_HANDLER_ADD);
	HAL_RCC_DeInit();
	__set_MSP(MCU_REG_ADD);
	Reset_Function();
}

