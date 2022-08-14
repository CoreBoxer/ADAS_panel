// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: ADAS

#include "ui.h"
extern int mqtt_remote_publish();
extern int mqtt_chat_publish();
void event_btn_send_1(lv_event_t * e)
{
	// Your code here
	mqtt_remote_publish();
}

void event_btn_send_2(lv_event_t * e)
{
	// Your code here
	mqtt_chat_publish();
}
