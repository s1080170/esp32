#include "wifi.h"
#include "audio.h"

esp_err_t app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    example_i2s_init();
    esp_log_level_set("I2S", ESP_LOG_INFO);

    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    initialise_wifi_event();
    initialise_wifi();

    //xTaskCreate(i2s_test_task, "i2s test task", 1024 * 6, NULL, 5, NULL);
    xTaskCreate(example_i2s_adc_dac, "example_i2s_adc_dac", 1024 * 6, NULL, 5, NULL);
    //xTaskCreate(adc_read_task, "ADC read task", 2048, NULL, 5, NULL);

    xTaskCreate(&printWiFiIP,"printWiFiIP",2048,NULL,5,NULL);
    xTaskCreate(&tcp_server,"tcp_server",4096,NULL,5,NULL);

    return ESP_OK;
}
