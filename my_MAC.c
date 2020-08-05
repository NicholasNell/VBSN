/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_MAC.h"
#include <string.h>

AlohaState_t AlohaState = SLEEP;
uint8_t seqid = 0;

uint8_t attempts = 0;
extern AppStates_t State;
extern uint8_t Buffer[];
uint8_t size;

uint16_t delay;

bool sendALOHAmessage( uint8_t *buffer, uint8_t size ) {

	HeaderStruct header;
	DataStruct data;

	memset(&header, 0, sizeof(header));
	memset(&data, 0, sizeof(data));

	header.fid = 0x0;
	header.no = (uint8_t) seqid++;
	data.pd0 = 0xff;
	data.pd1 = 0xff;

	createAlohaPacket(buffer, &header, &data);

	Radio.Send(buffer, 4);
	{
		uint8_t fifo[64];
		Radio.ReadBuffer(REG_FIFO, fifo, 64);
	}
	return true;

}

void sendAck( int id ) {
	HeaderStruct header;
	DataStruct data;

	memset(&header, 0, sizeof(header));
	memset(&data, 0, sizeof(data));

	header.fid = 0x1;
	header.no = (uint8_t) id;
	data.pd0 = 0x00;
	data.pd1 = 0x00;

	uint8_t buffer[4];
	createAlohaPacket(buffer, &header, &data);

	Radio.Send(buffer, 4);

}

uint8_t crc8( const uint8_t *data, int len ) {
	unsigned int crc = 0;
	int j;
	int i;
	for (j = len; j; j--, data++) {
		crc ^= (*data << 8);
		for (i = 8; i; i--) {
			if (crc & 0x8000) {
				crc ^= (0x1070 << 3);
			}
			crc <<= 1;
		}
	}

	return (uint8_t) (crc >> 8);
}

bool dissectAlohaPacket(
		uint8_t *input,
		HeaderStruct *header,
		DataStruct *data ) {
	// assume user has already prepare memory in *in

	// get header
	if (header) {
		header->fid = input[0] >> 4; // higher four bits
		header->no = input[0] & 0x0f; // lower four bits
	}

	// get data
	if (data) {
		data->pd0 = input[1];
		data->pd1 = input[2];
	}

	// check crc
	if (header && data) {
		return input[3] == crc8(input, 3);
	}
	else {
		return false;
	}
}

void createAlohaPacket(
		uint8_t *output,
		HeaderStruct *header,
		DataStruct *data ) {
	// assume user has already allocated memory for output

	// create header
	if (header) {
		output[0] = header->no;
		output[0] |= header->fid << 4;
	}

	// fit data
	if (data) {
		output[1] = data->pd0;
		output[2] = data->pd1;
	}

	// calculate CRC
	if (header && data) {
		output[3] = crc8(output, 3);
	}
}

bool MACSend( uint8_t *buffer, uint8_t size ) {
	return sendALOHAmessage(buffer, size);
}

bool MACStateMachine( ) {
	switch (State) {
		case RX:
		{
			// if the receive frame is an ack, then cancel the timer
			// otherwise process the frame
			HeaderStruct header;
			DataStruct data;

			memset(&header, 0, sizeof(header));
			memset(&data, 0, sizeof(data));

			// decode data
			dissectAlohaPacket(Buffer, &header, &data);

			// process packet
			switch (header.fid) {
				case 0x0: // data packet
					sendAck(header.no);
					break;

				case 0x1: // ack received

					AlohaState = SLEEP;
					break;

				default:
					break;
			}

			if (header.fid == 0x01) {
//				 AlohaAckTimeout.detach();
			}

			printf(
					"RECV::fid=0x%x, seqid=0x%x, pd0=0x%x, pd1=0x%x\r\n",
					header.fid,
					header.no,
					data.pd0,
					data.pd1);

			// enter listening mode
			Radio.Rx(0);

			State = LOWPOWER;
		}
			break;
		case TX:
			// transmit done
			// set the state to pending and attach the timer with random delay

			// we dont need ack for ack
			if (AlohaState == ACK_RESP) {
				AlohaState = SLEEP;
				break;
			}

			// set delay time
			if (AlohaState == IDLE) {
				delay = (Radio.Random() % 1000) / 1000.0f;
			}
			else if (AlohaState == RETRANSMIT) {
				delay *= 2;
			}
			stopLoRaTimer();
			startLoRaTimer (delay);

			// increase the attempt counter
			attempts += 1;

			// enter listening mode
			Radio.Rx(6000);

			// state transition
			AlohaState = PENDING;

			State = LOWPOWER;
			break;
		case RX_TIMEOUT:
			// enter listening mode
			Radio.Rx(6000);

			State = LOWPOWER;
			break;
		case RX_ERROR:
			// we don't handle crc failed situation
			// enter listening mode
			Radio.Rx(6000);

			State = LOWPOWER;
			break;
		case TX_TIMEOUT:
			// we don't handle hardware error
			// enter listening mode
			Radio.Rx(6000);

			State = LOWPOWER;
			break;
		case LOWPOWER:
			break;
		default:
			State = LOWPOWER;
			break;
	}

	switch (AlohaState) {
		case SLEEP:
			// transmit packet if any
			// printf(" IDLE\r\n");
			break;

		case PENDING:
			// set rx time
			// printf(" PENDING, delay=%f, attempt=%d\r\n",  delay,  attempts);
			break;

		case RETRANSMIT:
			// send the packet again
			MACSend(Buffer, size);

			// printf(" RETRANSMIT\r\n");
			break;
		case EXPIRED:
			// give up the transmission
			// back to idle
			attempts = 0;

			AlohaState = SLEEP;
			// printf(" EXPIRED\r\n");
			break;
		case ACK_RESP:
			break;
		default:
			break;
	}
	return true;
}
