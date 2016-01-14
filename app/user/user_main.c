/*
 * Copyright (c) 2016, Guillermo A. Amaral B. <g@maral.me>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "osapi.h"

#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"

#define GPIO0 (1 << 0)
#define GPIO2 (1 << 2)

static struct station_config s_station_config = {
    .ssid = "your_access_point",
    .password = "y0urp4ssw0rd",
    .bssid_set = 0
};

static esp_tcp s_server_conn_tcp = {
    .local_port = 9021
};

static struct espconn s_server_conn = {
    .type = ESPCONN_TCP,
    .state = ESPCONN_NONE,
    .proto.tcp = &s_server_conn_tcp
};

static void ICACHE_FLASH_ATTR
wifi_event_handler(System_Event_t *);

static void ICACHE_FLASH_ATTR
espconn_recv_handler(void *, char *, unsigned short);

/*****************************************************************************/

static void ICACHE_FLASH_ATTR
client_connection_setup(void)
{
	wifi_set_opmode(STATIONAP_MODE);
	wifi_station_set_reconnect_policy(true);
	wifi_station_set_config(&s_station_config);
	wifi_station_dhcpc_start();
}

static void ICACHE_FLASH_ATTR
wifi_event_handler(System_Event_t *e)
{
	switch (e->event) {
	case EVENT_STAMODE_CONNECTED:
		espconn_accept(&s_server_conn);
		espconn_regist_recvcb(&s_server_conn, espconn_recv_handler);
		break;

	case EVENT_STAMODE_DISCONNECTED:
		espconn_delete(&s_server_conn);
		break;

	case EVENT_STAMODE_GOT_IP:
	case EVENT_STAMODE_DHCP_TIMEOUT:
	case EVENT_STAMODE_AUTHMODE_CHANGE:
	case EVENT_SOFTAPMODE_STACONNECTED:
	case EVENT_SOFTAPMODE_STADISCONNECTED:
	case EVENT_SOFTAPMODE_PROBEREQRECVED:
	default: break;
	}
}

static void ICACHE_FLASH_ATTR
espconn_recv_handler(void *arg, char *pdata, unsigned short len)
{
	if (os_strcmp(pdata, "FOCUS HOLD") == 0)
		gpio_output_set(0, GPIO0, 0, 0);
	else if (os_strcmp(pdata, "FOCUS RELEASE") == 0)
		gpio_output_set(GPIO0, 0, 0, 0);

	else if (os_strcmp(pdata, "SHUTTER HOLD") == 0)
		gpio_output_set(0, GPIO2, 0, 0);
	else if (os_strcmp(pdata, "SHUTTER RELEASE") == 0)
		gpio_output_set(GPIO2, 0, 0, 0);
#ifndef NDEBUG
	else os_printf("Unknown command: %s\n", pdata);
#endif
}

void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
}

void ICACHE_FLASH_ATTR
user_init(void)
{
#ifndef NDEBUG
	uart_init(9600, 9600);
#endif

	gpio_init();

	/* set GPIO0 and GPIO2 as HIGH outputs */
	gpio_output_set(GPIO0 | GPIO2, 0, GPIO0 | GPIO2, 0);

	/* select GPIO2 function for GPIO2 pin */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);

	wifi_set_event_handler_cb(wifi_event_handler);

	client_connection_setup();
}

