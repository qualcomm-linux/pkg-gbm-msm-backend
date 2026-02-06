/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include "gbm_msm_int.h"

int allocate_buffer(const struct gbm_msm_device *msm_dev, uint32_t size, uint32_t usage, uint32_t *handle);
int import_gem_buffer(const struct gbm_msm_device *msm_dev, int fd, uint32_t *handle);
int get_fd(const struct gbm_msm_device *msm_dev, uint32_t handle, int *fd);
int free_buffer(const struct gbm_msm_device *msm_dev, uint32_t handle);
int bo_offset(int fd, uint32_t handle, uint64_t *offset);
