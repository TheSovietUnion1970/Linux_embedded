#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10 // CS pin for MCP2515 (adjust if different)
MCP_CAN CAN(CAN_CS_PIN);
int x = 0;

void setup() {
  Serial.begin(19200);
  while (!Serial); // Wait for Serial to initialize
  
  // Initialize MCP2515 at 500 kbps with 8 MHz clock
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN Receiver Initialized");
    CAN.setMode(MCP_NORMAL); // Set to normal mode
  } else {
    Serial.println("Error Initializing CAN...");
    while (1);
  }
}

void loop() {
  long unsigned int rxId;
  byte len = 0;
  byte rxBuf[8];
  
  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&rxId, &len, rxBuf); // Read message
    Serial.print("Received ID: 0x");
    Serial.print(rxId, HEX);
    Serial.print(" DLC: ");
    Serial.print(len);
    Serial.print(" Data: ");
    
    for (int i = 0; i < len; i++) {
      Serial.print(rxBuf[i], HEX);
      Serial.print(" ");
    }
    Serial.print(" x = ");
    Serial.println(x);

    x++;
    if (x == 255) x = 0;
  }

}
