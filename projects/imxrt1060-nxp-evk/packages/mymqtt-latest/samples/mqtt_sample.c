#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <rtthread.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME    "mqtt.sample"
#define DBG_LEVEL           DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "mqtt_client.h"
#include "ui.h"
#include "cjson.h"
/**
 * MQTT URI farmat:
 * domain mode
 * tcp://iot.eclipse.org:1883
 *
 * ipv4 mode
 * tcp://192.168.10.1:1883
 * ssl://192.168.10.1:1884
 *
 * ipv6 mode
 * tcp://[fe80::20c:29ff:fe9a:a07e]:1883
 * ssl://[fe80::20c:29ff:fe9a:a07e]:1884
 */
#define MQTT_URI                "tcp://your MQTT server IP:1883" //"tcp://mq.tongxinmao.com:18831"
#define MQTT_SUBTOPIC           "/chat"
#define MQTT_PUBTOPIC           "/remote"
#define MQTT_PUBTOPIC2          "/chat"
#define MQTT_WILLMSG            "Goodbye!"

char* cJSON_buffer;
char* chat_buffer;
cJSON* cjson_remote = NULL;
cJSON* cjson_chat = NULL;

/* define MQTT client context */
static mqtt_client client;
static int is_started = 0;

/* DIY begin */
int mqtt_remote_publish()
{
    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }
		int32_t val_light = lv_slider_get_value(ui_Slider1);
		int32_t val_temp = lv_arc_get_value(ui_ArcTemp);
		int32_t val_sw1 = lv_obj_has_state(ui_Switch1, LV_STATE_CHECKED) == true ? 1 : 0;
		int32_t val_sw2 = lv_obj_has_state(ui_Switch2, LV_STATE_CHECKED) == true ? 1 : 0;
		
		cjson_remote = cJSON_CreateObject();
		cJSON_AddNumberToObject(cjson_remote,"val_light", val_light);
		cJSON_AddNumberToObject(cjson_remote,"val_temp", val_temp);
		cJSON_AddNumberToObject(cjson_remote,"val_sw1", val_sw1);
		cJSON_AddNumberToObject(cjson_remote,"val_sw2", val_sw2);
		cJSON_buffer = cJSON_Print(cjson_remote);

    paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, cJSON_buffer, strlen(cJSON_buffer));
		LOG_D("[MQTT] SEND /remote %s.", cJSON_buffer);
		cJSON_Delete(cjson_remote);

    return 0;
}
int mqtt_chat_publish()
{
    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }
		const char* old = lv_textarea_get_text(ui_TextArea1);
		const char* input = lv_textarea_get_text(ui_TextInput);

		chat_buffer = rt_calloc(1, strlen(old) + strlen(input) + 10);
		rt_sprintf(chat_buffer, "%s\nMe: %s", old, input);
		lv_textarea_set_text(ui_TextArea1, chat_buffer);
		
    paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC2, (void*)input, strlen(input));
		LOG_D("[MQTT] SEND /chat %s.", input);
		rt_free(chat_buffer);

    return 0;
}

static void mqtt_sub_chat_callback(mqtt_client *c, message_data *msg_data)
{
    //*((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
		cjson_chat = cJSON_Parse(msg_data->message->payload);
		if (!cjson_chat) {
				LOG_D("get cjson_chat faild !\n");
				LOG_D("Error before: [%s]\n", cJSON_GetErrorPtr());
				return;
		}
		
		cJSON* name = cJSON_GetObjectItem(cjson_chat, "name");
		if (!name) {
				LOG_D("get name faild !\n");
				LOG_D("Error before: [%s]\n", cJSON_GetErrorPtr());
		}
		char* name_str = (char*)rt_malloc(strlen(name->valuestring));
		sprintf(name_str, "%s", name->valuestring);
		
		cJSON* msg = cJSON_GetObjectItem(cjson_chat, "msg");
		if (!name) {
				LOG_D("get msg faild !\n");
				LOG_D("Error before: [%s]\n", cJSON_GetErrorPtr());
		}
		char* msg_str = (char*)rt_malloc(strlen(msg->valuestring));
		sprintf(msg_str, "%s", msg->valuestring);
		
		cJSON_Delete(cjson_chat);

	  const char* old = lv_textarea_get_text(ui_TextArea1);

		chat_buffer = rt_calloc(1, strlen(old) + strlen(name_str) + strlen(msg_str) + 10);
		rt_sprintf(chat_buffer, "%s\n%s: %s", old, name_str, msg_str);
		lv_textarea_set_text(ui_TextArea1, chat_buffer);
		rt_free(chat_buffer);
		
    LOG_D("[MQTT] SUB: %.*s %.*s",
               msg_data->topic_name->lenstring.len,
               msg_data->topic_name->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
}
/* DIY end */

static void mqtt_sub_callback(mqtt_client *c, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub callback: %.*s %.*s",
               msg_data->topic_name->lenstring.len,
               msg_data->topic_name->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
}

static void mqtt_sub_default_callback(mqtt_client *c, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub default callback: %.*s %.*s",
               msg_data->topic_name->lenstring.len,
               msg_data->topic_name->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
}

static void mqtt_connect_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_connect_callback!");
}

