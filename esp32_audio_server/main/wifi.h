#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "audio.h"

/*---------------------------------------------------------------
                            EXAMPLE CONFIG
---------------------------------------------------------------*/
//
//#define SSID "XperiaZ4s1080170"
//#define PASSPHARSE "f46d123ec7fa"

#define SSID "SPWN_H36_95E736"
#define PASSPHARSE "1r555it47btyg55"

//
#define LISTENQ 2
//
#define BROADCAST_IP_ADDR "192.168.100.255" // you might need to change this acroding to the subnet mask and gateway address
//
#define TCP_PORT 		3000
#define UDP_RX_PORT 	4000
#define UDP_TX_PORT 	4001
#define BROADCAST_PORT 	4001

/*---------------------------------------------------------------
                            FUNCTION
---------------------------------------------------------------*/
/*
 * @brief
 */
void wifi_connect(void);

/*
 * @brief
 */
void initialise_wifi_event(void);

/*
 * @brief
 */
esp_err_t event_handler(void *ctx, system_event_t *event);

/*
 * @brief
 */
void initialise_wifi(void);

/*
 * @brief
 */
void printWiFiIP(void *pvParam);

/*
 * @brief
 */
void tcp_server(void *pvParam);
void udp_server(void *pvParam);
void udp_boardcast(void *pvParam);

#endif /* __WIFI_H__*/