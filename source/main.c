/*  SDK Included Files */

#include "fsl_debug_console.h"
#include "fsl_i2c.h"
#include <stdio.h>
#include "fsl_device_registers.h"
#include "Driver_I2C.h"
#include "fsl_i2c_cmsis.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "math.h"
#include "serial.h"
#include "fsl_lpsci.h"
#include "fsl_lpsci_dma.h"
#include "clock_config.h"

uint8_t txbuff[] = "LPSCI polling example\r\nSend back received data\r\n";
uint8_t rxbuff[20] = {0};


/* LPSCI user callback */
void LPSCI_UserCallback(UART0_Type *base, lpsci_dma_handle_t *handle,
		status_t status, void *userData);

//lpsci komunikacia

lpsci_dma_handle_t g_lpsciDmaHandle;
dma_handle_t g_lpsciTxDmaHandle;
dma_handle_t g_lpsciRxDmaHandle;
uint8_t g_tipString[] =
		"LPSCI DMA example\r\nSend back received data\r\nEcho every 8 characters\r\n";
uint8_t g_txBuffer[ECHO_BUFFER_LENGTH] = { 0 };
uint8_t g_rxBuffer[ECHO_BUFFER_LENGTH] = { 0 };
volatile bool rxBufferEmpty = true;
volatile bool txBufferFull = false;
volatile bool txOnGoing = false;
volatile bool rxOnGoing = false;

lpsci_dma_handle_t g_lpsciDmaHandle;
lpsci_transfer_t xfer;

void LPSCI_UserCallback(UART0_Type *base, lpsci_dma_handle_t *handle,
		status_t status, void *userData) {
	userData = userData;

	if (kStatus_LPSCI_TxIdle == status) {
		txBufferFull = false;
		txOnGoing = false;
	}

	if (kStatus_LPSCI_RxIdle == status) {
		rxBufferEmpty = false;
		rxOnGoing = false;
	}
}

static void i2c_release_bus_delay(void) {
	uint32_t i = 0;
	for (i = 0; i < I2C_RELEASE_BUS_COUNT; i++) {
		__NOP();
	}
}

void BOARD_I2C_ReleaseBus(void) {
	uint8_t i = 0;
	gpio_pin_config_t pin_config;
	port_pin_config_t i2c_pin_config = { 0 };

	/* Config pin mux as gpio */
	i2c_pin_config.pullSelect = kPORT_PullUp;
	i2c_pin_config.mux = kPORT_MuxAsGpio;

	pin_config.pinDirection = kGPIO_DigitalOutput;
	pin_config.outputLogic = 1U;
	CLOCK_EnableClock(kCLOCK_PortE);
	PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SCL_PIN,
			&i2c_pin_config);
	PORT_SetPinConfig(I2C_RELEASE_SDA_PORT, I2C_RELEASE_SDA_PIN,
			&i2c_pin_config);

	GPIO_PinInit(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, &pin_config);
	GPIO_PinInit(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, &pin_config);

	/* Drive SDA low first to simulate a start */
	GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
	i2c_release_bus_delay();

	/* Send 9 pulses on SCL and keep SDA low */
	for (i = 0; i < 9; i++) {
		GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
		i2c_release_bus_delay();

		GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
		i2c_release_bus_delay();

		GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
		i2c_release_bus_delay();
		i2c_release_bus_delay();
	}

	/* Send stop */
	GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
	i2c_release_bus_delay();

	GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
	i2c_release_bus_delay();

	GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
	i2c_release_bus_delay();

	GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
	i2c_release_bus_delay();
}


/**
 * @todo Konfiguruj LPSCI prenos todo
 */
