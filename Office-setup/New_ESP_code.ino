#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#define SUPPORT_LIGHT   11
#define KEY_LIGHT       12
#define BULB            14
#define BAMBOO_PRINTER  13

const char* ssid = "JioFiber-4G_EXT";
const char* password = "6383450870";

WebServer server(80);
bool invertLogic = true;

struct PinMap {
  const char* name;
  uint8_t pin;
} pinMappings[] = {
  { "BAMBOO_PRINTER", BAMBOO_PRINTER },
  { "SUPPORT_LIGHT", SUPPORT_LIGHT },
  { "KEY_LIGHT", KEY_LIGHT },
  { "BULB", BULB }
};

#define PIN_COUNT (sizeof(pinMappings) / sizeof(pinMappings[0]))

bool get_pin_by_name(const String& name, uint8_t &outPin) {
  for (int i = 0; i < PIN_COUNT; i++) {
    if (name.equalsIgnoreCase(pinMappings[i].name)) {
      outPin = pinMappings[i].pin;
      return true;
    }
  }
  return false;
}

void handle_get_pin_state() {
  String json = "{";
  for (int i = 0; i < PIN_COUNT; i++) {
    uint8_t pin = pinMappings[i].pin;
    String name = pinMappings[i].name;
    int value = digitalRead(pin);
    if (invertLogic) value = !value;
    json += "\"" + name + "\":\"" + (value ? "HIGH" : "LOW") + "\"";
    if (i < PIN_COUNT - 1) json += ",";
  }
  json += "}";
  server.send(200, "application/json", json);
}

// POST /set-pin?name=BULB&state=HIGH
void handle_set_pin() {
  DynamicJsonDocument response(512);
  JsonObject result = response.to<JsonObject>();

  if (server.hasArg("name") && server.hasArg("state")) {
    String name = server.arg("name");
    String state = server.arg("state");
    result["name"] = name;

    uint8_t pin;
    if (!get_pin_by_name(name, pin)) {
      result["status"] = "error";
      result["message"] = "Invalid name";
      Serial.println("Error: Invalid pin name '" + name + "'");
    } else {
      int level = -1;
      if (state.equalsIgnoreCase("HIGH")) level = HIGH;
      else if (state.equalsIgnoreCase("LOW")) level = LOW;

      if (level == -1) {
        result["status"] = "error";
        result["message"] = "Invalid state";
        Serial.println("Error: Invalid state '" + state + "' for pin '" + name + "'");
      } else {
        if (invertLogic) level = !level;
        digitalWrite(pin, level);
        result["status"] = "ok";
        result["set"] = state;

        // Log the change
        Serial.print("Pin Set: ");
        Serial.print(name);
        Serial.print(" (GPIO ");
        Serial.print(pin);
        Serial.print(") => ");
        Serial.println(state);
      }
    }
  } else {
    result["status"] = "error";
    result["message"] = "Missing name or state";
    Serial.println("Error: Missing 'name' or 'state' parameter in request");
  }

  String resp;
  serializeJson(response, resp);
  server.send(200, "application/json", resp);
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < PIN_COUNT; i++) {
    pinMode(pinMappings[i].pin, OUTPUT);
    digitalWrite(pinMappings[i].pin, invertLogic ? HIGH : LOW);
    Serial.print("Configured ");
    Serial.print(pinMappings[i].name);
    Serial.print(" (GPIO ");
    Serial.print(pinMappings[i].pin);
    Serial.println(") as OUTPUT and set to default OFF");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address: " + WiFi.localIP().toString());

  server.on("/pin-state", HTTP_GET, handle_get_pin_state);
  server.on("/set-pin", HTTP_POST, handle_set_pin);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
