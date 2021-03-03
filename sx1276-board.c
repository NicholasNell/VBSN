/*
 * sx1276-board.c
 *
 *  Created on: 03 Mar 2020
 *      Author: nicholas
 */

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/drivers/GPIO.h>
#include <string.h>
#include <stdint.h>
#include "radio.h"
#include "sx1276-board.h"
#include "board-config.h"
#include "my_spi.h"
#include "my_timer.h"
#include "my_RFM9x.h"
#include "sx1276Regs-Fsk.h"

/*!
 * \brief Gets the board PA selection configuration
 *
 * \param [IN] channel Channel frequency in Hz
 * \retval PaSelect RegPaConfig PaSelect value
 */
static uint8_t SX1276GetPaSelect( uint32_t channel );

extern void SX1276OnDio0Irq( );
extern void SX1276OnDio1Irq( );
extern void SX1276OnDio2Irq( );
extern void SX1276OnDio3Irq( );
//extern void SX1276OnDio4Irq();
//extern void SX1276OnDio5Irq();

/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio = { SX1276Init, SX1276GetStatus, SX1276SetModem,
								SX1276SetChannel, SX1276IsChannelFree,
								SX1276Random, SX1276SetRxConfig,
								SX1276SetTxConfig, SX1276CheckRfFrequency,
								SX1276GetTimeOnAir, SX1276Send, SX1276SetSleep,
								SX1276SetStby,
								SX1276SetRx,
								SX1276StartCad,
								NULL, // dont need contWave
								SX1276ReadRssi, spi_write_rfm, spi_read_rfm,
								SX1276WriteBuffer, SX1276ReadBuffer,
								SX1276SetMaxPayloadLength,
								SX1276SetPublicNetwork, SX1276GetWakeupTime,
								NULL, // void ( *IrqProcess )( void )
								NULL, // void ( *RxBoosted )( uint32_t timeout ) - SX126x Only
								NULL, // void ( *SetRxDutyCycle )( uint32_t rxTime, uint32_t sleepTime ) - SX126x Only
		};

