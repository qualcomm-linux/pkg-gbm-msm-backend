/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _GBM_MSM_INT_H_
#define _GBM_MSM_INT_H_

#include <gbm.h>
#include <gbm_backend_abi.h>
#include "gbm_msm.h"

#define GBM_FORMAT_RGBA32323232F fourcc_code('A', 'B', '8', 'F')

struct gbm_msm_device {
   struct gbm_device base;
};

struct gem_handle_ref {
    uint32_t handle;        // The GEM handle
    int refcount;      // Reference count
};

struct gem_handle_map {
    struct gem_handle_ref *handles; // Dynamic array of handles
    size_t size;                     // Current number of handles
};

struct gbm_bufdesc {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t usage;
    uint64_t modifiers;
};

struct gbm_msm_bo_ext {
    struct gbm_msm_bo msm_bo;
    uint32_t gbm_format;
};

#endif
