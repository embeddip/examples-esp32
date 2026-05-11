#include "esp32-hal-psram.h"

uint8_t *testBuff;

void setup() {
  Serial.begin(115200);

  if (psramFound()) {
    Serial.println("PSRAM detected and ready for use.");
    testBuff = (uint8_t *)ps_malloc(320 * 240 * 2);
    if (testBuff != NULL) {
      Serial.println("Image buffer successfully allocated in PSRAM.");
    } else {
      Serial.println("Failed to allocate image buffer in PSRAM.");
    }
  } else {
    Serial.println("PSRAM not detected. Check board configuration.");
  }
}

void loop() {
  delay(100);
}
