#include "tlfm_camvid_runner.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <cstdint>
#include <cstring>
#include <new>

namespace {

constexpr int kImagePixels = 128 * 128;
constexpr int kInputChannels = 3;

struct InferenceContext {
  const tflite::Model *model = nullptr;
  tflite::MicroInterpreter *interpreter = nullptr;
  TfLiteTensor *input = nullptr;
  TfLiteTensor *output = nullptr;
};

InferenceContext g_ctx;
tflite::MicroMutableOpResolver<32> g_resolver;
bool g_resolver_initialized = false;
alignas(tflite::MicroInterpreter)
    uint8_t g_interpreter_buffer[sizeof(tflite::MicroInterpreter)] = {};

void AddCommonOps(tflite::MicroMutableOpResolver<32> &resolver) {
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddTransposeConv();
  resolver.AddMaxPool2D();
  resolver.AddAveragePool2D();
  resolver.AddResizeNearestNeighbor();
  resolver.AddConcatenation();
  resolver.AddAdd();
  resolver.AddSub();
  resolver.AddMul();
  resolver.AddReshape();
  resolver.AddRelu();
  resolver.AddRelu6();
  resolver.AddLeakyRelu();
  resolver.AddLogistic();
  resolver.AddSoftmax();
  resolver.AddPad();
  resolver.AddPack();
  resolver.AddUnpack();
  resolver.AddStridedSlice();
  resolver.AddSlice();
  resolver.AddSplit();
  resolver.AddSplitV();
  resolver.AddTranspose();
  resolver.AddMean();
  resolver.AddQuantize();
  resolver.AddDequantize();
  resolver.AddCast();
  resolver.AddShape();
  resolver.AddExpandDims();
  resolver.AddSqueeze();
  resolver.AddFullyConnected();
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

int FillInputRgb(const uint8_t *pixels, TfLiteTensor *input) {
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
  const int n = (spatial < kImagePixels) ? spatial : kImagePixels;
  const int in_ch = (c > 0) ? c : kInputChannels;

  if (input->type == kTfLiteFloat32) {
    float *dst = input->data.f;
    for (int i = 0; i < n; ++i) {
      for (int ch = 0; ch < in_ch; ++ch) {
        const int src_ch = (ch < kInputChannels) ? ch : (kInputChannels - 1);
        const uint8_t v = pixels[i * kInputChannels + src_ch];
        dst[i * in_ch + ch] = v / 255.0f;
      }
    }
    return 0;
  }

  if (input->type == kTfLiteUInt8) {
    const int total_in = n * kInputChannels;
    const int copy_n = (static_cast<int>(input->bytes) < total_in)
                           ? static_cast<int>(input->bytes)
                           : total_in;
    memcpy(input->data.uint8, pixels, static_cast<size_t>(copy_n));
    return 0;
  }

  if (input->type == kTfLiteInt8) {
    int8_t *dst = input->data.int8;
    for (int i = 0; i < n; ++i) {
      for (int ch = 0; ch < in_ch; ++ch) {
        const int src_ch = (ch < kInputChannels) ? ch : (kInputChannels - 1);
        const int v = static_cast<int>(pixels[i * kInputChannels + src_ch]);
        dst[i * in_ch + ch] = static_cast<int8_t>(v - 128);
      }
    }
    return 0;
  }

  return -1;
}

uint8_t ScaleClassToU8(int class_id, int class_count) {
  if (class_count <= 1) {
    return 0;
  }
  return static_cast<uint8_t>((class_id * 255) / (class_count - 1));
}

int DecodeClassAndConfidence(const TfLiteTensor *output, uint8_t *class_map,
                             uint8_t *confidence_map) {
  if (!output || !class_map || !confidence_map) {
    return -1;
  }

  int h = 0;
  int w = 0;
  int c = 0;
  if (TensorHwc(output, &h, &w, &c) != 0 || h <= 0 || w <= 0 || c <= 0) {
    return -1;
  }

  const int spatial = h * w;
  const int n = (spatial < kImagePixels) ? spatial : kImagePixels;

  if (output->type == kTfLiteFloat32) {
    const float *src = output->data.f;
    for (int i = 0; i < n; ++i) {
      int best_class = 0;
      float best_score = src[i * c];
      for (int ch = 1; ch < c; ++ch) {
        const float score = src[i * c + ch];
        if (score > best_score) {
          best_score = score;
          best_class = ch;
        }
      }
      class_map[i] = ScaleClassToU8(best_class, c);
      if (best_score < 0.0f) {
        best_score = 0.0f;
      }
      if (best_score > 1.0f) {
        best_score = 1.0f;
      }
      confidence_map[i] = static_cast<uint8_t>(best_score * 255.0f);
    }
  } else if (output->type == kTfLiteUInt8) {
    const uint8_t *src = output->data.uint8;
    for (int i = 0; i < n; ++i) {
      int best_class = 0;
      uint8_t best_score = src[i * c];
      for (int ch = 1; ch < c; ++ch) {
        const uint8_t score = src[i * c + ch];
        if (score > best_score) {
          best_score = score;
          best_class = ch;
        }
      }
      class_map[i] = ScaleClassToU8(best_class, c);
      confidence_map[i] = best_score;
    }
  } else if (output->type == kTfLiteInt8) {
    const int8_t *src = output->data.int8;
    for (int i = 0; i < n; ++i) {
      int best_class = 0;
      int8_t best_score = src[i * c];
      for (int ch = 1; ch < c; ++ch) {
        const int8_t score = src[i * c + ch];
        if (score > best_score) {
          best_score = score;
          best_class = ch;
        }
      }
      class_map[i] = ScaleClassToU8(best_class, c);
      confidence_map[i] =
          static_cast<uint8_t>(static_cast<int>(best_score) + 128);
    }
  } else {
    return -1;
  }

  if (kImagePixels > n) {
    memset(class_map + n, 0, static_cast<size_t>(kImagePixels - n));
    memset(confidence_map + n, 0, static_cast<size_t>(kImagePixels - n));
  }

  return 0;
}

} // namespace

extern "C" int tlfm_camvid_init_with_arena(const unsigned char *model_data,
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
    AddCommonOps(g_resolver);
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

extern "C" int tlfm_camvid_infer(const uint8_t *rgb_pixels, uint8_t *class_map,
                                 uint8_t *confidence_map) {
  if (!g_ctx.interpreter || !g_ctx.input || !g_ctx.output) {
    return -1;
  }

  if (FillInputRgb(rgb_pixels, g_ctx.input) != 0) {
    return -1;
  }

  if (g_ctx.interpreter->Invoke() != kTfLiteOk) {
    return -1;
  }

  return DecodeClassAndConfidence(g_ctx.output, class_map, confidence_map);
}
