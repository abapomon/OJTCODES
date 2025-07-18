#include "who_camera.h"
#include "who_human_face_recognition.hpp"
#include "app_wifi.h"
#include "app_httpd.hpp"
#include "app_mdns.h"
#include "event_logic.hpp"

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueEventLogic = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;
static QueueHandle_t xQueueEvent = NULL;


extern "C" void app_main()
{
    app_wifi_main();

    xQueueEvent = xQueueCreate(1, sizeof(int));
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t *));

    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);
    app_mdns_main();

    register_button_events(xQueueEvent);  // <- FIXED HERE
    register_human_face_recognition(xQueueAIFrame, xQueueEvent, NULL, xQueueHttpFrame, true);
    register_httpd(xQueueHttpFrame, NULL, true);
}