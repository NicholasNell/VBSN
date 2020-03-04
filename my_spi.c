/*
 * my_spi.c
 *
 *  Created on: 19 Feb 2020
 *      Author: nicholas
 */
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "my_spi.h"

const eUSCI_SPI_MasterConfig spiMasterConfig =
{
     EUSCI_B_SPI_CLOCKSOURCE_SMCLK,         // SMCLK clock.
     3000000,                                                            // SMCLK = DCO = 3MHz
     1000000,                                                                 // SPICLK = 500kHz
     EUSCI_B_SPI_MSB_FIRST,                               // MSB first
     EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT,   //phase
     EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH,
     EUSCI_B_SPI_3PIN
};

void spi_open(void)
{
//    Setting up SPI mode and opening
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
                                                   GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

//    Chip select (NSS) Port 5 Pin 2 set High
    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN2);

    SPI_initMaster(EUSCI_B0_BASE,&spiMasterConfig);

    SPI_enableModule(EUSCI_B0_BASE);

}

void spi_close(void)
{
    // Disable SPI module
    SPI_disableModule(EUSCI_B0_BASE);
}

void spiWrite_RFM(uint16_t addr, uint8_t val)
{
    RFM95_NSS_LOW;
    uint8_t address = addr | RFM_SPI_WRITE_MASK;   //  read: mask 0x80 for write access; mask 0x7F for read access
    RFM_spi_read_write(address);
    val = RFM_spi_read_write(val);
    RFM95_NSS_HIGH;

}

uint8_t spiRead_RFM(uint16_t addr)
{
    uint8_t val;
    RFM95_NSS_LOW;  // To start SPI coms to rmf95 nss must be pulled low
    uint8_t value_to_send = addr & ~RFM_SPI_WRITE_MASK;   //  read: mask 0x80 for write access; mask 0x7F
    val = RFM_spi_read_write(value_to_send);
    val = RFM_spi_read_write(0);
    RFM95_NSS_HIGH; // To stop SPI coms to rmf95 nss must be pulled high
    return val;
}

int RFM_spi_read_write(uint8_t pBuff)
{
    int RXData;
    while (!(SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_SPI_TRANSMIT_INTERRUPT)));
    /* Transmitting data to slave */
    SPI_transmitData(EUSCI_B0_BASE, pBuff);
    while (!(SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT)));
    RXData = SPI_receiveData(EUSCI_B0_BASE);

    return RXData;
}
