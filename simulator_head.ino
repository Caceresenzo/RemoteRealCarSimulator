#include <Arduino.h>
#include <HardwareSerial.h>
#include <RH_RF95.h>
#include <stdint.h>
#include <SoftwareSerial.h>
#include <WString.h>

#define SERIAL_CLI_ENABLED true

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

bool gyroWorking = false, radioWorking = false;

void setup() {
	/* Arduino Init */
	Wire.begin();
	Serial.begin(115200);

	/* Pins */
	pinMode(led, OUTPUT);

	/* I2C */
	Serial.println(F("[Environment] [I2C] Initializing devices..."));

	/* Gyro */
	Serial.print(F("[Environment] [MPU6050] Testing device connections... "));
	accelgyro.initialize();
	gyroWorking = accelgyro.testConnection();
	Serial.println(gyroWorking ? "READY" : "FAILED");

	/* Network */
	Serial.println(F("[RadioNetworking] initializing..."));
	Serial.println(F("[RadioNetworking] [Client] initializing LoRas RH_RF95 radio..."));
	radioWorking = rf95.init();
	if (radioWorking) {
		Serial.print(F("[RadioNetworking] [Client] READY"));

		rf95.setFrequency(434.0);
	} else {
		Serial.print(F("[RadioNetworking] [Client] FAILED"));
	}
}

bool loopMessage = false;
void loop() {
	if (!loopMessage) {
		Serial.println(F("[Arduino] [Thread] First loop called!"));

		if (SERIAL_CLI_ENABLED) {
			Serial.println(F("[Arduino] [Debug] Command interface available!"));
		}

		updateGyro();

		loopMessage = true;
	}

	listenSerialInterface();
}
byte targetSpeed = 255;
void listenSerialInterface() {
	while (true) {
		if (Serial.available() != 0) {
			break;
		}
	}

	String line = "";
	while (Serial.available() != 0) {
		char character = Serial.read();

		Serial.println(character);

		if (character == ';' || character == 0) {
			break;
		}

		line = line + character;
	}

	Serial.println(line);

	byte oldSpeed = targetSpeed;
	if (line == "a") {
		targetSpeed = 150;
	} else if (line == "b") {
		targetSpeed = 200;
	} else if (line == "c") {
		targetSpeed = 255;
	}

	if (oldSpeed != targetSpeed) {
		Serial.print("[RadioNetworking] [Client] [Car] New speed saved speed is: ");
		Serial.println(targetSpeed);
		return;
	}

	if (line == "up") {
		sendProtocol('a', '1', targetSpeed);
	} else if (line == "down") {
		sendProtocol('a', '0', targetSpeed);
	} else if (line == "left") {
		sendProtocol('r', 50, 0);
	} else if (line == "right") {
		sendProtocol('r', 90, 0);
	} else if (line == "center") {
		sendProtocol('a', '1', 0);
	}

	/* Other */
	else if (line == "downright") {
		sendProtocol('c', 0, 0);
	}

	/* Unkown */
	else {
		Serial.print("[RadioNetworking] [Client] Unknown command: ");
		Serial.println(line);
	}
}

void sendProtocol(char protocol, byte value1, byte value2) {
	if (protocol == 'c') {
		uint8_t data[] = "c:x____,y____*******";

		updateGyro();

		String xString = String(x);
		String yString = String(y);

		for (byte i = 0; i < min(4, xString.length()); i++) {
			data[3 + i] = (char) xString[i];
		}
		for (byte i = 0; i < min(4, yString.length()); i++) {
			data[9 + i] = (char) yString[i];
		}

		Serial.print("[RadioNetworking] [Client] Camera protocol with data: x=");
		Serial.print(xString);
		Serial.print(", y=");
		Serial.println(yString);

		sendRawData(data);
	} else if (protocol == 'r') {
		uint8_t data[] = "r:a____*************"; //r,angle

		String angleString = String(value1);

		for (byte i = 0; i < min(4, angleString.length()); i++) {
			data[3 + i] = (char) angleString[i];
			//data[3 + i] = (char) line[2 + i];
		}

		Serial.print("[RadioNetworking] [Client] Rotation protocol with data: angle=");
		Serial.println(angleString);

		sendRawData(data);
	} else if (protocol == 'a') {
		uint8_t data[] = "a:d_,s____**********"; //a,1,255

		//String speedString = String(z);

		String speedString = String(value2);

		// data[3] = line[2];
		data[3] = value1;

		for (byte i = 0; i < min(4, speedString.length()); i++) {
			data[6 + i] = (char) speedString[i];
			// data[6 + i] = (char) line[4 + i];
		}

		Serial.print("[RadioNetworking] [Client] Action protocol with data: direction=");
		Serial.print(value1);
		Serial.print(", speed=");
		Serial.println(speedString);

		sendRawData(data);
	} else {
		uint8_t data[] = "************";

		data[0] = protocol;

		Serial.println("[RadioNetworking] [Client] Unknown protocol");

		sendRawData(data);
	}
}

void sendRawData(uint8_t rawData[]) {
	if (radioWorking) {
		Serial.print("[RadioNetworking] [Client] Sending: ");
		Serial.println((char*) rawData);

		rf95.send(rawData, sizeof(rawData));
		rf95.waitPacketSent();

		waitDataResponse();
	} else {
		Serial.print(F("[RadioNetworking] [Client] Can't send raw data: "));
		Serial.println((char*) rawData);
	}
}

void waitDataResponse() {
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

void updateGyro() {
	if (!gyroWorking) {
		return;
	}

	accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &x, &y, &z);

	if (GYRO_DEBUG) {
		Serial.print("a/g:\t");
		Serial.print(x);
		Serial.print("\t");
		Serial.print(y);
		Serial.print("\t");
		Serial.println(z);
	}
}
