/*
*         Copyright (c), NXP Semiconductors Caen / France
*
*                     (C)NXP Semiconductors
*       All rights are reserved. Reproduction in whole or in part is
*      prohibited without the written consent of the copyright owner.
*  NXP reserves the right to make changes without notice at any time.
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
*particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
*                          arising from its use.
*/

#include <stdint.h>
#include "board.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "fsl_i2c.h"
#include "fsl_i2c_freertos.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include <tool.h>

SemaphoreHandle_t IrqSem = NULL;
i2c_master_transfer_t masterXfer;

typedef enum {ERROR = 0, SUCCESS = !ERROR} Status;

void PORTC_IRQHandler(void)
{
	if (GPIO_ReadPinInput(BOARD_NXPNCI_IRQ_GPIO, BOARD_NXPNCI_IRQ_PIN) == 1)
	{
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		GPIO_ClearPinsInterruptFlags(BOARD_NXPNCI_IRQ_GPIO, 1U << BOARD_NXPNCI_IRQ_PIN);
		xSemaphoreGiveFromISR(IrqSem, &xHigherPriorityTaskWoken);
	}
}

static status_t I2C_WRITE(uint8_t *pBuff, uint16_t buffLen)
{
    masterXfer.direction = kI2C_Write;
    masterXfer.data = pBuff;
    masterXfer.dataSize = buffLen;

    return I2C_MasterTransferBlocking(BOARD_NXPNCI_I2C_INSTANCE, &masterXfer);
}

static status_t I2C_READ(uint8_t *pBuff, uint16_t buffLen)
{
    masterXfer.direction = kI2C_Read;
    masterXfer.data = pBuff;
    masterXfer.dataSize = buffLen;

    return I2C_MasterTransferBlocking(BOARD_NXPNCI_I2C_INSTANCE, &masterXfer);
}

static Status tml_Init(void) {
    i2c_master_config_t masterConfig;
    uint32_t sourceClock;

    gpio_pin_config_t irq_config = {kGPIO_DigitalInput, 0,};
    gpio_pin_config_t ven_config = {kGPIO_DigitalOutput, 0,};

    GPIO_PinInit(BOARD_NXPNCI_IRQ_GPIO, BOARD_NXPNCI_IRQ_PIN, &irq_config);
    GPIO_PinInit(BOARD_NXPNCI_VEN_GPIO, NXPNCI_VEN_PIN, &ven_config);

    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = BOARD_NXPNCI_I2C_BAUDRATE;
    sourceClock = CLOCK_GetFreq(I2C0_CLK_SRC);
    masterXfer.slaveAddress = BOARD_NXPNCI_I2C_ADDR;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    I2C_MasterInit(BOARD_NXPNCI_I2C_INSTANCE, &masterConfig, sourceClock);

    IrqSem = xSemaphoreCreateBinary();

    return SUCCESS;
}

static Status tml_DeInit(void) {
	vSemaphoreDelete(IrqSem);
	GPIO_ClearPinsOutput(BOARD_NXPNCI_VEN_GPIO, 1U << NXPNCI_VEN_PIN);
    return SUCCESS;
}

static Status tml_Reset(void) {
	GPIO_ClearPinsOutput(BOARD_NXPNCI_VEN_GPIO, 1U << NXPNCI_VEN_PIN);
	Sleep(10);
	GPIO_SetPinsOutput(BOARD_NXPNCI_VEN_GPIO, 1U << NXPNCI_VEN_PIN);
	Sleep(10);
	return SUCCESS;
}

static Status tml_Tx(uint8_t *pBuff, uint16_t buffLen) {
    if (I2C_WRITE(pBuff, buffLen) != kStatus_Success)
    {
    	Sleep(10);
    	if(I2C_WRITE(pBuff, buffLen) != kStatus_Success)
    	{
    		return ERROR;
    	}
    }

	return SUCCESS;
}

static Status tml_Rx(uint8_t *pBuff, uint16_t buffLen, uint16_t *pBytesRead) {
    if(I2C_READ(pBuff, 3) == kStatus_Success)
    {
    	if ((pBuff[2] + 3) <= buffLen)
    	{
			if (pBuff[2] > 0)
			{
				if(I2C_READ(&pBuff[3], pBuff[2]) == kStatus_Success)
				{
					*pBytesRead = pBuff[2] + 3;
				}
				else return ERROR;
			} else
			{
				*pBytesRead = 3;
			}
    	}
		else return ERROR;
   }
    else return ERROR;

	return SUCCESS;
}

static Status tml_WaitForRx(uint32_t timeout) {
	if (xSemaphoreTake(IrqSem, (timeout==0)?(portMAX_DELAY):(portTICK_PERIOD_MS*timeout)) != pdTRUE) return ERROR;
	return SUCCESS;
}

void tml_Connect(void) {
	tml_Init();
	tml_Reset();
}

void tml_Disconnect(void) {
	tml_DeInit();
}

void tml_Send(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytesSent) {
	if(tml_Tx(pBuffer, BufferLen) == ERROR) *pBytesSent = 0;
	else *pBytesSent = BufferLen;
}

void tml_Receive(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytes, uint16_t timeout) {
	if (tml_WaitForRx(timeout) == ERROR) *pBytes = 0;
	else tml_Rx(pBuffer, BufferLen, pBytes);
}


