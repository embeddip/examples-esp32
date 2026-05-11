#include <Arduino.h>
#include <embedDIP.hpp>
#include <vector>

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Camera camera(&esp32_ov2640);

embedDIP::Image previewRgb;
embedDIP::Image grayQvga;
embedDIP::Image work128;
embedDIP::Image tmp128;
embedDIP::Image out128;
embedDIP::Image fftImg;
embedDIP::Image filterImg;
embedDIP::Image outQvga;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static const std::vector<std::vector<float>> sharpenKernel = {
    {0, -1, 0}, {-1, 4, -1}, {0, -1, 0}};

static void apply_current_mode() {
  camera.stop();

  previewRgb.cvtColor(grayQvga, CVT_RGB565_TO_GRAYSCALE);
  grayQvga.resize(work128, 128, 128);
  work128.convertTo();

  switch (current_mode) {
  case 0: // Spatial denoise.
    work128.histEq(tmp128);
    tmp128.medianFilter(out128, 3);
    out128.convertTo();
    break;

  case 1: // Spatial sharpen.
    work128.filter2D(sharpenKernel, tmp128);
    work128.add(tmp128, out128);
    out128.convertTo();
    break;

  case 2: // FFT Gaussian low-pass.
    work128.fft(fftImg);
    fftImg.fftshift();
    filterImg.getFilter(FREQ_FILTER_GAUSSIAN_LOWPASS, 30.0f, 0.0f);
    fftImg.ffilter2D(filterImg, out128);
    out128.fftshift();
    out128.ifft(out128);
    out128.convertTo();
    break;

  case 3: // FFT Gaussian high-pass.
    work128.fft(fftImg);
    fftImg.fftshift();
    filterImg.getFilter(FREQ_FILTER_GAUSSIAN_HIGHPASS, 25.0f, 0.0f);
    fftImg.ffilter2D(filterImg, out128);
    out128.fftshift();
    out128.ifft(out128);
    out128.convertTo();
    break;

  case 4: // Highboost: original - LPF(original).
    work128.fft(fftImg);
    fftImg.fftshift();
    filterImg.getFilter(FREQ_FILTER_GAUSSIAN_LOWPASS, 20.0f, 0.0f);
    fftImg.ffilter2D(filterImg, tmp128);
    tmp128.fftshift();
    tmp128.ifft(tmp128);
    work128.difference(tmp128, out128);
    out128.convertTo();
    break;

  default:
    break;
  }

  out128.resize(outQvga, outQvga.width(), outQvga.height());
  outQvga.convertTo();
  serial.send(outQvga);

  camera.capture(CONTINUOUS, previewRgb);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial.init();
  camera.init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);

  previewRgb = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
  grayQvga = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  work128 = embedDIP::Image(128, 128, IMAGE_FORMAT_GRAYSCALE);
  tmp128 = embedDIP::Image(128, 128, IMAGE_FORMAT_GRAYSCALE);
  out128 = embedDIP::Image(128, 128, IMAGE_FORMAT_GRAYSCALE);
  fftImg = embedDIP::Image(128, 128, IMAGE_FORMAT_GRAYSCALE);
  filterImg = embedDIP::Image(128, 128, IMAGE_FORMAT_GRAYSCALE);
  outQvga = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastBtn) {
    lastChangeMs = millis();
    lastBtn = reading;
  }

  if ((millis() - lastChangeMs) > debounceMs) {
    if (!pressed && reading == LOW) {
      pressed = true;
      tDown = millis();
    }
    if (pressed && reading == HIGH) {
      unsigned long held = millis() - tDown;
      pressed = false;

      if (held >= longPressMs) {
        camera.capture(SINGLE, previewRgb);
        apply_current_mode();
      } else {
        current_mode = (current_mode + 1) % 5;
      }
    }
  }

  delay(5);
}
