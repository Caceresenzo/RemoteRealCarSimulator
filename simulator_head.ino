#include <Arduino.h>
#include <HardwareSerial.h>
#include <RH_RF95.h>
#include <stdint.h>
#include <SoftwareSerial.h>
#include <WString.h>

#define GYRO_DEBUG false

SoftwareSerial ss(5, 6);
RH_RF95 rf95(ss);

int led = 13;

#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"

MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t x, y, z;

bool blinkState = false;

void setup() {
	/*
	 * Arduino Init
	 */
	Wire.begin();
	Serial.begin(115200);
	pinMode(led, OUTPUT);

	/*
	 * Gyro & Network
	 */
	Serial.println("Initializing I2C devices...");
	accelgyro.initialize();

	Serial.println("Testing device connections...");
	Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

	if (!rf95.init()) {
		Serial.println("init failed");
		while (1) {
			;
		}
	} else {
		Serial.println("init ok");
	}

	rf95.setFrequency(434.0);
}

void loop() {
	listenSerialInterface();
	accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &x, &y, &z);

	if (GYRO_DEBUG) {
		Serial.print("a/g:\t");
		Serial.print(x);
		Serial.print("\t");
		Serial.print(y);
		Serial.print("\t");
		Serial.println(z);
	}

	uint8_t buffer[RH_RF95_MAX_MESSAGE_LEN];
	uint8_t length = sizeof(buffer);

	if (rf95.waitAvailableTimeout(4000)) {
		if (rf95.recv(buffer, &length)) {
			Serial.print("[RadioNetworking] [Client] Received: ");
			Serial.println((char*) buffer);
		} else {
			Serial.println("[RadioNetworking] [Server] Failed receiving packet");
		}
	} else {
		Serial.println("[RadioNetworking] [Client] No reply, is rf95_server running?");
	}
}

void listenSerialInterface() {
	while (true) {
		if (Serial.available() != 0) {
			break;
		}
	}
	String line = "";
	while (Serial.available() != 0) {
		char charectere = Serial.read();
		if (charectere == ';') {
			break;
		}
		line = line + charectere;
	}

	if (line == "c") {
		uint8_t data[] = "c:x____,y____*******";

		String xString = String(x);
		String yString = String(y);

		for (int i = 0; i < min(4, xString.length()); i++) {
			data[3 + i] = (char) xString[i];
		}
		for (int i = 0; i < min(4, yString.length()); i++) {
			data[9 + i] = (char) yString[i];
		}

		rf95.send(data, sizeof(data));
		rf95.waitPacketSent();

		Serial.print("[RadioNetworking] [Client] Camera protocol with data: x=");
		Serial.print(xString);
		Serial.print(", y=");
		Serial.println(yString);
		Serial.print("[RadioNetworking] [Client] Sending: ");
		Serial.println((char*) data);
	} else if (line[0] == 'r') {
		uint8_t data[] = "r:a____*************";

		String angleString = String(abs(z));

		for (int i = 0; i < min(4, angleString.length()); i++) {
			data[3 + i] = (char) angleString[i];
		}

		rf95.send(data, sizeof(data));
		rf95.waitPacketSent();

		Serial.print("[RadioNetworking] [Client] Rotation protocol with data: angle=");
		Serial.println(angleString);
		Serial.print("[RadioNetworking] [Client] Sending: ");
		Serial.println((char*) data);
	} else if (line[0] == 'a') {
		uint8_t data[] = "a:d_,s____**********"; //a,1,255

		//String speedString = String(z);

		String speedString = "255";

		data[3] = line[2];

		for (int i = 0; i < min(4, speedString.length()); i++) {
			//data[6 + i] = (char) speedString[i];
			data[6 + i] = (char) line[4 + i];
		}

		rf95.send(data, sizeof(data));
		rf95.waitPacketSent();

		Serial.print("[RadioNetworking] [Client] (WRONG) Action protocol with data: direction=1, speed=");
		Serial.println(speedString);
		Serial.print("[RadioNetworking] [Client] Sending: ");
		Serial.println((char*) data);
	} else {
		uint8_t data[] = "************";

		data[0] = line[0];
		rf95.send(data, sizeof(data));
		rf95.waitPacketSent();

		Serial.println("[RadioNetworking] [Client] Unknown protocol with data: no data");
		Serial.print("[RadioNetworking] [Client] Sending: ");
		Serial.println((char*) data);
	}

}
