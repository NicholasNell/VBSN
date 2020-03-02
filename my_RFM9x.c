/*
 * my_RFM9x.c
 *
 *  Created on: 24 Feb 2020
 *      Author: nicholas
 */
#include "my_RFM9x.h"
#include "my_timer.h"
#include <math.h>



/*
 * Local types definition
 */

/*!
 * Radio registers definition
 */
typedef struct
{
    RadioModems_t Modem;
    uint8_t       Addr;
    uint8_t       Value;
}RadioRegisters_t;

/*!
 * FSK bandwidth definition
 */
typedef struct
{
    uint32_t bandwidth;
    uint8_t  RegValue;
}FskBandwidth_t;


/*!
 * \brief Writes the buffer contents to the SX1276 FIFO
 *
 * \param [IN] buffer Buffer containing data to be put on the FIFO.
 * \param [IN] size Number of bytes to be written to the FIFO
 */
void SX1276WriteFifo( uint8_t *buffer, uint8_t size );

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

/*!
 * Precomputed FSK bandwidth registers values
 */
const FskBandwidth_t FskBandwidths[] =
{
    { 2600  , 0x17 },
    { 3100  , 0x0F },
    { 3900  , 0x07 },
    { 5200  , 0x16 },
    { 6300  , 0x0E },
    { 7800  , 0x06 },
    { 10400 , 0x15 },
    { 12500 , 0x0D },
    { 15600 , 0x05 },
    { 20800 , 0x14 },
    { 25000 , 0x0C },
    { 31300 , 0x04 },
    { 41700 , 0x13 },
    { 50000 , 0x0B },
    { 62500 , 0x03 },
    { 83333 , 0x12 },
    { 100000, 0x0A },
    { 125000, 0x02 },
    { 166700, 0x11 },
    { 200000, 0x09 },
    { 250000, 0x01 },
    { 300000, 0x00 }, // Invalid Bandwidth
};

/*
 * Private global variables
 */

/*!
 * Radio callbacks variable
 */
//static RadioEvents_t *RadioEvents;

/*!
 * Reception buffer
 */
//static uint8_t RxTxBuffer[RX_BUFFER_SIZE];

/*
 * Public global variables
 */

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

uint8_t GetFskBandwidthRegValue( uint32_t bandwidth )
{
    uint8_t i;

    for( i = 0; i < ( sizeof( FskBandwidths ) / sizeof( FskBandwidth_t ) ) - 1; i++ )
    {
        if( ( bandwidth >= FskBandwidths[i].bandwidth ) && ( bandwidth < FskBandwidths[i + 1].bandwidth ) )
        {
            return FskBandwidths[i].RegValue;
        }
    }
    // ERROR: Value not found
    while( 1 );
}

uint8_t SX1276Init(/*RadioEvents_t *events*/)
{

// Set NSS pin HIgh during Normal operation
    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN2);

// Set Reset pin of RFM95W high during normal operation
    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7);

    uint8_t i;

//    RadioEvents = events;

    SX1276Reset();

    RxChainCalibration();

    SX1276SetOpMode(RF_OPMODE_SLEEP);

    SX1276IoIrqInit( );

    for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        SX1276SetModem( RadioRegsInit[i].Modem );
        spiWrite_RFM( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }
    uint8_t j = spiRead_RFM(0x01);
    SX1276SetModem( MODEM_LORA );
    j = spiRead_RFM(0x01);

    SX1276.Settings.State = RF_IDLE;

//    Check RFM Chip Version
    uint8_t version = spiRead_RFM(REG_LR_VERSION);
    if(version != 0x12) return 0;


/*//    Put chip in sleep mode to enable programming

//    Setting up DIO0 as interupt
    GPIO_setAsInputPinWithPullDownResistor(GPIO_PORT_P2, GPIO_PIN4);
    GPIO_clearInterruptFlag(GPIO_PORT_P2, GPIO_PIN4);
    GPIO_enableInterrupt(GPIO_PORT_P2, GPIO_PIN4);
    GPIO_interruptEdgeSelect(GPIO_PORT_P2, GPIO_PIN4, GPIO_LOW_TO_HIGH_TRANSITION);
    Interrupt_enableInterrupt(INT_PORT2);

  Enable the use of the entire FIFO space for receive or transmit. 256 bytes
*   by setting the fifo address pointer to the bottom of memory for both the transmit and recieve buffers.

    i = spiRead_RFM(REG_LR_FIFORXBASEADDR);
    spiWrite_RFM(REG_LR_FIFORXBASEADDR, 0);
    spiWrite_RFM(REG_LR_FIFOTXBASEADDR, 0);
    i = spiRead_RFM(REG_LR_FIFORXBASEADDR);

//    Set LNA Boost: RFI_HF boost to 150%  LNA current
    i = spiRead_RFM(REG_LNA);
    spiWrite_RFM(REG_LNA, spiRead_RFM(REG_LNA) | RFLR_LNA_BOOST_HF_ON);
    i = spiRead_RFM(REG_LNA);

// set auto AGC: LNA gain set by the internal AGC loop
    spiWrite_RFM(REG_LR_MODEMCONFIG3, RFLR_MODEMCONFIG3_AGCAUTO_ON);

    i = spiRead_RFM(REG_OPMODE);
    RFM_stby_mode();
    i = spiRead_RFM(REG_OPMODE);
// set output power to 17 dBm
    RFM_set_TX_power(17);
    uint64_t bw = RFM_get_BW();
    RFM_set_BW(250E3);
    bw = RFM_get_BW();
    RFM_set_CR4(5);
    RFM_set_OCP(100);
    RFM_set_Preamble(8);
    RFM_set_SF(12);
    RFM_set_frequency(868100000);
    RFM_set_syncword(0x55);*/

    return 1;
}

