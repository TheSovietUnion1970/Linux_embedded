#include <Wire.h>

const int slaveAddress = 0x41;  // I2C slave address
String inputMessage = "";
bool messageReady = false;
int track = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);  // Wait for Serial connection on some boards (e.g., Leonardo)
  
  Wire.begin();
  Wire.setClock(100000);  // 100 kHz

  Serial.println("Type a message and press Enter to send via I2C:");
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

  // If a full message is ready, send via I2C
  if (messageReady) {
    Wire.beginTransmission(slaveAddress);

    // Only send up to 32 bytes due to Wire buffer limit
    int len = min(inputMessage.length(), 32);
    for (int i = 0; i < len; ++i) {
      Wire.write(inputMessage[i]);
    }

    byte err = Wire.endTransmission();
    if (err == 0) {
      Serial.print("Sent message #");
      Serial.print(track);
      Serial.println(": OK");
    } else {
      Serial.print("I2C error (");
      Serial.print(track);
      Serial.print("): ");
      Serial.println(err);
    }

    track++;
    inputMessage = "";
    messageReady = false;
  }
}

