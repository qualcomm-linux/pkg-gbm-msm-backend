/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _GBM_MSM_H_
#define _GBM_MSM_H_

#include <stdbool.h>
#include <gbm.h>
#include <gbm_backend_abi.h>
#include "drm_fourcc.h"

#define NUM_BACK_BUFFERS 3

#define DRM_FORMAT_MOD_QCOM_32F fourcc_mod_code(QCOM, 32)

struct gbm_msm_bo {
   struct gbm_bo base;
   int fd;
   uint32_t size;
   uint32_t aligned_width;
   uint32_t aligned_height;
   uint64_t modifier;
   uint32_t num_planes;
   void *map;
   int (*bo_dump_buffers)(struct gbm_bo *gbo, char *func);
   uint32_t (*bo_get_metabuffer_size)(struct gbm_bo *gbo, int plane);
   void (*bo_get_plane_aligned_width_height)(struct gbm_bo *gbm, int plane, uint32_t *aligned_width, uint32_t *aligned_height);
};


struct gbm_msm_surface {
   struct gbm_surface base;
   struct {
      int age;
      bool locked;
      struct gbm_bo *bo;
   } color_buffers[NUM_BACK_BUFFERS], *back, *current;
   struct gbm_bo *(*surface_get_back_bo)(struct gbm_surface *surface);
   int (*surface_swap_buffers)(struct gbm_surface *surface);
};

const struct gbm_backend *gbmint_get_backend(const struct gbm_core *gbm_core);
#endif
