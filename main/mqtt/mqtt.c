#include "header.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

void mqttOnConnect(esp_mqtt_client_handle_t client, esp_mqtt_event_handle_t event, int msg_id)
{
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
    ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
}

void mqttOnDisconnect(esp_mqtt_client_handle_t client, esp_mqtt_event_handle_t event, int msg_id)
{
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
}

void mqttOnSubscribe(esp_mqtt_client_handle_t client, esp_mqtt_event_handle_t event, int msg_id)
{
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}

void mqttOnUnsubscribe(esp_mqtt_client_handle_t client, esp_mqtt_event_handle_t event, int msg_id)
{
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
}

void mqttOnPublish(esp_mqtt_client_handle_t client, esp_mqtt_event_handle_t event, int msg_id)
{
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
}

void mqttOnData(esp_mqtt_client_handle_t client, esp_mqtt_event_handle_t event, int msg_id)
{
    sendString(event->data, event->data_len);
}

void mqttOnError(esp_mqtt_client_handle_t client, esp_mqtt_event_handle_t event, int msg_id)
{

    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
    {
        log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
        ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = 0;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        mqttOnConnect(client, event, msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqttOnDisconnect(client, event, msg_id);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        mqttOnSubscribe(client, event, msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        mqttOnUnsubscribe(client, event, msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        mqttOnPublish(client, event, msg_id);
        break;
    case MQTT_EVENT_DATA:
        mqttOnData(client, event, msg_id);
        break;
    case MQTT_EVENT_ERROR:
        mqttOnError(client, event, msg_id);
        break;
    default:
        break;
    }
}

static void initializeMQTT()
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://user:abc@alteratom.com:1883",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}