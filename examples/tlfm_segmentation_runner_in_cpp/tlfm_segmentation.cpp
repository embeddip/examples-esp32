#include "tlfm_segmentation_runner.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <cstdint>
#include <cstring>
#include <new>

namespace {

constexpr int kResolverOpCount = 21;

struct InferenceContext {
  const tflite::Model *model = nullptr;
  tflite::MicroInterpreter *interpreter = nullptr;
  TfLiteTensor *input = nullptr;
  TfLiteTensor *output = nullptr;
};

InferenceContext g_ctx;
tflite::MicroMutableOpResolver<kResolverOpCount> g_resolver;
bool g_resolver_initialized = false;
alignas(tflite::MicroInterpreter)
    uint8_t g_interpreter_buffer[sizeof(tflite::MicroInterpreter)] = {};

void AddSegmentationOps(
    tflite::MicroMutableOpResolver<kResolverOpCount> &resolver) {
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddTransposeConv();
  resolver.AddReshape();
  resolver.AddSoftmax();
  resolver.AddMean();
  resolver.AddFullyConnected();
  resolver.AddPad();
  resolver.AddRelu6();
  resolver.AddLogistic();
  resolver.AddConcatenation();
  resolver.AddMaxPool2D();
  resolver.AddRelu();
  resolver.AddAdd();
  resolver.AddMul();
  resolver.AddQuantize();
  resolver.AddDequantize();
  resolver.AddResizeBilinear();
  resolver.AddShape();
  resolver.AddStridedSlice();
  resolver.AddPack();
}

int TensorHwc(const TfLiteTensor *tensor, int *h, int *w, int *c) {
  if (!tensor || !tensor->dims || tensor->dims->size < 3) {
    return -1;
  }

  const TfLiteIntArray *dims = tensor->dims;
  const int n = dims->size;
  *h = dims->data[n - 3];
  *w = dims->data[n - 2];
  *c = dims->data[n - 1];
  return 0;
}

TfLiteTensor *LargestSpatialOutput(tflite::MicroInterpreter *interpreter) {
  if (!interpreter || interpreter->outputs_size() <= 0) {
    return nullptr;
  }

  TfLiteTensor *best = interpreter->output(0);
  int best_score = -1;

  for (int i = 0; i < interpreter->outputs_size(); ++i) {
    TfLiteTensor *candidate = interpreter->output(i);
    int h = 0;
    int w = 0;
    int c = 0;
    int score = 0;

    if (TensorHwc(candidate, &h, &w, &c) == 0) {
      score = h * w * c;
    } else {
      score = static_cast<int>(candidate->bytes);
    }

    if (score > best_score) {
      best_score = score;
      best = candidate;
    }
  }

  return best;
}

int FillInputGray(const uint8_t *pixels, TfLiteTensor *input) {
  if (!pixels || !input) {
    return -1;
  }

  int h = 0;
  int w = 0;
  int c = 0;
  if (TensorHwc(input, &h, &w, &c) != 0 || h <= 0 || w <= 0 || c <= 0) {
    return -1;
  }

  const int spatial = h * w;
  const int n =
      (spatial < TLFM_SEG_IMAGE_PIXELS) ? spatial : TLFM_SEG_IMAGE_PIXELS;

  if (input->type == kTfLiteFloat32) {
    float *dst = input->data.f;
    for (int i = 0; i < n; ++i) {
      const float v = static_cast<float>(pixels[i]) / 255.0f;
      for (int ch = 0; ch < c; ++ch) {
        dst[i * c + ch] = v;
      }
    }
    return 0;
  }

  if (input->type == kTfLiteUInt8) {
    const int copy_n = (static_cast<int>(input->bytes) < n)
                           ? static_cast<int>(input->bytes)
                           : n;
    memcpy(input->data.uint8, pixels, static_cast<size_t>(copy_n));
    return 0;
  }

  if (input->type == kTfLiteInt8) {
    int8_t *dst = input->data.int8;
    for (int i = 0; i < n; ++i) {
      const int8_t v =
          static_cast<int8_t>(static_cast<int>(pixels[i]) - 128);
      for (int ch = 0; ch < c; ++ch) {
        dst[i * c + ch] = v;
      }
    }
    return 0;
  }

  return -1;
}

uint8_t ScaleClassToMask(int class_id, int class_count) {
  if (class_count <= 1) {
    return class_id ? 255 : 0;
  }
  return static_cast<uint8_t>((class_id * 255) / (class_count - 1));
}

int DecodeToMask(const TfLiteTensor *output, uint8_t *mask) {
  if (!output || !mask) {
    return -1;
  }

  int h = 0;
  int w = 0;
  int c = 0;
  if (TensorHwc(output, &h, &w, &c) != 0 || h <= 0 || w <= 0 || c <= 0) {
    return -1;
  }

  const int spatial = h * w;
  const int n =
      (spatial < TLFM_SEG_IMAGE_PIXELS) ? spatial : TLFM_SEG_IMAGE_PIXELS;

  if (output->type == kTfLiteFloat32) {
    const float *src = output->data.f;
    for (int i = 0; i < n; ++i) {
      if (c == 1) {
        mask[i] = (src[i] > 0.5f) ? 255 : 0;
      } else {
        int best_idx = 0;
        float best_val = src[i * c];
        for (int ch = 1; ch < c; ++ch) {
          const float v = src[i * c + ch];
          if (v > best_val) {
            best_val = v;
            best_idx = ch;
          }
        }
        mask[i] = ScaleClassToMask(best_idx, c);
      }
    }
  } else if (output->type == kTfLiteUInt8) {
    const uint8_t *src = output->data.uint8;
    for (int i = 0; i < n; ++i) {
      if (c == 1) {
        mask[i] = src[i];
      } else {
        int best_idx = 0;
        uint8_t best_val = src[i * c];
        for (int ch = 1; ch < c; ++ch) {
          const uint8_t v = src[i * c + ch];
          if (v > best_val) {
            best_val = v;
            best_idx = ch;
          }
        }
        mask[i] = ScaleClassToMask(best_idx, c);
      }
    }
  } else if (output->type == kTfLiteInt8) {
    const int8_t *src = output->data.int8;
    for (int i = 0; i < n; ++i) {
      if (c == 1) {
        mask[i] = static_cast<uint8_t>(static_cast<int>(src[i]) + 128);
      } else {
        int best_idx = 0;
        int8_t best_val = src[i * c];
        for (int ch = 1; ch < c; ++ch) {
          const int8_t v = src[i * c + ch];
          if (v > best_val) {
            best_val = v;
            best_idx = ch;
          }
        }
        mask[i] = ScaleClassToMask(best_idx, c);
      }
    }
  } else {
    return -1;
  }

  if (TLFM_SEG_IMAGE_PIXELS > n) {
    memset(mask + n, 0, static_cast<size_t>(TLFM_SEG_IMAGE_PIXELS - n));
  }
  return 0;
}

} // namespace

