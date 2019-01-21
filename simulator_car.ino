#include <Arduino.h>
#include <RH_RF95.h>
#include <stdint.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <USBAPI.h>
#include <Wire.h>
#include <WString.h>
#include <stddef.h>

#define SERIAL_CLI_ENABLED true

#define ENGINE_CONTROLLER_PIN 5
#define ENGINE_DIRECTION_PIN 4
#define ENGINE_TURN_PIN 6
#define ENGINE_DEFAULT_SPEED 50

#define CAMERA_X_PIN 8
#define CAMERA_Y_PIN 9

#define NETWORKING_RADIO_MAX_RETRY 1
#define NETWORKING_RADIO_READ_PIN 2
#define NETWORKING_RADIO_WRITE_PIN 3
#define NETWORKING_RADIO_FREQUENCY 434.0

#define LED_PIN 13

#define PROTOCOL_CAMERA_ID  10
#define PROTOCOL_ROTATION_ID  20
#define PROTOCOL_MOVING_ID  30

class Engine {
	private:
		byte speed = 0;
		byte motorControllerPin = 0;
		byte motorDirectionPin = 0;
		Servo servoTurn;
	public:
		Engine() {
			/*// http://www.instructables.com/id/Arduino-Motor-Shield-Tutorial/
			 Function				Channel A		Channel B
			 Direction			Digital 12		Digital 13
			 Speed (PWM)			Digital 3		Digital 11
			 Brake				Digital 9		Digital 8
			 Current Sensing		Analog 0		Analog 1
			 */
		}
		void initialize(byte motorPin, byte directionPin, byte turnPin, byte defaultSpeed) {
			Serial.println("[Engine] initializing...");

			this->speed = defaultSpeed;
			this->motorControllerPin = motorPin;
			this->motorDirectionPin = directionPin;

			pinMode(motorPin, OUTPUT);
			pinMode(directionPin, OUTPUT);
			pinMode(turnPin, OUTPUT);

			digitalWrite(this->motorControllerPin, 255);
			digitalWrite(this->motorControllerPin, 0);
			this->servoTurn.attach(turnPin);

			Serial.println("[Engine] initialized.");
		}
		void move(byte newDirection, byte newSpeed) {
			digitalWrite(this->motorDirectionPin, newDirection);
			analogWrite(this->motorControllerPin, newSpeed);
		}
		void turn(word angle) {
			this->servoTurn.write(angle);
		}
};

class CameraServo {
	private:
		Servo servoX;
		Servo servoY;
	public:
		CameraServo() {
		}
		void initialize(byte xPin, byte yPin) {
			Serial.println("[CameraServo] initializing...");

			this->servoX.attach(xPin);
			this->servoY.attach(yPin);

			Serial.println("[CameraServo] initialized.");
		}
		void moveX(word x) {
			this->servoX.write(x);
		}
		void moveY(word y) {
			this->servoY.write(y);
		}
		void move(word x, word y) {
			this->servoX.write(x);
			this->servoY.write(y);
			Serial.print(F("Moving camera to position x: "));
			Serial.print(x);
			Serial.print(F(", y: "));
			Serial.println(y);
		}
		void moveFromGyro() {
			//word xPosition = this->processRotationValue((word) abs(mx), 20, 120);
			//word yPosition = this->processRotationValue((word) abs(my), 80, 120);
			//this->move(xPosition, yPosition);
		}
		int processRotationValue(word value, word minAngle, word maxAngle) {
			value = 90 - ((word) (value / 20)) * 20;
			return constrain(value, minAngle, maxAngle);
		}
};

