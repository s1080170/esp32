#include "wifi.h"

/*---------------------------------------------------------------
                            GLOBAL
---------------------------------------------------------------*/
//
static EventGroupHandle_t wifi_event_group;
//
const int CONNECTED_BIT = BIT0;
//
static const char *WIFI_TAG="TCP";

/*---------------------------------------------------------------
                            FUNCTION
---------------------------------------------------------------*/
/*
 * @brief
 */
void wifi_connect(){
    wifi_config_t cfg = {
        .sta = {
            .ssid = SSID,
            .password = PASSPHARSE,
        },
    };
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg) );
    ESP_ERROR_CHECK( esp_wifi_connect() );
}

/*
 * @brief
 */
void initialise_wifi_event(void){
    wifi_event_group = xEventGroupCreate();
}

/*
 * @brief
 */
esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/*
 * @brief
 */
void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_NONE); // disable wifi driver logging
    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

/*
 * @brief
 */
void printWiFiIP(void *pvParam){
    printf("print_WiFiIP task started \n");
    xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY);
    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
    printf("IP :  %s\n", ip4addr_ntoa(&ip_info.ip));
    vTaskDelete( NULL );
}

/*
 * @brief
 */
void tcp_server(void *pvParam){
    ESP_LOGI(WIFI_TAG,"tcp_server task started \n");
    struct sockaddr_in tcpServerAddr;
    tcpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_port = htons( 3000 );
    int s, r;
    int recv_total_size = 640;
    int recv_buf_idx;
    uint8_t recv_buf[1024];
    uint8_t out_buf[640];
    static struct sockaddr_in remote_addr;
    static unsigned int socklen;
    socklen = sizeof(remote_addr);
    int cs;//client socket
    xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY);
    while(1){
        s = socket(AF_INET, SOCK_STREAM, 0);
        if(s < 0) {
            ESP_LOGE(WIFI_TAG, "... Failed to allocate socket.\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(WIFI_TAG, "... allocated socket\n");
         if(bind(s, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr)) != 0) {
            ESP_LOGE(WIFI_TAG, "... socket bind failed errno=%d \n", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(WIFI_TAG, "... socket bind done \n");
        if(listen (s, LISTENQ) != 0) {
            ESP_LOGE(WIFI_TAG, "... socket listen failed errno=%d \n", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        while(1){
            cs=accept(s,(struct sockaddr *)&remote_addr, &socklen);
            ESP_LOGI(WIFI_TAG,"New connection request,Request data.");
            //set O_NONBLOCK so that recv will return, otherwise we need to impliment message end 
            //detection logic. If know the client message format you should instead impliment logic
            //detect the end of message
            fcntl(cs,F_SETFL,O_NONBLOCK);
            while(1){

                recv_buf_idx = 0;
                bzero(recv_buf, sizeof(recv_buf));

                while(1){
                    r = recv(cs, recv_buf + recv_buf_idx, recv_total_size - recv_buf_idx, 0);
                    
                    if(r < 0){
                        break;
                    }

                    if(r == 0){
                        break;
                    }

                    recv_buf_idx += r;
                } 

                
                if(recv_buf_idx != 0){
                    ESP_LOGI(WIFI_TAG, "rd socket. n=%d d1=%2x r=%2d e=%d",
                                                     recv_buf_idx, recv_buf[0], r, errno);
                    //printf("count=%d : %2x\n", recv_buf_idx, recv_buf[0]);

                    memcpy(out_buf, (uint8_t*)recv_buf, 640);
                    //if( xQueueSend(audio_out_queue, (void *)&out_buf, portMAX_DELAY) != pdTRUE){
                    if( xQueueSend(audio_out_queue, (void *)&out_buf, 0) != pdTRUE){
                        ESP_LOGE(WIFI_TAG, "... Failed to enqueue data.\n");
                        //vTaskDelay(1000 / portTICK_PERIOD_MS);
                        continue;
                    }


                    /*if( write(cs , recv_buf , recv_buf_idx) < 0)
                     {
                        ESP_LOGE(WIFI_TAG, "... Send failed \n");
                        vTaskDelay(4000 / portTICK_PERIOD_MS);
                        break;
                    }
                    ESP_LOGI(WIFI_TAG, "... socket send success\n");
                    */
                    /*for(int i = 0; i < recv_buf_idx; i++){
                        if( (i % 16) == 0 ) printf("\n %2d: ", (i / 16 + 1));
                        printf("%.2x ", recv_buf[i]);
                    }
                    printf("\n");*/
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
           }

            close(cs);
        }
        ESP_LOGI(WIFI_TAG, "... server will be opened in 5 seconds");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(WIFI_TAG, "...tcp_client task closed\n");
}
