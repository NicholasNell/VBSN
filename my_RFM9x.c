/*
 * my_RFM9x.c
 *
 *  Created on: 24 Feb 2020
 *      Author: nicholas
 */

#include <my_RFM9x.h>
#include <my_spi.h>
#include <my_timer.h>
#include <stddef.h>
#include <string.h>
#include <sx1276-board.h>
#include <sx1276Regs-Fsk.h>
#include <sx1276Regs-LoRa.h>
#include <timer.h>

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
void SX1276WriteFifo(uint8_t *buffer, uint8_t size);

/*!
 * \brief Sets the SX1276 in transmission mode for the given time
 * \param [IN] timeout Transmission timeout [ms] [0: continuous, others timeout]
 */
void SX1276SetTx(uint32_t timeout);

/*!
 * Performs the Rx chain calibration for LF and HF bands
 * \remark Must be called just after the reset so all registers are at their
 *         default values
 */
void RxChainCalibration(void);

/*!
 * \brief Sets the SX1276 operating mode
 *
 * \param [IN] opMode New operating mode
 */
void SX1276SetOpMode(uint8_t opMode);

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

/*!
 * Precomputed FSK bandwidth registers values
 */
const FskBandwidth_t FskBandwidths[] = { { 2600, 0x17 }, { 3100, 0x0F }, { 3900,
		0x07 }, { 5200, 0x16 }, { 6300, 0x0E }, { 7800, 0x06 }, { 10400, 0x15 },
		{ 12500, 0x0D }, { 15600, 0x05 }, { 20800, 0x14 }, { 25000, 0x0C }, {
				31300, 0x04 }, { 41700, 0x13 }, { 50000, 0x0B },
		{ 62500, 0x03 }, { 83333, 0x12 }, { 100000, 0x0A }, { 125000, 0x02 }, {
				166700, 0x11 }, { 200000, 0x09 }, { 250000, 0x01 }, { 300000,
				0x00 }, // Invalid Bandwidth
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

/**
 * Compute the numerator for GFSK time-on-air computation.
 *
 * \remark To get the actual time-on-air in second, this value has to be divided by the GFSK bitrate in bits per
 * second.
 *
 * \param [in] preambleLen
 * \param [in] fixLen
 * \param [in] payloadLen
 * \param [in] crcOn
 *
 * \returns GFSK time-on-air numerator
 */
static uint32_t SX1276GetGfskTimeOnAirNumerator(uint16_t preambleLen,
		bool fixLen, uint8_t payloadLen, bool crcOn);

/**
 * Compute the numerator for LoRa time-on-air computation.
 *
 * \remark To get the actual time-on-air in second, this value has to be divided by the LoRa bandwidth in Hertz.
 *
 * \param [in] bandwidth
 * \param [in] datarate
 * \param [in] coderate
 * \param [in] preambleLen
 * \param [in] fixLen
 * \param [in] payloadLen
 * \param [in] crcOn
 *
 * \returns LoRa time-on-air numerator
 */
static uint32_t SX1276GetLoRaTimeOnAirNumerator(uint32_t bandwidth,
		uint32_t datarate, uint8_t coderate, uint16_t preambleLen, bool fixLen,
		uint8_t payloadLen, bool crcOn);

/**
 * Get the actual value in Hertz of a given LoRa bandwidth
 *
 * \param [in] bw LoRa bandwidth parameter
 *
 * \returns Actual LoRa bandwidth in Hertz
 */
static uint32_t SX1276GetLoRaBandwidthInHz(uint32_t bw);
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

uint8_t GetFskBandwidthRegValue(uint32_t bandwidth) {
	uint8_t i;

	for (i = 0; i < (sizeof(FskBandwidths) / sizeof(FskBandwidth_t)) - 1; i++) {
		if ((bandwidth >= FskBandwidths[i].bandwidth)
				&& (bandwidth < FskBandwidths[i + 1].bandwidth)) {
			return FskBandwidths[i].RegValue;
		}
	}
	return 0;
}

void SX1276Init(RadioEvents_t *events) {
	uint8_t i;

	RadioEvents = events;

	// Initialize driver timeout timers
	timer_init(&TxTimeoutTimer, SX1276OnTimeoutIrq);
	timer_init(&RxTimeoutTimer, SX1276OnTimeoutIrq);
	timer_init(&RxTimeoutSyncWord, SX1276OnTimeoutIrq);

	SX1276Reset();

	RxChainCalibration();

	SX1276SetOpMode(RF_OPMODE_SLEEP);

	SX1276IoIrqInit();

	for (i = 0; i < sizeof(RadioRegsInit) / sizeof(RadioRegisters_t); i++) {
		SX1276SetModem(RadioRegsInit[i].Modem);
		spi_write_rfm(RadioRegsInit[i].Addr, RadioRegsInit[i].Value);
	}

	SX1276SetModem(MODEM_LORA);

	SX1276.Settings.State = RF_IDLE;

//    Check RFM Chip Version
//    uint8_t version = spiRead_RFM(REG_LR_VERSION);
//    if(version != 0x12) return 0;

}

static uint32_t SX1276GetGfskTimeOnAirNumerator(uint16_t preambleLen,
		bool fixLen, uint8_t payloadLen, bool crcOn) {
	const uint8_t syncWordLength = 3;

	return (preambleLen << 3) + ((fixLen == false) ? 8 : 0)
			+ (syncWordLength << 3) + ((payloadLen + (0) + // Address filter size
					((crcOn == true) ? 2 : 0)) << 3);
}

static uint32_t SX1276GetLoRaTimeOnAirNumerator(uint32_t bandwidth,
		uint32_t datarate, uint8_t coderate, uint16_t preambleLen, bool fixLen,
		uint8_t payloadLen, bool crcOn) {
	int32_t crDenom = coderate + 4;
	bool lowDatareOptimize = false;

	// Ensure that the preamble length is at least 12 symbols when using SF5 or
	// SF6
	if ((datarate == 5) || (datarate == 6)) {
		if (preambleLen < 12) {
			preambleLen = 12;
		}
	}

	if (((bandwidth == 0) && ((datarate == 11) || (datarate == 12)))
			|| ((bandwidth == 1) && (datarate == 12))) {
		lowDatareOptimize = true;
	}

	int32_t ceilDenominator;
	int32_t ceilNumerator = (payloadLen << 3) + (crcOn ? 16 : 0)
			- (4 * datarate) + (fixLen ? 0 : 20);

	if (datarate <= 6) {
		ceilDenominator = 4 * datarate;
	} else {
		ceilNumerator += 8;

		if (lowDatareOptimize == true) {
			ceilDenominator = 4 * (datarate - 2);
		} else {
			ceilDenominator = 4 * datarate;
		}
	}

	if (ceilNumerator < 0) {
		ceilNumerator = 0;
	}

	// Perform integral ceil()
	int32_t intermediate = ((ceilNumerator + ceilDenominator - 1)
			/ ceilDenominator) * crDenom + preambleLen + 12;

	if (datarate <= 6) {
		intermediate += 2;
	}

	return (uint32_t) ((4 * intermediate + 1) * (1 << (datarate - 2)));
}

RadioState_t SX1276GetStatus(void) {
	return SX1276.Settings.State;
}

void SX1276SetModem(RadioModems_t modem) {
	if ((spi_read_rfm( REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_ON) != 0) {
		SX1276.Settings.Modem = MODEM_LORA;
	} else {
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
		spi_write_rfm(
		REG_OPMODE,
				(spi_read_rfm( REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK)
						| RFLR_OPMODE_LONGRANGEMODE_OFF);

		spi_write_rfm( REG_DIOMAPPING1, 0x00);
		spi_write_rfm( REG_DIOMAPPING2, 0x30); // DIO5=ModeReady
		break;
	case MODEM_LORA:
		SX1276SetOpMode( RF_OPMODE_SLEEP);
		spi_write_rfm(
		REG_OPMODE,
				(spi_read_rfm( REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK)
						| RFLR_OPMODE_LONGRANGEMODE_ON);

		spi_write_rfm( REG_DIOMAPPING1, 0x00);
		spi_write_rfm( REG_DIOMAPPING2, 0x00);
		break;
	}
}

void SX1276SetChannel(uint32_t freq) {
	SX1276.Settings.Channel = freq;
	if (freq >= 860000000) { // grant access to the high frequency band specific registers

		spi_write_rfm(
		REG_LR_OPMODE, spi_read_rfm(REG_LR_OPMODE) & ~(1 << 3));
	}

	freq = (uint32_t) ((double) freq / (double) FREQ_STEP);
	spi_write_rfm( REG_FRFMSB, (uint8_t) ((freq >> 16) & 0xFF));
	spi_write_rfm( REG_FRFMID, (uint8_t) ((freq >> 8) & 0xFF));
	spi_write_rfm( REG_FRFLSB, (uint8_t) (freq & 0xFF));
}

bool SX1276IsChannelFree(RadioModems_t modem, uint32_t freq, int16_t rssiThresh,
		uint32_t maxCarrierSenseTime) {
	bool status = true;
	int16_t rssi = 0;

	SX1276SetSleep();

	SX1276SetModem(modem);

	SX1276SetChannel(freq);

	SX1276SetOpMode( RF_OPMODE_RECEIVER);

	delay_ms(5);
	start_timer_a_counter(maxCarrierSenseTime, NULL);

	// Perform carrier sense for maxCarrierSenseTime
	while (get_timer_a_counter_value() < maxCarrierSenseTime) {
		rssi = SX1276ReadRssi(modem);

		if (rssi > rssiThresh) {
			status = false;
			break;
		}
	}

//	SX1276SetSleep();
	return status;
}

uint32_t SX1276Random8Bit(void) {
	uint8_t i;
	uint8_t rnd = 0;

	/*
	 * Radio setup for random number generation
	 */
	// Set LoRa modem ON
	SX1276SetModem(MODEM_LORA);

	// Disable LoRa modem interrupts
	spi_write_rfm( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
	RFLR_IRQFLAGS_RXDONE |
	RFLR_IRQFLAGS_PAYLOADCRCERROR |
	RFLR_IRQFLAGS_VALIDHEADER |
	RFLR_IRQFLAGS_TXDONE |
	RFLR_IRQFLAGS_CADDONE |
	RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
	RFLR_IRQFLAGS_CADDETECTED);

	// Set radio in continuous reception
	SX1276SetOpMode( RF_OPMODE_RECEIVER);

	for (i = 0; i < 8; i++) {
		delay_ms(5);
		// Unfiltered RSSI value reading. Only takes the LSB value
		rnd |= ((uint8_t) spi_read_rfm( REG_LR_RSSIWIDEBAND) & 0x01) << i;
	}

	SX1276SetSleep();

	return rnd;
}

uint32_t SX1276Random(void) {
	uint8_t i;
	uint32_t rnd = 0;

	/*
	 * Radio setup for random number generation
	 */
	// Set LoRa modem ON
	SX1276SetModem(MODEM_LORA);

	// Disable LoRa modem interrupts
	spi_write_rfm( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
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
		delay_ms(5);
		// Unfiltered RSSI value reading. Only takes the LSB value
		rnd |= ((uint32_t) spi_read_rfm( REG_LR_RSSIWIDEBAND) & 0x01) << i;
	}

	SX1276SetSleep();

	return rnd;
}

void SX1276SetRxConfig(RadioModems_t modem, uint32_t bandwidth,
		uint32_t datarate, uint8_t coderate, uint32_t bandwidthAfc,
		uint16_t preambleLen, uint16_t symbTimeout, bool fixLen,
		uint8_t payloadLen, bool crcOn, bool freqHopOn, uint8_t hopPeriod,
		bool iqInverted, bool rxContinuous) {
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
		spi_write_rfm( REG_BITRATEMSB, (uint8_t) (datarate >> 8));
		spi_write_rfm( REG_BITRATELSB, (uint8_t) (datarate & 0xFF));

		spi_write_rfm( REG_RXBW, GetFskBandwidthRegValue(bandwidth));
		spi_write_rfm( REG_AFCBW, GetFskBandwidthRegValue(bandwidthAfc));

		spi_write_rfm(
		REG_PREAMBLEMSB, (uint8_t) ((preambleLen >> 8) & 0xFF));
		spi_write_rfm( REG_PREAMBLELSB, (uint8_t) (preambleLen & 0xFF));

		if (fixLen == 1) {
			spi_write_rfm( REG_PAYLOADLENGTH, payloadLen);
		} else {
			spi_write_rfm( REG_PAYLOADLENGTH, 0xFF); // Set payload length to the maximum
		}

		spi_write_rfm(
		REG_PACKETCONFIG1,
				(spi_read_rfm( REG_PACKETCONFIG1) &
				RF_PACKETCONFIG1_CRC_MASK &
				RF_PACKETCONFIG1_PACKETFORMAT_MASK)
						| ((fixLen == 1) ?
								RF_PACKETCONFIG1_PACKETFORMAT_FIXED :
								RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE)
						| (crcOn << 4));
		spi_write_rfm(
		REG_PACKETCONFIG2,
				(spi_read_rfm( REG_PACKETCONFIG2)
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
		} else if (datarate < 6) {
			datarate = 6;
		}

		if (((bandwidth == 7) && ((datarate == 11) || (datarate == 12)))
				|| ((bandwidth == 8) && (datarate == 12))) {
			SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
		} else {
			SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
		}

		spi_write_rfm(
		REG_LR_MODEMCONFIG1,
				(spi_read_rfm( REG_LR_MODEMCONFIG1) &
				RFLR_MODEMCONFIG1_BW_MASK &
				RFLR_MODEMCONFIG1_CODINGRATE_MASK &
				RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) | (bandwidth << 4)
						| (coderate << 1) | fixLen);

		spi_write_rfm(
		REG_LR_MODEMCONFIG2,
				(spi_read_rfm( REG_LR_MODEMCONFIG2) &
				RFLR_MODEMCONFIG2_SF_MASK &
				RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
				RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK) | (datarate << 4)
						| (crcOn << 2)
						| ((symbTimeout >> 8)
								& ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK));

		spi_write_rfm(
		REG_LR_MODEMCONFIG3,
				(spi_read_rfm( REG_LR_MODEMCONFIG3) &
				RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK)
						| (SX1276.Settings.LoRa.LowDatarateOptimize << 3));

		spi_write_rfm(
		REG_LR_SYMBTIMEOUTLSB, (uint8_t) (symbTimeout & 0xFF));

		spi_write_rfm(
		REG_LR_PREAMBLEMSB, (uint8_t) ((preambleLen >> 8) & 0xFF));
		spi_write_rfm( REG_LR_PREAMBLELSB, (uint8_t) (preambleLen & 0xFF));

		if (fixLen == 1) {
			spi_write_rfm( REG_LR_PAYLOADLENGTH, payloadLen);
		}

		if (SX1276.Settings.LoRa.FreqHopOn == true) {
			spi_write_rfm(
			REG_LR_PLLHOP,
					(spi_read_rfm( REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK)
							| RFLR_PLLHOP_FASTHOP_ON);
			spi_write_rfm( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod);
		}

		if ((bandwidth == 9)
				&& (SX1276.Settings.Channel > RF_MID_BAND_THRESH)) {
			// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
			spi_write_rfm( REG_LR_HIGHBWOPTIMIZE1, 0x02);
			spi_write_rfm( REG_LR_HIGHBWOPTIMIZE2, 0x64);
		} else if (bandwidth == 9) {
			// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
			spi_write_rfm( REG_LR_HIGHBWOPTIMIZE1, 0x02);
			spi_write_rfm( REG_LR_HIGHBWOPTIMIZE2, 0x7F);
		} else {
			// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
			spi_write_rfm( REG_LR_HIGHBWOPTIMIZE1, 0x03);
		}

		if (datarate == 6) {
			spi_write_rfm(
			REG_LR_DETECTOPTIMIZE, (spi_read_rfm( REG_LR_DETECTOPTIMIZE) &
			RFLR_DETECTIONOPTIMIZE_MASK) |
			RFLR_DETECTIONOPTIMIZE_SF6);
			spi_write_rfm( REG_LR_DETECTIONTHRESHOLD,
			RFLR_DETECTIONTHRESH_SF6);
		} else {
			spi_write_rfm(
			REG_LR_DETECTOPTIMIZE, (spi_read_rfm( REG_LR_DETECTOPTIMIZE) &
			RFLR_DETECTIONOPTIMIZE_MASK) |
			RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
			spi_write_rfm( REG_LR_DETECTIONTHRESHOLD,
			RFLR_DETECTIONTHRESH_SF7_TO_SF12);
		}
	}
		break;
	}
}

void SX1276SetTxConfig(RadioModems_t modem, int8_t power, uint32_t fdev,
		uint32_t bandwidth, uint32_t datarate, uint8_t coderate,
		uint16_t preambleLen, bool fixLen, bool crcOn, bool freqHopOn,
		uint8_t hopPeriod, bool iqInverted, uint32_t timeout) {
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
		spi_write_rfm( REG_FDEVMSB, (uint8_t) (fdev >> 8));
		spi_write_rfm( REG_FDEVLSB, (uint8_t) (fdev & 0xFF));

		datarate = (uint16_t) ((double) XTAL_FREQ / (double) datarate);
		spi_write_rfm( REG_BITRATEMSB, (uint8_t) (datarate >> 8));
		spi_write_rfm( REG_BITRATELSB, (uint8_t) (datarate & 0xFF));

		spi_write_rfm( REG_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
		spi_write_rfm( REG_PREAMBLELSB, preambleLen & 0xFF);

		spi_write_rfm(
		REG_PACKETCONFIG1,
				(spi_read_rfm( REG_PACKETCONFIG1) &
				RF_PACKETCONFIG1_CRC_MASK &
				RF_PACKETCONFIG1_PACKETFORMAT_MASK)
						| ((fixLen == 1) ?
								RF_PACKETCONFIG1_PACKETFORMAT_FIXED :
								RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE)
						| (crcOn << 4));
		spi_write_rfm(
		REG_PACKETCONFIG2,
				(spi_read_rfm( REG_PACKETCONFIG2)
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
		} else if (datarate < 6) {
			datarate = 6;
		}
		if (((bandwidth == 7) && ((datarate == 11) || (datarate == 12)))
				|| ((bandwidth == 8) && (datarate == 12))) {
			SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
		} else {
			SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
		}
		if (SX1276.Settings.LoRa.FreqHopOn == true) {
//				REG_LR_PLLHOP (0x44) is only available in FSK mode!
//				spiWrite_RFM(
//						REG_LR_PLLHOP,
//						(spiRead_RFM( REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK)
//								| RFLR_PLLHOP_FASTHOP_ON);
			spi_write_rfm( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod);
		}

		spi_write_rfm(
		REG_LR_MODEMCONFIG1,
				(spi_read_rfm( REG_LR_MODEMCONFIG1) &
				RFLR_MODEMCONFIG1_BW_MASK &
				RFLR_MODEMCONFIG1_CODINGRATE_MASK &
				RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) | (bandwidth << 4)
						| (coderate << 1) | fixLen);

		spi_write_rfm(
		REG_LR_MODEMCONFIG2, (spi_read_rfm( REG_LR_MODEMCONFIG2) &
		RFLR_MODEMCONFIG2_SF_MASK &
		RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK) | (datarate << 4) | (crcOn << 2));

		spi_write_rfm(
		REG_LR_MODEMCONFIG3,
				(spi_read_rfm( REG_LR_MODEMCONFIG3) &
				RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK)
						| (SX1276.Settings.LoRa.LowDatarateOptimize << 3));

		spi_write_rfm( REG_LR_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
		spi_write_rfm( REG_LR_PREAMBLELSB, preambleLen & 0xFF);

		if (datarate == 6) {
			spi_write_rfm(
			REG_LR_DETECTOPTIMIZE, (spi_read_rfm( REG_LR_DETECTOPTIMIZE) &
			RFLR_DETECTIONOPTIMIZE_MASK) |
			RFLR_DETECTIONOPTIMIZE_SF6);
			spi_write_rfm( REG_LR_DETECTIONTHRESHOLD,
			RFLR_DETECTIONTHRESH_SF6);
		} else {
			spi_write_rfm(
			REG_LR_DETECTOPTIMIZE, (spi_read_rfm( REG_LR_DETECTOPTIMIZE) &
			RFLR_DETECTIONOPTIMIZE_MASK) |
			RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
			spi_write_rfm( REG_LR_DETECTIONTHRESHOLD,
			RFLR_DETECTIONTHRESH_SF7_TO_SF12);
		}
	}
		break;
	}
}

uint32_t SX1276GetTimeOnAir(RadioModems_t modem, uint32_t bandwidth,
		uint32_t datarate, uint8_t coderate, uint16_t preambleLen, bool fixLen,
		uint8_t payloadLen, bool crcOn) {
	uint32_t numerator = 0;
	uint32_t denominator = 1;

	switch (modem) {
	case MODEM_FSK: {
		numerator = 1000U
				* SX1276GetGfskTimeOnAirNumerator(preambleLen, fixLen,
						payloadLen, crcOn);
		denominator = datarate;
	}
		break;
	case MODEM_LORA: {
		numerator = 1000U
				* SX1276GetLoRaTimeOnAirNumerator(bandwidth, datarate, coderate,
						preambleLen, fixLen, payloadLen, crcOn);
		denominator = SX1276GetLoRaBandwidthInHz(bandwidth);
	}
		break;
	}
	// Perform integral ceil()
	return (numerator + denominator - 1) / denominator;
}

static uint32_t SX1276GetLoRaBandwidthInHz(uint32_t bw) {
	uint32_t bandwidthInHz = 0;

	switch (bw) {
	case 7: // 125 kHz
		bandwidthInHz = 125000UL;
		break;
	case 8: // 250 kHz
		bandwidthInHz = 250000UL;
		break;
	case 9: // 500 kHz
		bandwidthInHz = 500000UL;
		break;
	}

	return bandwidthInHz;
}

void SX1276Send(uint8_t *buffer, uint8_t size) {
	uint32_t txTimeout = 0;

	switch (SX1276.Settings.Modem) {
	case MODEM_FSK: {
		SX1276.Settings.FskPacketHandler.NbBytes = 0;
		SX1276.Settings.FskPacketHandler.Size = size;

		if (SX1276.Settings.Fsk.FixLen == false) {
			SX1276WriteFifo((uint8_t*) &size, 1);
		} else {
			spi_write_rfm( REG_PAYLOADLENGTH, size);
		}

		if ((size > 0) && (size <= 64)) {
			SX1276.Settings.FskPacketHandler.ChunkSize = size;
		} else {
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
			spi_write_rfm(
			REG_LR_INVERTIQ,
					((spi_read_rfm( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
							& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF
							| RFLR_INVERTIQ_TX_ON));
			spi_write_rfm( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
		} else {
			spi_write_rfm(
			REG_LR_INVERTIQ,
					((spi_read_rfm( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
							& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF
							| RFLR_INVERTIQ_TX_OFF));
			spi_write_rfm( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
		}

		SX1276.Settings.LoRaPacketHandler.Size = size;

		// Initializes the payload size
		spi_write_rfm( REG_LR_PAYLOADLENGTH, size);

		// FIFO operations can not take place in Sleep mode
		if ((spi_read_rfm( REG_OPMODE) & ~RF_OPMODE_MASK) == RF_OPMODE_SLEEP) {
			SX1276SetStby();
			delay_ms(5);
		}
		// Full buffer used for Tx
		spi_write_rfm( REG_LR_FIFOTXBASEADDR, 0);
		spi_write_rfm( REG_LR_FIFOADDRPTR, 0);

		// Write payload buffer

		SX1276WriteFifo(buffer, size);

		txTimeout = SX1276.Settings.LoRa.TxTimeout;
	}
		break;
	}

	SX1276SetTx(txTimeout);
	spi_read_rfm(0x0d);
}

void SX1276SetOpMode(uint8_t opMode) {
	spi_write_rfm(
	REG_OPMODE, (spi_read_rfm( REG_OPMODE) & RF_OPMODE_MASK) | opMode);
}

int16_t SX1276ReadRssi(RadioModems_t modem) {
	int16_t rssi = 0;

	switch (modem) {
	case MODEM_FSK:
		rssi = -(spi_read_rfm( REG_RSSIVALUE) >> 1);
		break;
	case MODEM_LORA:
		if (SX1276.Settings.Channel > RF_MID_BAND_THRESH) {
			rssi = RSSI_OFFSET_HF + spi_read_rfm( REG_LR_RSSIVALUE);
		} else {
			rssi = RSSI_OFFSET_LF + spi_read_rfm( REG_LR_RSSIVALUE);
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
void SX1276SetSleep(void) {
	timer_stop(&RxTimeoutTimer);
	timer_stop(&TxTimeoutTimer);
	timer_stop(&RxTimeoutSyncWord);

//    Put into sleep mode and wait 100ms to settle
	SX1276SetOpMode( RF_OPMODE_SLEEP);

	SX1276.Settings.State = RF_IDLE;
	delay_ms(5);
}

void SX1276SetStby(void) {
	timer_stop(&RxTimeoutTimer);
	timer_stop(&TxTimeoutTimer);
	timer_stop(&RxTimeoutSyncWord);

	SX1276SetOpMode( RF_OPMODE_STANDBY);
	SX1276.Settings.State = RF_IDLE;
}

void RxChainCalibration() {
	uint8_t regPaConfigInitVal;
	uint32_t initialFreq;

	// Save context
	regPaConfigInitVal = spi_read_rfm( REG_LR_PACONFIG);
	initialFreq = (double) (((uint32_t) spi_read_rfm( REG_FRFMSB) << 16)
			| ((uint32_t) spi_read_rfm( REG_FRFMID) << 8)
			| ((uint32_t) spi_read_rfm( REG_FRFLSB))) * (double) FREQ_STEP;

	// Cut the PA just in case, RFO output, power = -1 dBm
	spi_write_rfm( REG_LR_PACONFIG, 0x00);

	// Launch Rx chain calibration for LF band
	spi_write_rfm(
	REG_IMAGECAL,
			(spi_read_rfm( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK)
					| RF_IMAGECAL_IMAGECAL_START);
	while ((spi_read_rfm( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING)
			== RF_IMAGECAL_IMAGECAL_RUNNING) {
	};

	// Sets a Frequency in HF band
	SX1276SetChannel(868000000);

	// Launch Rx chain calibration for HF band
	spi_write_rfm(
	REG_IMAGECAL,
			(spi_read_rfm( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK)
					| RF_IMAGECAL_IMAGECAL_START);
	while ((spi_read_rfm( REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING)
			== RF_IMAGECAL_IMAGECAL_RUNNING) {
	};

	// Restore context
	spi_write_rfm( REG_PACONFIG, regPaConfigInitVal);
	SX1276SetChannel(initialFreq);
}

void SX1276WriteFifo(uint8_t *buffer, uint8_t size) {
	SX1276WriteBuffer(0, buffer, size);
}

void SX1276ReadFifo(uint8_t *buffer, uint8_t size) {
	SX1276ReadBuffer(0, buffer, size);
}

void SX1276WriteBuffer(uint16_t addr, uint8_t *buffer, uint8_t size) {
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

void SX1276SetRx(uint32_t timeout) {
	bool rxContinuous = false;
	stop_lora_timer();

	switch (SX1276.Settings.Modem) {
	case MODEM_FSK: {
		rxContinuous = SX1276.Settings.Fsk.RxContinuous;

		// DIO0=PayloadReady
		// DIO1=FifoLevel
		// DIO2=SyncAddr
		// DIO3=FifoEmpty
		// DIO4=Preamble
		// DIO5=ModeReady
		spi_write_rfm(
		REG_DIOMAPPING1,
				(spi_read_rfm( REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK &
				RF_DIOMAPPING1_DIO1_MASK &
				RF_DIOMAPPING1_DIO2_MASK) |
				RF_DIOMAPPING1_DIO0_00 |
				RF_DIOMAPPING1_DIO1_00 |
				RF_DIOMAPPING1_DIO2_11);

		spi_write_rfm(
		REG_DIOMAPPING2,
				(spi_read_rfm( REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK &
				RF_DIOMAPPING2_MAP_MASK) |
				RF_DIOMAPPING2_DIO4_11 |
				RF_DIOMAPPING2_MAP_PREAMBLEDETECT);

		SX1276.Settings.FskPacketHandler.FifoThresh = spi_read_rfm(
		REG_FIFOTHRESH) & 0x3F;

		spi_write_rfm(
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
			spi_write_rfm(
			REG_LR_INVERTIQ,
					((spi_read_rfm( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
							& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_ON
							| RFLR_INVERTIQ_TX_OFF));
			spi_write_rfm( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
		} else {
			spi_write_rfm(
			REG_LR_INVERTIQ,
					((spi_read_rfm( REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK
							& RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF
							| RFLR_INVERTIQ_TX_OFF));
			spi_write_rfm( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
		}

		// ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
		if (SX1276.Settings.LoRa.Bandwidth < 9) {
			spi_write_rfm(
			REG_LR_DETECTOPTIMIZE, spi_read_rfm( REG_LR_DETECTOPTIMIZE) & 0x7F);
			spi_write_rfm( REG_LR_IFFREQ2, 0x00);
			switch (SX1276.Settings.LoRa.Bandwidth) {
			case 0: // 7.8 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x48);
//				SX1276SetChannel(SX1276.Settings.Channel + 7810);
				break;
			case 1: // 10.4 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 10420);
				break;
			case 2: // 15.6 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 15620);
				break;
			case 3: // 20.8 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 20830);
				break;
			case 4: // 31.2 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 31250);
				break;
			case 5: // 41.4 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x44);
//				SX1276SetChannel(SX1276.Settings.Channel + 41670);
				break;
			case 6: // 62.5 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x40);
				break;
			case 7: // 125 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x40);
				break;
			case 8: // 250 kHz
				spi_write_rfm( REG_LR_IFFREQ1, 0x40);
				break;
			}
		} else {
			spi_write_rfm(
			REG_LR_DETECTOPTIMIZE, spi_read_rfm( REG_LR_DETECTOPTIMIZE) | 0x80);
		}

		rxContinuous = SX1276.Settings.LoRa.RxContinuous;

		if (SX1276.Settings.LoRa.FreqHopOn == true) {
			spi_write_rfm( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
												//RFLR_IRQFLAGS_RXDONE |
												//RFLR_IRQFLAGS_PAYLOADCRCERROR |
					RFLR_IRQFLAGS_VALIDHEADER |
					RFLR_IRQFLAGS_TXDONE |
					RFLR_IRQFLAGS_CADDONE |
					//RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
							RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=RxDone, DIO2=FhssChangeChannel
			spi_write_rfm(
			REG_DIOMAPPING1,
					(spi_read_rfm( REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK
							& RFLR_DIOMAPPING1_DIO2_MASK)
							| RFLR_DIOMAPPING1_DIO0_00
							| RFLR_DIOMAPPING1_DIO2_00);
		} else {
			spi_write_rfm( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
												//RFLR_IRQFLAGS_RXDONE |
												//RFLR_IRQFLAGS_PAYLOADCRCERROR |
					RFLR_IRQFLAGS_VALIDHEADER |
					RFLR_IRQFLAGS_TXDONE |
					RFLR_IRQFLAGS_CADDONE |
					RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
					RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=RxDone
			spi_write_rfm(
			REG_DIOMAPPING1,
					(spi_read_rfm( REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK)
							| RFLR_DIOMAPPING1_DIO0_00);
		}
		spi_write_rfm( REG_LR_FIFORXBASEADDR, 0);
		spi_write_rfm( REG_LR_FIFOADDRPTR, 0);
	}
		break;
	}

	memset(RxTxBuffer, 0, (size_t) RX_BUFFER_SIZE);

	SX1276.Settings.State = RF_RX_RUNNING;
	if (timeout != 0) {
		start_lora_timer(timeout);
	}

	if (SX1276.Settings.Modem == MODEM_FSK) {
		SX1276SetOpMode( RF_OPMODE_RECEIVER);

		if (rxContinuous == false) {
			timer_set_value(&RxTimeoutSyncWord,
					SX1276.Settings.Fsk.RxSingleTimeout);
			timer_start(&RxTimeoutSyncWord);
		}
	} else {
		if (rxContinuous == true) {
			SX1276SetOpMode( RFLR_OPMODE_RECEIVER);
		} else {
			SX1276SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE);
		}
	}
}

void SX1276StartCad(void) {
	switch (SX1276.Settings.Modem) {
	case MODEM_FSK: {

	}
		break;
	case MODEM_LORA: {
		spi_write_rfm( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
		RFLR_IRQFLAGS_RXDONE |
		RFLR_IRQFLAGS_PAYLOADCRCERROR |
		RFLR_IRQFLAGS_VALIDHEADER |
		RFLR_IRQFLAGS_TXDONE |
		//RFLR_IRQFLAGS_CADDONE |
				RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL// |
		//RFLR_IRQFLAGS_CADDETECTED
				);

		// DIO3=CADDone
		spi_write_rfm(
		REG_DIOMAPPING1,
				(spi_read_rfm( REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO3_MASK)
						| RFLR_DIOMAPPING1_DIO3_00);

		SX1276.Settings.State = RF_CAD;
		SX1276SetOpMode( RFLR_OPMODE_CAD);
	}
		break;
	default:
		break;
	}
}

// Don't need this function, it only takes up space
/*
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
 */

void SX1276ReadBuffer(uint16_t addr, uint8_t *buffer, uint8_t size) {
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

void SX1276SetMaxPayloadLength(RadioModems_t modem, uint8_t max) {
	SX1276SetModem(modem);

	switch (modem) {
	case MODEM_FSK:
		if (SX1276.Settings.Fsk.FixLen == false) {
			spi_write_rfm( REG_PAYLOADLENGTH, max);
		}
		break;
	case MODEM_LORA:
		spi_write_rfm( REG_LR_PAYLOADMAXLENGTH, max);
		break;
	}
}

void SX1276SetPublicNetwork(bool enable, uint16_t syncword) {
	SX1276SetModem(MODEM_LORA);
	SX1276.Settings.LoRa.PublicNetwork = enable;
	if (enable == true) {
		// Change LoRa modem SyncWord
		spi_write_rfm( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD);
	} else {
		// Change LoRa modem SyncWord
		spi_write_rfm( REG_LR_SYNCWORD, syncword);
	}
}

void SX1276SetTx(uint32_t timeout) {
	stop_lora_timer();

	switch (SX1276.Settings.Modem) {
	case MODEM_FSK: {
		// DIO0=PacketSent
		// DIO1=FifoEmpty
		// DIO2=FifoFull
		// DIO3=FifoEmpty
		// DIO4=LowBat
		// DIO5=ModeReady
		spi_write_rfm(
		REG_DIOMAPPING1,
				(spi_read_rfm( REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK &
				RF_DIOMAPPING1_DIO1_MASK &
				RF_DIOMAPPING1_DIO2_MASK) |
				RF_DIOMAPPING1_DIO1_01);

		spi_write_rfm(
		REG_DIOMAPPING2,
				(spi_read_rfm( REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK &
				RF_DIOMAPPING2_MAP_MASK));
		SX1276.Settings.FskPacketHandler.FifoThresh = spi_read_rfm(
		REG_FIFOTHRESH) & 0x3F;
	}
		break;
	case MODEM_LORA: {
		if (SX1276.Settings.LoRa.FreqHopOn == true) {
			spi_write_rfm( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
			RFLR_IRQFLAGS_RXDONE |
			RFLR_IRQFLAGS_PAYLOADCRCERROR |
			RFLR_IRQFLAGS_VALIDHEADER |
			//RFLR_IRQFLAGS_TXDONE |
					RFLR_IRQFLAGS_CADDONE |
					//RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
					RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=TxDone, DIO2=FhssChangeChannel
			spi_write_rfm(
			REG_DIOMAPPING1,
					(spi_read_rfm( REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK
							& RFLR_DIOMAPPING1_DIO2_MASK)
							| RFLR_DIOMAPPING1_DIO0_01
							| RFLR_DIOMAPPING1_DIO2_00);
		} else {
			spi_write_rfm( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
			RFLR_IRQFLAGS_RXDONE |
			RFLR_IRQFLAGS_PAYLOADCRCERROR |
			RFLR_IRQFLAGS_VALIDHEADER |
			//RFLR_IRQFLAGS_TXDONE |
					RFLR_IRQFLAGS_CADDONE |
					RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
					RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=TxDone
			spi_write_rfm(
			REG_DIOMAPPING1,
					(spi_read_rfm( REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK)
							| RFLR_DIOMAPPING1_DIO0_01);
		}
	}
		break;
	}

	SX1276.Settings.State = RF_TX_RUNNING;
	if (timeout > 0)
		start_lora_timer(timeout);
	SX1276SetOpMode( RF_OPMODE_TRANSMITTER);
}

uint8_t SX1276GetPaSelect(int8_t power) {
	if (power > 14) {
		return RF_PACONFIG_PASELECT_PABOOST;
	} else {
		return RF_PACONFIG_PASELECT_RFO;
	}
}

uint32_t SX1276GetWakeupTime(void) {
	return SX1276GetBoardTcxoWakeupTime() + RADIO_WAKEUP_TIME;
}

int16_t SX1276GetPacketRSSI(void) {
	int v = spi_read_rfm( REG_LR_PKTRSSIVALUE);
	uint8_t temp = spi_read_rfm(REG_LR_OPMODE)
			& ~(RFLR_OPMODE_FREQMODE_ACCESS_MASK);
	if (temp == RFLR_OPMODE_FREQMODE_ACCESS_HF) {
		return RSSI_OFFSET_HF + v;

	} else if (temp == RFLR_OPMODE_FREQMODE_ACCESS_LF) {
		return RSSI_OFFSET_LF + v;
	} else {
		return 0;
	}
}

int16_t SX1276GetPacketSNR(void) {
	int v = spi_read_rfm(REG_LR_PKTSNRVALUE);
	if (v > 127) {
		return (float) (v - 256);
	} else {
		return (float) (v) / 4.0;
	}
}

uint8_t SX1276GetPacketSize(void) {
	return spi_read_rfm( REG_LR_RXNBBYTES);
}

void SX1276OnDio0Irq() {
	volatile uint8_t irqFlags = 0;

	switch (SX1276.Settings.State) {
	case RF_RX_RUNNING:
		//TimerStop( &RxTimeoutTimer );
		// RxDone interrupt
		switch (SX1276.Settings.Modem) {
		case MODEM_FSK:
			if (SX1276.Settings.Fsk.CrcOn == true) {
				irqFlags = spi_read_rfm( REG_IRQFLAGS2);
				if ((irqFlags & RF_IRQFLAGS2_CRCOK) != RF_IRQFLAGS2_CRCOK) {
					// Clear Irqs
					spi_write_rfm( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
					RF_IRQFLAGS1_PREAMBLEDETECT |
					RF_IRQFLAGS1_SYNCADDRESSMATCH);
					spi_write_rfm(
					REG_IRQFLAGS2,
					RF_IRQFLAGS2_FIFOOVERRUN);

					timer_stop(&RxTimeoutTimer);

					if (SX1276.Settings.Fsk.RxContinuous == false) {
						timer_stop(&RxTimeoutSyncWord);
						SX1276.Settings.State = RF_IDLE;
					} else {
						// Continuous mode restart Rx chain
						spi_write_rfm(
						REG_RXCONFIG, spi_read_rfm(
						REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
					}

					RadioEvents->RxError();

					SX1276.Settings.FskPacketHandler.PreambleDetected = false;
					SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
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
				} else {
					SX1276.Settings.FskPacketHandler.Size = spi_read_rfm(
					REG_PAYLOADLENGTH);
				}
				SX1276ReadFifo(
						RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes,
						SX1276.Settings.FskPacketHandler.Size
								- SX1276.Settings.FskPacketHandler.NbBytes);
				SX1276.Settings.FskPacketHandler.NbBytes +=
						(SX1276.Settings.FskPacketHandler.Size
								- SX1276.Settings.FskPacketHandler.NbBytes);
			} else {
				SX1276ReadFifo(
						RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes,
						SX1276.Settings.FskPacketHandler.Size
								- SX1276.Settings.FskPacketHandler.NbBytes);
				SX1276.Settings.FskPacketHandler.NbBytes +=
						(SX1276.Settings.FskPacketHandler.Size
								- SX1276.Settings.FskPacketHandler.NbBytes);
			}

			timer_stop(&RxTimeoutTimer);

			if (SX1276.Settings.Fsk.RxContinuous == false) {
				SX1276.Settings.State = RF_IDLE;
				timer_stop(&RxTimeoutSyncWord);
			} else {
				// Continuous mode restart Rx chain
				spi_write_rfm(
				REG_RXCONFIG, spi_read_rfm(
				REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
			}

			RadioEvents->RxDone(RxTxBuffer,
					SX1276.Settings.FskPacketHandler.Size,
					SX1276.Settings.FskPacketHandler.RssiValue, 0);

			SX1276.Settings.FskPacketHandler.PreambleDetected = false;
			SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
			SX1276.Settings.FskPacketHandler.NbBytes = 0;
			SX1276.Settings.FskPacketHandler.Size = 0;
			break;
		case MODEM_LORA: {
			// Clear Irq
			spi_write_rfm( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE);

			irqFlags = spi_read_rfm( REG_LR_IRQFLAGS);
			if ((irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK)
					== RFLR_IRQFLAGS_PAYLOADCRCERROR) {
				// Clear Irq
				spi_write_rfm(
				REG_LR_IRQFLAGS,
				RFLR_IRQFLAGS_PAYLOADCRCERROR);
				if (SX1276.Settings.LoRa.RxContinuous == false) {
					SX1276.Settings.State = RF_IDLE;
				}
				timer_stop(&RxTimeoutTimer);
				if ((RadioEvents != NULL) && (RadioEvents->RxError != NULL)) {
					RadioEvents->RxError();
				}
				break;
			}
			// Returns SNR value [dB] rounded to the nearest integer value
			SX1276.Settings.LoRaPacketHandler.SnrValue =
					(((int8_t) spi_read_rfm(
					REG_LR_PKTSNRVALUE)) + 2) >> 2;

			int16_t rssi = spi_read_rfm( REG_LR_PKTRSSIVALUE);
			if (SX1276.Settings.LoRaPacketHandler.SnrValue < 0) {
				if (SX1276.Settings.Channel > RF_MID_BAND_THRESH) {
					SX1276.Settings.LoRaPacketHandler.RssiValue =
					RSSI_OFFSET_HF + rssi + (rssi >> 4)
							+ SX1276.Settings.LoRaPacketHandler.SnrValue;
				} else {
					SX1276.Settings.LoRaPacketHandler.RssiValue =
					RSSI_OFFSET_LF + rssi + (rssi >> 4)
							+ SX1276.Settings.LoRaPacketHandler.SnrValue;
				}
			} else {
				if (SX1276.Settings.Channel > RF_MID_BAND_THRESH) {
					SX1276.Settings.LoRaPacketHandler.RssiValue =
					RSSI_OFFSET_HF + rssi + (rssi >> 4);
				} else {
					SX1276.Settings.LoRaPacketHandler.RssiValue =
					RSSI_OFFSET_LF + rssi + (rssi >> 4);
				}
			}

			SX1276.Settings.LoRaPacketHandler.Size = spi_read_rfm(
			REG_LR_RXNBBYTES);

			spi_write_rfm(
			REG_LR_FIFOADDRPTR, spi_read_rfm( REG_LR_FIFORXCURRENTADDR));
//
			SX1276ReadFifo(RxTxBuffer, SX1276.Settings.LoRaPacketHandler.Size);

			if (SX1276.Settings.LoRa.RxContinuous == false) {
				SX1276.Settings.State = RF_IDLE;
			}

			timer_stop(&RxTimeoutTimer);

			if ((RadioEvents != NULL) && (RadioEvents->RxDone != NULL)) {
				RadioEvents->RxDone(RxTxBuffer,
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
		timer_stop(&TxTimeoutTimer);
		// TxDone interrupt
		switch (SX1276.Settings.Modem) {
		case MODEM_LORA:
			// Clear Irq
			spi_write_rfm( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE);
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

void SX1276OnDio1Irq() {
	stop_lora_timer();
	switch (SX1276.Settings.State) {
	case RF_RX_RUNNING:
		switch (SX1276.Settings.Modem) {
		case MODEM_FSK:
			// Stop timer
			timer_stop(&RxTimeoutSyncWord);
			// FifoLevel interrupt
			// Read received packet size
			if ((SX1276.Settings.FskPacketHandler.Size == 0)
					&& (SX1276.Settings.FskPacketHandler.NbBytes == 0)) {
				if (SX1276.Settings.Fsk.FixLen == false) {
					SX1276ReadFifo(
							(uint8_t*) &SX1276.Settings.FskPacketHandler.Size,
							1);
				} else {
					SX1276.Settings.FskPacketHandler.Size = spi_read_rfm(
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
						(RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes),
						SX1276.Settings.FskPacketHandler.FifoThresh - 1);
				SX1276.Settings.FskPacketHandler.NbBytes +=
						SX1276.Settings.FskPacketHandler.FifoThresh - 1;
			} else {
				SX1276ReadFifo(
						(RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes),
						SX1276.Settings.FskPacketHandler.Size
								- SX1276.Settings.FskPacketHandler.NbBytes);
				SX1276.Settings.FskPacketHandler.NbBytes +=
						(SX1276.Settings.FskPacketHandler.Size
								- SX1276.Settings.FskPacketHandler.NbBytes);
			}
			break;
		case MODEM_LORA:
			// Sync time out
			timer_stop(&RxTimeoutTimer);
			// Clear Irq
			spi_write_rfm( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT);

			SX1276.Settings.State = RF_IDLE;
			if ((RadioEvents != NULL) && (RadioEvents->RxTimeout != NULL)) {
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
						(RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes),
						SX1276.Settings.FskPacketHandler.ChunkSize);
				SX1276.Settings.FskPacketHandler.NbBytes +=
						SX1276.Settings.FskPacketHandler.ChunkSize;
			} else {
				// Write the last chunk of data
				SX1276WriteFifo(
						RxTxBuffer + SX1276.Settings.FskPacketHandler.NbBytes,
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

void SX1276OnDio2Irq() {
	switch (SX1276.Settings.State) {
	case RF_RX_RUNNING:
		switch (SX1276.Settings.Modem) {
		case MODEM_FSK:
			// Checks if DIO4 is connected. If it is not PreambleDetected is set to true.
			if (SX1276.DIO4.port == NULL) {
				SX1276.Settings.FskPacketHandler.PreambleDetected = true;
			}

			if ((SX1276.Settings.FskPacketHandler.PreambleDetected == true)
					&& (SX1276.Settings.FskPacketHandler.SyncWordDetected
							== false)) {
				timer_stop(&RxTimeoutSyncWord);

				SX1276.Settings.FskPacketHandler.SyncWordDetected = true;

				SX1276.Settings.FskPacketHandler.RssiValue = -(spi_read_rfm(
				REG_RSSIVALUE) >> 1);

				SX1276.Settings.FskPacketHandler.AfcValue =
						(int32_t) (double) (((uint16_t) spi_read_rfm(
						REG_AFCMSB) << 8) | (uint16_t) spi_read_rfm( REG_AFCLSB))
								* (double) FREQ_STEP;
				SX1276.Settings.FskPacketHandler.RxGain = (spi_read_rfm(
				REG_LNA) >> 5) & 0x07;
			}
			break;
		case MODEM_LORA:
			if (SX1276.Settings.LoRa.FreqHopOn == true) {
				// Clear Irq
				spi_write_rfm(
				REG_LR_IRQFLAGS,
				RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

				if ((RadioEvents != NULL)
						&& (RadioEvents->FhssChangeChannel != NULL)) {
					RadioEvents->FhssChangeChannel(
							(spi_read_rfm( REG_LR_HOPCHANNEL)
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
				spi_write_rfm(
				REG_LR_IRQFLAGS,
				RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

				if ((RadioEvents != NULL)
						&& (RadioEvents->FhssChangeChannel != NULL)) {
					RadioEvents->FhssChangeChannel(
							(spi_read_rfm( REG_LR_HOPCHANNEL)
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

void SX1276OnDio3Irq() {
	stop_lora_timer();
	switch (SX1276.Settings.Modem) {
	case MODEM_FSK:
		break;
	case MODEM_LORA:
		if ((spi_read_rfm( REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_CADDETECTED)
				== RFLR_IRQFLAGS_CADDETECTED) {
			// Clear Irq
			spi_write_rfm(
			REG_LR_IRQFLAGS,
			RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE);
			if ((RadioEvents != NULL) && (RadioEvents->CadDone != NULL)) {
				RadioEvents->CadDone(true);
			}
		} else {
			// Clear Irq
			spi_write_rfm( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE);
			if ((RadioEvents != NULL) && (RadioEvents->CadDone != NULL)) {
				RadioEvents->CadDone(false);
			}
		}
		break;
	default:
		break;
	}
}

void SX1276OnDio4Irq() {
	stop_lora_timer();
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

void SX1276OnDio5Irq() {

	switch (SX1276.Settings.Modem) {
	case MODEM_FSK:
		break;
	case MODEM_LORA:
		break;
	default:
		break;
	}
}

void SX1276OnTimeoutIrq() {
	uint8_t i = 0;
	switch (SX1276.Settings.State) {
	case RF_RX_RUNNING:
		if (SX1276.Settings.Modem == MODEM_FSK) {
			SX1276.Settings.FskPacketHandler.PreambleDetected = false;
			SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
			SX1276.Settings.FskPacketHandler.NbBytes = 0;
			SX1276.Settings.FskPacketHandler.Size = 0;

			// Clear Irqs
			spi_write_rfm( REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
			RF_IRQFLAGS1_PREAMBLEDETECT |
			RF_IRQFLAGS1_SYNCADDRESSMATCH);
			spi_write_rfm( REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);

			if (SX1276.Settings.Fsk.RxContinuous == true) {
				// Continuous mode restart Rx chain
				spi_write_rfm(
				REG_RXCONFIG, spi_read_rfm(
				REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
			} else {
				SX1276.Settings.State = RF_IDLE;
				timer_stop(&RxTimeoutSyncWord);
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

		for (i = 0; i < sizeof(RadioRegsInit) / sizeof(RadioRegisters_t); i++) {
			SX1276SetModem(RadioRegsInit[i].Modem);
			spi_write_rfm(RadioRegsInit[i].Addr, RadioRegsInit[i].Value);
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

void SX1276clearIRQFlags(void) {
	spi_write_rfm(REG_LR_IRQFLAGS, 0xff); //writing a 1 clears the bit
}

