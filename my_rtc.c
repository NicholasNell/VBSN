/*

 * my_rtc.c
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas

 */

#include <helper.h>
#include <main.h>
#include <my_gpio.h>
#include <my_gps.h>
#include <my_MAC.h>
#include <my_scheduler.h>
#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include <my_rtc.h>
#include <my_UART.h>
#include <myNet.h>
#include <ti/devices/msp432p4xx/driverlib/wdt_a.h>

static bool rtcInitFlag = false; // Has the RTC time been initialised by the GPS?
volatile static bool macStateMachineEnable = false; // Is it time for the MAC state machine to be run? Is determined by the RTC ISR
volatile static bool uploadGSM = false; // Should the Gateway upload it's stored info? Set by RTC ISR
volatile static bool collectDataFlag = false; // Tells main to collect sensor data. Set in RTC ISR
volatile bool netOp = false; // Perform a net operation now? Set in MACStateMachine
volatile extern bool gpsWakeFlag;	// does the GPS need to wake up?
volatile static bool saveFlashData = false;	// Does the data need to be saved to flash
volatile extern NextNetOp_t nextNetOp;	// what is the next netOp
// GSM upload slots
static uint16_t gsmStopSlot[WINDOW_SCALER];
static uint16_t gsmStartSlot[WINDOW_SCALER];
extern volatile MACappState_t MACState; //next macState
volatile static bool resetFlag;	// does the system need to reset?
RTC_C_Calendar currentTime = RTC_ZERO_TIME;	// Calender time

void init_scheduler() {

	int i = 0;
	for (i = 0; i < WINDOW_SCALER; i++) {

		gsmStartSlot[i] = i * WINDOW_TIME_SEC;

		gsmStopSlot[i] = i * WINDOW_TIME_SEC + GSM_UPLOAD_DATAGRAMS_TIME;
	}
}

void rtc_init() {

	//[Simple RTC Example]
	/* Initializing RTC with current time as described in time in
	 * definitions section */

	MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

	/* Specify an interrupt to assert every minute */
//	MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_HOURCHANGE);
	RTC_C_setCalendarAlarm(0x05,
	RTC_C_ALARMCONDITION_OFF,
	RTC_C_ALARMCONDITION_OFF, RTC_C_ALARMCONDITION_OFF);

	/* Enable interrupt for RTC Ready Status, which asserts when the RTC
	 * Calendar registers are ready to read.
	 * Also, enable interrupts for the Calendar alarm and Calendar event. */
	MAP_RTC_C_clearInterruptFlag(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);
	MAP_RTC_C_enableInterrupt(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);

	/* Start RTC Clock */

	//![Simple RTC Example]
	/* Enable interrupts and go to sleep. */
	MAP_Interrupt_enableInterrupt(INT_RTC_C);
	rtcInitFlag = true;
	rtc_stop_clock();
}

void rtc_start_clock(void) {
	MAP_RTC_C_startClock();
}

void rtc_stop_clock(void) {
	MAP_RTC_C_holdClock();
}

void rtc_set_calendar_time(void) {
//	rtc_stop_clock();
	MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);
	int temp = convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().minutes) * 60
			+ convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().seconds);

	set_slot_count(temp);
//	rtc_start_clock();
}

