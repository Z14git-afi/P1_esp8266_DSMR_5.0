#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <map>

#define BAUD_RATE 115200
#define P1_MAXLINELENGTH 1024

#include "secrets.h"

// Wifi
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// MQTT
const char* mqtt_server = MQTT_SERVER;
const int   mqtt_port   = MQTT_PORT;
const char* mqtt_user   = MQTT_USER;
const char* mqtt_pass   = MQTT_PASS;

const char* mqtt_telegram_topic = "p1/fulltelegram";
const char* mqtt_alive_topic    = "p1/alive";
const char* mqtt_prefix_topic   = "p1/data";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Buferiai
char telegram[P1_MAXLINELENGTH];
String fullTelegram = "";
bool insideTelegram = false;

// Gyvybės signalas
unsigned long lastAliveSent = 0;
int aliveCounter = 0;

// OBIS žemėlapis (dalinis, papildyk pagal poreikį)
std::map<String, String> obisMap = {
  {"0-0:1.0.0", "Clock"},
  {"1-0:1.8.0", "Active_energy_import"},
  {"1-0:1.8.1", "Active_energy_import_rate_1"},
  {"1-0:1.8.2", "Active_energy_import_rate_2"},
  {"1-0:1.8.3", "Active_energy_import_rate_3"},
  {"1-0:1.8.4", "Active_energy_import_rate_4"},
  {"1-0:2.8.0", "Active_energy_export"},
  {"1-0:2.8.1", "Active_energy_export_rate_1"},
  {"1-0:2.8.2", "Active_energy_export_rate_2"},
  {"1-0:2.8.3", "Active_energy_export_rate_3"},
  {"1-0:2.8.4", "Active_energy_export_rate_4"},
  {"1-0:3.8.0", "Reactive_energy_import"},
  {"1-0:3.8.1", "Reactive_energy_rate_1"},
  {"1-0:3.8.2", "Reactive_energy_rate_2"},
  {"1-0:3.8.3", "Reactive_energy_rate_3"},
  {"1-0:3.8.4", "Reactive_energy_rate_4"},
  {"1-0:4.8.0", "Reactive_energy_export"},
  {"1-0:4.8.1", "Reactive_energy_export_rate_1"},
  {"1-0:4.8.2", "Reactive_energy_export_rate_2"},
  {"1-0:4.8.3", "Reactive_energy_export_rate_3"},
  {"1-0:4.8.4", "Reactive_energy_export_rate_4"},
  {"1-0:1.7.0", "Instantaneous_active_import_power"},
  {"1-0:2.7.0", "Instantaneous_active_export_power"},
  {"1-0:3.7.0", "Instantaneous_reactive_import_power"},
  {"1-0:4.7.0", "Instantaneous_reactive_export_power"},
  {"1-0:21.7.0", "Power_import_phase_L1"},
  {"1-0:41.7.0", "Power_import_phase_L2"},
  {"1-0:61.7.0", "Power_import_phase_L3"},
  {"1-0:22.7.0", "Power_export_phase_L1"},
  {"1-0:42.7.0", "Power_export_phase_L2"},
  {"1-0:62.7.0", "Power_export_phase_L3"},
  {"1-0:23.7.0", "Reactive_import_phase_L1"},
  {"1-0:43.7.0", "Reactive_import_phase_L2"},
  {"1-0:63.7.0", "Reactive_import_phase_L3"},
  {"1-0:24.7.0", "Reactive_export_phase_L1"},
  {"1-0:44.7.0", "Reactive_export_phase_L2"},
  {"1-0:64.7.0", "Reactive_export_phase_L3"},
  {"1-0:32.7.0", "Voltage_L1"},
  {"1-0:32.24.0", "Average_voltage_L1"},
  {"1-0:31.7.0", "Current_L1"},
  {"1-0:52.7.0", "Voltage_L2"},
  {"1-0:52.24.0", "Average_voltage_L2"},
  {"1-0:51.7.0", "Current_L2"},
  {"1-0:72.7.0", "Voltage_L3"},
  {"1-0:72.24.0", "Average_voltage_L3"},
  {"1-0:71.7.0", "Current_L3"},
  {"1-0:12.7.0", "Voltage_U"},
  {"1-0:11.7.0", "Current"},
  {"1-0:91.7.0", "Neutral_current"},
  {"1-0:90.7.0", "Sum_current"},
  {"1-0:14.7.0", "Frequency"},
  {"1-0:15.7.0", "Active_power"},
  {"1-0:9.7.0", "Apparent_import_power"},
  {"1-0:29.7.0", "Apparent_import_phase_L1"},
  {"1-0:49.7.0", "Apparent_import_phase_L2"},
  {"1-0:69.7.0", "Apparent_import_phase_L3"},
  {"1-0:10.7.0", "Apparent_export_power"},
  {"1-0:30.7.0", "Apparent_export_phase_L1"},
  {"1-0:50.7.0", "Apparent_export_phase_L2"},
  {"1-0:70.7.0", "Apparent_export_phase_L3"},
  {"1-0:1.24.0", "Average_import_power"},
  {"1-0:16.24.0", "Average_net_power"},
  {"1-0:15.24.0", "Average_total_power"},
  {"1-0:13.7.0", "Power_factor"},
  {"1-0:33.7.0", "Power_factor_L1"},
  {"1-0:53.7.0", "Power_factor_L2"},
  {"1-0:73.7.0", "Power_factor_L3"},
  {"1-0:13.3.0", "Min_power_factor"},
  {"1-0:0.8.2", "Measurement_period"},
  {"1-0:1.4.0", "Demand_active_import"},
  {"1-0:2.4.0", "Demand_active_export"},
  {"1-0:3.4.0", "Demand_reactive_import"},
  {"1-0:4.4.0", "Demand_reactive_export"},
  {"1-0:9.4.0", "Demand_apparent_import"},
  {"1-0:10.4.0", "Demand_apparent_export"},
  {"0-0:96.7.21", "Power_failures"},
  {"1-0:32.33.0", "Voltage_sag_duration_L1"},
  {"1-0:52.33.0", "Voltage_sag_duration_L2"},
  {"1-0:72.33.0", "Voltage_sag_duration_L3"},
  {"1-0:32.34.0", "Voltage_sag_magnitude_L1"},
  {"1-0:52.34.0", "Voltage_sag_magnitude_L2"},
  {"1-0:72.34.0", "Voltage_sag_magnitude_L3"},
  {"1-0:32.37.0", "Voltage_swell_duration_L1"},
  {"1-0:52.37.0", "Voltage_swell_duration_L2"},
  {"1-0:72.37.0", "Voltage_swell_duration_L3"},
  {"1-0:32.38.0", "Voltage_swell_magnitude_L1"},
  {"1-0:52.38.0", "Voltage_swell_magnitude_L2"},
  {"1-0:72.38.0", "Voltage_swell_magnitude_L3"},
  {"1-0:0.2.0", "Firmware_id"},
  {"1-0:0.2.8", "Firmware_signature"},
  {"1-1:0.2.0", "Firmware_id_1"},
  {"1-1:0.2.8", "Firmware_signature_1"},
  {"0-0:96.13.0", "Consumer_message"},
  {"1-0:32.32.0", "Voltage_sags_L1"},
  {"1-0:52.32.0", "Voltage_sags_L2"},
  {"1-0:72.32.0", "Voltage_sags_L3"},
  {"1-0:32.36.0", "Voltage_swells_L1"},
  {"1-0:52.36.0", "Voltage_swells_L2"},
  {"1-0:72.36.0", "Voltage_swells_L3"},
  {"0-0:96.1.1", "Meter_identifier"}
};
// ===================== CRC skaičiavimas =====================
unsigned int CRC16(unsigned int crc, const uint8_t *buf, int len) {
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool validate_crc(const String& telegram) {
  int startIndex = telegram.indexOf('/');
  int endIndex = telegram.lastIndexOf('!');

  if (startIndex < 0 || endIndex < 0 || endIndex + 5 > telegram.length()) {
    return false; // neteisinga struktūra
  }

  String content = telegram.substring(startIndex, endIndex + 1);
  String crcString = telegram.substring(endIndex + 1, endIndex + 5);

  unsigned int calculatedCRC = CRC16(0x0000, (const uint8_t*)content.c_str(), content.length());
  unsigned int receivedCRC = (unsigned int)strtol(crcString.c_str(), NULL, 16);

  return calculatedCRC == receivedCRC;
}

// ===================== WiFi / MQTT =====================
void setup_wifi() {
  delay(10);
  Serial.print("Jungiamasi prie WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 30) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi prisijungta!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi prisijungti nepavyko.");
  }
}

