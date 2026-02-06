/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <string.h>
#include <xf86drm.h>
#include <errno.h>
#include <msm_drm.h>
#include "drm_fourcc.h"
#include "buffer_alloc.h"
#include <sys/stat.h>

int allocate_buffer(const struct gbm_msm_device *msm_dev, uint32_t size, uint32_t usage, uint32_t *handle) {
   if (!msm_dev || !handle)
      return -1;

   struct drm_msm_gem_new args;
   memset(&args, 0, sizeof(args));
   args.size = size;
   if (usage & GBM_BO_USE_CURSOR)
     args.flags = MSM_BO_CACHED_COHERENT;
   else
     args.flags = MSM_BO_CACHED;

   if(drmIoctl(msm_dev->base.v0.fd, DRM_IOCTL_MSM_GEM_NEW, &args)) {
      fprintf(stderr, "MSM_GEM_NEW failed \n");
      return -1;
   }

   *handle = args.handle;
   fprintf(stderr, "MSM_GEM_NEW returned handle[%u] for drm_fd=%d buffer flag=%d buffer size=%d \n",
           args.handle, msm_dev->base.v0.fd, args.flags, size);
   return 0;
}

int get_fd(const struct gbm_msm_device *msm_dev, uint32_t handle, int *fd) {
   if (!msm_dev || !handle || !fd)
      return -1;

   struct drm_prime_handle args;
   memset(&args, 0, sizeof(args));
   args.handle = handle;
   if(drmIoctl(msm_dev->base.v0.fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args)) {
      fprintf(stderr, "DRM_IOCTL_PRIME_HANDLE_TO_FD failed for data fd errono: %d (%s) "
              "drm fd: %d\n", errno, strerror(errno), msm_dev->base.v0.fd);
      return -1;
   }

   *fd = args.fd;
   return 0;
}

int free_buffer(const struct gbm_msm_device *msm_dev, uint32_t handle) {
   if (!msm_dev || !handle)
      return -1;
   struct drm_gem_close gem_close;
   memset(&gem_close, 0, sizeof(gem_close));
   gem_close.handle = handle;
   if (drmIoctl(msm_dev->base.v0.fd, DRM_IOCTL_GEM_CLOSE, &gem_close)) {
      fprintf(stderr, "Failed to close GEM handle for BO=%u\n error=%s", handle, strerror(errno));
      return -1;
   }

   fprintf(stderr, "GEM Handle for BO=%u closed \n", handle);
   return 0;
}

int import_gem_buffer(const struct gbm_msm_device *msm_dev, int fd, uint32_t *handle) {
   if (!msm_dev || (msm_dev->base.v0.fd < 0) || (fd < 0))
      return -1;

   struct drm_prime_handle gemimport_req;
   memset(&gemimport_req, 0, sizeof(gemimport_req));
   gemimport_req.fd = fd;
   if (drmIoctl(msm_dev->base.v0.fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &gemimport_req)) {
      fprintf(stderr, "DRM_IOCTL_PRIME_FD_TO_HANDLE failed for data fd:%d errono: %d (%s) "
                      "drm fd: %d\n", fd, errno, strerror(errno), msm_dev->base.v0.fd);
      return -1;
   }

   *handle = gemimport_req.handle;
   return 0;
}

int bo_offset(int fd, uint32_t handle, uint64_t *offset) {
   struct drm_msm_gem_info req = {
      .handle = handle,
      .info = MSM_INFO_GET_OFFSET,
   };

   if (drmIoctl(fd, DRM_IOCTL_MSM_GEM_INFO, &req)) {
      perror("DRM_IOCTL_MSM_GEM_INFO");
      return -1;
   }

   *offset = req.value;
   return 0;
}