void SX1276IoIrqInit( void ) {
    //  Set interupt
/*        MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN4);
        MAP_GPIO_clearInterruptFlag(GPIO_PORT_P2, GPIO_PIN4);
        MAP_GPIO_enableInterrupt(GPIO_PORT_P2, GPIO_PIN4);
        MAP_Interrupt_enableInterrupt(INT_PORT2);*/
}

RadioState_t SX1276GetStatus( void )
{
    return SX1276.Settings.State;
}

void SX1276SetModem( RadioModems_t modem )
{
    if( ( spiRead_RFM( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) != 0 )
    {
        SX1276.Settings.Modem = MODEM_LORA;
    }
    else
    {
        SX1276.Settings.Modem = MODEM_FSK;
    }

    if( SX1276.Settings.Modem == modem )
    {
        return;
    }

    SX1276.Settings.Modem = modem;
    switch( SX1276.Settings.Modem )
    {
    default:
    case MODEM_FSK:
        SX1276SetOpMode( RF_OPMODE_SLEEP );
        spiWrite_RFM( REG_OPMODE, ( spiRead_RFM( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );

        spiWrite_RFM( REG_DIOMAPPING1, 0x00 );
        spiWrite_RFM( REG_DIOMAPPING2, 0x30 ); // DIO5=ModeReady
        break;
    case MODEM_LORA:
        SX1276SetOpMode( RF_OPMODE_SLEEP );
        spiWrite_RFM( REG_OPMODE, ( spiRead_RFM( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

        spiWrite_RFM( REG_DIOMAPPING1, 0x00 );
        spiWrite_RFM( REG_DIOMAPPING2, 0x00 );
        break;
    }
}

void SX1276SetChannel(uint32_t freq)
{
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    spiWrite_RFM( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    spiWrite_RFM( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    spiWrite_RFM( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

bool SX1276IsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime )
{
    bool status = true;
//    int16_t rssi = 0;
//    uint32_t carrierSenseTime = 0;

    SX1276SetSleep( );

    SX1276SetModem( modem );

    SX1276SetChannel( freq );

    SX1276SetOpMode( RF_OPMODE_RECEIVER );

    Delayms( 1 );

/*    carrierSenseTime = TimerGetCurrentTime( );

    // Perform carrier sense for maxCarrierSenseTime
    while( TimerGetElapsedTime( carrierSenseTime ) < maxCarrierSenseTime )
    {
        rssi = RFM_read_rssi( modem );

        if( rssi > rssiThresh )
        {
            status = false;
            break;
        }
    }
    RFM_sleep_mode( );*/
    return status;
}

uint32_t SX1276Random( void )
{
    uint8_t i;
    uint32_t rnd = 0;

    /*
     * Radio setup for random number generation
     */
    // Set LoRa modem ON
    SX1276SetModem( MODEM_LORA );

    // Disable LoRa modem interrupts
    spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                  RFLR_IRQFLAGS_RXDONE |
                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                  RFLR_IRQFLAGS_VALIDHEADER |
                  RFLR_IRQFLAGS_TXDONE |
                  RFLR_IRQFLAGS_CADDONE |
                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                  RFLR_IRQFLAGS_CADDETECTED );

    // Set radio in continuous reception
    SX1276SetOpMode( RF_OPMODE_RECEIVER );

    for( i = 0; i < 32; i++ )
    {
        Delayms( 1 );
        // Unfiltered RSSI value reading. Only takes the LSB value
        rnd |= ( ( uint32_t )spiRead_RFM( REG_LR_RSSIWIDEBAND ) & 0x01 ) << i;
    }

    SX1276SetSleep( );

    return rnd;
}

void SX1276SetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted, bool rxContinuous )
{
    SX1276SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        {
            SX1276.Settings.Fsk.Bandwidth = bandwidth;
            SX1276.Settings.Fsk.Datarate = datarate;
            SX1276.Settings.Fsk.BandwidthAfc = bandwidthAfc;
            SX1276.Settings.Fsk.FixLen = fixLen;
            SX1276.Settings.Fsk.PayloadLen = payloadLen;
            SX1276.Settings.Fsk.CrcOn = crcOn;
            SX1276.Settings.Fsk.IqInverted = iqInverted;
            SX1276.Settings.Fsk.RxContinuous = rxContinuous;
            SX1276.Settings.Fsk.PreambleLen = preambleLen;
            SX1276.Settings.Fsk.RxSingleTimeout = ( uint32_t )( symbTimeout * ( ( 1.0 / ( double )datarate ) * 8.0 ) * 1000 );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            spiWrite_RFM( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            spiWrite_RFM( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            spiWrite_RFM( REG_RXBW, GetFskBandwidthRegValue( bandwidth ) );
            spiWrite_RFM( REG_AFCBW, GetFskBandwidthRegValue( bandwidthAfc ) );

            spiWrite_RFM( REG_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            spiWrite_RFM( REG_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                spiWrite_RFM( REG_PAYLOADLENGTH, payloadLen );
            }
            else
            {
                spiWrite_RFM( REG_PAYLOADLENGTH, 0xFF ); // Set payload length to the maximum
            }

            spiWrite_RFM( REG_PACKETCONFIG1,
                         ( spiRead_RFM( REG_PACKETCONFIG1 ) &
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
            spiWrite_RFM( REG_PACKETCONFIG2, ( spiRead_RFM( REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
        }
        break;
    case MODEM_LORA:
        {
            if( bandwidth > 2 )
            {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;
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

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }

            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            spiWrite_RFM( REG_LR_MODEMCONFIG1,
                         ( spiRead_RFM( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) |
                           fixLen );

            spiWrite_RFM( REG_LR_MODEMCONFIG2,
                         ( spiRead_RFM( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
                           RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) |
                           ( ( symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

            spiWrite_RFM( REG_LR_MODEMCONFIG3,
                         ( spiRead_RFM( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( SX1276.Settings.LoRa.LowDatarateOptimize << 3 ) );

            spiWrite_RFM( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbTimeout & 0xFF ) );

            spiWrite_RFM( REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
            spiWrite_RFM( REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

            if( fixLen == 1 )
            {
                spiWrite_RFM( REG_LR_PAYLOADLENGTH, payloadLen );
            }

            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                spiWrite_RFM( REG_LR_PLLHOP, ( spiRead_RFM( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                spiWrite_RFM( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod );
            }

            if( ( bandwidth == 9 ) && ( SX1276.Settings.Channel > RF_MID_BAND_THRESH ) )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE1, 0x02 );
                spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE2, 0x64 );
            }
            else if( bandwidth == 9 )
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE1, 0x02 );
                spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE2, 0x7F );
            }
            else
            {
                // ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
                spiWrite_RFM( REG_LR_HIGHBWOPTIMIZE1, 0x03 );
            }

            if( datarate == 6 )
            {
                spiWrite_RFM( REG_LR_DETECTOPTIMIZE,
                             ( spiRead_RFM( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                spiWrite_RFM( REG_LR_DETECTOPTIMIZE,
                             ( spiRead_RFM( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

void SX1276SetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    SX1276SetModem( modem );

    SX1276SetRfTxPower( power );

    switch( modem )
    {
    case MODEM_FSK:
        {
            SX1276.Settings.Fsk.Power = power;
            SX1276.Settings.Fsk.Fdev = fdev;
            SX1276.Settings.Fsk.Bandwidth = bandwidth;
            SX1276.Settings.Fsk.Datarate = datarate;
            SX1276.Settings.Fsk.PreambleLen = preambleLen;
            SX1276.Settings.Fsk.FixLen = fixLen;
            SX1276.Settings.Fsk.CrcOn = crcOn;
            SX1276.Settings.Fsk.IqInverted = iqInverted;
            SX1276.Settings.Fsk.TxTimeout = timeout;

            fdev = ( uint16_t )( ( double )fdev / ( double )FREQ_STEP );
            spiWrite_RFM( REG_FDEVMSB, ( uint8_t )( fdev >> 8 ) );
            spiWrite_RFM( REG_FDEVLSB, ( uint8_t )( fdev & 0xFF ) );

            datarate = ( uint16_t )( ( double )XTAL_FREQ / ( double )datarate );
            spiWrite_RFM( REG_BITRATEMSB, ( uint8_t )( datarate >> 8 ) );
            spiWrite_RFM( REG_BITRATELSB, ( uint8_t )( datarate & 0xFF ) );

            spiWrite_RFM( REG_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            spiWrite_RFM( REG_PREAMBLELSB, preambleLen & 0xFF );

            spiWrite_RFM( REG_PACKETCONFIG1,
                         ( spiRead_RFM( REG_PACKETCONFIG1 ) &
                           RF_PACKETCONFIG1_CRC_MASK &
                           RF_PACKETCONFIG1_PACKETFORMAT_MASK ) |
                           ( ( fixLen == 1 ) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE ) |
                           ( crcOn << 4 ) );
            spiWrite_RFM( REG_PACKETCONFIG2, ( spiRead_RFM( REG_PACKETCONFIG2 ) | RF_PACKETCONFIG2_DATAMODE_PACKET ) );
        }
        break;
    case MODEM_LORA:
        {
            SX1276.Settings.LoRa.Power = power;
            if( bandwidth > 2 )
            {
                // Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
                while( 1 );
            }
            bandwidth += 7;
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

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }
            if( ( ( bandwidth == 7 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 8 ) && ( datarate == 12 ) ) )
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1276.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                spiWrite_RFM( REG_LR_PLLHOP, ( spiRead_RFM( REG_LR_PLLHOP ) & RFLR_PLLHOP_FASTHOP_MASK ) | RFLR_PLLHOP_FASTHOP_ON );
                spiWrite_RFM( REG_LR_HOPPERIOD, SX1276.Settings.LoRa.HopPeriod );
            }

            spiWrite_RFM( REG_LR_MODEMCONFIG1,
                         ( spiRead_RFM( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
                           ( bandwidth << 4 ) | ( coderate << 1 ) |
                           fixLen );

            spiWrite_RFM( REG_LR_MODEMCONFIG2,
                         ( spiRead_RFM( REG_LR_MODEMCONFIG2 ) &
                           RFLR_MODEMCONFIG2_SF_MASK &
                           RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) |
                           ( datarate << 4 ) | ( crcOn << 2 ) );

            spiWrite_RFM( REG_LR_MODEMCONFIG3,
                         ( spiRead_RFM( REG_LR_MODEMCONFIG3 ) &
                           RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) |
                           ( SX1276.Settings.LoRa.LowDatarateOptimize << 3 ) );

            spiWrite_RFM( REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            spiWrite_RFM( REG_LR_PREAMBLELSB, preambleLen & 0xFF );

            if( datarate == 6 )
            {
                spiWrite_RFM( REG_LR_DETECTOPTIMIZE,
                             ( spiRead_RFM( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                spiWrite_RFM( REG_LR_DETECTOPTIMIZE,
                             ( spiRead_RFM( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                spiWrite_RFM( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
        }
        break;
    }
}

uint32_t SX1276GetTimeOnAir( RadioModems_t modem, uint8_t pktLen )
{
    uint32_t airTime = 0;

    switch( modem )
    {
    case MODEM_FSK:
        {
            airTime = ( uint32_t )round( ( 8 * ( SX1276.Settings.Fsk.PreambleLen +
                                     ( ( spiRead_RFM( REG_SYNCCONFIG ) & ~RF_SYNCCONFIG_SYNCSIZE_MASK ) + 1 ) +
                                     ( ( SX1276.Settings.Fsk.FixLen == 0x01 ) ? 0.0 : 1.0 ) +
                                     ( ( ( spiRead_RFM( REG_PACKETCONFIG1 ) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK ) != 0x00 ) ? 1.0 : 0 ) +
                                     pktLen +
                                     ( ( SX1276.Settings.Fsk.CrcOn == 0x01 ) ? 2.0 : 0 ) ) /
                    SX1276.Settings.Fsk.Datarate ) * 1000 );
        }
        break;
    case MODEM_LORA:
        {
            double bw = 0.0;
            // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
            switch( SX1276.Settings.LoRa.Bandwidth )
            {
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
            double rs = bw / ( 1 << SX1276.Settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( SX1276.Settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = ceil( ( 8 * pktLen - 4 * SX1276.Settings.LoRa.Datarate +
                                 28 + 16 * SX1276.Settings.LoRa.CrcOn -
                                 ( SX1276.Settings.LoRa.FixLen ? 20 : 0 ) ) /
                                 ( double )( 4 * ( SX1276.Settings.LoRa.Datarate -
                                 ( ( SX1276.Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) ) *
                                 ( SX1276.Settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
            double tPayload = nPayload * ts;
            // Time on air
            double tOnAir = tPreamble + tPayload;
            // return ms secs
            airTime = ( uint32_t )floor( tOnAir * 1000 + 0.999 );
        }
        break;
    }
    return airTime;
}

void SX1276Send( uint8_t *buffer, uint8_t size )
{
    uint32_t txTimeout = 0;

    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            SX1276.Settings.FskPacketHandler.NbBytes = 0;
            SX1276.Settings.FskPacketHandler.Size = size;

            if( SX1276.Settings.Fsk.FixLen == false )
            {
                SX1276WriteFifo( ( uint8_t* )&size, 1 );
            }
            else
            {
                spiWrite_RFM( REG_PAYLOADLENGTH, size );
            }

            if( ( size > 0 ) && ( size <= 64 ) )
            {
                SX1276.Settings.FskPacketHandler.ChunkSize = size;
            }
            else
            {
//                memcpy1( RxTxBuffer, buffer, size );
                SX1276.Settings.FskPacketHandler.ChunkSize = 32;
            }

            // Write payload buffer
            SX1276WriteFifo( buffer, SX1276.Settings.FskPacketHandler.ChunkSize );
            SX1276.Settings.FskPacketHandler.NbBytes += SX1276.Settings.FskPacketHandler.ChunkSize;
            txTimeout = SX1276.Settings.Fsk.TxTimeout;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276.Settings.LoRa.IqInverted == true )
            {
                spiWrite_RFM( REG_LR_INVERTIQ, ( ( spiRead_RFM( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
                spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                spiWrite_RFM( REG_LR_INVERTIQ, ( ( spiRead_RFM( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            SX1276.Settings.LoRaPacketHandler.Size = size;

            // Initializes the payload size
            spiWrite_RFM( REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx
            spiWrite_RFM( REG_LR_FIFOTXBASEADDR, 0 );
            spiWrite_RFM( REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( spiRead_RFM( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                SX1276SetStby( );
                Delayms( 1 );
            }
            // Write payload buffer
            SX1276WriteFifo( buffer, size );
            txTimeout = SX1276.Settings.LoRa.TxTimeout;
        }
        break;
    }

    SX1276SetTx( txTimeout );
}




void SX1276SetOpMode( uint8_t opMode )
{
    spiWrite_RFM( REG_OPMODE, ( spiRead_RFM( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
}





int16_t SX1276ReadRssi( RadioModems_t modem )
{
    int16_t rssi = 0;

    switch( modem )
    {
    case MODEM_FSK:
        rssi = -( spiRead_RFM( REG_RSSIVALUE ) >> 1 );
        break;
    case MODEM_LORA:
        if( SX1276.Settings.Channel > RF_MID_BAND_THRESH )
        {
            rssi = RSSI_OFFSET_HF + spiRead_RFM( REG_LR_RSSIVALUE );
        }
        else
        {
            rssi = RSSI_OFFSET_LF + spiRead_RFM( REG_LR_RSSIVALUE );
        }
        break;
    default:
        rssi = -1;
        break;
    }
    return rssi;
}




/*void RFM_IoIrqInit(  )
{

}*/

/*
Set the RFM95 chip to sleep mode
*/
bool SX1276SetSleep(void)
{
//    Put into sleep mode and wait 100ms to settle
    spiWrite_RFM(REG_LR_OPMODE, RFLR_OPMODE_LONGRANGEMODE_ON | RFLR_OPMODE_SLEEP);
    Delayms( 1 );
//    Check to see if written correctly
    if (spiRead_RFM(REG_LR_OPMODE) != (RFLR_OPMODE_SLEEP | RFLR_OPMODE_LONGRANGEMODE_ON)) return false;
    return true;
}

/*void RFM_lora_mode(void)
{
    uint8_t opmode = spiRead_RFM(REG_OPMODE);
    opmode |= RFLR_OPMODE_LONGRANGEMODE_ON;
    spiWrite_RFM(REG_OPMODE, opmode);
}*/

void SX1276SetStby(void)
{
//    TimerStop( &RxTimeoutTimer );
//    TimerStop( &TxTimeoutTimer );
//    TimerStop( &RxTimeoutSyncWord );

    SX1276SetOpMode( RF_OPMODE_STANDBY );
    SX1276.Settings.State = RF_IDLE;
}



/*void PORT4_IRQHandler(void)
{

}*/



void SX1276Reset(void)
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);
    Delayms( 1 );
    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
    Delayms( 5 );
}

void RxChainCalibration()
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;

    // Save context
    regPaConfigInitVal = spiRead_RFM( REG_LR_PACONFIG );
    initialFreq = ( double )( ( ( uint32_t )spiRead_RFM( REG_FRFMSB ) << 16 ) |
                              ( ( uint32_t )spiRead_RFM( REG_FRFMID ) << 8 ) |
                              ( ( uint32_t )spiRead_RFM( REG_FRFLSB ) ) ) * ( double )FREQ_STEP;

    // Cut the PA just in case, RFO output, power = -1 dBm
    spiWrite_RFM( REG_LR_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    spiWrite_RFM( REG_IMAGECAL, ( spiRead_RFM( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( spiRead_RFM( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    SX1276SetChannel( 868000000 );

    // Launch Rx chain calibration for HF band
    spiWrite_RFM( REG_IMAGECAL, ( spiRead_RFM( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( spiRead_RFM( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    spiWrite_RFM( REG_PACONFIG, regPaConfigInitVal );
    SX1276SetChannel( initialFreq );
}

void SX1276SetRfTxPower(int8_t power)
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = spiRead_RFM( REG_PACONFIG );
    paDac = spiRead_RFM( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | SX1276GetPaSelect( power );

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
   {
       if( power > 17 )
       {
           paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
       }
       else
       {
           paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
       }
       if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
       {
           if( power < 5 )
           {
               power = 5;
           }
           if( power > 20 )
           {
               power = 20;
           }
           paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
       }
       else
       {
           if( power < 2 )
           {
               power = 2;
           }
           if( power > 17 )
           {
               power = 17;
           }
           paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
       }
   }
   else
   {
       if( power > 0 )
       {
           if( power > 15 )
           {
               power = 15;
           }
           paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( 7 << 4 ) | ( power );
       }
       else
       {
           if( power < -4 )
           {
               power = -4;
           }
           paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( 0 << 4 ) | ( power + 4 );
       }
   }
    spiWrite_RFM( REG_PACONFIG, paConfig );
    spiWrite_RFM( REG_PADAC, paDac );
}

/*void RFM_set_SF(uint8_t sf)
{
    if (sf < 6) {
        sf = 6;
      } else if (sf > 12) {
        sf = 12;
      }

      if (sf == 6)
      {
          spiWrite_RFM(REG_LR_DETECTOPTIMIZE, 0xc5);
          spiWrite_RFM(REG_LR_DETECTIONTHRESHOLD, 0x0c);
      } else
      {
          spiWrite_RFM(REG_LR_DETECTOPTIMIZE, 0xc3);
          spiWrite_RFM(REG_LR_DETECTIONTHRESHOLD, 0x0a);
      }

      spiWrite_RFM(REG_LR_MODEMCONFIG2, (spiRead_RFM(REG_LR_MODEMCONFIG2) & 0x0f) | ((sf << 4) & 0xf0));
      RFM_set_Ldo_Flag(); //Low datarate optimisation
}*/

/*void RFM_set_BW(uint64_t sbw)
{
    uint32_t bw;

      if (sbw <= 7.8E3)
      {
        bw = 0;
      } else if (sbw <= 10.4E3)
      {
        bw = 1;
      } else if (sbw <= 15.6E3)
      {
        bw = 2;
      } else if (sbw <= 20.8E3)
      {
        bw = 3;
      } else if (sbw <= 31.25E3)
      {
        bw = 4;
      } else if (sbw <= 41.7E3)
      {
        bw = 5;
      } else if (sbw <= 62.5E3)
      {
        bw = 6;
      } else if (sbw <= 125E3)
      {
        bw = 7;
      } else if (sbw <= 250E3)
      {
        bw = 8;
      } else if (sbw <= 250E3)
      {
        bw = 9;
      }

      spiWrite_RFM(REG_LR_MODEMCONFIG1, (spiRead_RFM(REG_LR_MODEMCONFIG1) & 0x0f) | (bw << 4));
      RFM_set_Ldo_Flag();
}*/

/*void RFM_set_Ldo_Flag(void)
{
    // Section 4.1.1.5
      uint32_t symbolDuration = 1000 / ( RFM_get_BW() / (1L << RFM_get_SF()) ) ;

      // Section 4.1.1.6
      uint8_t ldoOn = 0b00000100;
      uint8_t config3 = spiRead_RFM(REG_LR_MODEMCONFIG3);

      if (symbolDuration > 16)
      {
          config3 |= ldoOn;
      }
      else
      {
          config3 &= ~ldoOn;
      }

      spiWrite_RFM(REG_LR_MODEMCONFIG3, config3);
}*/

/*void RFM_set_CR4(uint8_t rate)
{
    if (rate < 5) {
        rate = 5;
      } else if (rate > 8) {
        rate = 8;
      }

      uint8_t cr = rate - 4;

      spiWrite_RFM(REG_LR_MODEMCONFIG1, (spiRead_RFM(REG_LR_MODEMCONFIG1) & 0xf1) | (cr << 1));
}*/

/*void RFM_set_Preamble(uint8_t len)
{
    spiWrite_RFM(REG_LR_PREAMBLEMSB, (uint8_t)(len >> 8));
    spiWrite_RFM(REG_LR_PREAMBLELSB, (uint8_t)(len >> 0));
}*/

/*void RFM_set_syncword(uint32_t sw)
{
    spiWrite_RFM(REG_LR_SYNCWORD, sw);
}*/

/*void RFM_set_OCP(uint8_t mA)
{
    uint8_t ocpTrim = 27;

      if (mA <= 120) {
        ocpTrim = (mA - 45) / 5;
      } else if (mA <=240) {
        ocpTrim = (mA + 30) / 10;
      }

      spiWrite_RFM(REG_OCP, 0x20 | (0x1F & ocpTrim));
}*/

/*uint64_t RFM_get_BW(void)
{
    uint8_t bw = (spiRead_RFM(REG_LR_MODEMCONFIG1) >> 4);

      switch (bw) {
        case 0: return 7.8E3;
        case 1: return 10.4E3;
        case 2: return 15.6E3;
        case 3: return 20.8E3;
        case 4: return 31.25E3;
        case 5: return 41.7E3;
        case 6: return 62.5E3;
        case 7: return 125E3;
        case 8: return 250E3;
        case 9: return 500E3;
      }

      return 0;
}*/

/*uint8_t RFM_get_SF(void)
{
    return spiRead_RFM(REG_LR_MODEMCONFIG2) >> 4;
}*/

void SX1276WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1276WriteBuffer( 0, buffer, size );
}

void SX1276WriteBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    RFM95_NSS_LOW;

    RFM_spi_read_write(addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        RFM_spi_read_write( buffer[i] );
    }

    //NSS = 1;
    RFM95_NSS_HIGH;
}

void SX1276SetRx( uint32_t timeout )
{
    bool rxContinuous = false;
//    TimerStop( &TxTimeoutTimer );

    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            rxContinuous = SX1276.Settings.Fsk.RxContinuous;

            // DIO0=PayloadReady
            // DIO1=FifoLevel
            // DIO2=SyncAddr
            // DIO3=FifoEmpty
            // DIO4=Preamble
            // DIO5=ModeReady
            spiWrite_RFM( REG_DIOMAPPING1, ( spiRead_RFM( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO0_00 |
                                                                            RF_DIOMAPPING1_DIO1_00 |
                                                                            RF_DIOMAPPING1_DIO2_11 );

            spiWrite_RFM( REG_DIOMAPPING2, ( spiRead_RFM( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) |
                                                                            RF_DIOMAPPING2_DIO4_11 |
                                                                            RF_DIOMAPPING2_MAP_PREAMBLEDETECT );

            SX1276.Settings.FskPacketHandler.FifoThresh = spiRead_RFM( REG_FIFOTHRESH ) & 0x3F;

            spiWrite_RFM( REG_RXCONFIG, RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON | RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT );

            SX1276.Settings.FskPacketHandler.PreambleDetected = false;
            SX1276.Settings.FskPacketHandler.SyncWordDetected = false;
            SX1276.Settings.FskPacketHandler.NbBytes = 0;
            SX1276.Settings.FskPacketHandler.Size = 0;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276.Settings.LoRa.IqInverted == true )
            {
                spiWrite_RFM( REG_LR_INVERTIQ, ( ( spiRead_RFM( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
                spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                spiWrite_RFM( REG_LR_INVERTIQ, ( ( spiRead_RFM( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                spiWrite_RFM( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            // ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
            if( SX1276.Settings.LoRa.Bandwidth < 9 )
            {
                spiWrite_RFM( REG_LR_DETECTOPTIMIZE, spiRead_RFM( REG_LR_DETECTOPTIMIZE ) & 0x7F );
                spiWrite_RFM( REG_LR_IFFREQ2, 0x00 );
                switch( SX1276.Settings.LoRa.Bandwidth )
                {
                case 0: // 7.8 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x48 );
                    SX1276SetChannel(SX1276.Settings.Channel + 7810 );
                    break;
                case 1: // 10.4 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x44 );
                    SX1276SetChannel(SX1276.Settings.Channel + 10420 );
                    break;
                case 2: // 15.6 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x44 );
                    SX1276SetChannel(SX1276.Settings.Channel + 15620 );
                    break;
                case 3: // 20.8 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x44 );
                    SX1276SetChannel(SX1276.Settings.Channel + 20830 );
                    break;
                case 4: // 31.2 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x44 );
                    SX1276SetChannel(SX1276.Settings.Channel + 31250 );
                    break;
                case 5: // 41.4 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x44 );
                    SX1276SetChannel(SX1276.Settings.Channel + 41670 );
                    break;
                case 6: // 62.5 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x40 );
                    break;
                case 7: // 125 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x40 );
                    break;
                case 8: // 250 kHz
                    spiWrite_RFM( REG_LR_IFFREQ1, 0x40 );
                    break;
                }
            }
            else
            {
                spiWrite_RFM( REG_LR_DETECTOPTIMIZE, spiRead_RFM( REG_LR_DETECTOPTIMIZE ) | 0x80 );
            }

            rxContinuous = SX1276.Settings.LoRa.RxContinuous;

            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                spiWrite_RFM( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone, DIO2=FhssChangeChannel
                spiWrite_RFM( REG_DIOMAPPING1, ( spiRead_RFM( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK  ) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                spiWrite_RFM( REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
                                                  //RFLR_IRQFLAGS_RXDONE |
                                                  //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=RxDone
                spiWrite_RFM( REG_DIOMAPPING1, ( spiRead_RFM( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );
            }
            spiWrite_RFM( REG_LR_FIFORXBASEADDR, 0 );
            spiWrite_RFM( REG_LR_FIFOADDRPTR, 0 );
        }
        break;
    }

//    memset1( RxTxBuffer, 0, ( size_t )RX_BUFFER_SIZE );

    SX1276.Settings.State = RF_RX_RUNNING;
    if( timeout != 0 )
    {
/*        TimerSetValue( &RxTimeoutTimer, timeout );
        TimerStart( &RxTimeoutTimer );*/
    }

    if( SX1276.Settings.Modem == MODEM_FSK )
    {
        SX1276SetOpMode( RF_OPMODE_RECEIVER );

/*        TimerSetValue( &RxTimeoutSyncWord, RFM95.Settings.Fsk.RxSingleTimeout );
        TimerStart( &RxTimeoutSyncWord );*/
    }
    else
    {
        if( rxContinuous == true )
        {
            SX1276SetOpMode( RFLR_OPMODE_RECEIVER );
        }
        else
        {
            SX1276SetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
        }
    }
}

void SX1276StartCad( void )
{
    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {

        }
        break;
    case MODEM_LORA:
        {
            spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        RFLR_IRQFLAGS_TXDONE |
                                        //RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //RFLR_IRQFLAGS_CADDETECTED
                                        );

            // DIO3=CADDone
            spiWrite_RFM( REG_DIOMAPPING1, ( spiRead_RFM( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO3_MASK ) | RFLR_DIOMAPPING1_DIO3_00 );

            SX1276.Settings.State = RF_CAD;
            SX1276SetOpMode( RFLR_OPMODE_CAD );
        }
        break;
    default:
        break;
    }
}


void SX1276SetTxContinuousWave( uint32_t freq, int8_t power, uint16_t time )
{
    uint32_t timeout = ( uint32_t )( time * 1000 );

    SX1276SetChannel( freq );

    SX1276SetTxConfig( MODEM_FSK, power, 0, 0, 4800, 0, 5, false, false, 0, 0, 0, timeout );

    spiWrite_RFM( REG_PACKETCONFIG2, ( spiRead_RFM( REG_PACKETCONFIG2 ) & RF_PACKETCONFIG2_DATAMODE_MASK ) );
    // Disable radio interrupts
    spiWrite_RFM( REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_11 | RF_DIOMAPPING1_DIO1_11 );
    spiWrite_RFM( REG_DIOMAPPING2, RF_DIOMAPPING2_DIO4_10 | RF_DIOMAPPING2_DIO5_10 );

//    TimerSetValue( &TxTimeoutTimer, timeout );

    SX1276.Settings.State = RF_TX_RUNNING;
//    TimerStart( &TxTimeoutTimer );
    SX1276SetOpMode( RF_OPMODE_TRANSMITTER );
}

void SX1276ReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    RFM95_NSS_LOW;

    RFM_spi_read_write( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = RFM_spi_read_write( 0 );
    }

    //NSS = 1;
    RFM95_NSS_HIGH;
}

void SX1276SetMaxPayloadLength( RadioModems_t modem, uint8_t max )
{
    SX1276SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        if( SX1276.Settings.Fsk.FixLen == false )
        {
            spiWrite_RFM( REG_PAYLOADLENGTH, max );
        }
        break;
    case MODEM_LORA:
        spiWrite_RFM( REG_LR_PAYLOADMAXLENGTH, max );
        break;
    }
}

void SX1276SetPublicNetwork( bool enable )
{
    SX1276SetModem( MODEM_LORA );
    SX1276.Settings.LoRa.PublicNetwork = enable;
    if( enable == true )
    {
        // Change LoRa modem SyncWord
        spiWrite_RFM( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD );
    }
    else
    {
        // Change LoRa modem SyncWord
        spiWrite_RFM( REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD );
    }
}

void SX1276SetTx( uint32_t timeout )
{
/*    TimerStop( &RxTimeoutTimer );

    TimerSetValue( &TxTimeoutTimer, timeout );*/

    switch( SX1276.Settings.Modem )
    {
    case MODEM_FSK:
        {
            // DIO0=PacketSent
            // DIO1=FifoEmpty
            // DIO2=FifoFull
            // DIO3=FifoEmpty
            // DIO4=LowBat
            // DIO5=ModeReady
            spiWrite_RFM( REG_DIOMAPPING1, ( spiRead_RFM( REG_DIOMAPPING1 ) & RF_DIOMAPPING1_DIO0_MASK &
                                                                            RF_DIOMAPPING1_DIO1_MASK &
                                                                            RF_DIOMAPPING1_DIO2_MASK ) |
                                                                            RF_DIOMAPPING1_DIO1_01 );

            spiWrite_RFM( REG_DIOMAPPING2, ( spiRead_RFM( REG_DIOMAPPING2 ) & RF_DIOMAPPING2_DIO4_MASK &
                                                                            RF_DIOMAPPING2_MAP_MASK ) );
            SX1276.Settings.FskPacketHandler.FifoThresh = spiRead_RFM( REG_FIFOTHRESH ) & 0x3F;
        }
        break;
    case MODEM_LORA:
        {
            if( SX1276.Settings.LoRa.FreqHopOn == true )
            {
                spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone, DIO2=FhssChangeChannel
                spiWrite_RFM( REG_DIOMAPPING1, ( spiRead_RFM( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK ) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00 );
            }
            else
            {
                spiWrite_RFM( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                                  RFLR_IRQFLAGS_RXDONE |
                                                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                                  RFLR_IRQFLAGS_VALIDHEADER |
                                                  //RFLR_IRQFLAGS_TXDONE |
                                                  RFLR_IRQFLAGS_CADDONE |
                                                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                                  RFLR_IRQFLAGS_CADDETECTED );

                // DIO0=TxDone
                spiWrite_RFM( REG_DIOMAPPING1, ( spiRead_RFM( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );
            }
        }
        break;
    }

    SX1276.Settings.State = RF_TX_RUNNING;
//    TimerStart( &TxTimeoutTimer );
    SX1276SetOpMode( RF_OPMODE_TRANSMITTER );
}

uint8_t SX1276GetPaSelect( int8_t power )
{
    if( power > 14 )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