static void mqtt_online_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_online_callback!");
}

static void mqtt_offline_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_offline_callback!");
}

static int mqtt_start(int argc, char **argv)
{
    /* init condata param by using MQTTPacket_connectData_initializer */
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    static char cid[20] = { 0 };
    

    if (argc != 1)
    {
        rt_kprintf("mqtt_start    --start a mqtt worker thread.\n");
        return -1;
    }

    if (is_started)
    {
        LOG_E("mqtt client is already connected.");
        return -1;
    }
    /* config MQTT context param */
    {
        client.isconnected = 0;
        client.uri = MQTT_URI;

        /* generate the random client ID */
        rt_snprintf(cid, sizeof(cid), "rtthread%d", rt_tick_get());
        /* config connect param */
        memcpy(&client.condata, &condata, sizeof(condata));
        client.condata.clientID.cstring = cid;
        client.condata.username.cstring = "username";
        client.condata.password.cstring = "password";
        client.condata.keepAliveInterval = 30;
        client.condata.cleansession = 1;

        /* config MQTT will param. */
        client.condata.willFlag = 1;
        client.condata.will.qos = 1;
        client.condata.will.retained = 0;
        client.condata.will.topicName.cstring = MQTT_PUBTOPIC;
        client.condata.will.message.cstring = MQTT_WILLMSG;

        /* malloc buffer. */
        client.buf_size = client.readbuf_size = 1024;
        client.buf = rt_calloc(1, client.buf_size);
        client.readbuf = rt_calloc(1, client.readbuf_size);
        if (!(client.buf && client.readbuf))
        {
            LOG_E("no memory for MQTT client buffer!");
            return -1;
        }

        /* set event callback function */
        client.connect_callback = mqtt_connect_callback;
        client.online_callback = mqtt_online_callback;
        client.offline_callback = mqtt_offline_callback;

        /* set subscribe table and event callback */
        client.message_handlers[0].topicFilter = rt_strdup(MQTT_SUBTOPIC);
        client.message_handlers[0].callback = mqtt_sub_chat_callback;
        client.message_handlers[0].qos = QOS1;

        /* set default subscribe event callback */
        client.default_message_handlers = mqtt_sub_default_callback;
    }
    
    {
      int value;
      uint16_t u16Value;
      value = 5;
      paho_mqtt_control(&client, MQTT_CTRL_SET_CONN_TIMEO, &value);
      value = 5;
      paho_mqtt_control(&client, MQTT_CTRL_SET_MSG_TIMEO, &value);
      value = 5;
      paho_mqtt_control(&client, MQTT_CTRL_SET_RECONN_INTERVAL, &value);
      value = 30;
      paho_mqtt_control(&client, MQTT_CTRL_SET_KEEPALIVE_INTERVAL, &value);
      u16Value = 3;
      paho_mqtt_control(&client, MQTT_CTRL_SET_KEEPALIVE_COUNT, &u16Value);
    }

    /* run mqtt client */
    paho_mqtt_start(&client, 8196, 20);
    is_started = 1;

    return 0;
}

static int mqtt_stop(int argc, char **argv)
{
    if (argc != 1)
    {
        rt_kprintf("mqtt_stop    --stop mqtt worker thread and free mqtt client object.\n");
    }

    is_started = 0;

    return paho_mqtt_stop(&client);
}

static int mqtt_publish(int argc, char **argv)
{
    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    if (argc == 2)
    {
        paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, argv[1], strlen(argv[1]));
    }
    else if (argc == 3)
    {
        paho_mqtt_publish(&client, QOS1, argv[1], argv[2],strlen(argv[2]));
    }
    else
    {
        rt_kprintf("mqtt_publish <topic> [message]  --mqtt publish message to specified topic.\n");
        return -1;
    }

    return 0;
}

static void mqtt_new_sub_callback(mqtt_client *client, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt new subscribe callback: %.*s %.*s",
               msg_data->topic_name->lenstring.len,
               msg_data->topic_name->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
}

static int mqtt_subscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_subscribe [topic]  --send an mqtt subscribe packet and wait for suback before returning.\n");
        return -1;
    }
	
	if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    return paho_mqtt_subscribe(&client, QOS1, argv[1], mqtt_new_sub_callback);
}

static int mqtt_unsubscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_unsubscribe [topic]  --send an mqtt unsubscribe packet and wait for suback before returning.\n");
        return -1;
    }
	
	if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }

    return paho_mqtt_unsubscribe(&client, argv[1]);
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(mqtt_start, startup mqtt client);
MSH_CMD_EXPORT(mqtt_stop, stop mqtt client);
MSH_CMD_EXPORT(mqtt_publish, mqtt publish message to specified topic);
MSH_CMD_EXPORT(mqtt_subscribe,  mqtt subscribe topic);
MSH_CMD_EXPORT(mqtt_unsubscribe, mqtt unsubscribe topic);
#endif /* FINSH_USING_MSH */

