#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define BAUD_RATE 115200
#define P1_MAXLINELENGTH 1024

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

  String content = telegram.substring(startIndex, endIndex + 1); // nuo '/' iki '!'
  String crcString = telegram.substring(endIndex + 1, endIndex + 5); // 4 heks simboliai

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
  if (mqttClient.publish(mqtt_telegram_topic, data.c_str())) {
    Serial.println("✅ MQTT telegrama issiusta.");
  } else {
    Serial.println("❌ MQTT siuntimas nepavyko.");
  }

  // Papildomai: siųsti kiekvieną eilutę kaip atskirą temą
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
        value = value.substring(0, unitIndex); // pašalina vienetus
      }

      obis.replace(":", "_"); // MQTT temoje vietoj ':' naudosime '_'

      String topic = String(mqtt_prefix_topic) + "/" + obis;
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