void LPSI_conf(){
		lpsci_config_t config;
		CLOCK_SetLpsci0Clock(0x1U);
		LPSCI_GetDefaultConfig(&config);
		config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
		config.enableTx = true;
		config.enableRx = true;
		LPSCI_Init(DEMO_LPSCI, &config, DEMO_LPSCI_CLK_FREQ);
		DMAMUX_Init(EXAMPLE_LPSCI_DMAMUX_BASEADDR);
		DMAMUX_SetSource(EXAMPLE_LPSCI_DMAMUX_BASEADDR, LPSCI_TX_DMA_CHANNEL,
				LPSCI_TX_DMA_REQUEST);
		DMAMUX_EnableChannel(EXAMPLE_LPSCI_DMAMUX_BASEADDR, LPSCI_TX_DMA_CHANNEL);
		DMAMUX_SetSource(EXAMPLE_LPSCI_DMAMUX_BASEADDR, LPSCI_RX_DMA_CHANNEL,
				LPSCI_RX_DMA_REQUEST);
		DMAMUX_EnableChannel(EXAMPLE_LPSCI_DMAMUX_BASEADDR, LPSCI_RX_DMA_CHANNEL);
		DMA_Init(EXAMPLE_LPSCI_DMA_BASEADDR);
		DMA_CreateHandle(&g_lpsciTxDmaHandle, EXAMPLE_LPSCI_DMA_BASEADDR,
				LPSCI_TX_DMA_CHANNEL);
		DMA_CreateHandle(&g_lpsciRxDmaHandle, EXAMPLE_LPSCI_DMA_BASEADDR,
				LPSCI_RX_DMA_CHANNEL);
		LPSCI_TransferCreateHandleDMA(DEMO_LPSCI, &g_lpsciDmaHandle,
				LPSCI_UserCallback, NULL, &g_lpsciTxDmaHandle, &g_lpsciRxDmaHandle);

}

/**
 * @param address je adresa kcd chcem packet poslat
 * @param size je velkost dat
 * @param data je obsah správy
 * @todo Poši packet zadanej velkosti na danu adresu
 */
void sendPacket(char address, char size, char data){
	char pole[6];
	char vypocet[3];
	pole[0] = 0xa0;
	pole[1] = address;
	pole[2] = 0x00;
	pole[3] = size;
	pole[4] = data;

	vypocet[0] = address;
	vypocet[1] = 0x00;
	vypocet[2] = data;

	pole[5] = crc8_calc(vypocet, 3);

	xfer.data = pole;
	xfer.dataSize = 6;
	txOnGoing = true;
	LPSCI_TransferSendDMA(DEMO_LPSCI, &g_lpsciDmaHandle, &xfer);

	int z = 0;
	while (z < 5000000) {
		z++;
	}

}

/**
 * @param address je adresa na ktoru posielam potvrzovací packet ACK
 * @todo Posli potvrzovací packet na danu adresu todo
 */
void sendAck(char address){
	char pole[5];
	char vypocet[2];
	pole[0] = 0xa1;
	pole[1] = address;
	pole[2] = 0x00;
	pole[3] = 0x00;


	vypocet[0] = address;
	vypocet[1] = 0x00;


	pole[4] = crc8_calc(vypocet, 2);

	xfer.data = pole;
	xfer.dataSize = 5;
	txOnGoing = true;
	LPSCI_TransferSendDMA(DEMO_LPSCI, &g_lpsciDmaHandle, &xfer);
	PRINTF("ACK SENT: %x\n", address);

	int z = 0;
	while (z < 500000) {
		z++;
	}

}

/**
 * @param data je požadovaná rychlost výtahu
 * @todo Nastav rýchlosť výtahu na požadovanú hodnotu todo
 */
void elevatorSpeed(int32_t data){
	char pole[10];
	char vypocet[7];

	pole[0] = 0xa0;
	pole[1] = 0xf1;
	pole[2] = 0x00;
	pole[3] = 0x05;
	memcpy(pole+5, &data, 4);

	//co vyžadujem po motore
	pole[4] = 0x02;

	vypocet[0] = 0xf1;
	vypocet[1] = 0x00;
	vypocet[2] = pole[4];
	vypocet[3] = pole[5];
	vypocet[4] = pole[6];
	vypocet[5] = pole[7];
	vypocet[6] = pole[8];


	pole[9] = crc8_calc(vypocet, 7);

	switchOnDisplay(data);


	xfer.data = pole;
	xfer.dataSize = 10;
	txOnGoing = true;
	LPSCI_TransferSendDMA(DEMO_LPSCI, &g_lpsciDmaHandle, &xfer);

	int z = 0;
	while (z < 3000000) {
		z++;
	}

}


