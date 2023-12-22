#include "hid/hid.h"
#include "bluetooth/bluetooth.h"

void app_main(void)
{    
    // Magic wifi shit
    //ESP_ERROR_CHECK(nvs_flash_init());
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    //ESP_ERROR_CHECK(example_connect());

    initializeHID();
    initializeBT();
    //initializeMQTT();
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //sendString("MtS637hQH4i9gED6DGmgIOYe", 24);
}