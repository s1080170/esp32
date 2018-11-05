#include "wifi.h"

/*---------------------------------------------------------------
                            GLOBAL
---------------------------------------------------------------*/
//
static EventGroupHandle_t wifi_event_group;
//
const int CONNECTED_BIT = BIT0;
//
static const char *WIFI_TAG="UDP";
//
static const char *TCP_TAG="TCP";

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
void udp_boardcast(void *pvParam){
    ESP_LOGI(WIFI_TAG,"udp_boardcast task started \n");

    int s, r;
    struct sockaddr_in cast_addr;
    memset(&cast_addr, 0, sizeof(struct sockaddr_in));
    cast_addr.sin_family = AF_INET;
    cast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP_ADDR);
    cast_addr.sin_port = htons(BROADCAST_PORT);

    while(1){
        xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY);
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if(s < 0) {
            ESP_LOGE(WIFI_TAG, "... Failed to allocate socket.\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(WIFI_TAG, "... allocated socket\n");
        //int getsockopt(int s, int level, int optname, void *optval,socklen_t *optlen);
        //http://pubs.opengroup.org/onlinepubs/007908799/xns/setsockopt.html
        //http://www.retran.com/beej/setsockoptman.html
        int opt_flag =1;
        setsockopt(s,SOL_SOCKET,SO_BROADCAST,&opt_flag,sizeof(opt_flag));

        while(1){
            int i2s_read_len = EXAMPLE_I2S_READ_LEN;
            size_t bytes_read;
            //size_t bytes_written;

            uint8_t* i2s_read_buff  = (uint8_t*) calloc(i2s_read_len, sizeof(char));
            uint8_t* send_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));           

            while(1){
                i2s_read(EXAMPLE_I2S_NUM, i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
                example_i2s_adc_data_scale2(send_buff, i2s_read_buff, i2s_read_len);

                i2s_read(EXAMPLE_I2S_NUM, i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
                example_i2s_adc_data_scale2(send_buff + i2s_read_len / 2, i2s_read_buff, i2s_read_len);

                ESP_LOGI("AUD", "rd buffer %d bytes.\n", bytes_read * 2);

                //example_disp_buf((uint8_t*) i2s_read_buff, 32);

                r = sendto(s, send_buff, i2s_read_len, 0, (struct sockaddr*)&cast_addr, sizeof(cast_addr));
                ESP_LOGI(WIFI_TAG, "wr socket. n=%d d1=%4d r=%2d e=%d\n",
                                        i2s_read_len, (int)(send_buff[0]-128), r, errno);

                vTaskDelay(10 / portTICK_PERIOD_MS); // every 10 seconds
            }

        }

        close(s);
        ESP_LOGI(WIFI_TAG, "... new sockeet will be opened in 10 seconds");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

/*
 * @brief
 */
void udp_server(void *pvParam){
    ESP_LOGI(WIFI_TAG,"udp_server task started \n");

    struct sockaddr_in udpServerAddr;
    udpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_port = htons( UDP_RX_PORT );
    int s, r;

    static struct sockaddr_in remote_addr;
    static unsigned int socklen;
    socklen = sizeof(remote_addr);
    int cs;//client socket
    
    while(1){
        xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY);
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if(s < 0) {
            ESP_LOGE(WIFI_TAG, "... Failed to allocate socket.\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(WIFI_TAG, "... allocated socket\n");
         if(bind(s, (struct sockaddr *)&udpServerAddr, sizeof(udpServerAddr)) != 0) {
            ESP_LOGE(WIFI_TAG, "... socket bind failed errno=%d \n", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(WIFI_TAG, "... socket bind done \n");

        while(1){
            int recv_total_size = EXAMPLE_I2S_READ_LEN;
            int recv_buf_idx;
            uint8_t* recv_buf = (uint8_t*) calloc(recv_total_size, sizeof(char));

            size_t bytes_written;
            uint8_t* i2s_write_buff = (uint8_t*) calloc(recv_total_size * 2, sizeof(char));
            
            while(1){

                recv_buf_idx = 0;
                bzero(recv_buf, sizeof(char) * recv_total_size);

                while(1){
                    r = recvfrom(s, recv_buf + recv_buf_idx, recv_total_size - recv_buf_idx, 0,
                                (struct sockaddr *)&remote_addr, &socklen);
                    
                    recv_buf_idx += r;

                    if(recv_buf_idx == recv_total_size){
                        break;
                    }
                } 

                if(recv_buf_idx != 0){
                    ESP_LOGI(WIFI_TAG, "rd socket. n=%d d1=%4d r=%2d e=%d",
                                        recv_buf_idx, (int)(recv_buf[0]-128), r, errno);

                    int i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, recv_buf, recv_total_size);
                    i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
                    ESP_LOGI("AUD", "wr buffer %d bytes.\n", i2s_wr_len);
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }

            close(cs);
        }
        ESP_LOGI(WIFI_TAG, "... server will be opened in 5 seconds");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(WIFI_TAG, "...udp_client task closed\n");
}

/*
 * @brief
 */
void tcp_server(void *pvParam){
    ESP_LOGI(TCP_TAG,"tcp_server task started \n");
    struct sockaddr_in tcpServerAddr;
    tcpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_port = htons( TCP_PORT );
    int s, r;
    //int recv_total_size = 1;
    //int recv_buf_idx;
    uint8_t recv_buf;
    //uint8_t recv_buf[recv_total_size];
    //uint8_t* recv_buf = (uint8_t*) calloc(recv_total_size, sizeof(char));
    static struct sockaddr_in remote_addr;
    static unsigned int socklen;
    socklen = sizeof(remote_addr);
    int cs;//client socket

    xTaskHandle xhandle_udp_server;
    xTaskCreate(&udp_server,"udp_server",1024*4,NULL,5,&xhandle_udp_server);
    vTaskSuspend( xhandle_udp_server );

    xTaskHandle xhandle_udp_boardcast;
    xTaskCreate(&udp_boardcast,"udp_boardcast",1024*4,NULL,5,&xhandle_udp_boardcast);
    vTaskSuspend( xhandle_udp_boardcast );

    xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY);
    while(1){
        s = socket(AF_INET, SOCK_STREAM, 0);
        if(s < 0) {
            ESP_LOGE(TCP_TAG, "... Failed to allocate socket.\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TCP_TAG, "... allocated socket\n");
         if(bind(s, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr)) != 0) {
            ESP_LOGE(TCP_TAG, "... socket bind failed errno=%d \n", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TCP_TAG, "... socket bind done \n");
        if(listen (s, LISTENQ) != 0) {
            ESP_LOGE(TCP_TAG, "... socket listen failed errno=%d \n", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        while(1){
            cs=accept(s,(struct sockaddr *)&remote_addr, &socklen);
            ESP_LOGI(TCP_TAG,"New connection request,Request data.");
            //set O_NONBLOCK so that recv will return, otherwise we need to impliment message end 
            //detection logic. If know the client message format you should instead impliment logic
            //detect the end of message
            fcntl(cs,F_SETFL,O_NONBLOCK);
            while(1){

                r = recv(cs, &recv_buf, 1, 0);

                if( r == 0 )
                {
                    ESP_LOGI(TCP_TAG, "rd tcp socket. Client disconnect. r=%2d e=%d", r, errno);
                    break;
                }

                if( r > 0 )
                {
                    ESP_LOGI(TCP_TAG, "rd tcp socket. n=1 d=0x%2x r=%2d e=%d", recv_buf, r, errno);

                    /****************
                        ACK
                    *****************/
                    //if( write(cs , recv_buf , 1) < 0)
                    if( write(cs , "ACK" , 3) < 0)
                    {
                        ESP_LOGE(TCP_TAG, "... Send failed \n");
                        close(s);
                        vTaskDelay(4000 / portTICK_PERIOD_MS);
                        break;
                    }
                    //ESP_LOGI(TCP_TAG, "wr tcp socket. n=1 d=0x%2x", recv_buf);

                    /****************
                        Command
                    *****************/
                    if( recv_buf == 0x31 )
                    {
                        vTaskSuspend( xhandle_udp_boardcast );
                        vTaskResume( xhandle_udp_server );
                    }
                    else if ( recv_buf == 0x32 )
                    {
                        vTaskSuspend( xhandle_udp_server );
                        vTaskResume( xhandle_udp_boardcast );
                    }
                    else if ( recv_buf == 0x33 )
                    {
                        // live check command.
                        // ACK only.
                    }
                    else
                    {
                        vTaskSuspend( xhandle_udp_server );
                        vTaskSuspend( xhandle_udp_boardcast );
                    }

                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }

            close(cs);
        }
        ESP_LOGI(TCP_TAG, "... server will be opened in 1 seconds");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TCP_TAG, "...tcp_client task closed\n");
}