/**
 * @todo Konfiguruj sériovú komunikáciu todo
 *
 */
void serialConf(){
	lpsci_config_t config;
	CLOCK_SetLpsci0Clock(0x1U);
	LPSCI_GetDefaultConfig(&config);
	config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;

	LPSCI_Init(DEMO_LPSCI, &config, DEMO_LPSCI_CLK_FREQ);
	LPSCI_EnableTx(DEMO_LPSCI, true);
	LPSCI_EnableRx(DEMO_LPSCI, true);

	LPSCI_WriteBlocking(DEMO_LPSCI, txbuff, sizeof(txbuff) - 1);
}



int main(void) {

	uint8_t ch;
	BOARD_InitPins();
	BOARD_BootClockRUN();
	BOARD_I2C_ReleaseBus();
	LPSI_conf();
	serialConf();


	char addr = 0x00;
	int volba;
	uint8_t mess[7];
	int fl = 0;


	while(true){
	while (fl == 0)
	    {

			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1); //A0
			PRINTF("%d", ch);
			PRINTF("READ...");


	     	LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);//0x00
	     	PRINTF("ch = %d\n", ch);
			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);//0x00
			PRINTF("ch = %d\n", ch);
			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
			mess[0] = ch;
			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);

			mess[1] = ch;
			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
			mess[2] = ch;
			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
			mess[3] = ch;
			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
			mess[4] = ch;
			LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
			mess[5] = ch;

			PRINTF("0B: %x\n", mess[0]); //Adresa
			PRINTF("1B: %x\n", mess[1]); //Adresa
			PRINTF("2B: %x\n", mess[2]); //NULL
			PRINTF("3B: %x\n", mess[3]); //size
			PRINTF("4B: %x\n", mess[4]); //size
			PRINTF("5B: %x\n", mess[5]); //size
			if (mess[0] == 160 && mess[1] == 0){
				fl = 2;
			}

			addr = mess[2];
			sendAck(addr);
	    }

	while (fl == 1)
	{

		     	LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);//0x00
		     	PRINTF("ch = %d", ch);
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);//0x00
				PRINTF(": ch = %d\n", ch);
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[0] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);

				mess[1] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[2] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[3] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[4] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[5] = ch;

				PRINTF("0B: %x\n", mess[0]); //Adresa
				PRINTF("1B: %x\n", mess[1]); //Adresa
				PRINTF("2B: %x\n", mess[2]); //NULL
				PRINTF("3B: %x\n", mess[3]); //size
				PRINTF("4B: %x\n", mess[4]); //size
				PRINTF("5B: %x\n", mess[5]); //size
				if (mess[0] == 160 && mess[1] == 0){
					fl = 2;
				}

				addr = mess[2];
				sendAck(addr);
		    }


				if (addr == 0xc4 || addr == 0xb4){
					volba = 4;
					PRINTF("Vybrali ste kabinu 4\n");
					sendPacket(0x14,0x01,0x01);
				}

				if (addr == 0xc3 || addr == 0xb3){
					volba = 3;
					PRINTF("Vybrali ste kabinu 3\n");
					sendPacket(0x13,0x01,0x01);
				}

				if (addr == 0xc2 || addr == 0xb2){
					volba = 2;
					PRINTF("Vybrali ste kabinu 2\n");
					sendPacket(0x12,0x01,0x01);
				}

				if (addr == 0xc1 || addr == 0xb1){
					volba = 1;
					PRINTF("Vybrali ste kabinu 1\n");
					sendPacket(0x11,0x01,0x01);
				}

				if (addr == 0xc0 || addr == 0xb0){
					volba = 0;
					PRINTF("Vybrali ste kabinu na prizemi\n");
					sendPacket(0x10,0x01,0x01);
				}

		switch (volba) {
		case 4:
			PRINTF("Kabina 4 v pohybe\n");
			sendPacket(0xf,0x01,0x00);
			cabinLock();
			wait();
			elevatorSpeed(75);
			PRINTF("Kabina zavreta a rychlost vytahu bude == 75\n");
			int i = 0;

			while (i < 2) {
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[0] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[1] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[2] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[3] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[4] = ch;
				LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
				mess[5] = ch;

				PRINTF("0B: %x\n", mess[0]); //Adresa
				PRINTF("1B: %x\n", mess[1]); //Adresa
				PRINTF("2B: %x\n", mess[2]); //NULL
				PRINTF("3B: %x\n", mess[3]); //size
				PRINTF("4B: %x\n", mess[4]); //size
				PRINTF("5B: %x\n", mess[5]); //size

				char where = mess[2];
				char command = mess[4];
				PRINTF("Kde = %x\n", mess[2]);
				PRINTF("Co = %x\n", mess[4]);

				if (where == 0xe4 && command == 0x02) {
					PRINTF("Vytah je blizko switch 4\n");
					elevatorSpeed(10);
					i = 1;
				} else {

					sendAck(where);

				}
				if (where == 0xe4 && command == 0x01) {
					PRINTF("Vytah zastal\n");
					elevatorStop();
					sendPacket(0x14,0x01,0x00);
					cabinUnlock();
					switchOffDisplay();
					i = 4;

				} else {

					sendAck(where);
				}
				volba = 10;
				addr = 0x00;

			}
			volba = 10;
			addr = 0x00;
			fl = 1;

			break;

		case 3:
						PRINTF("case 3 started\n");
				        sendPacket(0xf,0x01,0x00);
						cabinLock();
						wait();
						elevatorSpeed(75);
						PRINTF("cabin locked & elevator speed == 75\n");
						i = 0;

						while (i < 2) {
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							mess[0] = ch;
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							mess[1] = ch;
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							mess[2] = ch;
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							mess[3] = ch;
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							mess[4] = ch;
							LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
							mess[5] = ch;

							PRINTF("0B: %x\n", mess[0]); //Adresa
							PRINTF("1B: %x\n", mess[1]); //Adresa
							PRINTF("2B: %x\n", mess[2]); //NULL
							PRINTF("3B: %x\n", mess[3]); //size
							PRINTF("4B: %x\n", mess[4]); //size
							PRINTF("5B: %x\n", mess[5]); //size

							char where = mess[2];
							char command = mess[4];
							PRINTF("Where = %x\n", mess[2]);
							PRINTF("What = %x\n", mess[4]);

							if (where == 0xe3 && command == 0x02) {
								PRINTF("elevator is close to switch 1\n");
								elevatorSpeed(10);
								i = 1;
								} else {

							sendAck(where);

							   }
							if (where == 0xe3 && command == 0x01) {
								PRINTF("elevator stop\n");
								elevatorStop();
								sendPacket(0x13,0x01,0x00);
								cabinUnlock();
								switchOffDisplay();
								i = 3;

								} else {

									sendAck(where);
							   }
								volba = 10;
								addr = 0x00;

						}
								volba = 10;
								addr = 0x00;
								fl = 1;

						break;
		case 2:
							PRINTF("case 2 started\n");
							sendPacket(0xf,0x01,0x00);
							cabinLock();
							wait();
							elevatorSpeed(75);
							PRINTF("cabin locked & elevator speed == 75\n");
						    i = 0;

							while (i < 2) {
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								mess[0] = ch;
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								mess[1] = ch;
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								mess[2] = ch;
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								mess[3] = ch;
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								mess[4] = ch;
								LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
								mess[5] = ch;

								PRINTF("0B: %x\n", mess[0]); //Adresa
								PRINTF("1B: %x\n", mess[1]); //Adresa
								PRINTF("2B: %x\n", mess[2]); //NULL
								PRINTF("3B: %x\n", mess[3]); //size
								PRINTF("4B: %x\n", mess[4]); //size
								PRINTF("5B: %x\n", mess[5]); //size

								char where = mess[2];
								char command = mess[4];
								PRINTF("Where = %x\n", mess[2]);
								PRINTF("What = %x\n", mess[4]);

								if (where == 0xe2 && command == 0x02) {
									PRINTF("elevator is close to switch 2\n");
									elevatorSpeed(10);
									i = 1;
								} else {

									sendAck(where);

								}
								if (where == 0xe2 && command == 0x01) {
									PRINTF("elevator stop\n");
									elevatorStop();
									sendPacket(0x12,0x01,0x00);
									cabinUnlock();
									switchOffDisplay();
									i = 2;

								} else {

									sendAck(where);
								}
								volba = 10;
								addr = 0x00;

							}
							volba = 10;
							addr = 0x00;
							fl = 1;

							break;

		case 1:
				PRINTF("case 1 started\n");
		        sendPacket(0xf,0x01,0x00);
				cabinLock();
				wait();
				elevatorSpeed(75);
				PRINTF("cabin locked & elevator speed == 75\n");
				i = 0;

				while (i < 2) {
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[0] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[1] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[2] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[3] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[4] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[5] = ch;

					PRINTF("0B: %x\n", mess[0]); //Adresa
					PRINTF("1B: %x\n", mess[1]); //Adresa
					PRINTF("2B: %x\n", mess[2]); //NULL
					PRINTF("3B: %x\n", mess[3]); //size
					PRINTF("4B: %x\n", mess[4]); //size
					PRINTF("5B: %x\n", mess[5]); //size

					char where = mess[2];
					char command = mess[4];
					PRINTF("Where = %x\n", mess[2]);
					PRINTF("What = %x\n", mess[4]);

					if (where == 0xe1 && command == 0x02) {
						PRINTF("elevator is close to switch 1\n");
						elevatorSpeed(10);
						i = 1;
						} else {

					sendAck(where);

					   }
					if (where == 0xe1 && command == 0x01) {
						PRINTF("elevator stop\n");
						elevatorStop();
						sendPacket(0x11,0x01,0x00);
						cabinUnlock();
						switchOffDisplay();
						i = 2;

						} else {

							sendAck(where);
					   }
						volba = 10;
						addr = 0x00;

				}
						volba = 10;
						addr = 0x00;
						fl = 1;

				break;
		case 0:
				PRINTF("Kabina 1 v pohybe\n");
				cancelBreak();
				cabinLock();
				wait();

				elevatorSpeed(-75);
				PRINTF("cabin locked & elevator speed == -75\n");
				i = 0;

				while (i < 2) {

					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[0] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[1] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[2] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[3] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[4] = ch;
					LPSCI_ReadBlocking(DEMO_LPSCI, &ch, 1);
					mess[5] = ch;

					PRINTF("0B: %x\n", mess[0]); //Adresa
					PRINTF("1B: %x\n", mess[1]); //Adresa
					PRINTF("2B: %x\n", mess[2]); //NULL
					PRINTF("3B: %x\n", mess[3]); //size
					PRINTF("4B: %x\n", mess[4]); //size
					PRINTF("5B: %x\n", mess[5]); //size

					char where = mess[2];
					char command = mess[4];
					PRINTF("Where = %x\n", mess[2]);
					PRINTF("What = %x\n", mess[4]);

					if (where == 0xe0 && command == 0x02) {
						PRINTF("elevator is close to switch 0\n");
						elevatorSpeed(-10);
						i = 1;
					} else {

						sendAck(where);


					}
					if (where == 0xe0 && command == 0x01) {
						PRINTF("elevator stop\n");
						elevatorStop();
						sendPacket(0x10,0x01,0x00);
						cabinUnlock();
						switchOffDisplay();
						i = 4;
					} else {

						sendAck(where);
					}
					volba = 10;
					addr = 0x00;

				}
				fl = 1;

		}
		fl = 1;
	}
}




