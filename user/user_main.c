#include <esp8266.h>
#include <config.h>

#ifdef SYSLOG
#include "syslog.h"
#define NOTICE(format, ...) do {                                            \
  LOG_NOTICE(format, ## __VA_ARGS__ );                                      \
  os_printf(format "\n", ## __VA_ARGS__);                                   \
} while ( 0 )
#else
#define NOTICE(format, ...) do {                                            \
  os_printf(format "\n", ## __VA_ARGS__);                                   \
} while ( 0 )
#endif


#include "mqtt.h"
#include "mqtt_client.h"
extern MQTT_Client mqttClient;

// Every 5 sec...
#define MQTT_STATUS_INTERVAL2 (5*1000)

static ETSTimer mqttStatusTimer2;


// Timer callback to send an RSSI update to a monitoring system
static void ICACHE_FLASH_ATTR mqttStatusCb2(void *v) {
  if (mqttClient.connState != MQTT_CONNECTED)
    return;

  char json_ble_send [500];
  memset(json_ble_send, 0, 500);
  os_sprintf(json_ble_send, "{\"hostname\": \"host\",\r\n\"beacon_type\": \"ibeacon\",\r\n\"mac\": \"mac\",\r\n\"rssi\": rssi,\r\n\"uuid\": \"uuid\",\r\n\"major\": \"major\",\r\n\"minor\": \"minor\",\r\n\"tx_power\": \"tx\"}");
  char topic [130];
  memset(topic, 0, 130);
  os_sprintf(topic, "happy-bubbles-ble");

  bool status = MQTT_Publish(&mqttClient, topic, json_ble_send, os_strlen(json_ble_send), 1, 0);
  NOTICE("sent mqtt message result: %d", status);

}


void app_init() {




  NOTICE("SFW: initializing MQTT");

  #ifdef MQTT
    os_timer_disarm(&mqttStatusTimer2);
    os_timer_setfn(&mqttStatusTimer2, mqttStatusCb2, NULL);
    os_timer_arm(&mqttStatusTimer2, MQTT_STATUS_INTERVAL2, 1); // recurring timer
  #endif // MQTT


}
