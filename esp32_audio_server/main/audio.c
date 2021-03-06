#include "audio.h"
#include "audio_example_file.h"

/*---------------------------------------------------------------
                            GLOBAL
---------------------------------------------------------------*/
const char* AUDIO_TAG = "AUD";

/*---------------------------------------------------------------
                            FUNCTION
---------------------------------------------------------------*/
/**
 * @brief I2S ADC/DAC mode init.
 */
void example_i2s_init()
{
	 int i2s_num = EXAMPLE_I2S_NUM;
	 i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN,
        .sample_rate =  EXAMPLE_I2S_SAMPLE_RATE,
        .bits_per_sample = EXAMPLE_I2S_SAMPLE_BITS,
	    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
	    .channel_format = EXAMPLE_I2S_FORMAT,
	    .intr_alloc_flags = 0,
	    .dma_buf_count = 4,
        .dma_buf_len = EXAMPLE_I2S_READ_LEN
	 };
	 //install and start i2s driver
	 i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
	 //init DAC pad
	 i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
	 //init ADC pad
	 i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);
     //start to use I2S built-in ADC mode.
     //this function would acquire the lock of ADC.
     i2s_adc_enable(EXAMPLE_I2S_NUM);

    //audio_in_queue = xQueueCreate(2, 1024);
    //audio_out_queue = xQueueCreate(2, 640);    

    //i2s_set_clk(EXAMPLE_I2S_NUM, 8000, EXAMPLE_I2S_SAMPLE_BITS, 1);

}

/*
 * @brief erase flash for recording
 */
/*void example_erase_flash()
{
#if RECORD_IN_FLASH_EN
    printf("Erasing flash \n");
    const esp_partition_t *data_partition = NULL;
    data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);
    if (data_partition != NULL) {
        printf("partiton addr: 0x%08x; size: %d; label: %s\n", data_partition->address, data_partition->size, data_partition->label);
    }
    printf("Erase size: %d Bytes\n", FLASH_ERASE_SIZE);
    ESP_ERROR_CHECK(esp_partition_erase_range(data_partition, 0, FLASH_ERASE_SIZE));
#else
    printf("Skip flash erasing...\n");
#endif
}*/

/**
 * @brief debug buffer data
 */
void example_disp_buf(uint8_t* buf, int length)
{
#if EXAMPLE_I2S_BUF_DEBUG
//    printf("======\n");
    for (int i = 1; i < length; i+=2) {
        printf("%3d ", buf[i]);
        if ((i + 1) % 32 == 0) {
            printf("\n");
        }
    }
//    printf("======\n");
#endif
}

/**
 * @brief debug buffer data
 */
void example_disp_buf2(uint8_t* buf, int length)
{
#if EXAMPLE_I2S_BUF_DEBUG
//    printf("======\n");
    int count = 0;

    for (int i = 1; i < length; i+=2) {
        if( buf[i] == 0 ){
            printf("*");
            count++;           
        }
    }
    printf("\n");

//    printf("======\n");
#endif
}

/**
 * @brief Reset i2s clock and mode
 */
/*void example_reset_play_mode()
{
    i2s_set_clk(EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE, EXAMPLE_I2S_SAMPLE_BITS, EXAMPLE_I2S_CHANNEL_NUM);
}*/

/**
 * @brief Set i2s clock for example audio file
 */
/*void example_set_file_play_mode()
{
    i2s_set_clk(EXAMPLE_I2S_NUM, 8000, EXAMPLE_I2S_SAMPLE_BITS, 1);
}*/

/**
 * @brief Scale data to 16bit/32bit for I2S DMA output.
 *        DAC can only output 8bit data value.
 *        I2S DMA will still send 16 bit or 32bit data, the highest 8bit contains DAC data.
 */
int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 2);
#else
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 4);
#endif
}

/**
 * @brief Scale data to 8bit for data from ADC.
 *        Data from ADC are 12bit width by default.
 *        DAC can only output 8 bit data.
 *        Scale each 12bit ADC data to 8bit DAC data.
 */
void example_i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;

//        if( d_buff[j - 1] == 0 ){
//            d_buff[j - 1] = 140;
//        }
    }
}

void example_i2s_adc_data_scale2(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;    
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = dac_value * 256 / 4096;
    }
}

/**
 * @brief
 */
void example_i2s_adc_dac(void*arg)
{
    int i2s_read_len = EXAMPLE_I2S_READ_LEN;
    size_t bytes_read, bytes_written;

    uint8_t* i2s_read_buff  = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    i2s_adc_enable(EXAMPLE_I2S_NUM);

    while (1) {
        i2s_read(EXAMPLE_I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

        example_i2s_adc_data_scale(i2s_write_buff, i2s_read_buff, i2s_read_len);
        i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, i2s_read_len, &bytes_written, portMAX_DELAY);
        //i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, i2s_read_len, &bytes_written, 0);

        if( bytes_written != 0 ){
            printf("%3d %3d ", bytes_read, bytes_written);
        } else{
            printf("        ");
        }

        example_disp_buf((uint8_t*) i2s_write_buff, 32);
    }

    i2s_adc_disable(EXAMPLE_I2S_NUM);

    free(i2s_read_buff);
    i2s_read_buff = NULL;
    free(i2s_write_buff);
    i2s_write_buff = NULL;
    vTaskDelete(NULL);
}


/**
 * @brief
  */
void i2s_test_task(void* arg)
{
    int offset = 0;
    int tot_size = sizeof(audio_table);
    uint8_t out_buf[640];
    uint8_t in_buf[1024];

    while(1){
        while (offset < tot_size) {
            // ADC
            if( xQueueReceive(audio_in_queue, (void *)&in_buf, portMAX_DELAY) != pdTRUE){
                ESP_LOGE(AUDIO_TAG, "... Failed to dequeue adc data.\n");
                //vTaskDelay(1000 / portTICK_PERIOD_MS);
                //continue;
            }
            //printf("TEST TASK:in dequeue: data[0]=%x \n", in_buf[0]);
            //example_disp_buf((uint8_t*) in_buf, 16);


            // OUT
            int play_len = ((tot_size - offset) > (640)) ? (640) : (tot_size - offset);
            memcpy(out_buf, (uint8_t*)(audio_table + offset), play_len);
            if( xQueueSend(audio_out_queue, (void *)&out_buf, portMAX_DELAY) != pdTRUE){
                ESP_LOGE(AUDIO_TAG, "... Failed to enqueue dac data.\n");
                //vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue;
            }
            offset += play_len;
            //printf("TEST TASK:out enqueue: total=%d play_len=%d sample=%d\n", offset, play_len, tot_size);
            //vTaskDelay(80 / portTICK_PERIOD_MS);
        }
        offset = 0;
    }
}


