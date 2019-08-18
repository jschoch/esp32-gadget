#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "alertsDeleteAlertDirective.pb.h"
#include "alertsSetAlertDirective.pb.h"
#include "alexa.h"
#include "alexaDiscoveryDiscoverDirective.pb.h"
#include "alexaDiscoveryDiscoverResponseEvent.pb.h"
#include "alexaGadgetSpeechDataSpeechmarksDirective.pb.h"
#include "alexaGadgetStateListenerStateUpdateDirective.pb.h"
#include "btstack_debug.h"
#include "directiveParser.pb.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "eventParser.pb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "notificationsSetIndicatorDirective.pb.h"
#include "output.h"
#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "freertos/queue.h"

static const char* TAG = "alexa.c";

uint8_t out_buffer[1000];
uint16_t out_buffer_len;
static time_t timer;
static char timer_token[100];


extern xQueueHandle timer_queue;

void create_discovery_response() {
  ESP_LOGI(TAG, "Creating discover response event");
  pb_ostream_t stream = pb_ostream_from_buffer(out_buffer, sizeof(out_buffer));
  static alexaDiscovery_DiscoverResponseEventProto env =
      alexaDiscovery_DiscoverResponseEventProto_init_zero;

  strcpy(env.event.header.namespace, "Alexa.Discovery");
  strcpy(env.event.header.name, "Discover.Response");

  env.event.payload.endpoints_count = 1;
  strcpy(env.event.payload.endpoints[0].endpointId, "ledot00003");
  strcpy(env.event.payload.endpoints[0].friendlyName, "ledot 03");
  strcpy(env.event.payload.endpoints[0].description, "da ledot");
  strcpy(env.event.payload.endpoints[0].manufacturerName, "Jesse Schoch");

  env.event.payload.endpoints[0].capabilities_count = 2;
  strcpy(env.event.payload.endpoints[0].capabilities[0].type, "AlexaInterface");
  strcpy(env.event.payload.endpoints[0].capabilities[0].interface, "Alerts");
  strcpy(env.event.payload.endpoints[0].capabilities[0].version, "1.1");
  strcpy(env.event.payload.endpoints[0].capabilities[1].type, "AlexaInterface");
  strcpy(env.event.payload.endpoints[0].capabilities[1].interface,
         "Alexa.Gadget.StateListener");
  strcpy(env.event.payload.endpoints[0].capabilities[1].version, "1.0");
  env.event.payload.endpoints[0]
      .capabilities[1]
      .configuration.supportedTypes_count = 3;
  strcpy(env.event.payload.endpoints[0]
             .capabilities[1]
             .configuration.supportedTypes[0]
             .name,
         "timers");
  strcpy(env.event.payload.endpoints[0]
             .capabilities[1]
             .configuration.supportedTypes[1]
             .name,
         "timeinfo");
  strcpy(env.event.payload.endpoints[0]
             .capabilities[1]
             .configuration.supportedTypes[2]
             .name,
         "wakeword");

  strcpy(
      env.event.payload.endpoints[0].additionalIdentification.firmwareVersion,
      "0");
  strcpy(env.event.payload.endpoints[0].additionalIdentification.deviceToken,
         "b'a6f589a5a8f11dff86d88f6ac16f9161fa3a7b02fe61f947d8b1170c7ccde0d6'");
  strcpy(env.event.payload.endpoints[0]
             .additionalIdentification.deviceTokenEncryptionType,
         "1");
  strcpy(
      env.event.payload.endpoints[0].additionalIdentification.amazonDeviceType,
      "A3OPCPURPKGDTE");
  strcpy(env.event.payload.endpoints[0].additionalIdentification.modelName,
         "ledot 03");
  strcpy(env.event.payload.endpoints[0].additionalIdentification.radioAddress,
         //"3C71BF9ABE66");
	// a4:cf:12:6c:2d:a4
	"A4CF126C2DA6");

  bool status = pb_encode(
      &stream, alexaDiscovery_DiscoverResponseEventProto_fields, &env);
  if (!status) {
    ESP_LOGE(TAG, "Error encoding message: %s", PB_GET_ERROR(&stream));
    return;
  }

  out_buffer_len = stream.bytes_written;
  send_data_callback(out_buffer, out_buffer_len);
  memset(out_buffer, 0, sizeof(out_buffer));
  out_buffer_len = 0;
}

void handle_discovery_request(pb_istream_t stream) {
  static alexaDiscovery_DiscoverDirectiveProto envelope =
      alexaDiscovery_DiscoverDirectiveProto_init_default;
  pb_decode(&stream, alexaDiscovery_DiscoverDirectiveProto_fields, &envelope);
  printf("scope type: %s\n", envelope.directive.payload.scope.type);
  printf("scope token: %s\n", envelope.directive.payload.scope.token);

  create_discovery_response();
}