extern "C" int tlfm_seg_init_with_arena(const unsigned char *model_data,
                                        unsigned int model_data_len,
                                        uint8_t *tensor_arena,
                                        int tensor_arena_size) {
  if (!model_data || model_data_len == 0 || !tensor_arena ||
      tensor_arena_size <= 0) {
    return -1;
  }

  if (g_ctx.interpreter) {
    return 0;
  }

  g_ctx.model = tflite::GetModel(model_data);
  if (!g_ctx.model || g_ctx.model->version() != TFLITE_SCHEMA_VERSION) {
    return -1;
  }

  if (!g_resolver_initialized) {
    AddSegmentationOps(g_resolver);
    g_resolver_initialized = true;
  }

  g_ctx.interpreter = new (g_interpreter_buffer) tflite::MicroInterpreter(
      g_ctx.model, g_resolver, tensor_arena, tensor_arena_size);

  if (g_ctx.interpreter->AllocateTensors() != kTfLiteOk) {
    return -1;
  }

  g_ctx.input = g_ctx.interpreter->input(0);
  g_ctx.output = LargestSpatialOutput(g_ctx.interpreter);

  return (g_ctx.input && g_ctx.output) ? 0 : -1;
}

extern "C" int tlfm_seg_infer(const uint8_t *pixels, uint8_t *mask) {
  if (!g_ctx.interpreter || !g_ctx.input || !g_ctx.output) {
    return -1;
  }

  if (FillInputGray(pixels, g_ctx.input) != 0) {
    return -1;
  }

  if (g_ctx.interpreter->Invoke() != kTfLiteOk) {
    return -1;
  }

  return DecodeToMask(g_ctx.output, mask);
}
