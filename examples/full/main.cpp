#include <Arduino.h>
#include "A9G.h"

#define A9G_RESET_PIN 26
#define A9G_INIT_PIN 4

#define NETWORK_APN (char*)"xpto.com.br"

#define MQTT_BROKER_ADDR (char*)"mqtt.xpto.io"
#define MQTT_BROKER_PORT 1883
#define MQTT_BROKER_AUTH_USER (char*)"xpto"
#define MQTT_BROKER_AUTH_PASSWORD (char*)"xpto"

#define MQTT_CLIENT_ID (char*)"A9G"
#define MQTT_CLIENT_KEEPALIVE_SECS 120

#define MQTT_PUB_TOPIC (char*)"pub"
#define MQTT_PUB_PAYLOAD (char*)"Hello World!"
#define MQTT_PUB_QOS 1

#define MQTT_SUB_TOPIC (char*)"sub"
#define MQTT_SUB_QOS 1

#define GPS_REFRESH_INTERVAL_SECS 5

#define MODEM_INIT_TIMEOUT_SECS 120

A9G_Controller A9G(A9G_RESET_PIN, A9G_INIT_PIN);
GPS_Controller GPS(Serial1);
GPRS_Controller GPRS(Serial2);

void mqtt_callback(char* topic, char* payload, int length) {
	Serial.printf("\r\nFrom MQTT subscription: topic: %s, payload: %s, length: %d\r\n\r\n", topic, payload, length);
	// GPRS.mqtt_unsubscribe(MQTT_SUB_TOPIC, MQTT_SUB_QOS);
}

void setup() {

	Serial.begin(115200);

	uint8_t step = 0;

	do {
		switch (step) {

			case 0: {
				while (!A9G.turn_on(MODEM_INIT_TIMEOUT_SECS)) {
					Serial.println("\r\nA9G init fail. Retrying...\r\n");
					A9G.turn_off();
				}
				Serial.println("\r\n");
				step++;
				break;
			}

			case 1: {
				while (!A9G.echo(true)) {
					Serial.println("\r\nA9G echo mode fail. Retrying...\r\n");
					step = 0;
					break;
				}
				if (step)
					step++;
				break;
			}

			case 2: {
				while (!GPRS.cellular_network_connect(NETWORK_APN)) {
					Serial.println("\r\nGPRS network connection fail. Retrying...\r\n");
					step = 0;
					break;
				}
				if (step) {
					Serial.printf("\r\n\r\nIMEI: %s\r\n", GPRS.get_imei());
					Serial.printf("\rSignal quality: %u%%\r\n", GPRS.signal_level(PERCENTAGE_LEVEL));
					step++;
				}
				break;
			}

			case 3: {
				while (!GPS.enable(GPS_REFRESH_INTERVAL_SECS)) {
					Serial.println("\rGPS communication fail. Retrying...\r\n");
					step = 0;
					break;
				}
				if (step)
					step++;
				break;
			}

			case 4: {
				while (!GPRS.mqtt_connect_broker(MQTT_BROKER_ADDR, MQTT_BROKER_PORT, MQTT_BROKER_AUTH_USER, MQTT_BROKER_AUTH_PASSWORD, MQTT_CLIENT_ID, MQTT_CLIENT_KEEPALIVE_SECS)) {
					Serial.println("\r\nMQTT connection fail. Retrying...\r\n");
					step = 0;
					break;
				}
				if (step)
					step++;
				break;
			}

			case 5: {
				while (!GPRS.mqtt_subscribe(MQTT_SUB_TOPIC, MQTT_SUB_QOS, mqtt_callback)) {
					Serial.println("\r\nMQTT subscription fail. Retrying...\r\n");
					step = 0;
					break;
				}
				if (step)
					step++;
				break;
			}

			case 6: {
				while (!GPRS.mqtt_publish(MQTT_PUB_TOPIC, MQTT_PUB_PAYLOAD, MQTT_PUB_QOS)) {
					Serial.println("\r\nMQTT publish fail. Retrying...\r\n");
					step = 0;
					break;
				}
				if (step)
					step++;
				break;
			}
		}
	} while (step <= 6);
}

void loop() {

	A9G.loop();
	GPRS.mqtt_loop();

	static uint32_t t0 = millis();

	if (millis() - t0 > 1000) {
		t0 = millis();

		static char JSON_payload[100];

		sprintf(JSON_payload, "{\"variable\":\"location\",\"value\":\"A9G\",\"location\":{\"lat\":%.8f,\"lng\":%.8f}}", GPS.location(LAT), GPS.location(LNG));

		GPRS.mqtt_publish((char*)"GPS", JSON_payload, MQTT_PUB_QOS);
	}
}