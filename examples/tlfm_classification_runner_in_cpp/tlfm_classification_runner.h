#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int tlfm_cls_init_with_arena(const unsigned char *model_data,
                             unsigned int model_data_len,
                             uint8_t *tensor_arena,
                             int tensor_arena_size);

int tlfm_cls_infer(const uint8_t *pixels, int pixel_count, uint8_t *class_id);

#ifdef __cplusplus
}
#endif
