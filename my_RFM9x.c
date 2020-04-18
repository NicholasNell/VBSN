/*
 * my_RFM9x.c
 *
 *  Created on: 24 Feb 2020
 *      Author: nicholas
 */

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <math.h>
#include <string.h>
#include "my_timer.h"
#include "radio.h"
#include "my_RFM9x.h"
#include "sx1276-board.h"
#include "timer.h"

/*
 * Local types definition
 */

/*!
 * Radio registers definition
 */
typedef struct {
		RadioModems_t Modem;
		uint8_t Addr;
		uint8_t Value;
} RadioRegisters_t;

/*!
 * FSK bandwidth definition
 */
typedef struct {
		uint32_t bandwidth;
		uint8_t RegValue;
} FskBandwidth_t;

/*!
 * \brief Writes the buffer contents to the SX1276 FIFO
 *
 * \param [IN] buffer Buffer containing data to be put on the FIFO.
 * \param [IN] size Number of bytes to be written to the FIFO
 */
void SX1276WriteFifo( uint8_t *buffer, uint8_t size );

/*!
 * \brief Sets the SX1276 in transmission mode for the given time
 * \param [IN] timeout Transmission timeout [ms] [0: continuous, others timeout]
 */
void SX1276SetTx( uint32_t timeout );

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
void RxChainCalibration( void );

/*!
 * \brief Tx & Rx timeout timer callback
 */
void SX1276OnTimeoutIrq( void* context );

/*!
 * \brief Sets the SX1276 operating mode
 *
 * \param [IN] opMode New operating mode
 */
void SX1276SetOpMode( uint8_t opMode );

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

/*!
 * Precomputed FSK bandwidth registers values
 */
const FskBandwidth_t FskBandwidths[] = { { 2600, 0x17 }, { 3100, 0x0F },
											{ 3900, 0x07 }, { 5200, 0x16 }, {
													6300, 0x0E },
											{ 7800, 0x06 }, { 10400, 0x15 }, {
													12500, 0x0D },
											{ 15600, 0x05 }, { 20800, 0x14 }, {
													25000, 0x0C },
											{ 31300, 0x04 }, { 41700, 0x13 }, {
													50000, 0x0B },
											{ 62500, 0x03 }, { 83333, 0x12 }, {
													100000, 0x0A },
											{ 125000, 0x02 }, { 166700, 0x11 },
											{ 200000, 0x09 }, { 250000, 0x01 },
											{ 300000, 0x00 }, // Invalid Bandwidth
		};

/*
 * Private global variables
 */
/*!
 * Radio callbacks variable
 */
static RadioEvents_t *RadioEvents;

/*!
 * Reception buffer
 */
static uint8_t RxTxBuffer[RX_BUFFER_SIZE];
/*
 * Public global variables
 */

/*!
 * Tx and Rx timers
 */
TimerEvent_t TxTimeoutTimer;
TimerEvent_t RxTimeoutTimer;
TimerEvent_t RxTimeoutSyncWord;

/*!
 * Radio hardware and global parameters
 */
SX1276_t SX1276;

/*!
 * Radio hardware registers initialization
 *
 * \remark RADIO_INIT_REGISTERS_VALUE is defined in my_RFM9x.h file
 */
const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

uint8_t GetFskBandwidthRegValue( uint32_t bandwidth ) {
	uint8_t i;

	for (i = 0; i < (sizeof(FskBandwidths) / sizeof(FskBandwidth_t)) - 1; i++) {
		if ((bandwidth >= FskBandwidths[i].bandwidth)
				&& (bandwidth < FskBandwidths[i + 1].bandwidth)) {
			return FskBandwidths[i].RegValue;
		}
	}
	return 0;
}

void SX1276Init( RadioEvents_t *events ) {
	uint8_t i;

	RadioEvents = events;

	// Initialize driver timeout timers
	TimerInit(&TxTimeoutTimer, SX1276OnTimeoutIrq);
	TimerInit(&RxTimeoutTimer, SX1276OnTimeoutIrq);
	TimerInit(&RxTimeoutSyncWord, SX1276OnTimeoutIrq);

	SX1276Reset();

	RxChainCalibration();

	SX1276SetOpMode(RF_OPMODE_SLEEP);

	SX1276IoIrqInit();

	for (i = 0; i < sizeof(RadioRegsInit) / sizeof(RadioRegisters_t); i++) {
		SX1276SetModem(RadioRegsInit[i].Modem);
		spiWrite_RFM(RadioRegsInit[i].Addr, RadioRegsInit[i].Value);
	}

	SX1276SetModem(MODEM_LORA);

	SX1276.Settings.State = RF_IDLE;

//    Check RFM Chip Version
//    uint8_t version = spiRead_RFM(REG_LR_VERSION);
//    if(version != 0x12) return 0;

}

RadioState_t SX1276GetStatus( void ) {
	return SX1276.Settings.State;
}

