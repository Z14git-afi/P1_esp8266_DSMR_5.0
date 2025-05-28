# P1 Elektros Skaitiklio Skaitytuvas su MQTT (ESP8266)

Projektas skirtas nuskaityti P1 (DSMR) elektros skaitiklio telegramas naudojant ESP8266 (pvz., Wemos D1 Mini), validuoti CRC ir siųsti visą telegramą bei gyvybės signalą per MQTT.

---

## 🔧 Naudojama įranga

* ESP8266 (pvz. Wemos D1 Mini)
* P1 skaitiklio prievadas (RJ12 arba TTL perėjimas)
* MQTT brokeris

## 📦 Funkcionalumas

* Prisijungia prie WiFi
* Prisijungia prie MQTT brokerio
* Nuskaito P1 telegramas per UART
* Tikrina CRC
* Siunčia visą telegramą į MQTT temą `p1/fulltelegram`
* Kas sekundę siunčia gyvybės žinutę į `p1/alive`

## 📁 Failų struktūra

```
P1-MQTT-Reader/
├── P1.ino            # Pagrindinis Arduino failas
├── secrets.h           # Slapti duomenys (WiFi ir MQTT)
├── secrets_template.h  # Šablonas slaptiems duomenims
├── .gitignore          # Ignoruojami failai
└── README.md           # Projekto aprašymas
```

## 🔐 `secrets_template.h`

```cpp
#pragma once

const char* WIFI_SSID     = "TavoWiFi";
const char* WIFI_PASSWORD = "TavoSlaptazodis";

const char* MQTT_SERVER   = "192.168.1.100";
const int   MQTT_PORT     = 1883;
const char* MQTT_USER     = "mqtt_user";
const char* MQTT_PASS     = "mqtt_pass";
```

## 📦 MQTT temos

| Tema              | Aprašymas                    |
| ----------------- | ---------------------------- |
| `p1/fulltelegram` | Visa telegrama iš skaitiklio |
| `p1/alive`        | Gyvybės skaitliukas (0–100)  |

## 📜 Telegramos formatas

Telegrama prasideda `/`, baigiasi `!` ir turi 4 simbolių CRC. Pvz:

```
/XYZ5\12345678_A
1-0:1.8.0(001234.567*kWh)
1-0:2.8.0(000123.456*kWh)
!1234
```

## 🛠️ Kompiliavimas

* Naudoti Arduino IDE
* Platforma: ESP8266 (Board: Wemos D1 R2 & Mini)
* Biblitekos:

  * `PubSubClient` (MQTT)
  * `ESP8266WiFi`

## 🧠 Papildomos pastabos

* Jei skaitiklis naudoja invertuotą signalą, RX gali reikėti invertuoti programiškai (naudojama `USC0(UART0) |= BIT(UCRXI);`)
* Galima praplėsti, kad parskaitytų kiekvieną OBIS lauką atskirai

## ✅ Plėtra

* Atskiri OBIS kodo duomenys į atskiras MQTT temas
* Tiesioginis integravimas su Home Assistant per autodiscovery

---

Sukurta naudojant ESP8266 + MQTT. Jei iškilo klausimų – kreipkis!
