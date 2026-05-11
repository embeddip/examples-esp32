#include "tlfm_classification_runner.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <cstdint>
#include <cstring>
#include <new>

namespace {

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

int TensorElementCount(const TfLiteTensor *tensor) {
  if (!tensor || !tensor->dims || tensor->dims->size <= 0) {
    return -1;
  }

  int count = 1;
  for (int i = 0; i < tensor->dims->size; ++i) {
    const int d = tensor->dims->data[i];
    if (d <= 0) {
      return -1;
    }
    count *= d;
  }
  return count;
}

int FillInputGray(const uint8_t *pixels, int pixel_count, TfLiteTensor *input) {
  if (!pixels || !input || pixel_count <= 0) {
    return -1;
  }

  const int input_elems = TensorElementCount(input);
  if (input_elems <= 0) {
    return -1;
  }

  const int n = (input_elems < pixel_count) ? input_elems : pixel_count;

  if (input->type == kTfLiteFloat32) {
    float *dst = input->data.f;
    for (int i = 0; i < n; ++i) {
      dst[i] = pixels[i] / 255.0f;
    }
    for (int i = n; i < input_elems; ++i) {
      dst[i] = 0.0f;
    }
    return 0;
  }

  if (input->type == kTfLiteUInt8) {
    memcpy(input->data.uint8, pixels, static_cast<size_t>(n));
    if (input_elems > n) {
      memset(input->data.uint8 + n, 0, static_cast<size_t>(input_elems - n));
    }
    return 0;
  }

  if (input->type == kTfLiteInt8) {
    int8_t *dst = input->data.int8;
    for (int i = 0; i < n; ++i) {
      dst[i] = static_cast<int8_t>(static_cast<int>(pixels[i]) - 128);
    }
    for (int i = n; i < input_elems; ++i) {
      dst[i] = 0;
    }
    return 0;
  }

  return -1;
}

template <typename T>
int Argmax(const T *data, int count) {
  int best_idx = 0;
  T best_val = data[0];
  for (int i = 1; i < count; ++i) {
    if (data[i] > best_val) {
      best_val = data[i];
      best_idx = i;
    }
  }
  return best_idx;
}

int DecodeClass(const TfLiteTensor *output, uint8_t *class_id) {
  if (!output || !class_id) {
    return -1;
  }

  const int count = TensorElementCount(output);
  if (count <= 0) {
    return -1;
  }

  int idx = 0;
  if (output->type == kTfLiteFloat32) {
    idx = Argmax(output->data.f, count);
  } else if (output->type == kTfLiteUInt8) {
    idx = Argmax(output->data.uint8, count);
  } else if (output->type == kTfLiteInt8) {
    idx = Argmax(output->data.int8, count);
  } else {
    return -1;
  }

  *class_id = static_cast<uint8_t>(idx);
  return 0;
}

} // namespace

extern "C" int tlfm_cls_init_with_arena(const unsigned char *model_data,
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
  g_ctx.output = g_ctx.interpreter->output(0);

  return (g_ctx.input && g_ctx.output) ? 0 : -1;
}

extern "C" int tlfm_cls_infer(const uint8_t *pixels, int pixel_count,
                              uint8_t *class_id) {
  if (!g_ctx.interpreter || !g_ctx.input || !g_ctx.output) {
    return -1;
  }

  if (FillInputGray(pixels, pixel_count, g_ctx.input) != 0) {
    return -1;
  }

  if (g_ctx.interpreter->Invoke() != kTfLiteOk) {
    return -1;
  }

  return DecodeClass(g_ctx.output, class_id);
}
