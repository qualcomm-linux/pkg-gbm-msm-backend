/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>

int get_num_planes(struct gbm_bufdesc *descriptor, uint32_t *num_planes);
bool ubwc_enabled(struct gbm_bufdesc *descriptor);
int get_aligned_width_and_height(struct gbm_bufdesc *descriptor, int plane,
                                 uint32_t *alignedw, uint32_t *alignedh);
int get_stride(struct gbm_bufdesc *descriptor, int plane, uint32_t *stride);
int get_data_plane_size(struct gbm_bufdesc *descriptor, int plane,
                        uint32_t *plane_size);
int get_meta_buffer_size(struct gbm_bufdesc *descriptor, int plane, 
                         uint32_t *metabuffer_size);
int get_size(struct gbm_bufdesc *descriptor, uint32_t *buffer_size);
int get_plane_offset(struct gbm_bufdesc *descriptor, int plane, 
                     uint32_t *plane_offset);
void get_plane_width_and_height(uint32_t format, int plane,
                                uint32_t *buffer_width, uint32_t *buffer_height);
bool ubwc_supported(struct gbm_bufdesc *descriptor);
int get_best_layout(const unsigned int count, const uint64_t *modifiers,
                    struct gbm_bufdesc *descriptor);