void RTC_C_IRQHandler(void) {
	static uint8_t resetCounter = 0;

	uint32_t status;
	status = MAP_RTC_C_getEnabledInterruptStatus();
	MAP_RTC_C_clearInterruptFlag(status);
#if DEBUG
	gpio_toggle(&Led_user_red);
#endif

	if (rtcInitFlag) {
		int curSlot = convert_hex_to_dec_by_byte(
				RTC_C_getCalendarTime().minutes) * 60
				+ convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().seconds);

		set_slot_count(curSlot);

		if (curSlot % (WINDOW_TIME_SEC * 3) == 0) {
			if (!get_is_root()) {
				resetFlag = true;
			}
		}

		if (get_num_routes() >= MAX_ROUTES) {
			reset_num_routes();
		}
		MACState = MAC_SLEEP;	// by default, sleep

		int i = 0;
		static bool hasSentGSM = false;
		i = curSlot / WINDOW_TIME_SEC;

		if (curSlot % GPS_WAKEUP_TIME == 0) {
			gpsWakeFlag = true;
		}

		if (curSlot % FLASH_SAVE_DATA == 0) {
			saveFlashData = true;
		}

		if ((curSlot >= gsmStartSlot[i]) && (curSlot <= gsmStopSlot[i])) {
			if (get_is_root()) {
				if (!hasSentGSM) {
					resetCounter++;
					send_uart_pc("Uploading Stored datagrams\n");
					uploadGSM = true;
					if (resetCounter >= 3) {
						resetCounter = 0;
						resetFlag = true;
					}
					hasSentGSM = true;
				}
			} else {
				MACState = MAC_SLEEP;
				macStateMachineEnable = true;
			}
		} else {
			hasSentGSM = false;

			if (curSlot % GLOBAL_RX == 0) { // Global Sync Slots
				RouteEntry_t tempRoute;
				if (netOp) { // Perform a network layer operation in the global RX window
					netOp = false;
					MACState = MAC_NET_OP;

				} else if (get_hop_message_flag()) { // hop the received message in the global RX window
					MACState = MAC_HOP_MESSAGE;
					if (get_num_retries() > 3) {
						reset_num_retries();
						MACState = MAC_LISTEN;
					}
				} else if ((get_num_retries() > 0)) {
					if (!get_is_root()) {
						MACState = MAC_RTS;
						if (get_num_retries() > 3) {
							reset_num_retries();
							MACState = MAC_LISTEN;
						}
					}
				} else if (get_sync()) {
					if (!has_route_to_node(GATEWAY_ADDRESS, &tempRoute)
							&& !get_is_root()) {
						MACState = MAC_NET_OP;
						nextNetOp = NET_BROADCAST_RREQ;
					} else {
						MACState = MAC_SYNC_BROADCAST;
					}
				} else {
					MACState = MAC_LISTEN;
				}
				macStateMachineEnable = true;
			}

			if (curSlot == (get_tx_slots()[i] - 5)) { // if slotcount equals collectdataSlot then collect sensor data and flag the hasData flag
				collectDataFlag = true;
			}

			int var = 0;
			Neighbour_t *table;
			table = get_neighbour_table();
			for (var = 0; var < get_num_neighbours(); ++var) { // loop through all neighbours and listen if any of them are expected to transmit a message
				if (table[var].neighbourTxSlot
						== (curSlot - (i * WINDOW_TIME_SEC))) {
					MACState = MAC_LISTEN;
					send_uart_pc("Neighbour Tx Slot\n");
					macStateMachineEnable = true;
					break;
				}
			}

			if (curSlot == get_tx_slots()[i]) { // if it is the node's tx slot and it has data to send and it is in its correct bracket
				if (!get_is_root()) {
					MACState = MAC_RTS;
					macStateMachineEnable = true;
				}
			}
		}
	}

	if (status & RTC_C_CLOCK_ALARM_INTERRUPT) {	// Resync with GPS
		gpsWakeFlag = true;
//		resetFlag = true;
	}

}

RTC_C_Calendar get_current_time() {
	return currentTime;
}

void set_current_time(RTC_C_Calendar time) {
	currentTime = time;
}

bool get_mac_state_machine_enabled() {
	return macStateMachineEnable;
}

void reset_mac_state_machine_enabled() {
	macStateMachineEnable = false;
}

bool get_upload_gsm_flag() {
	return uploadGSM;
}

void reset_upload_gsm_flag() {
	uploadGSM = false;
}

bool get_collect_data_flag() {
	return collectDataFlag;
}

void reset_collect_data_flag() {
	collectDataFlag = false;
}

bool get_save_flash_data_flag() {
	return saveFlashData;
}

void reset_save_flash_data_flag() {
	saveFlashData = false;
}

bool get_reset_flag() {
	return resetFlag;
}

void reset_reset_flag() {
	resetFlag = false;
}
