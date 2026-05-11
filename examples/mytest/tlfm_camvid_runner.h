#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int tlfm_camvid_init_with_arena(const unsigned char *model_data,
                                unsigned int model_data_len,
                                uint8_t *tensor_arena,
                                int tensor_arena_size);

int tlfm_camvid_infer(const uint8_t *rgb_pixels, uint8_t *class_map,
                      uint8_t *confidence_map);

#ifdef __cplusplus
}
#endif