class RadioNetworking {
	private:
		SoftwareSerial radioSoftwareSerial;
		RH_RF95 rf95; //
		bool radioWorking = false;
	public:
		RadioNetworking(byte netReadPin, byte netWritePin) :
				radioSoftwareSerial(netReadPin, netWritePin), rf95(radioSoftwareSerial) {
		}
		void initialize() {
			Serial.println(F("Init LoRas RH_RF95 radio..."));
			int retry = 0;
			bool success = false;
			while (!success) {
				success = this->rf95.init();
				retry++;
				if (retry > NETWORKING_RADIO_MAX_RETRY || success) {
					break;
				}
				Serial.print(F("FAILED ("));
				Serial.print(retry);
				Serial.print(F("/"));
				Serial.print(NETWORKING_RADIO_MAX_RETRY);
				Serial.print(F("), "));
				delay(100);
			}

			this->radioWorking = success;
			if (success) {
				Serial.println(F("OK"));
			} else {
				Serial.println(F("GIVING UP"));
			}
		}
		void startServer(float frequency) {
			this->rf95.setFrequency(frequency);
			Serial.println(F("Listening..."));
		}
		void handle(Engine* engine, CameraServo* cameraServo) {
			if (rf95.available()) {
				uint8_t buffer[RH_RF95_MAX_MESSAGE_LEN];
				uint8_t length = sizeof(buffer);

				if (rf95.recv(buffer, &length)) {
					Serial.print(F("Received: "));
					Serial.println((char*) buffer);

					byte protocol = buffer[0];
					byte* request = (byte*) buffer;

					if (protocol == PROTOCOL_ROTATION_ID) { /* Wheel Rotation */
						byte angle = buffer[1];

						Serial.print(F("Angle="));
						Serial.println(angle);

						engine->turn(angle);
					} else if (protocol == PROTOCOL_MOVING_ID) { /* Moving */
						byte direction = buffer[1];
						byte speed = buffer[2];

						Serial.print(F("Moving: dir="));
						if (direction == 0) {
							Serial.print(F("FORWARD"));
						} else {
							Serial.print(F("B"));
						}
						Serial.print(F(", speed="));
						Serial.println(speed);

						engine->move(direction == 0 ? 0 : 1, speed);
					} else { // UNKNOWN
						Serial.print(F("Unknown: "));
						Serial.println(protocol);
					}
				} else {
					Serial.println(F("[RadioNetworking] [Server] Failed receiving packet"));
				}

				memset(buffer, 0, length);
			}
		}
		void sendPacket(uint8_t* data, bool wait) {
			Serial.print("[RadioNetworking] [Server] Sending: ");
			Serial.println((char*) data);
			this->rf95.send(data, sizeof(data));
			if (wait) {
				this->rf95.waitPacketSent();
			}
		}
		bool isRadioWorking() {
			return this->radioWorking;
		}
		RH_RF95 getRf95() {
			return this->rf95;
		}
};

Engine *engine;
CameraServo *cameraServo;
RadioNetworking *radioNetworking;

void setup() {
	Wire.begin();
	Serial.begin(115200);

	Serial.println("[Arduino] woke up!");
	Serial.println("[Arduino] [Setup] starting...");

	pinMode(13, OUTPUT);

	engine = new Engine();
	cameraServo = new CameraServo();
	radioNetworking = new RadioNetworking(NETWORKING_RADIO_READ_PIN, NETWORKING_RADIO_WRITE_PIN);

	engine->initialize(ENGINE_CONTROLLER_PIN, ENGINE_DIRECTION_PIN, ENGINE_TURN_PIN, ENGINE_DEFAULT_SPEED);
	cameraServo->initialize(CAMERA_X_PIN, CAMERA_Y_PIN);
	radioNetworking->initialize();
	radioNetworking->startServer(NETWORKING_RADIO_FREQUENCY);

	Serial.println("[Arduino] [Setup] finished.");
}

bool loopMessage = false, ledState = false;
word ledBlink = 0;
void loop() {
	if (!loopMessage) {
		Serial.println(F("[Arduino] [Thread] First loop called!"));

		if (SERIAL_CLI_ENABLED) {
			Serial.println(F("[Arduino][Debug] Command interface available!"));
		}

		/*
		 environment->updateMotion();
		 environment->debugGyro();
		 cameraServo->moveFromGyro();*/

		loopMessage = true;
	}

	if (ledBlink++ > 800) {
		ledState = !ledState;

		digitalWrite(13, ledState ? HIGH : LOW);
		//Serial.println(ledState);
		ledBlink = 0;
		Serial.print((byte) ledState);
	}

	radioNetworking->handle(engine, cameraServo);

	listenSerialInterface();
	//delay(300);
}

void listenSerialInterface() {
	if (!SERIAL_CLI_ENABLED) {
		return;
	}

	if (Serial.available() != 0) {
		String line = "";
		while (Serial.available() != 0) {
			char character = Serial.read();
			if (character == ';') {
				break;
			}
			line = line + character;
		}

		Serial.print(F("Command: "));
		Serial.println(line);
	}
}