void SX1276SetModem( RadioModems_t modem ) {
	if ((spiRead_RFM( REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_ON) != 0) {
		SX1276.Settings.Modem = MODEM_LORA;
	}
	else {
		SX1276.Settings.Modem = MODEM_FSK;
	}

	if (SX1276.Settings.Modem == modem) {
		return;
	}

	SX1276.Settings.Modem = modem;
	switch (SX1276.Settings.Modem) {
		default:
		case MODEM_FSK:
			SX1276SetOpMode( RF_OPMODE_SLEEP);
			spiWrite_RFM(
					REG_OPMODE,
					(spiRead_RFM( REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK)
							| RFLR_OPMODE_LONGRANGEMODE_OFF);

			spiWrite_RFM( REG_DIOMAPPING1, 0x00);
			spiWrite_RFM( REG_DIOMAPPING2, 0x30); // DIO5=ModeReady
			break;
		case MODEM_LORA:
			SX1276SetOpMode( RF_OPMODE_SLEEP);
			spiWrite_RFM(
					REG_OPMODE,
					(spiRead_RFM( REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK)
							| RFLR_OPMODE_LONGRANGEMODE_ON);

			spiWrite_RFM( REG_DIOMAPPING1, 0x00);
			spiWrite_RFM( REG_DIOMAPPING2, 0x00);
			break;
	}
}

void SX1276SetChannel( uint32_t freq ) {
	SX1276.Settings.Channel = freq;
	freq = (uint32_t) ((double) freq / (double) FREQ_STEP);
	spiWrite_RFM( REG_FRFMSB, (uint8_t) ((freq >> 16) & 0xFF));
	spiWrite_RFM( REG_FRFMID, (uint8_t) ((freq >> 8) & 0xFF));
	spiWrite_RFM( REG_FRFLSB, (uint8_t) (freq & 0xFF));
}

bool SX1276IsChannelFree(
		RadioModems_t modem,
		uint32_t freq,
		int16_t rssiThresh,
		uint32_t maxCarrierSenseTime ) {
	bool status = true;
	int16_t rssi = 0;
	uint32_t carrierSenseTime = 0;

	SX1276SetSleep();

	SX1276SetModem(modem);

	SX1276SetChannel(freq);

	SX1276SetOpMode( RF_OPMODE_RECEIVER);

	Delayms(5);
	carrierSenseTime = TimerGetCurrentTime();

	// Perform carrier sense for maxCarrierSenseTime
	while (TimerGetElapsedTime(carrierSenseTime) < maxCarrierSenseTime) {
		rssi = SX1276ReadRssi(modem);

		if (rssi > rssiThresh) {
			status = false;
			break;
		}
	}

	SX1276SetSleep();
	return status;
}

uint32_t SX1276Random( void ) {
	uint8_t i;
	uint32_t rnd = 0;

	/*
	 * Radio setup for random number generation
	 */
	// Set LoRa modem ON
	SX1276SetModem(MODEM_LORA);

	// Disable LoRa modem interrupts
	spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
	RFLR_IRQFLAGS_RXDONE |
	RFLR_IRQFLAGS_PAYLOADCRCERROR |
	RFLR_IRQFLAGS_VALIDHEADER |
	RFLR_IRQFLAGS_TXDONE |
	RFLR_IRQFLAGS_CADDONE |
	RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
	RFLR_IRQFLAGS_CADDETECTED);

	// Set radio in continuous reception
	SX1276SetOpMode( RF_OPMODE_RECEIVER);

	for (i = 0; i < 32; i++) {
		Delayms(5);
		// Unfiltered RSSI value reading. Only takes the LSB value
		rnd |= ((uint32_t) spiRead_RFM( REG_LR_RSSIWIDEBAND) & 0x01) << i;
	}

	SX1276SetSleep();

	return rnd;
}

void SX1276SetRxConfig(
		RadioModems_t modem,
		uint32_t bandwidth,
		uint32_t datarate,
		uint8_t coderate,
		uint32_t bandwidthAfc,
		uint16_t preambleLen,
		uint16_t symbTimeout,
		bool fixLen,
		uint8_t payloadLen,
		bool crcOn,
		bool freqHopOn,
		uint8_t hopPeriod,
		bool iqInverted,
		bool rxContinuous ) {
	SX1276SetModem(modem);

	switch (modem) {
		case MODEM_FSK: {
			SX1276.Settings.Fsk.Bandwidth = bandwidth;
			SX1276.Settings.Fsk.Datarate = datarate;
			SX1276.Settings.Fsk.BandwidthAfc = bandwidthAfc;
			SX1276.Settings.Fsk.FixLen = fixLen;
			SX1276.Settings.Fsk.PayloadLen = payloadLen;
			SX1276.Settings.Fsk.CrcOn = crcOn;
			SX1276.Settings.Fsk.IqInverted = iqInverted;
			SX1276.Settings.Fsk.RxContinuous = rxContinuous;
			SX1276.Settings.Fsk.PreambleLen = preambleLen;
			SX1276.Settings.Fsk.RxSingleTimeout = (uint32_t) (symbTimeout
					* ((1.0 / (double) datarate) * 8.0) * 1000);

			datarate = (uint16_t) ((double) XTAL_FREQ / (double) datarate);
			spiWrite_RFM( REG_BITRATEMSB, (uint8_t) (datarate >> 8));
			spiWrite_RFM( REG_BITRATELSB, (uint8_t) (datarate & 0xFF));

			spiWrite_RFM( REG_RXBW, GetFskBandwidthRegValue(bandwidth));
			spiWrite_RFM( REG_AFCBW, GetFskBandwidthRegValue(bandwidthAfc));

			spiWrite_RFM(
			REG_PREAMBLEMSB, (uint8_t) ((preambleLen >> 8) & 0xFF));
			spiWrite_RFM( REG_PREAMBLELSB, (uint8_t) (preambleLen & 0xFF));

			if (fixLen == 1) {
				spiWrite_RFM( REG_PAYLOADLENGTH, payloadLen);
			}
			else {
				spiWrite_RFM( REG_PAYLOADLENGTH, 0xFF); // Set payload length to the maximum
			}

			spiWrite_RFM(
					REG_PACKETCONFIG1,
					(spiRead_RFM( REG_PACKETCONFIG1) &
					RF_PACKETCONFIG1_CRC_MASK &
					RF_PACKETCONFIG1_PACKETFORMAT_MASK)
							| ((fixLen == 1) ?
									RF_PACKETCONFIG1_PACKETFORMAT_FIXED :
									RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE)
							| (crcOn << 4));
			spiWrite_RFM(
					REG_PACKETCONFIG2,
					(spiRead_RFM( REG_PACKETCONFIG2)
							| RF_PACKETCONFIG2_DATAMODE_PACKET));
		}
			break;
		case MODEM_LORA: {
			if (bandwidth > 9) { // changed this from 2 to 9, this library wrongly thought that the sx1276 coudl only achiever bandwidths from 125kHz o 500kHz. which is not the case
				// Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported!!!!!! <-- WRONG!
				break;
			}
//		bandwidth += 7;
			SX1276.Settings.LoRa.Bandwidth = bandwidth;
			SX1276.Settings.LoRa.Datarate = datarate;
			SX1276.Settings.LoRa.Coderate = coderate;
			SX1276.Settings.LoRa.PreambleLen = preambleLen;
			SX1276.Settings.LoRa.FixLen = fixLen;
			SX1276.Settings.LoRa.PayloadLen = payloadLen;
			SX1276.Settings.LoRa.CrcOn = crcOn;
			SX1276.Settings.LoRa.FreqHopOn = freqHopOn;
			SX1276.Settings.LoRa.HopPeriod = hopPeriod;
			SX1276.Settings.LoRa.IqInverted = iqInverted;
			SX1276.Settings.LoRa.RxContinuous = rxContinuous;

			if (datarate > 12) {
				datarate = 12;
			}
			else if (datarate < 6) {
				datarate = 6;
			}

			if (((bandwidth == 7) && ((datarate == 11) || (datarate == 12)))
					|| ((bandwidth == 8) && (datarate == 12))) {
				SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
			}
			else {
				SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
			}

			spiWrite_RFM(
					REG_LR_MODEMCONFIG1,
					(spiRead_RFM( REG_LR_MODEMCONFIG1) &
					RFLR_MODEMCONFIG1_BW_MASK &
					RFLR_MODEMCONFIG1_CODINGRATE_MASK &
					RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) | (bandwidth << 4)
							| (coderate << 1) | fixLen);

			spiWrite_RFM(
					REG_LR_MODEMCONFIG2,
					(spiRead_RFM( REG_LR_MODEMCONFIG2) &
					RFLR_MODEMCONFIG2_SF_MASK &
					RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
					RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK) | (datarate << 4)
							| (crcOn << 2)
							| ((symbTimeout >> 8)
									& ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK));

			spiWrite_RFM(
					REG_LR_MODEMCONFIG3,
					(spiRead_RFM( REG_LR_MODEMCONFIG3) &
					RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK)
							| (SX1276.Settings.LoRa.LowDatarateOptimize << 3));

			spiWrite_RFM(
			REG_LR_SYMBTIMEOUTLSB, (uint8_t) (symbTimeout & 0xFF));

			spiWrite_RFM(
			REG_LR_PREAMBLEMSB, (uint8_t) ((preambleLen >> 8) & 0xFF));
			spiWrite_RFM( REG_LR_PREAMBLELSB, (uint8_t) (preambleLen & 0xFF));

			if (fixLen == 1) {
				spiWrite_RFM( REG_LR_PAYLOADLENGTH, payloadLen);
			}

			if (SX1276.Settings.LoRa.FreqHopOn == true) {
				spiWrite_RFM(
						REG_LR_PLLHOP,
						(spiRead_RFM( REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK)
								| RFLR_PLLHOP_FASTHOP_ON);
				spiWrite_RFM( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod);
			}

			if ((bandwidth == 9)
					&& (SX1276.Settings.Channel > RF_MID_BAND_THRESH)) {
				// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
				spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE1, 0x02);
				spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE2, 0x64);
			}
			else if (bandwidth == 9) {
				// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
				spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE1, 0x02);
				spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE2, 0x7F);
			}
			else {
				// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
				spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE1, 0x03);
			}

			if (datarate == 6) {
				spiWrite_RFM(
				REG_LR_DETECTOPTIMIZE, (spiRead_RFM( REG_LR_DETECTOPTIMIZE) &
				RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF6);
				spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF6);
			}
			else {
				spiWrite_RFM(
				REG_LR_DETECTOPTIMIZE, (spiRead_RFM( REG_LR_DETECTOPTIMIZE) &
				RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
				spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF7_TO_SF12);
			}
		}
			break;
	}
}