void SX1276IoInit( void ) {

	gpio_init(
			&SX1276.Reset,
			RADIO_RESET,
			PIN_OUTPUT,
			PIN_PUSH_PULL,
			PIN_NO_PULL,
			1);
	gpio_init(&SX1276.NSS, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
	spi_open();

	gpio_init(
			&SX1276.DIO0,
			RADIO_DIO_0,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_PULL_UP,
			0);
	gpio_init(
			&SX1276.DIO1,
			RADIO_DIO_1,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_PULL_UP,
			0);
	gpio_init(
			&SX1276.DIO2,
			RADIO_DIO_2,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_PULL_UP,
			0);
	gpio_init(
			&SX1276.DIO3,
			RADIO_DIO_3,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_PULL_UP,
			0);
//    GpioInit( &SX1276.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
}

/*static void Dio0IrqHandler( void );
 static void Dio1IrqHandler( void );
 static void Dio2IrqHandler( void );
 static void Dio3IrqHandler( void );
 static void Dio4IrqHandler( void );
 static void Dio5IrqHandler( void );

 static Gpio_t *DioIrqs[] = {
 &SX1276.DIO0,
 &SX1276.DIO1,
 &SX1276.DIO2,
 &SX1276.DIO3,
 &SX1276.DIO4,
 &SX1276.DIO5
 };

 static void*ExtIrqHandlers[] = {
 Dio0IrqHandler,
 Dio1IrqHandler,
 Dio2IrqHandler,
 Dio3IrqHandler,
 Dio4IrqHandler,
 Dio5IrqHandler
 };

 static void DioIrqHanlderProcess( uint8_t index )
 {
 if( ( DioIrqs[index] != NULL ) && ( DioIrqs[index]->IrqHandler != NULL ) )
 {
 DioIrqs[index]->IrqHandler( DioIrqs[index]->Context );
 }
 }

 static void Dio0IrqHandler( void )
 {
 DioIrqHanlderProcess( 0 );
 }

 static void Dio1IrqHandler( void )
 {
 DioIrqHanlderProcess( 1 );
 }

 static void Dio2IrqHandler( void )
 {
 DioIrqHanlderProcess( 2 );
 }

 static void Dio3IrqHandler( void )
 {
 DioIrqHanlderProcess( 3 );
 }

 static void Dio4IrqHandler( void )
 {
 DioIrqHanlderProcess( 4 );
 }

 static void Dio5IrqHandler( void )
 {
 DioIrqHanlderProcess( 5 );
 }*/

/*static void IoIrqInit( uint8_t index, DioIrqHandler *irqHandler )
 {
 DioIrqs[index]->IrqHandler = irqHandler;
 GPIO_registerInterrupt(DioIrqs[index]->portIndex, ExtIrqHandlers[index]);
 }*/

void SX1276IoIrqInit( void ) {
	gpio_set_interrupt(&SX1276.DIO0, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY);
	gpio_set_interrupt(&SX1276.DIO1, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY);
	gpio_set_interrupt(&SX1276.DIO2, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY);
	gpio_set_interrupt(&SX1276.DIO3, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY);
//	GpioSetInterrupt(&SX1276.DIO4, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY);
//	GpioSetInterrupt(&SX1276.DIO5, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY);
}

void SX1276IoDeInit( void ) {
	gpio_init(&SX1276.NSS, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1);
	spi_close();

	gpio_init(
			&SX1276.DIO0,
			RADIO_DIO_0,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_NO_PULL,
			0);
	gpio_init(
			&SX1276.DIO1,
			RADIO_DIO_1,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_NO_PULL,
			0);
	gpio_init(
			&SX1276.DIO2,
			RADIO_DIO_2,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_NO_PULL,
			0);
	gpio_init(
			&SX1276.DIO3,
			RADIO_DIO_3,
			PIN_INPUT,
			PIN_PUSH_PULL,
			PIN_NO_PULL,
			0);
//	GpioInit(&SX1276.DIO4, RADIO_DIO_4, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL,
//			0);
//	GpioInit(&SX1276.DIO5, RADIO_DIO_5, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL,
//			0);
}

void SX1276IoDbgInit( void ) {

}

void SX1276IoTcxoInit( void ) {
	// No TCXO component available on this board design.
}

void SX1276SetBoardTcxo( uint8_t state ) {
}

uint32_t SX1276GetBoardTcxoWakeupTime( void ) {
	return BOARD_TCXO_WAKEUP_TIME;
}

void SX1276Reset( void ) {
	gpio_write(&SX1276.Reset, 0);
//    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);
	delay_ms(5);
	gpio_write(&SX1276.Reset, 1);
//    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
	delay_ms(6);
}

void SX1276SetRfTxPower( int8_t power ) {
	uint8_t paConfig = 0;
	uint8_t paDac = 0;

	paConfig = spi_read_rfm( REG_PACONFIG);
	paDac = spi_read_rfm( REG_PADAC);

	paConfig = (paConfig & RF_PACONFIG_PASELECT_MASK)
			| SX1276GetPaSelect(power);

	if ((paConfig & RF_PACONFIG_PASELECT_PABOOST)
			== RF_PACONFIG_PASELECT_PABOOST) {
		if (power > 17) {
			paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_ON;
		}
		else {
			paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_OFF;
		}
		if ((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON) {
			if (power < 5) {
				power = 5;
			}
			if (power > 20) {
				power = 20;
			}
			paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK)
					| (uint8_t) ((uint16_t) (power - 5) & 0x0F);
		}
		else {
			if (power < 2) {
				power = 2;
			}
			if (power > 17) {
				power = 17;
			}
			paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK)
					| (uint8_t) ((uint16_t) (power - 2) & 0x0F);
		}
	}
	else {
		if (power > 0) {
			if (power > 15) {
				power = 15;
			}
			paConfig = (paConfig & RF_PACONFIG_MAX_POWER_MASK
					& RF_PACONFIG_OUTPUTPOWER_MASK) | (7 << 4) | (power);
		}
		else {
			if (power < -4) {
				power = -4;
			}
			paConfig = (paConfig & RF_PACONFIG_MAX_POWER_MASK
					& RF_PACONFIG_OUTPUTPOWER_MASK) | (0 << 4) | (power + 4);
		}
	}
	spi_write_rfm( REG_PACONFIG, paConfig);
	spi_write_rfm( REG_PADAC, paDac);
}

static uint8_t SX1276GetPaSelect( uint32_t channel ) {
	return RF_PACONFIG_PASELECT_PABOOST;
}

void SX1276SetAntSw( uint8_t opMode ) {
	// No antenna switch available
}

bool SX1276CheckRfFrequency( uint32_t frequency ) {
	// Implement check. Currently all frequencies are supported
	return true;
}

#if defined( USE_RADIO_DEBUG )
void SX1276DbgPinTxWrite( uint8_t state )
{
    gpio_write( &DbgPinTx, state );
}

void SX1276DbgPinRxWrite( uint8_t state )
{
    gpio_write( &DbgPinRx, state );
}
#endif
