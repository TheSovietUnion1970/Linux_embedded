#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10 // CS pin for MCP2515 (adjust if different)
MCP_CAN CAN(CAN_CS_PIN);

// Declare data array globally to persist across loops
byte data[8] = {0x00, 0x11, 0x22, 0x33, 0x00, 0x11, 0x22, 0x33}; // Initial data
byte len = 8; // Data length
unsigned long id = 0x156; // CAN ID

void setup() {
  Serial.begin(19200);
  while (!Serial); // Wait for Serial to initialize
  
  // Initialize MCP2515 at 500 kbps with 8 MHz clock
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN Transmitter Initialized");
    CAN.setMode(MCP_NORMAL); // Set to normal mode
  } else {
    Serial.println("Error Initializing CAN...");
    while (1);
  }
}

void loop() {
  if (CAN.sendMsgBuf(id, 0, len, data) == CAN_OK) {
    Serial.print("Message Sent: ID=0x");
    Serial.print(id, HEX);
    Serial.print(", Data=");
    for (int i = 0; i < len; i++) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
      if (data[i] == 0x44) data[i] = 0x00; // Reset if reaches 0x44
      data[i]++; // Increment after printing
    }
    Serial.println();
  } else {
    Serial.println("Error Sending Message");
  }

  delay(1000); // Send every 1 second
}
