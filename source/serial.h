/*
 * serial.h
 *
 *  Created on: 5. 12. 2019
 *      Author: Dell
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include "board.h"


#define EXAMPLE_I2C_MASTER Driver_I2C0
#define I2C_RELEASE_SDA_PORT PORTE
#define I2C_RELEASE_SCL_PORT PORTE
#define I2C_RELEASE_SDA_GPIO GPIOE
#define I2C_RELEASE_SDA_PIN 25U
#define I2C_RELEASE_SCL_GPIO GPIOE
#define I2C_RELEASE_SCL_PIN 24U
#define I2C_RELEASE_BUS_COUNT 100U
#define FXOS8700_WHOAMI 0xC7U
#define MMA8451_WHOAMI 0x1AU
#define ACCEL_STATUS 0x00U
#define ACCEL_XYZ_DATA_CFG 0x0EU
#define ACCEL_CTRL_REG1 0x2AU
#define ACCEL_XYZ_DATA 0x01
#define ALPHA 0.059

#define ACCEL_WHOAMI_REG 0x0DU
#define ACCEL_READ_TIMES 10U

#define DEMO_LPSCI UART0
#define DEMO_LPSCI_CLKSRC kCLOCK_CoreSysClk
#define DEMO_LPSCI_CLK_FREQ CLOCK_GetFreq(kCLOCK_CoreSysClk)

#define LPSCI_TX_DMA_CHANNEL 0U
#define LPSCI_RX_DMA_CHANNEL 1U
#define LPSCI_TX_DMA_REQUEST kDmaRequestMux0LPSCI0Tx
#define LPSCI_RX_DMA_REQUEST kDmaRequestMux0LPSCI0Rx
#define EXAMPLE_LPSCI_DMAMUX_BASEADDR DMAMUX0
#define EXAMPLE_LPSCI_DMA_BASEADDR DMA0
#define ECHO_BUFFER_LENGTH 5





unsigned char crc8_calc(unsigned char data[], size_t elements);
void wait();


void cancelBreak();
void elevatorStop();
void cabinLock();
void cabinUnlock();

#endif /* SERIAL_H_ */
