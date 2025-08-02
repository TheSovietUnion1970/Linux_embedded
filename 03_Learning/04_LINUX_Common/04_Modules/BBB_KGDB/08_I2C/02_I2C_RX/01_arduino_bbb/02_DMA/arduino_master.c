#include <Wire.h>

const int slaveAddress = 0x40;  // I2C slave address
String inputMessage = "";
bool messageReady = false;
bool lengthSent = false;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000;  // 2 seconds
int track = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);  // Wait for Serial connection on some boards

  Wire.begin();        // I2C master
  Wire.setClock(100000);  // 100 kHz

  Serial.println("Type a message and press Enter.");
}

void loop() {
  // Read input from Serial terminal
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      messageReady = true;
      inputMessage += c;
      break;
    } else if (c != '\r') {  // Ignore carriage return
      inputMessage += c;
    }
  }

  if (messageReady && !lengthSent) {
    // Send message length
    int msgLen = inputMessage.length();
    if (msgLen > 32) msgLen = 32;  // Limit to 32 bytes

    Wire.beginTransmission(slaveAddress);
    Wire.write((byte)msgLen);  // Send the length as one byte
    byte err = Wire.endTransmission();

    if (err == 0) {
      Serial.print("Sent length ");
      Serial.print(msgLen);
      Serial.println(": OK");
      lengthSent = true;
      lastSendTime = millis();  // Start the 2-second timer
    } else {
      Serial.print("I2C error sending length: ");
      Serial.println(err);
    }
  }

  // After 2s, send the actual message
  if (messageReady && lengthSent && millis() - lastSendTime >= sendInterval) {
    int len = min(inputMessage.length(), 32);

    Wire.beginTransmission(slaveAddress);
    for (int i = 0; i < len; ++i) {
      Wire.write(inputMessage[i]);
    }
    byte err = Wire.endTransmission();

    if (err == 0) {
      Serial.print("Sent message #");
      Serial.print(track++);
      Serial.println(": OK");
    } else {
      Serial.print("I2C error sending message: ");
      Serial.println(err);
    }

    // Reset
    inputMessage = "";
    messageReady = false;
    lengthSent = false;
  }
}

