#include "BluetoothSerial.h"
#include "ELMduino.h"

// FORWARD_GEAR: 3 = D, 4 = S

// Constants
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial
#define RELAY_PIN 27
#define BT_PIN "1234"
#define SPEED_THRESHOLD 15
#define FORWARD_GEAR 3
#define UPDATE_INTERVAL 100  // milliseconds

// Bluetooth address
uint8_t address[6] = { 0x13, 0xE0, 0x2F, 0x8D, 0x54, 0x21 };

// Command strings stored in flash memory to save RAM
const char CMD_CAMERA[] PROGMEM = "22 18 45";
const char CMD_GEAR[] PROGMEM = "22 38 15";
const char CMD_SPEED[] PROGMEM = "01 0D";
const char HEADER_CAMERA[] PROGMEM = "ATSH 773";
const char HEADER_GEAR[] PROGMEM = "ATSH 7E1";
const char HEADER_SPEED[] PROGMEM = "ATSH 7E0";

BluetoothSerial SerialBT;
ELM327 myELM327;

// State variables
int8_t speed = -1;
int8_t gear = -1;
int8_t camera = -1;
unsigned long lastUpdateTime = 0;

int getData(const char *header, const char *pid, bool filtering = false) {
  if (filtering) {
    myELM327.sendCommand_Blocking("AT CRA 7DD");
  } else {
    myELM327.sendCommand_Blocking("AT CRA");
  }

  myELM327.sendCommand_Blocking(header);
  myELM327.sendCommand_Blocking(pid);

  if (!myELM327.get_response()) {
    return -1;  // Error handling
  }

  return filtering ? ((long long)strtoll(myELM327.payload, NULL, 16) & 0xFF) : ((int)strtol(myELM327.payload, NULL, 16) & 0xFF);
}

bool initializeELM() {
  if (!ELM_PORT.connect(address)) {
    DEBUG_PORT.println(F("Couldn't connect to ELM327 device."));
    return false;
  }

  if (!myELM327.begin(ELM_PORT, false, 2000, ISO_15765_11_BIT_500_KBAUD)) {
    DEBUG_PORT.println(F("ELM327 device couldn't connect to ECU."));
    return false;
  }

  DEBUG_PORT.println(F("Connected to ELM327"));
  return true;
}

void setup() {
  DEBUG_PORT.begin(115200);
  // SerialBT.setPin(BT_PIN);
  ELM_PORT.begin("ArduHUD", true);
  pinMode(RELAY_PIN, OUTPUT);

  while (!initializeELM()) {
    delay(1000);  // Wait before retrying
  }
}

void loop() {
  unsigned long currentTime = millis();

  // Only update at specified interval
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    camera = getData(HEADER_CAMERA, CMD_CAMERA, true);
    gear = getData(HEADER_GEAR, CMD_GEAR);
    speed = getData(HEADER_SPEED, CMD_SPEED);

    // Update relay state
    digitalWrite(RELAY_PIN, (camera == 1 && speed <= SPEED_THRESHOLD && gear >= FORWARD_GEAR) ? HIGH : LOW);

    lastUpdateTime = currentTime;
  }
}