void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("MQTT jungimasis... ");
    if (mqttClient.connect("p1reader", mqtt_user, mqtt_pass)) {
      Serial.println("prisijungta.");
    } else {
      Serial.print("klaida, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" bandom po 5s");
      delay(5000);
    }
  }
}

void send_full_telegram(const String& data) {
  if (!mqttClient.connected()) return;
  mqttClient.publish(mqtt_telegram_topic, data.c_str());
  Serial.println("✅ MQTT telegrama issiusta.");

  int start = 0;
  while (start < data.length()) {
    int end = data.indexOf('\n', start);
    if (end == -1) break;
    String line = data.substring(start, end);
    start = end + 1;

    if (line.indexOf('(') > 0) {
      String obis = line.substring(0, line.indexOf('('));
      String value = line.substring(line.indexOf('(') + 1);
      value.replace(")", "");
      int unitIndex = value.indexOf('*');
      if (unitIndex >= 0) {
        value = value.substring(0, unitIndex);
      }

      obis.trim();
      String readable = obisMap.count(obis) ? obisMap[obis] : obis;
      readable.replace(" ", "_");

      String topic = String(mqtt_prefix_topic) + "/" + readable;
      mqttClient.publish(topic.c_str(), value.c_str());
    }
  }
}

void send_alive_status() {
  char payload[8];
  itoa(aliveCounter, payload, 10);
  mqttClient.publish(mqtt_alive_topic, payload);
  Serial.print("Gyvas: ");
  Serial.println(payload);

  aliveCounter = (aliveCounter + 1) % 101;
}

// ===================== Main =====================
void setup() {
  Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_FULL);
  Serial.setRxBufferSize(P1_MAXLINELENGTH);
  Serial.println("P1 skaitymas su CRC + MQTT siuntimas");

  USC0(UART0) |= BIT(UCRXI);

  setup_wifi();
  mqttClient.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop();

  if (Serial.available()) {
    int len = Serial.readBytesUntil('\n', telegram, P1_MAXLINELENGTH - 1);
    telegram[len] = '\0';
    String line = String(telegram);

    Serial.println(line);

    if (line.startsWith("/")) {
      fullTelegram = line + "\n";
      insideTelegram = true;
    } else if (insideTelegram) {
      fullTelegram += line + "\n";
      if (line.startsWith("!")) {
        fullTelegram += "\n";

        if (validate_crc(fullTelegram)) {
          send_full_telegram(fullTelegram);
        } else {
          Serial.println("❌ Neteisingas CRC – telegrama atmesta.");
        }

        insideTelegram = false;
        fullTelegram = "";
      }
    }
  }

  unsigned long now = millis();
  if (now - lastAliveSent > 1000) {
    send_alive_status();
    lastAliveSent = now;
  }
}
