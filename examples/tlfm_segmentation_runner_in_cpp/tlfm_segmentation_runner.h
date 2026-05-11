#pragma once

#include <stdint.h>

#define TLFM_SEG_INPUT_WIDTH 128
#define TLFM_SEG_INPUT_HEIGHT 128
#define TLFM_SEG_INPUT_CHANNELS 1
#define TLFM_SEG_OUTPUT_CHANNELS 2
#define TLFM_SEG_IMAGE_PIXELS (TLFM_SEG_INPUT_WIDTH * TLFM_SEG_INPUT_HEIGHT)

#ifdef __cplusplus
extern "C" {
#endif

int tlfm_seg_init_with_arena(const unsigned char *model_data,
                             unsigned int model_data_len,
                             uint8_t *tensor_arena,
                             int tensor_arena_size);

int tlfm_seg_infer(const uint8_t *pixels, uint8_t *mask);

#ifdef __cplusplus
}
#endif