void handle_time_info(char* current_time) {
  static struct tm tm;
  strptime(current_time, "%Y-%m-%dT%H:%M:%S", &tm);
  time_t t = mktime(&tm);
  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);

  ESP_LOGI(TAG, "time info: %d %d %d %d %d %d, %ld", tm.tm_year, tm.tm_mon,
           tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, t);
}
int active = 0;
void set_timer(char* token, char* scheduled_time) {
  static struct tm tm;
  strptime(scheduled_time, "%Y-%m-%dT%H:%M:%S", &tm);
  timer = mktime(&tm);
  int active = 1;
  memset(timer_token, 0, sizeof(timer_token));
  strcpy(timer_token, token);
  ESP_LOGI(TAG, "timer: %s %d %d %d %d %d %d", token, tm.tm_year, tm.tm_mon,
           tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  xQueueSendToBack(timer_queue, &active, 1000);
}

void cancel_timer() {
  ESP_LOGI(TAG, "Cancelling timer");
  memset(timer_token, 0, sizeof(timer_token));
  timer = 0;
  active = 0;
  xQueueSendToBack(timer_queue, &active, 1000);
}

void handle_set_alert(pb_istream_t stream) {
  static alerts_SetAlertDirectiveProto envelope =
      alerts_SetAlertDirectiveProto_init_default;
  pb_decode(&stream, alerts_SetAlertDirectiveProto_fields, &envelope);
  printf("alert type: %s\n", envelope.directive.payload.type);
  printf("alert token: %s\n", envelope.directive.payload.token);
  printf("alert scheduled time: %s\n",
         envelope.directive.payload.scheduledTime);

  if (0 == strcmp(envelope.directive.payload.type, "TIMER")) {
    set_timer(envelope.directive.payload.token,
              envelope.directive.payload.scheduledTime);
  }
}

void handle_delete_alert(pb_istream_t stream) {
  static alerts_DeleteAlertDirectiveProto envelope =
      alerts_DeleteAlertDirectiveProto_init_default;
  pb_decode(&stream, alerts_SetAlertDirectiveProto_fields, &envelope);
  printf("alert token: %s\n", envelope.directive.payload.token);

  if (0 == strcmp(timer_token, envelope.directive.payload.token)) {
    cancel_timer();
  }
}

void handle_alexa_payload(uint8_t* buffer, uint16_t len) {
  ESP_LOGI(TAG, "Parsing Directive");
  static directive_DirectiveParserProto envelope =
      directive_DirectiveParserProto_init_default;

  pb_istream_t stream = pb_istream_from_buffer(buffer, len);
  pb_decode(&stream, directive_DirectiveParserProto_fields, &envelope);
  stream = pb_istream_from_buffer(buffer, len);

  ESP_LOGI(TAG, "name = %s, namespace=%s", envelope.directive.header.name,
           envelope.directive.header.namespace);

  char* name = envelope.directive.header.name;
  char* namespace = envelope.directive.header.namespace;
  if (0 == strcmp(name, "SetIndicator") &&
      (0 == strcmp(namespace, "Notifications"))) {
    // Parse the notification payload now
    static notifications_SetIndicatorDirectiveProto notifications_envelope =
        notifications_SetIndicatorDirectiveProto_init_default;
    pb_decode(&stream, notifications_SetIndicatorDirectiveProto_fields,
              &notifications_envelope);

    printf("visualIndicator:%d, audioIndicator=%d, assetId=%s, url=%s\n",
           notifications_envelope.directive.payload.persistVisualIndicator,
           notifications_envelope.directive.payload.playAudioIndicator,
           notifications_envelope.directive.payload.asset.assetId,
           notifications_envelope.directive.payload.asset.url);
  } else if (0 == strcmp(name, "Discover") &&
             (0 == strcmp(namespace, "Alexa.Discovery"))) {
    handle_discovery_request(stream);
  } else if (0 == strcmp(name, "SetAlert") &&
             0 == strcmp(namespace, "Alerts")) {
    handle_set_alert(stream);
  } else if (0 == strcmp(name, "DeleteAlert") &&
             0 == strcmp(namespace, "Alerts")) {
    handle_delete_alert(stream);
  } else if (0 == strcmp(name, "StateUpdate") &&
             (0 == strcmp(namespace, "Alexa.Gadget.StateListener"))) {
    static alexaGadgetStateListener_StateUpdateDirectiveProto envelope =
        alexaGadgetStateListener_StateUpdateDirectiveProto_init_default;
    pb_decode(&stream,
              alexaGadgetStateListener_StateUpdateDirectiveProto_fields,
              &envelope);
    int states_count = envelope.directive.payload.states_count;
    for (int i = 0; i < states_count; ++i) {
      char* name = envelope.directive.payload.states[i].name;
      char* value = envelope.directive.payload.states[i].value;
      printf("state name: %s\n", name);
      printf("state value: %s\n", value);
      if (0 == strcmp(name, "timeinfo")) {
        handle_time_info(value);
      }
    }
  } else if (0 == strcmp(name, "Speechmarks") &&
             (0 == strcmp(namespace, "Alexa.Gadget.SpeechData"))) {
    static alexaGadgetSpeechData_SpeechmarksDirectiveProto envelope =
        alexaGadgetSpeechData_SpeechmarksDirectiveProto_init_default;
    pb_decode(&stream, alexaGadgetSpeechData_SpeechmarksDirectiveProto_fields,
              &envelope);
    int speechmarks_count = envelope.directive.payload.speechmarksData_count;
    printf("player offset: %d\n",
           envelope.directive.payload.playerOffsetInMilliSeconds);
    for (int i = 0; i < speechmarks_count; ++i) {
      printf("speechmark type: %s\n",
             envelope.directive.payload.speechmarksData[i].type);
      printf("speechmark value: %s\n",
             envelope.directive.payload.speechmarksData[i].value);
      printf("speechmark start offset: %d\n",
             envelope.directive.payload.speechmarksData[i]
                 .startOffsetInMilliSeconds);
    }
  } else {
    ESP_LOGE(TAG, "Error: unknown directive. Name: %s, namespace: %s", name,
             namespace);
  }
}

void register_send_data_callback(send_data_callback_t callback) {
  send_data_callback = callback;
}



