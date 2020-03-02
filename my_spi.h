/*
 * my_spi.h
 *
 *  Created on: 19 Feb 2020
 *      Author: nicholas
 */

#ifndef MY_SPI_H_
#define MY_SPI_H_

#define RFM_SPI_WRITE_MASK 0x80
#define RFM95_NSS_HIGH GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2)
#define RFM95_NSS_LOW GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2)


void spi_open(void);
void spi_close(void);
uint8_t spiRead_RFM(uint8_t addr);
void spiWrite_RFM(uint8_t addr, uint8_t val);
int RFM_spi_read_write(uint8_t pBuff);


#endif /* MY_SPI_H_ */
