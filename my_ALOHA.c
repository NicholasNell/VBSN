/*
 * my_ALOHA.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_ALOHA.h"
uint8_t seqid = 0;

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

