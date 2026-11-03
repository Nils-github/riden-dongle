#include "RidenStatus.h"
#include <assert.h>
#include <LittleFS.h>
#include <riden_modbus/riden_modbus_registers.h>
#include <cmath>

using namespace RidenDongle;

RidenStatusClass RidenStatus;

static String jsonNumber(double value)
{
    return std::isfinite(value) ? String(value) : "null";
}

void RidenStatusClass::begin(AsyncWebServer *server, RidenModbus *modbus, const char *url)
{
    _server = server;
    _modbus = modbus;

    if (_ws) {
        delete _ws;
        _ws = nullptr;
    }
    _ws = new AsyncWebSocket("/ws_ridenstatus");

    _ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        switch (type) {
        case WS_EVT_PING:
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%lu connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            client->setCloseClientOnQueueFull(false);
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%lu disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            // AwsFrameInfo *info = (AwsFrameInfo*)arg;
            // if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            //   data[len] = 0;
            // }
            break;
        }
    });

    if (_ws != nullptr) {
        _server->addHandler(_ws);
    }

    if (url && *url) {
        String statusRoute = String(url);
        String statusHtmlRoute = statusRoute.endsWith(".html") ? statusRoute : statusRoute + ".html";

        _server->on(statusRoute.c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
            if (LittleFS.exists("/html/riden_status.html")) {
                request->send(LittleFS, "/html/riden_status.html", "text/html");
            } else {
                request->send(404, "text/plain", "Riden status page not found.");
            }
        });

        if (statusRoute != statusHtmlRoute) {
            _server->on(statusHtmlRoute.c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
                if (LittleFS.exists("/html/riden_status.html")) {
                    request->send(LittleFS, "/html/riden_status.html", "text/html");
                } else {
                    request->send(404, "text/plain", "Riden status page not found.");
                }
            });
        }
    }
}

size_t RidenStatusClass::getConnectionCount()
{
    return _ws->count();
}

static unsigned long lasttime = 0;
void RidenStatusClass::loop()
{
  unsigned long now = millis();
  if (now - lasttime > 200) {
      if (_started && getConnectionCount())
        if (updateStatus(false))
          sendStatus();
      lasttime = now;
  }
}

// Serializes all values from the provided AllValues struct into a JSON string for WebSocket transmission.
// Input: const RidenDongle::AllValues& v - the structure containing all status values.
// Output: String - a JSON representation of all values.
String RidenStatusClass::_serializeAllValues(const RidenDongle::AllValues& v) {
    String json = "{";

    // Scalars
    json += "\"system_temperature_celsius\":" + jsonNumber(v.system_temperature_celsius) + ",";
    json += "\"system_temperature_fahrenheit\":" + jsonNumber(v.system_temperature_fahrenheit) + ",";
    json += "\"voltage_set\":" + jsonNumber(v.voltage_set) + ",";
    json += "\"current_set\":" + jsonNumber(v.current_set) + ",";
    json += "\"voltage_out\":" + jsonNumber(v.voltage_out) + ",";
    json += "\"current_out\":" + jsonNumber(v.current_out) + ",";
    json += "\"power_out\":" + jsonNumber(v.power_out) + ",";
    json += "\"voltage_in\":" + jsonNumber(v.voltage_in) + ",";
    json += "\"keypad_locked\":" + String(v.keypad_locked ? "true" : "false") + ",";
    json += "\"protection\":" + String(static_cast<int>(v.protection)) + ",";
    json += "\"output_mode\":" + String(static_cast<int>(v.output_mode)) + ",";
    json += "\"output_on\":" + String(v.output_on ? "true" : "false") + ",";
    json += "\"current_range\":" + String(v.current_range) + ",";
    json += "\"is_battery_mode\":" + String(v.is_battery_mode ? "true" : "false") + ",";
    json += "\"voltage_battery\":" + jsonNumber(v.voltage_battery) + ",";
    json += "\"probe_temperature_celsius\":" + jsonNumber(v.probe_temperature_celsius) + ",";
    json += "\"probe_temperature_fahrenheit\":" + jsonNumber(v.probe_temperature_fahrenheit) + ",";
    json += "\"ah\":" + jsonNumber(v.ah) + ",";
    json += "\"wh\":" + jsonNumber(v.wh) + ",";
    json += "\"voltage_max\":" + String(_modbus->get_max_voltage()) + ",";
    json += "\"current_max\":" + String(_modbus->get_max_current()) + ",";
    json += "\"is_take_ok\":" + String(v.is_take_ok ? "true" : "false") + ",";
    json += "\"is_take_out\":" + String(v.is_take_out ? "true" : "false") + ",";
    json += "\"is_power_on_boot\":" + String(v.is_power_on_boot ? "true" : "false") + ",";
    json += "\"is_buzzer_enabled\":" + String(v.is_buzzer_enabled ? "true" : "false") + ",";
    json += "\"is_logo\":" + String(v.is_logo ? "true" : "false") + ",";
    json += "\"language\":" + String(v.language) + ",";
    json += "\"brightness\":" + String(v.brightness) + ",";

    // Presets as array of objects
    json += "\"presets\":[";
    for (size_t i = 0; i < NUMBER_OF_PRESETS; ++i) {
        // Ensure all preset fields are initialized
        float voltage = std::isfinite(v.presets[i].voltage) ? v.presets[i].voltage : 0.0f;
        float current = std::isfinite(v.presets[i].current) ? v.presets[i].current : 0.0f;
        float ovp = std::isfinite(v.presets[i].over_voltage_protection) ? v.presets[i].over_voltage_protection : 0.0f;
        float ocp = std::isfinite(v.presets[i].over_current_protection) ? v.presets[i].over_current_protection : 0.0f;

        if (i > 0) json += ",";
        json += "{";
        json += "\"voltage\":" + String(voltage) + ",";
        json += "\"current\":" + String(current) + ",";
        json += "\"over_voltage_protection\":" + String(ovp) + ",";
        json += "\"over_current_protection\":" + String(ocp);
        json += "}";
    }
    json += "]";

    json += "}";

    return json;
}


void RidenStatusClass::sendStatus()
{
    _ws->textAll(_serializeAllValues(_allValues));
}

bool RidenStatusClass::updateStatus(bool full)
{
    return _modbus->get_all_values(_allValues, !full);
}

void RidenStatusClass::end()
{
  _started = false;
}

void RidenStatusClass::start()
{
  _started = true;
  updateStatus(true);
}
