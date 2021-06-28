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
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <my_rtc.h>
#include <my_UART.h>
#include <myNet.h>

static volatile bool rtcInitFlag = false; // Has the RTC time been initialised by the GPS?
volatile static bool macStateMachineEnable = false; // Is it time for the MAC state machine to be run? Is determined by the RTC ISR
volatile static bool uploadGSM = false; // Should the Gateway upload it's stored info? Set by RTC ISR
volatile static bool collectDataFlag = false; // Tells main to collect sensor data. Set in RTC ISR
volatile static bool saveFlashData = false;	// Does the data need to be saved to flash
// GSM upload slots
static uint16_t gsmStopSlot[WINDOW_SCALER];
static uint16_t gsmStartSlot[WINDOW_SCALER];
volatile static bool resetFlag;	// does the system need to reset?
RTC_C_Calendar currentTime = RTC_ZERO_TIME;	// Calender time
extern Gpio_t Led_user_red;
static volatile uint16_t timeToRoute = 0;
static volatile bool timeToRouteCounterFlag = false;

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

	RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

	/* Specify an interrupt to assert every minute */
//	MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_HOURCHANGE);
	RTC_C_setCalendarAlarm(RTC_C_ALARMCONDITION_OFF, 0x0,
	RTC_C_ALARMCONDITION_OFF, RTC_C_ALARMCONDITION_OFF); // set an alarm for midnight
	/* Enable interrupt for RTC Ready Status, which asserts when the RTC
	 * Calendar registers are ready to read.
	 * Also, enable interrupts for the Calendar alarm and Calendar event. */
	RTC_C_clearInterruptFlag(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);
	RTC_C_enableInterrupt(
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
	RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);
	int temp = convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().minutes) * 60
			+ convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().seconds);

	set_slot_count(temp);
//	rtc_start_clock();
}

void RTC_C_IRQHandler(void) {
	uint32_t status;
	status = MAP_RTC_C_getEnabledInterruptStatus();
	MAP_RTC_C_clearInterruptFlag(status);
#if DEBUG
	gpio_toggle(&Led_user_red);
#endif

	if (rtcInitFlag) {
		if (timeToRouteCounterFlag) {
			timeToRoute++;
		}

		RTC_C_Calendar time = RTC_C_getCalendarTime();
		int curSlot = convert_hex_to_dec_by_byte(time.minutes) * 60
				+ convert_hex_to_dec_by_byte(time.seconds);

		set_slot_count(curSlot);

		if (get_num_routes() >= MAX_ROUTES) {
			reset_num_routes();
		}
		set_mac_app_state(MAC_SLEEP);	// by default, sleep

		int i = 0;
		static bool hasSentGSM = false;
		i = curSlot / WINDOW_TIME_SEC;

		if (curSlot % GPS_WAKEUP_TIME == 0) {
			set_gps_wake_flag();
		}

		if (curSlot % FLASH_SAVE_DATA == 0) {
			saveFlashData = true;
		}

		if ((curSlot >= gsmStartSlot[i]) && (curSlot <= gsmStopSlot[i])) {
			if (get_is_root()) {
				if (!hasSentGSM) {

					send_uart_pc("Uploading Stored datagrams\n");
					reset_gsm_upload_done_flag();
					uploadGSM = true;
					hasSentGSM = true;
				}
			} else {
				macStateMachineEnable = false;
				if (get_mac_app_state() != MAC_SLEEP) {
					set_mac_app_state(MAC_SLEEP);
					macStateMachineEnable = true;
				}
			}
		} else {
			hasSentGSM = false;

			//Try to only remove routes if retries is exceeded???
			static bool routeExpired = false;
//			RouteEntry_t *routes;
//			routes = get_routing_table();
//			for (var = 0; var < get_num_routes(); ++var) {
//				if (routes[var].expiration_time == curSlot) {
//					routeExpired = true;
//					remove_neighbour(routes->next_hop);
//					remove_route_with_node(routes->next_hop);
//				}
//			}

			if (curSlot % GLOBAL_RX == 0) { // Global Sync Slots
				RouteEntry_t tempRoute;
				if (get_net_op_flag()) { // Perform a network layer operation in the global RX window
					reset_net_op_flag();
					set_mac_app_state(MAC_NET_OP);

				} else if (get_hop_message_flag()) { // hop the received message in the global RX window
					set_mac_app_state(MAC_HOP_MESSAGE);
					if (get_num_retries() > 3) {
						reset_num_retries();
						set_mac_app_state(MAC_LISTEN);
					}
				} else if ((get_num_retries() > 0)) {
					if (!get_is_root()) {
						set_mac_app_state(MAC_RTS);
						if (get_num_retries() > 3) {
							reset_num_retries();
							set_mac_app_state(MAC_LISTEN);
						}
					}
				} else if (!has_route_to_node(GATEWAY_ADDRESS, &tempRoute)
						&& !get_is_root()) {
					if (get_sync(SYNC_PROB_ROUTE_DISC)) {
						set_mac_app_state(MAC_NET_OP);
						set_next_net_op(NET_BROADCAST_RREQ);
					}
				} else if (get_sync(SYNC_PROB_HELLO_MSG) && get_is_root()) {
//					set_mac_app_state(MAC_SYNC_BROADCAST); // root will periodically send out hello messages
					set_mac_app_state(MAC_LISTEN); // comment this out later, disables hello messages
				} else if (routeExpired) {
					set_mac_app_state(MAC_NET_OP);
					set_next_net_op(NET_BROKEN_LINK);
				} else {
					set_mac_app_state(MAC_LISTEN);
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
					set_mac_app_state(MAC_LISTEN);
					send_uart_pc("Neighbour Tx Slot\n");
					macStateMachineEnable = true;
					break;
				}
			}

			if (curSlot == get_tx_slots()[i]) { // if it is the node's tx slot and it has data to send and it is in its correct bracket
				if (!get_is_root()) {
					set_mac_app_state(MAC_RTS);
					macStateMachineEnable = true;
				}
			}
			if (curSlot == (WINDOW_TIME_SEC / 2 + 1) * (i + 1)) {
				if (!get_gsm_upload_done_flag() && get_is_root()) {
					reset_gsm_upload_done_flag();
					uploadGSM = true;
				}

			}
		}
	}

	if (status & RTC_C_CLOCK_ALARM_INTERRUPT) {	// Resync with GPS
		set_gps_wake_flag();
		resetFlag = true;
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

bool get_rtc_init_flag() {
	return rtcInitFlag;
}

void set_rtc_init_flag() {
	rtcInitFlag = true;
}

void reset_rtc_init_flag() {
	rtcInitFlag = false;
}

uint16_t get_time_to_route() {
	if (timeToRouteCounterFlag) {
		return 0;	// no route yet
	} else {
		return timeToRoute;	//time to route discovery
	}
}

void set_time_to_route_flag() {
	timeToRouteCounterFlag = true;
}

void reset_time_to_route_flag() {
	timeToRouteCounterFlag = false;
}
