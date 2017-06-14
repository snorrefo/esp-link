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

// static char json_ble_send[500];
static os_timer_t mqtt_timer;

// #include "stdout.h"
#include "mqtt.h"
#include "mqtt_client.h"
extern MQTT_Client mqttClient;


extern void mqtt_client_init(void);
static void ICACHE_FLASH_ATTR mqtt_check(void *args)
{

  if (mqttClient.connState != MQTT_CONNECTED)
	{
		NOTICE("====================== MQTT BAD, restarting\r\n");
		//force a watchdog timeout and reset by doing something stupid like this here loop
		while(1){};
	}
}


void app_init() {


  // stdoutInit();



  NOTICE("SFW: initializing MQTT");
  mqtt_client_init();

  //MQTT Timer
	os_timer_disarm(&mqtt_timer);
	os_timer_setfn(&mqtt_timer, (os_timer_func_t *)mqtt_check, NULL);
	os_timer_arm(&mqtt_timer, 60000, 1);
}
