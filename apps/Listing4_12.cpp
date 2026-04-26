#include <Arduino.h>
#include <embedDIP.hpp>

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image rgbImg;
embedDIP::Image rgb565Img;
embedDIP::Image rgbRoundTripImg;
embedDIP::Image grayOrig;
embedDIP::Image grayRoundTrip;
embedDIP::Image diffImg;

void setup() {
  serial.init();

  rgbImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
  rgb565Img = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
  rgbRoundTripImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
  grayOrig = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  grayRoundTrip = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  diffImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  serial.capture(rgbImg);

  rgbImg.cvtColor(rgb565Img, CVT_RGB888_TO_RGB565);
  rgb565Img.cvtColor(rgbRoundTripImg, CVT_RGB565_TO_RGB888);

  rgbImg.cvtColor(grayOrig, CVT_RGB888_TO_GRAYSCALE);
  rgbRoundTripImg.cvtColor(grayRoundTrip, CVT_RGB888_TO_GRAYSCALE);
  grayOrig.difference(grayRoundTrip, diffImg);

  diffImg.convertTo();
  serial.send(diffImg);
}

void loop() {
  delay(10);
}
