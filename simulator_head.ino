#include <Arduino.h>
#include <HardwareSerial.h>
#include <RH_RF95.h>
#include <stdint.h>
#include <SoftwareSerial.h>
#include <WString.h>
#include <Servo.h>

#include "Wire.h"
#include "I2Cdev.h"
//#include "MPU6050.h"

#define SERIAL_CLI_ENABLED true

//#define GYRO_DEBUG true

SoftwareSerial ss(2, 3);
RH_RF95 rf95(ss);

int led = 13;

//MPU6050 accelgyro;

/*int16_t ax, ay, az;
 int16_t gx, gy, gz;
 int16_t x, y, z;*/

bool /* gyroWorking = false,*/radioWorking = false;

void setup() {
	/* Arduino Init */
	Wire.begin();
	Serial.begin(115200);

	/* Pins */
	pinMode(led, OUTPUT);

	/* I2C */
	Serial.println(F("[Environment] [I2C] Initializing devices..."));

	/* Gyro
	 Serial.print(F("[Environment] [MPU6050] Testing device connections... "));
	 accelgyro.initialize();
	 gyroWorking = accelgyro.testConnection();
	 Serial.println(gyroWorking ? "READY" : "FAILED");*/

	/* Network */
	Serial.println(F("[RadioNetworking] initializing..."));
	Serial.println(F("[RadioNetworking] [Client] initializing LoRas RH_RF95 radio..."));
	radioWorking = rf95.init();
	if (radioWorking) {
		Serial.println(F("[RadioNetworking] [Client] READY"));

		rf95.setFrequency(434.0);
		//rf95.setTxPower(23, true);
	} else {
		Serial.println(F("[RadioNetworking] [Client] FAILED"));
	}
}

bool blinkState = false;
bool loopMessage = false;
word ledBlink = 0;
void loop() {
	if (!loopMessage) {
		Serial.println(F("[Arduino] [Thread] First loop called!"));

		if (SERIAL_CLI_ENABLED) {
			Serial.println(F("[Arduino] [Debug] Command interface available!"));
		}

		//updateGyro();

		loopMessage = true;
	}

	blinkState = !blinkState;
	digitalWrite(led, blinkState ? HIGH : LOW);

	//updateGyro();

	//  sendProtocol('c', 0, 0);
	listenSerialInterface();

	delay(20);
}
byte targetSpeed = 255;
void listenSerialInterface() {
	byte updater = 0;

	/*while (true) { */
	if (Serial.available() != 0) {
		String line = "";

		while (Serial.available() != 0) {
			char character = Serial.read();

			Serial.println(character);

			if (character == ';' || character == 0) {
				break;
			}

			line = line + character;
		}

		//break;

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
			sendProtocol('a', 1, targetSpeed);
		} else if (line == "down") {
			sendProtocol('a', 0, targetSpeed);
		} else if (line == "left") {
			sendProtocol('r', 120, 0);
		} else if (line == "right") {
			sendProtocol('r', 60, 0);
		} else if (line == "center") {
			sendProtocol('a', 1, 0);
		} else if (line[0] == 'r' && line[1] == 'o' && line[2] == 't' && line[3] == 'a') { /* Rotation */

			Serial.print(F("Rotation: raw=\""));
			Serial.print(line);
			Serial.print(F("\" -- removed: \""));
			line.remove(0, 8);
			Serial.print(line);
			Serial.print(F("\" value: "));
			byte rotationValue = 180 - line.toInt();
			Serial.println(rotationValue);

			sendProtocol('r', rotationValue, 0);
		}

		/* Other */
		else if (line == "downright") {
			sendProtocol('c', 0, 0);
		} else if (line == "upright") {
			sendProtocol('r', 90, 0);
		}

		/* Unkown */
		else {
			Serial.print(F("[RadioNetworking] [Client] Unknown command: "));
			Serial.println(line);
		}
	}

	/*if (updater++ == 255) {
	 line = "downright"; // camera
	 updater = 0;
	 break;
	 }

	 delay(11);
	 }*/

}

void sendProtocol(char protocol, byte value1, byte value2) {
	if (protocol == 'c') {
		uint8_t data[3];

		//updateGyro();
		/*
		 data[0] = 1;
		 data[1] = x;
		 data[2] = y;

		 Serial.print("[RadioNetworking] [Client] Camera protocol with data: x=");
		 Serial.print(x);
		 Serial.print(", y=");
		 Serial.println(y);

		 //sendRawData(data);

		 Serial.print("[RadioNetworking] [Client] Sending: ");

		 for (byte i = 0; i < 3; i++) {
		 Serial.print((byte) i);
		 Serial.print(F("="));
		 Serial.print((byte) data[i]);
		 Serial.print(F(" "));
		 }
		 Serial.println();

		 rf95.send(data, sizeof(data));
		 //rf95.waitPacketSent(); */
	} else if (protocol == 'r') {
		uint8_t data[2];

		data[0] = 20;
		data[1] = value1;

		Serial.print(F("[RadioNetworking] [Client] Rotation protocol with data: angle="));
		Serial.println(value1);

		//sendRawData(data);

		Serial.print("[RadioNetworking] [Client] Sending: ");

		for (byte i = 0; i < 2; i++) {
			Serial.print((byte) i);
			Serial.print(F("="));
			Serial.print((byte) data[i]);
			Serial.print(F(" "));
		}
		Serial.println();

		rf95.send(data, sizeof(data));
		//rf95.waitPacketSent();
	} else if (protocol == 'a') {
		uint8_t data[3];

		data[0] = 30;
		data[1] = value1; /* Direction */
		data[2] = value2; /* Speed */

		Serial.print(F("[RadioNetworking] [Client] Action protocol with data: direction="));
		Serial.print((char) value1);
		Serial.print(", speed=");
		Serial.println(value2);

		//sendRawData(data);

		Serial.print(F("[RadioNetworking] [Client] Sending: "));

		for (byte i = 0; i < 3; i++) {
			Serial.print((byte) i);
			Serial.print(F("="));
			Serial.print((byte) data[i]);
			Serial.print(F(" "));
		}
		Serial.println();

		rf95.send(data, sizeof(data));
		//rf95.waitPacketSent();
	} else {
		uint8_t data[1];

		data[0] = protocol;

		Serial.println(F("[RadioNetworking] [Client] Unknown protocol"));

		//sendRawData(data);
	}
}

/*
 void sendRawData(uint8_t rawData[]) {
 if (radioWorking) {
 Serial.print("[RadioNetworking] [Client] Sending: ");
 Serial.println((char*) rawData);

 rf95.send(rawData, sizeof(rawData));
 rf95.waitPacketSent();

 //waitDataResponse();
 }
 else {
 Serial.print(F("[RadioNetworking] [Client] Can't send raw data: "));
 Serial.println((char*) rawData);
 }
 }
 */

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

/*void updateGyro() {
 if (!gyroWorking) {
 return;
 }

 accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &x, &y, &z);

 if (GYRO_DEBUG) {
 Serial.print("a/g:\t");
 Serial.print(ax);
 Serial.print("\t");
 Serial.print(ay);
 Serial.print("\t");
 Serial.print(az);
 Serial.print("\t");
 Serial.print(gx);
 Serial.print("\t");
 Serial.print(gy);
 Serial.print("\t");
 Serial.print(gz);
 Serial.print("\t");
 Serial.print(x);
 Serial.print("\t");
 Serial.print(y);
 Serial.print("\t");
 Serial.println(z);
 }
 }*/
