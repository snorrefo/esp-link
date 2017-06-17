#include <esp8266.h>
#include <config.h>
#include <uart.h>

#ifdef SYSLOG
#include <syslog.h>
#define NOTICE(format, ...) do {                                            \
  LOG_NOTICE(format, ## __VA_ARGS__ );                                      \
  os_printf(format "\n", ## __VA_ARGS__);                                   \
} while ( 0 )
#else
#define NOTICE(format, ...) do {                                            \
  os_printf(format "\n", ## __VA_ARGS__);                                   \
} while ( 0 )
#endif


#include <mqtt.h>
#include <mqtt_client.h>
extern MQTT_Client mqttClient;

// Every 5 sec...
#define MQTT_STATUS_INTERVAL2 (5*1000)


static char json_ble_send[1000];
static char topic [200]; // Changed from char to static char
static char ble_mac_addr[13];
static char ble_rssi [5];
static char ble_is_scan_resp [2];
static char ble_type [2];
static char ble_data[100];

static char serial_buffer[200];

// static ETSTimer mqttStatusTimer2;
//
//
// // Timer callback to send an RSSI update to a monitoring system
// static void ICACHE_FLASH_ATTR mqttStatusCb2(void *v) {
//   if (mqttClient.connState != MQTT_CONNECTED)
//     return;
//
//
//   memset(json_ble_send, 0, 1000);
//   os_sprintf(json_ble_send, "{\"hostname\": \"living-room\",\r\n\"mac\": \"dd6ed85b7a80\",\r\n\"rssi\": -94,\r\n\"is_scan_response\": \"0\",\r\n\"type\": \"3\",\r\n\"data\": \"0201061aff4c000215e2c56db5dffb48d2b060d0f5a71096e000680068c5\"}");
//   memset(topic, 0, 200);
//   os_sprintf(topic, "happy-bubbles/ble/living-room/raw/dd6ed85b7a80");
//   bool status = MQTT_Publish(&mqttClient, topic, json_ble_send, os_strlen(json_ble_send), 1, 0);
//   NOTICE("*** user_main.c: : sent mqtt message result: %d", status);
//
// }




static void ICACHE_FLASH_ATTR sendMQTTraw(char * mac, char * rssi, char * is_scan, char * type, char * data)
{
  // if (mqttClient.connState != MQTT_CONNECTED)
	// {
	// 	//os_printf("mqtt not connected, returning\r\n");
  //   return;
	// }

	memset(json_ble_send, 0, 1000);
	os_sprintf(json_ble_send, "{\"hostname\": \"%s\",\r\n\"mac\": \"%s\",\r\n\"rssi\": %s,\r\n\"is_scan_response\": \"%s\",\r\n\"type\": \"%s\",\r\n\"data\": \"%s\"}", flashConfig.hostname, mac, rssi, is_scan, type, data);
	memset(topic, 0, 200);
	os_sprintf(topic, "happy-bubbles/ble/%s/raw/%s", flashConfig.hostname, mac);
  bool status = MQTT_Publish(&mqttClient, topic, json_ble_send, os_strlen(json_ble_send), 0, 0);
  NOTICE("*** user_main.c:sendMQTTraw: sent mqtt message result: %d", status);
}



void ICACHE_FLASH_ATTR
process_serial(char *buf)
{
	NOTICE("*** user_main.c:process_serial: got serial to process %d ** %s \n\r", strlen(buf), buf);
	char * pch;
	pch = strtok(buf, "|");
	int i = 0;

	memset(ble_mac_addr, 0, 12);
	memset(ble_rssi, 0, 5);
	memset(ble_is_scan_resp, 0, 1);
	memset(ble_type, 0, 1);
	memset(ble_data, 0, 100);

	while(pch != NULL)
	{
		switch(i)
		{
			case 0:
				{
					strcpy(ble_mac_addr, pch);
					break;
				}
			case 1:
				{
					strcpy(ble_rssi, pch);
					break;
				}
			case 2:
				{
					strcpy(ble_is_scan_resp, pch);
					break;
				}
			case 3:
				{
					strcpy(ble_type, pch);
					break;
				}
			case 4:
				{
					strcpy(ble_data, pch);
					break;
				}
		}
		i++;
		NOTICE("*** user_main.c:process_serial: token: %s\n", pch);
		pch = strtok(NULL, "|");
	}

	//basic filters
	//check mac is 12 chars
	int should_send = 0;
	if(strlen(ble_mac_addr) != 12)
	{
		NOTICE("*** user_main.c:process_serial: bad mac: string length not 12: %d, %s\n", strlen(ble_mac_addr), ble_mac_addr);
		should_send = 1;
	}
	//check mac is all hex
	if (!ble_mac_addr[strspn(ble_mac_addr, "0123456789abcdefABCDEF")] == 0)
	{
		NOTICE("*** user_main.c:process_serial: bad mac: not all hex: %s\n", ble_mac_addr);
		should_send = 2;
	}
	//check data is all hex
	if (strlen(ble_data) < 8 || !ble_data[strspn(ble_data, "0123456789abcdefABCDEF")] == 0 )
	{
		NOTICE("*** user_main.c:process_serial: bad data: string length < 8 or not all hex: %s\n", ble_data);
		should_send = 3;
	}
	//check rssi is all numbers
	if (strlen(ble_rssi) < 1 || !ble_rssi[strspn(ble_rssi, "-0123456789")] == 0)
	{
		NOTICE("*** user_main.c:process_serial: bad rssi: string length < 1 or not numeric: %s\n", ble_rssi);
		should_send = 4;
	}
	//check scan_response is 0 or 1
	if (strlen(ble_is_scan_resp) < 1 || !ble_is_scan_resp[strspn(ble_is_scan_resp, "01")] == 0)
	{
		NOTICE("*** user_main.c:process_serial: bad scanresp: not 0 or 1: %s\n", ble_is_scan_resp);
		should_send = 5;
	}
	//check ble_type is hex
	if (strlen(ble_type) < 1 || !ble_type[strspn(ble_type, "0123456789abcdefABCDEF")] == 0)
	{
		NOTICE("*** user_main.c:process_serial: bad type: string length < 1 or not hex: %s\n", ble_type);
		should_send = 6;
	}
	if(should_send == 0)
	{
		NOTICE("*** user_main.c:process_serial: BLE advertisment PASSED filters!\n");
	}
	else
	{
		NOTICE("*** user_main.c:process_serial: BLE advertisment FAILED filters: %d \r\n", should_send);
		NOTICE("*** user_main.c:process_serial: BLE advertisment:  \"%s || %s || %s || %s || %s\"\n", ble_mac_addr, ble_rssi, ble_is_scan_resp, ble_type, ble_data);
		return;
	}

	//os_printf("send some mqtt for mac %s =====%d===\n", ble_mac_addr, strlen(ble_mac_addr));
	sendMQTTraw(ble_mac_addr, ble_rssi, ble_is_scan_resp, ble_type, ble_data);
}





// callback with a buffer of characters that have arrived on the uart
void ICACHE_FLASH_ATTR
bleBridgeUartCb(char *buf, short length)
{
	NOTICE("*** user_main.c:bleBridgeUartCb: got serial %d **  %s \n\r", length, buf);
	// append to serial_buffer
	strncat(serial_buffer, buf, length);

	// check if buffer contains a \n or \r, if does, then is end of string and pass it for processing.
	char * nl = NULL;
	char * cr = NULL;
	nl = strchr(serial_buffer, '\n');
	cr = strchr(serial_buffer, '\r');
	if(nl != NULL)
	{
		if(cr != NULL)
		{
			serial_buffer[cr-serial_buffer] = '\0';
		}

		// pass on for processing
		process_serial(serial_buffer);
		// clear the buffer
		memset(serial_buffer, 0, 200);
	}
}

void app_init() {




  NOTICE("*** user_main.c:app_init: initializing MQTT...");

  // os_timer_disarm(&mqttStatusTimer2);
  // os_timer_setfn(&mqttStatusTimer2, mqttStatusCb2, NULL);
  // os_timer_arm(&mqttStatusTimer2, MQTT_STATUS_INTERVAL2, 1); // recurring timer

  NOTICE("*** user_main.c:app_init: initializing UART receive...");
  uart_add_recv_cb(&bleBridgeUartCb);
  // Baud 115200 8N1 Only TX-RX and RX-TX. Use whole line sending

}
