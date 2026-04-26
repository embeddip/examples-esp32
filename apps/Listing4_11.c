#include <Arduino.h>
#include <embedDIP.h>

serial_t *serial = &esp32_uart;

Image *rgbImg = NULL;
Image *rgb565Img = NULL;
Image *rgbRoundTripImg = NULL;
Image *grayOrig = NULL;
Image *grayRoundTrip = NULL;
Image *diffImg = NULL;

void setup() {
  serial->init();

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &rgbImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &rgb565Img);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &rgbRoundTripImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &grayOrig);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &grayRoundTrip);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &diffImg);

  serial->capture(rgbImg);

  cvtColor(rgbImg, rgb565Img, CVT_RGB888_TO_RGB565);
  cvtColor(rgb565Img, rgbRoundTripImg, CVT_RGB565_TO_RGB888);

  cvtColor(rgbImg, grayOrig, CVT_RGB888_TO_GRAYSCALE);
  cvtColor(rgbRoundTripImg, grayRoundTrip, CVT_RGB888_TO_GRAYSCALE);

  difference(grayOrig, grayRoundTrip, diffImg);
  convertTo(diffImg);
  serial->send(diffImg);
}

void loop() {
  delay(10);
}