void SX1276SetTxConfig(
		RadioModems_t modem,
		int8_t power,
		uint32_t fdev,
		uint32_t bandwidth,
		uint32_t datarate,
		uint8_t coderate,
		uint16_t preambleLen,
		bool fixLen,
		bool crcOn,
		bool freqHopOn,
		uint8_t hopPeriod,
		bool iqInverted,
		uint32_t timeout ) {
	SX1276SetModem(modem);

	SX1276SetRfTxPower(power);

	switch (modem) {
		case MODEM_FSK: {
			SX1276.Settings.Fsk.Power = power;
			SX1276.Settings.Fsk.Fdev = fdev;
			SX1276.Settings.Fsk.Bandwidth = bandwidth;
			SX1276.Settings.Fsk.Datarate = datarate;
			SX1276.Settings.Fsk.PreambleLen = preambleLen;
			SX1276.Settings.Fsk.FixLen = fixLen;
			SX1276.Settings.Fsk.CrcOn = crcOn;
			SX1276.Settings.Fsk.IqInverted = iqInverted;
			SX1276.Settings.Fsk.TxTimeout = timeout;

			fdev = (uint16_t) ((double) fdev / (double) FREQ_STEP);
			spiWrite_RFM( REG_FDEVMSB, (uint8_t) (fdev >> 8));
			spiWrite_RFM( REG_FDEVLSB, (uint8_t) (fdev & 0xFF));

			datarate = (uint16_t) ((double) XTAL_FREQ / (double) datarate);
			spiWrite_RFM( REG_BITRATEMSB, (uint8_t) (datarate >> 8));
			spiWrite_RFM( REG_BITRATELSB, (uint8_t) (datarate & 0xFF));

			spiWrite_RFM( REG_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
			spiWrite_RFM( REG_PREAMBLELSB, preambleLen & 0xFF);

			spiWrite_RFM(
					REG_PACKETCONFIG1,
					(spiRead_RFM( REG_PACKETCONFIG1) &
					RF_PACKETCONFIG1_CRC_MASK &
					RF_PACKETCONFIG1_PACKETFORMAT_MASK)
							| ((fixLen == 1) ?
									RF_PACKETCONFIG1_PACKETFORMAT_FIXED :
									RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE)
							| (crcOn << 4));
			spiWrite_RFM(
					REG_PACKETCONFIG2,
					(spiRead_RFM( REG_PACKETCONFIG2)
							| RF_PACKETCONFIG2_DATAMODE_PACKET));
		}
			break;
		case MODEM_LORA: {
			SX1276.Settings.LoRa.Power = power;
			if (bandwidth > 9) { // changed this from 2 to 9, this library wrongly thought that the sx1276 coudl only achiever bandwidths from 125kHz o 500kHz. which is not the case
				// Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported!!!!!! <-- WRONG!
				break;
			}
//		bandwidth += 7;
			SX1276.Settings.LoRa.Bandwidth = bandwidth;
			SX1276.Settings.LoRa.Datarate = datarate;
			SX1276.Settings.LoRa.Coderate = coderate;
			SX1276.Settings.LoRa.PreambleLen = preambleLen;
			SX1276.Settings.LoRa.FixLen = fixLen;
			SX1276.Settings.LoRa.FreqHopOn = freqHopOn;
			SX1276.Settings.LoRa.HopPeriod = hopPeriod;
			SX1276.Settings.LoRa.CrcOn = crcOn;
			SX1276.Settings.LoRa.IqInverted = iqInverted;
			SX1276.Settings.LoRa.TxTimeout = timeout;

			if (datarate > 12) {
				datarate = 12;
			}
			else if (datarate < 6) {
				datarate = 6;
			}
			if (((bandwidth == 7) && ((datarate == 11) || (datarate == 12)))
					|| ((bandwidth == 8) && (datarate == 12))) {
				SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
			}
			else {
				SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
			}

			if (SX1276.Settings.LoRa.FreqHopOn == true) {
				spiWrite_RFM(
						REG_LR_PLLHOP,
						(spiRead_RFM( REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK)
								| RFLR_PLLHOP_FASTHOP_ON);
				spiWrite_RFM( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod);
			}

			spiWrite_RFM(
					REG_LR_MODEMCONFIG1,
					(spiRead_RFM( REG_LR_MODEMCONFIG1) &
					RFLR_MODEMCONFIG1_BW_MASK &
					RFLR_MODEMCONFIG1_CODINGRATE_MASK &
					RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) | (bandwidth << 4)
							| (coderate << 1) | fixLen);

			spiWrite_RFM(
					REG_LR_MODEMCONFIG2,
					(spiRead_RFM( REG_LR_MODEMCONFIG2) &
					RFLR_MODEMCONFIG2_SF_MASK &
					RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK) | (datarate << 4)
							| (crcOn << 2));

			spiWrite_RFM(
					REG_LR_MODEMCONFIG3,
					(spiRead_RFM( REG_LR_MODEMCONFIG3) &
					RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK)
							| (SX1276.Settings.LoRa.LowDatarateOptimize << 3));

			spiWrite_RFM( REG_LR_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
			spiWrite_RFM( REG_LR_PREAMBLELSB, preambleLen & 0xFF);

			if (datarate == 6) {
				spiWrite_RFM(
				REG_LR_DETECTOPTIMIZE, (spiRead_RFM( REG_LR_DETECTOPTIMIZE) &
				RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF6);
				spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF6);
			}
			else {
				spiWrite_RFM(
				REG_LR_DETECTOPTIMIZE, (spiRead_RFM( REG_LR_DETECTOPTIMIZE) &
				RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
				spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF7_TO_SF12);
			}
		}
			break;
	}
}

uint32_t SX1276GetTimeOnAir( RadioModems_t modem, uint8_t pktLen ) {
	uint32_t airTime = 0;

	switch (modem) {
		case MODEM_FSK: {
			airTime =
					(uint32_t) round(
							(8
									* (SX1276.Settings.Fsk.PreambleLen
											+ ((spiRead_RFM( REG_SYNCCONFIG)
													& ~RF_SYNCCONFIG_SYNCSIZE_MASK)
													+ 1)
											+ ((SX1276.Settings.Fsk.FixLen
													== 0x01) ? 0.0 : 1.0)
											+ (((spiRead_RFM( REG_PACKETCONFIG1)
													& ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK)
													!= 0x00) ? 1.0 : 0) + pktLen
											+ ((SX1276.Settings.Fsk.CrcOn
													== 0x01) ? 2.0 : 0))
									/ SX1276.Settings.Fsk.Datarate) * 1000);
		}
			break;
		case MODEM_LORA: {
			double bw = 0.0;
			// REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
			switch (SX1276.Settings.LoRa.Bandwidth) {
				case 0: // 7.8 kHz
					bw = 7800;
					break;
				case 1: // 10.4 kHz
					bw = 10400;
					break;
				case 2: // 15.6 kHz
					bw = 15600;
					break;
				case 3: // 20.8 kHz
					bw = 20800;
					break;
				case 4: // 31.2 kHz
					bw = 31200;
					break;
				case 5: // 41.4 kHz
					bw = 41400;
					break;
				case 6: // 62.5 kHz
					bw = 62500;
					break;
				case 7: // 125 kHz
					bw = 125000;
					break;
				case 8: // 250 kHz
					bw = 250000;
					break;
				case 9: // 500 kHz
					bw = 500000;
					break;
			}

			// Symbol rate : time for one symbol (secs)
			double rs = bw / (1 << SX1276.Settings.LoRa.Datarate);
			double ts = 1 / rs;
			// time of preamble
			double tPreamble = (SX1276.Settings.LoRa.PreambleLen + 4.25) * ts;
			// Symbol length of payload and time
			double tmp =
					ceil(
							(8 * pktLen - 4 * SX1276.Settings.LoRa.Datarate + 28
									+ 16 * SX1276.Settings.LoRa.CrcOn
									- (SX1276.Settings.LoRa.FixLen ? 20 : 0))
									/ (double) (4
											* (SX1276.Settings.LoRa.Datarate
													- ((SX1276.Settings.LoRa.LowDatarateOptimize
															> 0) ? 2 : 0))))
							* (SX1276.Settings.LoRa.Coderate + 4);
			double nPayload = 8 + ((tmp > 0) ? tmp : 0);
			double tPayload = nPayload * ts;
			// Time on air
			double tOnAir = tPreamble + tPayload;
			// return ms secs
			airTime = (uint32_t) floor(tOnAir * 1000 + 0.999);
		}
			break;
	}
	return airTime;
}

void SX1276Send( uint8_t *buffer, uint8_t size ) {
	uint32_t txTimeout = 0;

	switch (SX1276.Settings.Modem) {
		case MODEM_FSK: {
			SX1276.Settings.FskPacketHandler.NbBytes = 0;
			SX1276.Settings.FskPacketHandler.Size = size;

			if (SX1276.Settings.Fsk.FixLen == false) {
				SX1276WriteFifo((uint8_t*) &size, 1);
			}
			else {
				spiWrite_RFM( REG_PAYLOADLENGTH, size);
			}

			if ((size > 0) && (size <= 64)) {
				SX1276.Settings.FskPacketHandler.ChunkSize = size;
			}
			else {
				memcpy(RxTxBuffer, buffer, size);
				SX1276.Settings.FskPacketHandler.ChunkSize = 32;
			}

			// Write payload buffer
			SX1276WriteFifo(buffer, SX1276.Settings.FskPacketHandler.ChunkSize);
			SX1276.Settings.FskPacketHandler.NbBytes +=
					SX1276.Settings.FskPacketHandler.ChunkSize;
			txTimeout = SX1276.Settings.Fsk.TxTimeout;
		}
			break;
		case MODEM_LORA: {
			if (SX1276.Settings.LoRa.IqInverted == true) {
				spiWrite_RFM(
						REG_LR_INVERTIQ,
						((spiRead_RFM( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
								& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF
								| RFLR_INVERTIQ_TX_ON));
				spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
			}
			else {
				spiWrite_RFM(
						REG_LR_INVERTIQ,
						((spiRead_RFM( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
								& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF
								| RFLR_INVERTIQ_TX_OFF));
				spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
			}

			SX1276.Settings.LoRaPacketHandler.Size = size;

			// Initializes the payload size
			spiWrite_RFM( REG_LR_PAYLOADLENGTH, size);

			// Full buffer used for Tx
			spiWrite_RFM( REG_LR_FIFOTXBASEADDR, 0);
			spiWrite_RFM( REG_LR_FIFOADDRPTR, 0);

			// FIFO operations can not take place in Sleep mode
			if ((spiRead_RFM( REG_OPMODE) & ~RF_OPMODE_MASK) == RF_OPMODE_SLEEP) {
				SX1276SetStby();
				Delayms(5);
			}
			// Write payload buffer
			SX1276WriteFifo(buffer, size);
			txTimeout = SX1276.Settings.LoRa.TxTimeout;
		}
			break;
	}

	SX1276SetTx(txTimeout);
}

void SX1276SetOpMode( uint8_t opMode ) {
	spiWrite_RFM(
	REG_OPMODE, (spiRead_RFM( REG_OPMODE) & RF_OPMODE_MASK) | opMode);
}

int16_t SX1276ReadRssi( RadioModems_t modem ) {
	int16_t rssi = 0;

	switch (modem) {
		case MODEM_FSK:
			rssi = -(spiRead_RFM( REG_RSSIVALUE) >> 1);
			break;
		case MODEM_LORA:
			if (SX1276.Settings.Channel > RF_MID_BAND_THRESH) {
				rssi = RSSI_OFFSET_HF + spiRead_RFM( REG_LR_RSSIVALUE);
			}
			else {
				rssi = RSSI_OFFSET_LF + spiRead_RFM( REG_LR_RSSIVALUE);
			}
			break;
		default:
			rssi = -1;
			break;
	}
	return rssi;
}

/*
 Set the RFM95 chip to sleep mode
 */
void SX1276SetSleep( void ) {
	TimerStop(&RxTimeoutTimer);
	TimerStop(&TxTimeoutTimer);
	TimerStop(&RxTimeoutSyncWord);

//    Put into sleep mode and wait 100ms to settle
	SX1276SetOpMode( RF_OPMODE_SLEEP);

	SX1276.Settings.State = RF_IDLE;
	Delayms(99);
}

void SX1276SetStby( void ) {
	TimerStop(&RxTimeoutTimer);
	TimerStop(&TxTimeoutTimer);
	TimerStop(&RxTimeoutSyncWord);

	SX1276SetOpMode( RF_OPMODE_STANDBY);
	SX1276.Settings.State = RF_IDLE;
}

void RxChainCalibration( ) {
	uint8_t regPaConfigInitVal;
	uint32_t initialFreq;

	// Save context
	regPaConfigInitVal = spiRead_RFM( REG_LR_PACONFIG);
	initialFreq = (double) (((uint32_t) spiRead_RFM( REG_FRFMSB) << 16)
			| ((uint32_t) spiRead_RFM( REG_FRFMID) << 8)
			| ((uint32_t) spiRead_RFM( REG_FRFLSB))) * (double) FREQ_STEP;

	// Cut the PA just in case, RFO output, power = -1 dBm
	spiWrite_RFM( REG_LR_PACONFIG, 0x00);

	// Launch Rx chain calibration for LF band
	spiWrite_RFM(
			REG_IMAGECAL,
			(spiRead_RFM( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK)
					| RF_IMAGECAL_IMAGECAL_START);
	while ((spiRead_RFM( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING)
			== RF_IMAGECAL_IMAGECAL_RUNNING) {
	};

	// Sets a Frequency in HF band
	SX1276SetChannel(868000000);

	// Launch Rx chain calibration for HF band
	spiWrite_RFM(
			REG_IMAGECAL,
			(spiRead_RFM( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK)
					| RF_IMAGECAL_IMAGECAL_START);
	while ((spiRead_RFM( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING)
			== RF_IMAGECAL_IMAGECAL_RUNNING) {
	};

	// Restore context
	spiWrite_RFM( REG_PACONFIG, regPaConfigInitVal);
	SX1276SetChannel(initialFreq);
}

void SX1276WriteFifo( uint8_t *buffer, uint8_t size ) {
	SX1276WriteBuffer(0, buffer, size);
}

void SX1276ReadFifo( uint8_t *buffer, uint8_t size ) {
	SX1276ReadBuffer(0, buffer, size);
}

void SX1276WriteBuffer( uint16_t addr, uint8_t *buffer, uint8_t size ) {
	uint8_t i;

	//NSS = 0;
	RFM95_NSS_LOW;

	spi_read_write(addr | 0x80);
	for (i = 0; i < size; i++) {
		spi_read_write(buffer[i]);
	}

	//NSS = 1;
	RFM95_NSS_HIGH;
}

void SX1276SetRx( uint32_t timeout ) {
	bool rxContinuous = false;
	TimerStop(&TxTimeoutTimer);

	switch (SX1276.Settings.Modem) {
		case MODEM_FSK: {
			rxContinuous = SX1276.Settings.Fsk.RxContinuous;

			// DIO0=PayloadReady
			// DIO1=FifoLevel
			// DIO2=SyncAddr
			// DIO3=FifoEmpty
			// DIO4=Preamble
			// DIO5=ModeReady
			spiWrite_RFM(
					REG_DIOMAPPING1,
					(spiRead_RFM( REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK &
					RF_DIOMAPPING1_DIO1_MASK &
					RF_DIOMAPPING1_DIO2_MASK) |
					RF_DIOMAPPING1_DIO0_00 |
					RF_DIOMAPPING1_DIO1_00 |
					RF_DIOMAPPING1_DIO2_11);

			spiWrite_RFM(
					REG_DIOMAPPING2,
					(spiRead_RFM( REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK &
					RF_DIOMAPPING2_MAP_MASK) |
					RF_DIOMAPPING2_DIO4_11 |
					RF_DIOMAPPING2_MAP_PREAMBLEDETECT);

			SX1276.Settings.FskPacketHandler.FifoThresh = spiRead_RFM(
			REG_FIFOTHRESH) & 0x3F;

			spiWrite_RFM(
					REG_RXCONFIG,
					RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON
							| RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT);

			SX1276.Settings.FskPacketHandler.PreambleDetected = false;
			SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
			SX1276.Settings.FskPacketHandler.NbBytes = 0;
			SX1276.Settings.FskPacketHandler.Size = 0;
		}
			break;
		case MODEM_LORA: {
			if (SX1276.Settings.LoRa.IqInverted == true) {
				spiWrite_RFM(
						REG_LR_INVERTIQ,
						((spiRead_RFM( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
								& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_ON
								| RFLR_INVERTIQ_TX_OFF));
				spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
			}
			else {
				spiWrite_RFM(
						REG_LR_INVERTIQ,
						((spiRead_RFM( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
								& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF
								| RFLR_INVERTIQ_TX_OFF));
				spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
			}

			// ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
			if (SX1276.Settings.LoRa.Bandwidth < 9) {
				spiWrite_RFM(
						REG_LR_DETECTOPTIMIZE,
						spiRead_RFM( REG_LR_DETECTOPTIMIZE) & 0x7F);
				spiWrite_RFM( REG_LR_IFFREQ2, 0x00);
				switch (SX1276.Settings.LoRa.Bandwidth) {
					case 0: // 7.8 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x48);
//				SX1276SetChannel(SX1276.Settings.Channel + 7810);
						break;
					case 1: // 10.4 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 10420);
						break;
					case 2: // 15.6 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 15620);
						break;
					case 3: // 20.8 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 20830);
						break;
					case 4: // 31.2 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 31250);
						break;
					case 5: // 41.4 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 41670);
						break;
					case 6: // 62.5 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x40);
						break;
					case 7: // 125 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x40);
						break;
					case 8: // 250 kHz
						spiWrite_RFM( REG_LR_IFFREQ1, 0x40);
						break;
				}
			}
			else {
				spiWrite_RFM(
						REG_LR_DETECTOPTIMIZE,
						spiRead_RFM( REG_LR_DETECTOPTIMIZE) | 0x80);
			}

			rxContinuous = SX1276.Settings.LoRa.RxContinuous;

			if (SX1276.Settings.LoRa.FreqHopOn == true) {
				spiWrite_RFM( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
												   //RFLR_IRQFLAGS_RXDONE |
												   //RFLR_IRQFLAGS_PAYLOADCRCERROR |
						RFLR_IRQFLAGS_VALIDHEADER |
						RFLR_IRQFLAGS_TXDONE |
						RFLR_IRQFLAGS_CADDONE |
						//RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
								RFLR_IRQFLAGS_CADDETECTED);

				// DIO0=RxDone, DIO2=FhssChangeChannel
				spiWrite_RFM(
						REG_DIOMAPPING1,
						(spiRead_RFM( REG_DIOMAPPING1)
								& RFLR_DIOMAPPING1_DIO0_MASK
								& RFLR_DIOMAPPING1_DIO2_MASK)
								| RFLR_DIOMAPPING1_DIO0_00
								| RFLR_DIOMAPPING1_DIO2_00);
			}
			else {
				spiWrite_RFM( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
												   //RFLR_IRQFLAGS_RXDONE |
												   //RFLR_IRQFLAGS_PAYLOADCRCERROR |
						RFLR_IRQFLAGS_VALIDHEADER |
						RFLR_IRQFLAGS_TXDONE |
						RFLR_IRQFLAGS_CADDONE |
						RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
						RFLR_IRQFLAGS_CADDETECTED);

				// DIO0=RxDone
				spiWrite_RFM(
						REG_DIOMAPPING1,
						(spiRead_RFM( REG_DIOMAPPING1)
								& RFLR_DIOMAPPING1_DIO0_MASK)
								| RFLR_DIOMAPPING1_DIO0_00);
			}
			spiWrite_RFM( REG_LR_FIFORXBASEADDR, 0);
			spiWrite_RFM( REG_LR_FIFOADDRPTR, 0);
		}
			break;
	}

	memset(RxTxBuffer, 0, (size_t) RX_BUFFER_SIZE);

	SX1276.Settings.State = RF_RX_RUNNING;
	if (timeout != 0) {
		TimerSetValue(&RxTimeoutTimer, timeout);
		TimerStart(&RxTimeoutTimer);
	}

	if (SX1276.Settings.Modem == MODEM_FSK) {
		SX1276SetOpMode( RF_OPMODE_RECEIVER);

		if (rxContinuous == false) {
			TimerSetValue(
					&RxTimeoutSyncWord,
					SX1276.Settings.Fsk.RxSingleTimeout);
			TimerStart(&RxTimeoutSyncWord);
		}
	}
	else {
		if (rxContinuous == true) {
			SX1276SetOpMode( RFLR_OPMODE_RECEIVER);
		}
		else {
			SX1276SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE);
		}
	}
}

void SX1276StartCad( void ) {
	switch (SX1276.Settings.Modem) {
		case MODEM_FSK: {

		}
			break;
		case MODEM_LORA: {
			spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
			RFLR_IRQFLAGS_RXDONE |
			RFLR_IRQFLAGS_PAYLOADCRCERROR |
			RFLR_IRQFLAGS_VALIDHEADER |
			RFLR_IRQFLAGS_TXDONE |
			//RFLR_IRQFLAGS_CADDONE |
					RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL// |
			//RFLR_IRQFLAGS_CADDETECTED
					);

			// DIO3=CADDone
			spiWrite_RFM(
					REG_DIOMAPPING1,
					(spiRead_RFM( REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO3_MASK)
							| RFLR_DIOMAPPING1_DIO3_00);

			SX1276.Settings.State = RF_CAD;
			SX1276SetOpMode( RFLR_OPMODE_CAD);
		}
			break;
		default:
			break;
	}
}

void SX1276SetTxContinuousWave( uint32_t freq, int8_t power, uint16_t time ) {
	uint32_t timeout = (uint32_t) (time * 1000);

	SX1276SetChannel(freq);

	SX1276SetTxConfig(
			MODEM_FSK,
			power,
			0,
			0,
			4800,
			0,
			5,
			false,
			false,
			0,
			0,
			0,
			timeout);

	spiWrite_RFM(
			REG_PACKETCONFIG2,
			(spiRead_RFM( REG_PACKETCONFIG2) & RF_PACKETCONFIG2_DATAMODE_MASK));
	// Disable radio interrupts
	spiWrite_RFM( REG_DIOMAPPING1,
	RF_DIOMAPPING1_DIO0_11 | RF_DIOMAPPING1_DIO1_11);
	spiWrite_RFM( REG_DIOMAPPING2,
	RF_DIOMAPPING2_DIO4_10 | RF_DIOMAPPING2_DIO5_10);

	TimerSetValue(&TxTimeoutTimer, timeout);
	SX1276.Settings.State = RF_TX_RUNNING;
	TimerStart(&TxTimeoutTimer);
	SX1276SetOpMode( RF_OPMODE_TRANSMITTER);
}

void SX1276ReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size ) {
	uint8_t i;

	//NSS = 0;
	RFM95_NSS_LOW;

	spi_read_write(addr & 0x7F);

	for (i = 0; i < size; i++) {
		buffer[i] = spi_read_write(0);
	}

	//NSS = 1;
	RFM95_NSS_HIGH;
}

void SX1276SetMaxPayloadLength( RadioModems_t modem, uint8_t max ) {
	SX1276SetModem(modem);

	switch (modem) {
		case MODEM_FSK:
			if (SX1276.Settings.Fsk.FixLen == false) {
				spiWrite_RFM( REG_PAYLOADLENGTH, max);
			}
			break;
		case MODEM_LORA:
			spiWrite_RFM( REG_LR_PAYLOADMAXLENGTH, max);
			break;
	}
}

void SX1276SetPublicNetwork( bool enable, uint16_t syncword ) {
	SX1276SetModem(MODEM_LORA);
	SX1276.Settings.LoRa.PublicNetwork = enable;
	if (enable == true) {
		// Change LoRa modem SyncWord
		spiWrite_RFM( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD);
	}
	else {
		// Change LoRa modem SyncWord
		spiWrite_RFM( REG_LR_SYNCWORD, syncword);
	}
}

void SX1276SetTx( uint32_t timeout ) {
	TimerStop(&RxTimeoutTimer);
	if (timeout > 0) TimerSetValue(&TxTimeoutTimer, timeout);

	switch (SX1276.Settings.Modem) {
		case MODEM_FSK: {
			// DIO0=PacketSent
			// DIO1=FifoEmpty
			// DIO2=FifoFull
			// DIO3=FifoEmpty
			// DIO4=LowBat
			// DIO5=ModeReady
			spiWrite_RFM(
					REG_DIOMAPPING1,
					(spiRead_RFM( REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK &
					RF_DIOMAPPING1_DIO1_MASK &
					RF_DIOMAPPING1_DIO2_MASK) |
					RF_DIOMAPPING1_DIO1_01);

			spiWrite_RFM(
					REG_DIOMAPPING2,
					(spiRead_RFM( REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK &
					RF_DIOMAPPING2_MAP_MASK));
			SX1276.Settings.FskPacketHandler.FifoThresh = spiRead_RFM(
			REG_FIFOTHRESH) & 0x3F;
		}
			break;
		case MODEM_LORA: {
			if (SX1276.Settings.LoRa.FreqHopOn == true) {
				spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
				RFLR_IRQFLAGS_RXDONE |
				RFLR_IRQFLAGS_PAYLOADCRCERROR |
				RFLR_IRQFLAGS_VALIDHEADER |
				//RFLR_IRQFLAGS_TXDONE |
						RFLR_IRQFLAGS_CADDONE |
						//RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
						RFLR_IRQFLAGS_CADDETECTED);

				// DIO0=TxDone, DIO2=FhssChangeChannel
				spiWrite_RFM(
						REG_DIOMAPPING1,
						(spiRead_RFM( REG_DIOMAPPING1)
								& RFLR_DIOMAPPING1_DIO0_MASK
								& RFLR_DIOMAPPING1_DIO2_MASK)
								| RFLR_DIOMAPPING1_DIO0_01
								| RFLR_DIOMAPPING1_DIO2_00);
			}
			else {
				spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
				RFLR_IRQFLAGS_RXDONE |
				RFLR_IRQFLAGS_PAYLOADCRCERROR |
				RFLR_IRQFLAGS_VALIDHEADER |
				//RFLR_IRQFLAGS_TXDONE |
						RFLR_IRQFLAGS_CADDONE |
						RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
						RFLR_IRQFLAGS_CADDETECTED);

				// DIO0=TxDone
				spiWrite_RFM(
						REG_DIOMAPPING1,
						(spiRead_RFM( REG_DIOMAPPING1)
								& RFLR_DIOMAPPING1_DIO0_MASK)
								| RFLR_DIOMAPPING1_DIO0_01);
			}
		}
			break;
	}

	SX1276.Settings.State = RF_TX_RUNNING;
	if (timeout > 0) TimerStart(&TxTimeoutTimer);
	SX1276SetOpMode( RF_OPMODE_TRANSMITTER);
}

uint8_t SX1276GetPaSelect( int8_t power ) {
	if (power > 14) {
		return RF_PACONFIG_PASELECT_PABOOST;
	}
	else {
		return RF_PACONFIG_PASELECT_RFO;
	}
}

uint32_t SX1276GetWakeupTime( void ) {
	return SX1276GetBoardTcxoWakeupTime() + RADIO_WAKEUP_TIME;
}

int16_t SX1276GetPacketRSSI( void ) {
	int v = spiRead_RFM( REG_LR_PKTRSSIVALUE);
	uint8_t temp = spiRead_RFM(REG_LR_OPMODE)
			& ~(RFLR_OPMODE_FREQMODE_ACCESS_MASK);
	if (temp == RFLR_OPMODE_FREQMODE_ACCESS_HF) {
		return RSSI_OFFSET_HF + v;

	}
	else if (temp == RFLR_OPMODE_FREQMODE_ACCESS_LF) {
		return RSSI_OFFSET_LF + v;
	}
	else {
		return 0;
	}
}

int16_t SX1276GetPacketSNR( void ) {
	int v = spiRead_RFM(REG_LR_PKTSNRVALUE);
	if (v > 127) {
		return (float) (v - 256);
	}
	else {
		return (float) (v) / 4.0;
	}
}

uint8_t SX1276GetPacketSize( void ) {
	return spiRead_RFM( REG_LR_RXNBBYTES);
}

void SX1276OnDio0Irq( ) {
	volatile uint8_t irqFlags = 0;

	switch (SX1276.Settings.State) {
		case RF_RX_RUNNING:
			//TimerStop( &RxTimeoutTimer );
			// RxDone interrupt
			switch (SX1276.Settings.Modem) {
				case MODEM_FSK:
					if (SX1276.Settings.Fsk.CrcOn == true) {
						irqFlags = spiRead_RFM( REG_IRQFLAGS2);
						if ((irqFlags & RF_IRQFLAGS2_CRCOK)
								!= RF_IRQFLAGS2_CRCOK) {
							// Clear Irqs
							spiWrite_RFM( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
							RF_IRQFLAGS1_PREAMBLEDETECT |
							RF_IRQFLAGS1_SYNCADDRESSMATCH);
							spiWrite_RFM(
							REG_IRQFLAGS2,
							RF_IRQFLAGS2_FIFOOVERRUN);

							TimerStop(&RxTimeoutTimer);

							if (SX1276.Settings.Fsk.RxContinuous == false) {
								TimerStop(&RxTimeoutSyncWord);
								SX1276.Settings.State = RF_IDLE;
							}
							else {
								// Continuous mode restart Rx chain
								spiWrite_RFM(
										REG_RXCONFIG,
										spiRead_RFM(
												REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
							}

							RadioEvents->RxError();

							SX1276.Settings.FskPacketHandler.PreambleDetected =
									false;
							SX1276.Settings.FskPacketHandler.SyncWordDetected =
									false;
							SX1276.Settings.FskPacketHandler.NbBytes = 0;
							SX1276.Settings.FskPacketHandler.Size = 0;
							break;
						}
					}

					// Read received packet size
					if ((SX1276.Settings.FskPacketHandler.Size == 0)
							&& (SX1276.Settings.FskPacketHandler.NbBytes == 0)) {
						if (SX1276.Settings.Fsk.FixLen == false) {
							SX1276ReadFifo(
									(uint8_t*) &SX1276.Settings.FskPacketHandler.Size,
									1);
						}
						else {
							SX1276.Settings.FskPacketHandler.Size = spiRead_RFM(
							REG_PAYLOADLENGTH);
						}
						SX1276ReadFifo(
								RxTxBuffer
										+ SX1276.Settings.FskPacketHandler.NbBytes,
								SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes);
						SX1276.Settings.FskPacketHandler.NbBytes +=
								(SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes);
					}
					else {
						SX1276ReadFifo(
								RxTxBuffer
										+ SX1276.Settings.FskPacketHandler.NbBytes,
								SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes);
						SX1276.Settings.FskPacketHandler.NbBytes +=
								(SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes);
					}

					TimerStop(&RxTimeoutTimer);

					if (SX1276.Settings.Fsk.RxContinuous == false) {
						SX1276.Settings.State = RF_IDLE;
						TimerStop(&RxTimeoutSyncWord);
					}
					else {
						// Continuous mode restart Rx chain
						spiWrite_RFM(
						REG_RXCONFIG, spiRead_RFM(
						REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
					}

					RadioEvents->RxDone(
							RxTxBuffer,
							SX1276.Settings.FskPacketHandler.Size,
							SX1276.Settings.FskPacketHandler.RssiValue,
							0);

					SX1276.Settings.FskPacketHandler.PreambleDetected = false;
					SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
					SX1276.Settings.FskPacketHandler.NbBytes = 0;
					SX1276.Settings.FskPacketHandler.Size = 0;
					break;
				case MODEM_LORA: {
					// Clear Irq
					spiWrite_RFM( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE);

					irqFlags = spiRead_RFM( REG_LR_IRQFLAGS);
					if ((irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK)
							== RFLR_IRQFLAGS_PAYLOADCRCERROR) {
						// Clear Irq
						spiWrite_RFM(
						REG_LR_IRQFLAGS,
						RFLR_IRQFLAGS_PAYLOADCRCERROR);
						if (SX1276.Settings.LoRa.RxContinuous == false) {
							SX1276.Settings.State = RF_IDLE;
						}
						TimerStop(&RxTimeoutTimer);
						if ((RadioEvents != NULL)
								&& (RadioEvents->RxError != NULL)) {
							RadioEvents->RxError();
						}
						break;
					}
					// Returns SNR value [dB] rounded to the nearest integer value
					SX1276.Settings.LoRaPacketHandler.SnrValue =
							(((int8_t) spiRead_RFM(
							REG_LR_PKTSNRVALUE)) + 2) >> 2;

					int16_t rssi = spiRead_RFM( REG_LR_PKTRSSIVALUE);
					if (SX1276.Settings.LoRaPacketHandler.SnrValue < 0) {
						if (SX1276.Settings.Channel > RF_MID_BAND_THRESH) {
							SX1276.Settings.LoRaPacketHandler.RssiValue =
									RSSI_OFFSET_HF + rssi + (rssi >> 4)
											+ SX1276.Settings.LoRaPacketHandler.SnrValue;
						}
						else {
							SX1276.Settings.LoRaPacketHandler.RssiValue =
									RSSI_OFFSET_LF + rssi + (rssi >> 4)
											+ SX1276.Settings.LoRaPacketHandler.SnrValue;
						}
					}
					else {
						if (SX1276.Settings.Channel > RF_MID_BAND_THRESH) {
							SX1276.Settings.LoRaPacketHandler.RssiValue =
							RSSI_OFFSET_HF + rssi + (rssi >> 4);
						}
						else {
							SX1276.Settings.LoRaPacketHandler.RssiValue =
							RSSI_OFFSET_LF + rssi + (rssi >> 4);
						}
					}

					SX1276.Settings.LoRaPacketHandler.Size = spiRead_RFM(
					REG_LR_RXNBBYTES);
					spiWrite_RFM(
					REG_LR_FIFOADDRPTR, spiRead_RFM( REG_LR_FIFORXCURRENTADDR));
					SX1276ReadFifo(
							RxTxBuffer,
							SX1276.Settings.LoRaPacketHandler.Size);

					if (SX1276.Settings.LoRa.RxContinuous == false) {
						SX1276.Settings.State = RF_IDLE;
					}

					TimerStop(&RxTimeoutTimer);

					if ((RadioEvents != NULL)
							&& (RadioEvents->RxDone != NULL)) {
						RadioEvents->RxDone(
								RxTxBuffer,
								SX1276.Settings.LoRaPacketHandler.Size,
								SX1276.Settings.LoRaPacketHandler.RssiValue,
								SX1276.Settings.LoRaPacketHandler.SnrValue);
					}
				}
					break;
				default:
					break;
			}
			break;
		case RF_TX_RUNNING:
			TimerStop(&TxTimeoutTimer);
			// TxDone interrupt
			switch (SX1276.Settings.Modem) {
				case MODEM_LORA:
					// Clear Irq
					spiWrite_RFM( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE);
					// Intentional fall through
				case MODEM_FSK:
				default:
					SX1276.Settings.State = RF_IDLE;

					RadioEvents->TxDone();

					break;
			}
			break;
		default:
			break;
	}
}

void SX1276OnDio1Irq( ) {
	stopLoRaTimer();
	switch (SX1276.Settings.State) {
		case RF_RX_RUNNING:
			switch (SX1276.Settings.Modem) {
				case MODEM_FSK:
					// Stop timer
					TimerStop(&RxTimeoutSyncWord);
					// FifoLevel interrupt
					// Read received packet size
					if ((SX1276.Settings.FskPacketHandler.Size == 0)
							&& (SX1276.Settings.FskPacketHandler.NbBytes == 0)) {
						if (SX1276.Settings.Fsk.FixLen == false) {
							SX1276ReadFifo(
									(uint8_t*) &SX1276.Settings.FskPacketHandler.Size,
									1);
						}
						else {
							SX1276.Settings.FskPacketHandler.Size = spiRead_RFM(
							REG_PAYLOADLENGTH);
						}
					}

					// ERRATA 3.1 - PayloadReady Set for 31.25ns if FIFO is Empty
					//
					//              When FifoLevel interrupt is used to offload the
					//              FIFO, the microcontroller should  monitor  both
					//              PayloadReady  and FifoLevel interrupts, and
					//              read only (FifoThreshold-1) bytes off the FIFO
					//              when FifoLevel fires
					if ((SX1276.Settings.FskPacketHandler.Size
							- SX1276.Settings.FskPacketHandler.NbBytes)
							>= SX1276.Settings.FskPacketHandler.FifoThresh) {
						SX1276ReadFifo(
								(RxTxBuffer
										+ SX1276.Settings.FskPacketHandler.NbBytes),
								SX1276.Settings.FskPacketHandler.FifoThresh
										- 1);
						SX1276.Settings.FskPacketHandler.NbBytes +=
								SX1276.Settings.FskPacketHandler.FifoThresh - 1;
					}
					else {
						SX1276ReadFifo(
								(RxTxBuffer
										+ SX1276.Settings.FskPacketHandler.NbBytes),
								SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes);
						SX1276.Settings.FskPacketHandler.NbBytes +=
								(SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes);
					}
					break;
				case MODEM_LORA:
					// Sync time out
					TimerStop(&RxTimeoutTimer);
					// Clear Irq
					spiWrite_RFM( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT);

					SX1276.Settings.State = RF_IDLE;
					if ((RadioEvents != NULL)
							&& (RadioEvents->RxTimeout != NULL)) {
						RadioEvents->RxTimeout();
					}
					break;
				default:
					break;
			}
			break;
		case RF_TX_RUNNING:
			switch (SX1276.Settings.Modem) {
				case MODEM_FSK:
					// FifoEmpty interrupt
					if ((SX1276.Settings.FskPacketHandler.Size
							- SX1276.Settings.FskPacketHandler.NbBytes)
							> SX1276.Settings.FskPacketHandler.ChunkSize) {
						SX1276WriteFifo(
								(RxTxBuffer
										+ SX1276.Settings.FskPacketHandler.NbBytes),
								SX1276.Settings.FskPacketHandler.ChunkSize);
						SX1276.Settings.FskPacketHandler.NbBytes +=
								SX1276.Settings.FskPacketHandler.ChunkSize;
					}
					else {
						// Write the last chunk of data
						SX1276WriteFifo(
								RxTxBuffer
										+ SX1276.Settings.FskPacketHandler.NbBytes,
								SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes);
						SX1276.Settings.FskPacketHandler.NbBytes +=
								SX1276.Settings.FskPacketHandler.Size
										- SX1276.Settings.FskPacketHandler.NbBytes;
					}
					break;
				case MODEM_LORA:
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void SX1276OnDio2Irq( ) {
	switch (SX1276.Settings.State) {
		case RF_RX_RUNNING:
			switch (SX1276.Settings.Modem) {
				case MODEM_FSK:
					// Checks if DIO4 is connected. If it is not PreambleDetected is set to true.
					if (SX1276.DIO4.port == NULL) {
						SX1276.Settings.FskPacketHandler.PreambleDetected =
								true;
					}

					if ((SX1276.Settings.FskPacketHandler.PreambleDetected
							== true)
							&& (SX1276.Settings.FskPacketHandler.SyncWordDetected
									== false)) {
						TimerStop(&RxTimeoutSyncWord);

						SX1276.Settings.FskPacketHandler.SyncWordDetected =
								true;

						SX1276.Settings.FskPacketHandler.RssiValue =
								-(spiRead_RFM(
								REG_RSSIVALUE) >> 1);

						SX1276.Settings.FskPacketHandler.AfcValue =
								(int32_t) (double) (((uint16_t) spiRead_RFM(
								REG_AFCMSB) << 8)
										| (uint16_t) spiRead_RFM( REG_AFCLSB))
										* (double) FREQ_STEP;
						SX1276.Settings.FskPacketHandler.RxGain = (spiRead_RFM(
						REG_LNA) >> 5) & 0x07;
					}
					break;
				case MODEM_LORA:
					if (SX1276.Settings.LoRa.FreqHopOn == true) {
						// Clear Irq
						spiWrite_RFM(
						REG_LR_IRQFLAGS,
						RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

						if ((RadioEvents != NULL)
								&& (RadioEvents->FhssChangeChannel != NULL)) {
							RadioEvents->FhssChangeChannel(
									(spiRead_RFM( REG_LR_HOPCHANNEL)
											& RFLR_HOPCHANNEL_CHANNEL_MASK));
						}
					}
					break;
				default:
					break;
			}
			break;
		case RF_TX_RUNNING:
			switch (SX1276.Settings.Modem) {
				case MODEM_FSK:
					break;
				case MODEM_LORA:
					if (SX1276.Settings.LoRa.FreqHopOn == true) {
						// Clear Irq
						spiWrite_RFM(
						REG_LR_IRQFLAGS,
						RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

						if ((RadioEvents != NULL)
								&& (RadioEvents->FhssChangeChannel != NULL)) {
							RadioEvents->FhssChangeChannel(
									(spiRead_RFM( REG_LR_HOPCHANNEL)
											& RFLR_HOPCHANNEL_CHANNEL_MASK));
						}
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void SX1276OnDio3Irq( ) {
	stopLoRaTimer();
	switch (SX1276.Settings.Modem) {
		case MODEM_FSK:
			break;
		case MODEM_LORA:
			if ((spiRead_RFM( REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_CADDETECTED)
					== RFLR_IRQFLAGS_CADDETECTED) {
				// Clear Irq
				spiWrite_RFM(
				REG_LR_IRQFLAGS,
				RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE);
				if ((RadioEvents != NULL) && (RadioEvents->CadDone != NULL)) {
					RadioEvents->CadDone(true);
				}
			}
			else {
				// Clear Irq
				spiWrite_RFM( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE);
				if ((RadioEvents != NULL) && (RadioEvents->CadDone != NULL)) {
					RadioEvents->CadDone(false);
				}
			}
			break;
		default:
			break;
	}
}

void SX1276OnDio4Irq( ) {
	stopLoRaTimer();
	switch (SX1276.Settings.Modem) {
		case MODEM_FSK: {
			if (SX1276.Settings.FskPacketHandler.PreambleDetected == false) {
				SX1276.Settings.FskPacketHandler.PreambleDetected = true;
			}
		}
			break;
		case MODEM_LORA:
			break;
		default:
			break;
	}
}

void SX1276OnDio5Irq( ) {

	switch (SX1276.Settings.Modem) {
		case MODEM_FSK:
			break;
		case MODEM_LORA:
			break;
		default:
			break;
	}
}

void SX1276OnTimeoutIrq( void* context ) {
	uint8_t i = 0;
	switch (SX1276.Settings.State) {
		case RF_RX_RUNNING:
			if (SX1276.Settings.Modem == MODEM_FSK) {
				SX1276.Settings.FskPacketHandler.PreambleDetected = false;
				SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
				SX1276.Settings.FskPacketHandler.NbBytes = 0;
				SX1276.Settings.FskPacketHandler.Size = 0;

				// Clear Irqs
				spiWrite_RFM( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
				RF_IRQFLAGS1_PREAMBLEDETECT |
				RF_IRQFLAGS1_SYNCADDRESSMATCH);
				spiWrite_RFM( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);

				if (SX1276.Settings.Fsk.RxContinuous == true) {
					// Continuous mode restart Rx chain
					spiWrite_RFM(
					REG_RXCONFIG, spiRead_RFM(
					REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
				}
				else {
					SX1276.Settings.State = RF_IDLE;
					TimerStop(&RxTimeoutSyncWord);
				}
			}
			if ((RadioEvents != NULL) && (RadioEvents->RxTimeout != NULL)) {
				RadioEvents->RxTimeout();
			}
			break;
		case RF_TX_RUNNING:
			// Tx timeout shouldn't happen.
			// Reported issue of SPI data corruption resulting in TX TIMEOUT
			// is NOT related to a bug in radio transceiver.
			// It is mainly caused by improper PCB routing of SPI lines and/or
			// violation of SPI specifications.
			// To mitigate redesign, Semtech offers a workaround which resets
			// the radio transceiver and putting it into a known state.

			// BEGIN WORKAROUND

			// Reset the radio
			SX1276Reset();

			// Calibrate Rx chain
			RxChainCalibration();

			// Initialize radio default values
			SX1276SetOpMode( RF_OPMODE_SLEEP);

			for (i = 0; i < sizeof(RadioRegsInit) / sizeof(RadioRegisters_t);
					i++) {
				SX1276SetModem(RadioRegsInit[i].Modem);
				spiWrite_RFM(RadioRegsInit[i].Addr, RadioRegsInit[i].Value);
			}
			SX1276SetModem(MODEM_FSK);

			// Restore previous network type setting.
//			SX1276SetPublicNetwork(SX1276.Settings.LoRa.PublicNetwork);
			// END WORKAROUND

			SX1276.Settings.State = RF_IDLE;
			if ((RadioEvents != NULL) && (RadioEvents->TxTimeout != NULL)) {
				RadioEvents->TxTimeout();
			}
			break;
		default:
			break;
	}
}

