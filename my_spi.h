/*
 * my_spi.h
 *
 *  Created on: 19 Feb 2020
 *      Author: nicholas
 */

#ifndef MY_SPI_H_
#define MY_SPI_H_

#define RFM_SPI_WRITE_MASK 0x80
#define RFM95_NSS_HIGH P5->OUT |= BIT2 //GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2)
#define RFM95_NSS_LOW P5->OUT &= ~BIT2 //GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2)
#define BME280_CS_LOW P3->OUT &= ~BIT6
#define BME280_CS_HIGH P3->OUT |= BIT6

void spi_open(void);
void spi_close(void);
uint8_t spi_read_rfm(uint16_t addr);
void spi_write_rfm(uint16_t addr, uint8_t val);
int spi_read_write(uint8_t pBuff);
int8_t user_spi_write_bme280(uint8_t reg_addr, const uint8_t *reg_data,
		uint32_t len, void *intf_ptr);
int8_t user_spi_read_bme280(uint8_t reg_addr, uint8_t *reg_data, uint32_t len,
		void *intf_ptr);
#endif /* MY_SPI_H_ */
