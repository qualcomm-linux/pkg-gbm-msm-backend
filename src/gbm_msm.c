/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "gbm_msm.h"
#include "gbm_msm_int.h"
#include "buffer_layout.h"
#include "schema_parser.h"
#include "buffer_alloc.h"

#include <gbm_backend_abi.h>
#include <gbm.h>

/* For importing wl_buffer */
#if HAVE_WAYLAND_PLATFORM
#include "wayland-drm.h"
#endif

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

pthread_mutex_t gem_handle_mutex;

#define LOCK(mutex) pthread_mutex_lock(&mutex)
#define UNLOCK(mutex) pthread_mutex_unlock(&mutex)

static const struct gbm_core *gbm_core_;
static struct gem_handle_map map;

static inline struct gbm_msm_device *
gbm_msm_device(struct gbm_device *gbm)
{
   return (struct gbm_msm_device *) gbm;
}

static inline struct gbm_msm_bo *
gbm_msm_bo(struct gbm_bo *bo)
{
   return (struct gbm_msm_bo *) bo;
}

static inline struct gbm_msm_bo_ext *
gbm_msm_bo_ext(struct gbm_msm_bo *msm_bo)
{
   return (struct gbm_msm_bo_ext *) msm_bo;
}

static inline struct gbm_msm_surface *
gbm_msm_surface(struct gbm_surface *surf)
{
   return (struct gbm_msm_surface *) surf;
}

static void add_gem_handle(uint32_t handle)
{
   LOCK(gem_handle_mutex);
   for (size_t i = 0; i < map.size; i++) {
      if (map.handles[i].handle == handle) {
         map.handles[i].refcount++;
         UNLOCK(gem_handle_mutex);
         return;
      }
   }

   map.handles = realloc(map.handles, (map.size + 1) * sizeof(struct gem_handle_ref));
   if (map.handles == NULL) {
      perror("Failed to allocate memory");
      UNLOCK(gem_handle_mutex);
      exit(EXIT_FAILURE);
   }

   map.handles[map.size].handle = handle;
   map.handles[map.size].refcount = 1;
   map.size++;
   UNLOCK(gem_handle_mutex);
}

static void remove_gem_handle(struct gbm_msm_device *msm_dev, uint32_t handle)
{
   if (!handle) {
      return;
   }

   LOCK(gem_handle_mutex);
   for (size_t i = 0; i < map.size; i++) {
      if (map.handles[i].handle != handle) {
         continue;
      }
      if (--map.handles[i].refcount > 0) {
         UNLOCK(gem_handle_mutex);
         return;
      }

      int ret = free_buffer(msm_dev, handle);
      if (ret != 0) {
	 fprintf(stderr, "Failed to close GEM handle for BO=%u\n error=%s", handle, strerror(errno));
	 UNLOCK(gem_handle_mutex);
	 return;
      }

      for (size_t j = i; j < map.size - 1; j++) {
         map.handles[j] = map.handles[j + 1];
      }
      map.size--;
      map.handles = realloc(map.handles, map.size * sizeof(struct gem_handle_ref));
      break;
   }

   UNLOCK(gem_handle_mutex);
}

static void free_gem_handle_map(struct gbm_msm_device *msm_dev)
{
   LOCK(gem_handle_mutex);
   for (size_t i = 0; i < map.size; i++) {
      free_buffer(msm_dev, map.handles[i].handle);
   }

   free(map.handles);
   UNLOCK(gem_handle_mutex);
}

static void *
gbm_msm_bo_map(struct gbm_bo *_bo, uint32_t x, uint32_t y,
               uint32_t width, uint32_t height,
               uint32_t flags, uint32_t *stride, void **map_data)
{
   void *cpuaddr = NULL;
   struct gbm_msm_device *msm_dev = gbm_msm_device(_bo->gbm);
   struct gbm_msm_bo *msm_bo = gbm_msm_bo(_bo);

   if(msm_bo->map != NULL) {
     *map_data = msm_bo->map;
     return msm_bo->map;
   }

   uint64_t mmap_offset;
   if (bo_offset(_bo->gbm->v0.fd, _bo->v0.handle.u32, &mmap_offset))
      return NULL;

   cpuaddr = mmap(0, msm_bo->size, PROT_READ|PROT_WRITE, MAP_SHARED, _bo->gbm->v0.fd, mmap_offset);
   if(cpuaddr == MAP_FAILED) {
      perror("mmap");
      msm_bo->map = NULL;
      return NULL;
   }

   *map_data = cpuaddr;
   msm_bo->map = cpuaddr;

   return cpuaddr;
}

static void
gbm_msm_bo_unmap(struct gbm_bo *_bo, void *map_data)
{
   struct gbm_msm_bo *msm_bo = gbm_msm_bo(_bo);
   munmap(map_data, msm_bo->size);
   msm_bo->map = NULL;
}

static int
gbm_msm_bo_dump(struct gbm_bo * gbo, char *func)
{
   FILE *fptr = NULL;
   static int count = 1;
   const char file_nme[100] = "/var/cache/display/";
   if (!gbo)
      return 0;

   struct gbm_msm_bo *msm_bo = (struct gbm_msm_bo*)gbo;
   if (!msm_bo)
      return 0;

   uint32_t size = msm_bo->size;
   int ret = 0;
   char tmp_str[50];
   uint32_t width = gbo->v0.width;
   uint32_t height = gbo->v0.height;

   struct gbm_msm_bo_ext *msm_bo_ext = gbm_msm_bo_ext(msm_bo);
   uint32_t format = msm_bo_ext->gbm_format;

   ret = mkdir(file_nme, 777);
   if ((ret != 0) && errno != EEXIST) {
      printf("Failed to create %s directory errno = %d, desc = %s",
              file_nme, errno, strerror(errno));
      return -1;
   }

   snprintf(tmp_str, sizeof(tmp_str), "gbm_dump_c%d_%d_%d_%d_%s.dat",
                    count++,width,height,getpid(), func);

   strlcat(file_nme, tmp_str, sizeof(file_nme));
   fptr=fopen(file_nme, "w+");
   if(fptr == NULL) {
      printf("Failed to open file %s\n",file_nme);
      return -1;
   }

   void *map_data, *buffer;
   buffer = map_data = NULL;
   uint32_t stride = 0;
   buffer = gbm_msm_bo_map(gbo, 0, 0, width, height, GBM_BO_TRANSFER_READ_WRITE, &stride, &map_data);
   ret = fwrite(buffer, 1, size, fptr);
   if(ret != size) {
      printf("File write size mismatch i/p=%d o/p=%d\n %s\n",size,ret,strerror(errno));
      ret = -1;
   } else
      ret = 0;

   if(fptr)
      fclose(fptr);

   return ret;
}

static uint32_t
gbm_msm_bo_metabuffer_size(struct gbm_bo *bo, int plane)
{
   struct gbm_msm_bo *msm_bo = gbm_msm_bo(bo);
   if (!msm_bo)
      return 0;

   uint32_t size = 0;
   struct gbm_bufdesc bufdesc = {};
   bufdesc.width = msm_bo->base.v0.width;
   bufdesc.height = msm_bo->base.v0.height;
   struct gbm_msm_bo_ext *msm_bo_ext = gbm_msm_bo_ext(msm_bo);
   bufdesc.format = msm_bo_ext->gbm_format;
   bufdesc.modifiers = msm_bo->modifier;
   if(get_meta_buffer_size(&bufdesc, plane, &size) != 0) {
      fprintf(stderr, "Error: failed to get metabuffer size \n");
      return 0;
   }
   return size;
}

static void
gbm_msm_bo_get_aligned_width_height(struct gbm_bo *_bo, int plane,
                                    uint32_t *aligned_width,
                                    uint32_t *aligned_height)
{
   struct gbm_msm_bo *msm_bo = gbm_msm_bo(_bo);
   if (!msm_bo)
      return;

   struct gbm_bufdesc bufdesc = {};
   bufdesc.width = msm_bo->base.v0.width;
   bufdesc.height = msm_bo->base.v0.height;
   struct gbm_msm_bo_ext *msm_bo_ext = gbm_msm_bo_ext(msm_bo);
   bufdesc.format = msm_bo_ext->gbm_format;
   bufdesc.modifiers = msm_bo->modifier;

   if (get_aligned_width_and_height(&bufdesc, plane, aligned_width, aligned_height) != 0) {
      aligned_width = aligned_height = 0;
   }
}

static struct gbm_bo *
gbm_msm_bo_create(struct gbm_device *gbm,
              uint32_t width, uint32_t height,
              uint32_t format, uint32_t usage,
              const uint64_t *modifiers,
              const unsigned int count)
{
   struct gbm_msm_device *msm_dev = gbm_msm_device(gbm);
   struct gbm_msm_bo_ext *msm_bo_ext = NULL;
   uint64_t modifiers_mask = 0;
   format = gbm_core_->v0.format_canonicalize(format);
   msm_bo_ext = calloc(1, sizeof *msm_bo_ext);
   if (msm_bo_ext == NULL)
      return NULL;

   struct gbm_msm_bo *bo = &msm_bo_ext->msm_bo;
   bo->base.gbm = gbm;
   bo->base.v0.width = width;
   bo->base.v0.height = height;
   bo->base.v0.format = format;
   for (int m = 0; m < count; m++) {
      modifiers_mask |= modifiers[m];
   }

   struct gbm_bufdesc bufdesc = {width, height, format, usage, modifiers_mask};

   if (get_best_layout(count, modifiers, &bufdesc) != 0)
      return NULL;

   if (get_num_planes(&bufdesc, &(bo->num_planes)) != 0)
      return NULL;

   if (get_aligned_width_and_height(&bufdesc, 0, &(bo->aligned_width), &(bo->aligned_height)) != 0)
      return NULL;

   uint32_t size = 0;
   if (get_size(&bufdesc, &size) != 0)
      return NULL;

   if (allocate_buffer(msm_dev, size, usage, &(bo->base.v0.handle.u32)) != 0)
      return NULL;

   add_gem_handle(bo->base.v0.handle.u32);

   if (get_stride(&bufdesc, 0, &(bo->base.v0.stride)) != 0)
      return NULL;

   bo->size = size;
   bo->modifier = bufdesc.modifiers;
   bo->bo_dump_buffers = gbm_msm_bo_dump;
   bo->bo_get_metabuffer_size = gbm_msm_bo_metabuffer_size;
   bo->bo_get_plane_aligned_width_height = gbm_msm_bo_get_aligned_width_height;

   msm_bo_ext->gbm_format = format;

   return &bo->base;
}

static struct gbm_bo *
gbm_msm_bo_import(struct gbm_device *gbm,
                  uint32_t type, void *buffer, uint32_t usage)
{
   struct gbm_msm_device *msm_dev = gbm_msm_device(gbm);
   struct gbm_msm_bo_ext *msm_bo_ext = NULL;
   struct gbm_bufdesc bufdesc = {};
   uint32_t handle = 0;

   msm_bo_ext = calloc(1, sizeof *msm_bo_ext);
   if (msm_bo_ext == NULL)
      return NULL;

   struct gbm_msm_bo *bo = &msm_bo_ext->msm_bo;

   switch (type) {
   case GBM_BO_IMPORT_FD:
      struct gbm_import_fd_data *fd_data = buffer;
      if (import_gem_buffer(msm_dev, fd_data->fd, &handle)) {
         return NULL;
      }

      bo->base.v0.handle.u32 = handle;
      add_gem_handle(handle);
      bo->base.gbm = gbm;
      bo->base.v0.width = fd_data->width;
      bo->base.v0.height = fd_data->height;
      bo->base.v0.format = fd_data->format;
      msm_bo_ext->gbm_format = bo->base.v0.format;

      bufdesc.width = bo->base.v0.width;
      bufdesc.height = bo->base.v0.height;
      bufdesc.format = msm_bo_ext->gbm_format;
      bufdesc.modifiers = 0;

      if (get_num_planes(&bufdesc, &(bo->num_planes)) != 0)
         return NULL;

      if (get_aligned_width_and_height(&bufdesc, 0, &(bo->aligned_width), &(bo->aligned_height)) != 0)
         return NULL;

      if (get_size(&bufdesc, &(bo->size)) != 0)
         return NULL;

      if (get_stride(&bufdesc, 0, &(bo->base.v0.stride)) != 0)
         return NULL;

      bo->modifier = 0;
      break;

   case GBM_BO_IMPORT_FD_MODIFIER:
      struct gbm_import_fd_modifier_data *fd_modifer_data = buffer;
      // ToDo: Add support for multiple fds.
      if (import_gem_buffer(msm_dev, fd_modifer_data->fds[0], &handle)) {
         return NULL;
      }

      bo->base.v0.handle.u32 = handle;
      add_gem_handle(handle);
      bo->base.gbm = gbm;
      bo->base.v0.width = fd_modifer_data->width;
      bo->base.v0.height = fd_modifer_data->height;
      bo->base.v0.format = fd_modifer_data->format;
      if ((bo->base.v0.format == GBM_FORMAT_XBGR16161616F) &&
          ((fd_modifer_data->modifier & DRM_FORMAT_MOD_QCOM_32F) == DRM_FORMAT_MOD_QCOM_32F)) {
         msm_bo_ext->gbm_format = GBM_FORMAT_RGBA32323232F;
      } else {
         msm_bo_ext->gbm_format = bo->base.v0.format;
      }
      bufdesc.width = bo->base.v0.width;
      bufdesc.height = bo->base.v0.height;
      bufdesc.format = msm_bo_ext->gbm_format;
      bufdesc.modifiers = fd_modifer_data->modifier;

      if (get_num_planes(&bufdesc, &(bo->num_planes)) != 0)
         return NULL;

      if (get_aligned_width_and_height(&bufdesc, 0, &(bo->aligned_width), &(bo->aligned_height)) != 0)
         return NULL;

      if (get_size(&bufdesc, &(bo->size)) != 0)
         return NULL;

      if (get_stride(&bufdesc, 0, &(bo->base.v0.stride)) != 0)
         return NULL;

      bo->modifier = bufdesc.modifiers;
      break;

   default:
      errno = ENOSYS;
      return NULL;
   }

   bo->bo_dump_buffers = gbm_msm_bo_dump;
   bo->bo_get_metabuffer_size = gbm_msm_bo_metabuffer_size;
   bo->bo_get_plane_aligned_width_height = gbm_msm_bo_get_aligned_width_height;
   return &bo->base;
}

static void
gbm_msm_bo_destroy(struct gbm_bo *_bo)
{
   struct gbm_msm_device *msm_dev = gbm_msm_device(_bo->gbm);
   struct gbm_msm_bo *bo = gbm_msm_bo(_bo);
   if (bo == NULL)
      return;

   remove_gem_handle(msm_dev, bo->base.v0.handle.u32);

   if (bo->map) {
     void *map_data = bo->map;
     gbm_msm_bo_unmap(_bo, map_data);
   }

   struct gbm_msm_bo_ext *msm_bo_ext = gbm_msm_bo_ext(bo);
   free(msm_bo_ext);
   msm_bo_ext = NULL;
}

static int
gbm_msm_bo_get_fd(struct gbm_bo *_bo)
{
   struct gbm_msm_bo *msm_bo = (struct gbm_msm_bo*)_bo;

   int fd;
   struct gbm_msm_device *msm_dev = gbm_msm_device(_bo->gbm);
   if (get_fd(msm_dev, _bo->v0.handle.u32, &fd) != 0) {
      return -1;
   }

   return fd;
}

static uint64_t
gbm_msm_bo_get_modifier(struct gbm_bo *_bo)
{
   struct gbm_msm_bo *msm_bo = (struct gbm_msm_bo*)_bo;
   return msm_bo->modifier;
}

static uint32_t
gbm_msm_bo_get_stride(struct gbm_bo *bo, int plane)
{
   struct gbm_msm_bo *msm_bo = gbm_msm_bo(bo);
   struct gbm_bufdesc bufdesc = {};
   bufdesc.width = msm_bo->base.v0.width;
   bufdesc.height = msm_bo->base.v0.height;
   struct gbm_msm_bo_ext *msm_bo_ext = gbm_msm_bo_ext(msm_bo);
   bufdesc.format = msm_bo_ext->gbm_format;
   bufdesc.modifiers = msm_bo->modifier;

   uint32_t stride = 0;
   if (get_stride(&bufdesc, plane, &stride) != 0)
      return 0;

   return stride;
}

static int
gbm_msm_bo_get_planes(struct gbm_bo *_bo)
{
   struct gbm_msm_bo *msm_bo = (struct gbm_msm_bo*)_bo;
   return msm_bo->num_planes;
}

static uint32_t
gbm_msm_bo_get_offset(struct gbm_bo *_bo, int plane)
{
   struct gbm_msm_bo *msm_bo = gbm_msm_bo(_bo);
   struct gbm_bufdesc bufdesc = {};
   bufdesc.width = msm_bo->base.v0.width;
   bufdesc.height = msm_bo->base.v0.height;
   struct gbm_msm_bo_ext *msm_bo_ext = gbm_msm_bo_ext(msm_bo);
   bufdesc.format = msm_bo_ext->gbm_format;
   bufdesc.modifiers = msm_bo->modifier;

   uint32_t offset = 0;
   if (get_plane_offset(&bufdesc, plane, &offset) != 0)
      return 0;

   return offset;
}

static union gbm_bo_handle
gbm_msm_bo_get_handle(struct gbm_bo *_bo, int plane)
{
   return _bo->v0.handle;
}

static int
gbm_msm_bo_get_plane_fd(struct gbm_bo *_bo, int plane)
{
   return gbm_msm_bo_get_fd(_bo);
}

static int
gbm_msm_bo_write(struct gbm_bo *_bo, const void *buf, size_t count)
{
   struct gbm_msm_bo *msm_bo = (struct gbm_msm_bo*)_bo;
   void *cpuaddr, *mapdata = NULL;

   if (msm_bo->map == NULL) {
      uint32_t stride = 0;
      cpuaddr = gbm_msm_bo_map(_bo, 0, 0, _bo->v0.width, _bo->v0.height,
                               GBM_BO_TRANSFER_READ_WRITE, &stride, &mapdata);
      msm_bo->map = cpuaddr;
   }

   memcpy(msm_bo->map, buf, count);
   return 0;
}

static void
gbm_msm_destroy(struct gbm_device *gbm)
{
   struct gbm_msm_device *msm_dev = gbm_msm_device(gbm);
   if(msm_dev == NULL)
      return;
   free_gem_handle_map(msm_dev);
   free(msm_dev);
   msm_dev = NULL;
}

static int
has_free_buffers(struct gbm_surface *surf)
{
   struct gbm_msm_surface *msm_surf = gbm_msm_surface(surf);

   for (unsigned i = 0; i < ARRAY_SIZE(msm_surf->color_buffers); i++)
      if (!msm_surf->color_buffers[i].locked)
         return 1;

   return 0;
}

static struct gbm_bo *
get_back_bo(struct gbm_surface *surf)
{
   struct gbm_msm_surface *msm_surf = gbm_msm_surface(surf);
   int age = 0;
   struct gbm_bo *bo = NULL;

   if (msm_surf->back == NULL) {
      for (unsigned i = 0; i < ARRAY_SIZE(msm_surf->color_buffers); i++) {
         if (!msm_surf->color_buffers[i].locked &&
              msm_surf->color_buffers[i].age >= age) {
            msm_surf->back = &msm_surf->color_buffers[i];
            age = msm_surf->color_buffers[i].age;
         }
      }
   }

   if (msm_surf->back == NULL) {
      fprintf(stderr, "Error: msm surface is null");
      return bo;
   }

   if (msm_surf->back->bo == NULL) {
      unsigned flags = msm_surf->base.v0.flags;
      bo = gbm_msm_bo_create(msm_surf->base.gbm, msm_surf->base.v0.width,
                             msm_surf->base.v0.height, msm_surf->base.v0.format, flags,
                             msm_surf->base.v0.modifiers, msm_surf->base.v0.count);
      msm_surf->back->bo = bo;
   }
   if (msm_surf->back->bo == NULL) {
      fprintf(stderr, "Error: msm surface back bo is null");
      return bo;
   }

   struct gbm_msm_bo *msm_bo = gbm_msm_bo(msm_surf->back->bo);
   return msm_surf->back->bo;
}

static int
swap_buffers(struct gbm_surface *surf)
{
   struct gbm_msm_surface *msm_surf = gbm_msm_surface(surf);
   for (unsigned i = 0; i < ARRAY_SIZE(msm_surf->color_buffers); i++) {
      if (msm_surf->color_buffers[i].age > 0)
         msm_surf->color_buffers[i].age++;
   }

   if (get_back_bo(surf) < 0) {
      fprintf(stderr, "error in get_back_bo");
      return -1;
   }
   msm_surf->current =  msm_surf->back;
   msm_surf->current->age = 1;
   msm_surf->back = NULL;

   return 0;
}

static struct gbm_bo *
lock_front_buffer(struct gbm_surface *surf)
{
   struct gbm_msm_surface *msm_surf = gbm_msm_surface(surf);
   struct gbm_bo *bo;

   if (msm_surf->current == NULL)
      return NULL;

   bo = msm_surf->current->bo;
   msm_surf->current->locked = true;
   msm_surf->current = NULL;

   return bo;
}

static void
release_buffer(struct gbm_surface *surf, struct gbm_bo *bo)
{
   struct gbm_msm_surface *msm_surf = gbm_msm_surface(surf);

   for (unsigned i = 0; i < ARRAY_SIZE(msm_surf->color_buffers); i++) {
      if (msm_surf->color_buffers[i].bo == bo) {
         msm_surf->color_buffers[i].locked = false;
         break;
      }
   }
}

static void
gbm_msm_surface_destroy(struct gbm_surface* surf)
{
   struct gbm_msm_surface *msm_surf = gbm_msm_surface(surf);
   free(msm_surf);
   msm_surf = NULL;
}

static struct gbm_surface *
gbm_msm_surface_create(struct gbm_device *gbm,
                       uint32_t width, uint32_t height,
                       uint32_t format, uint32_t flags,
                       const uint64_t *modifiers, const unsigned count)
{
   struct gbm_msm_device *msm_dev = gbm_msm_device(gbm);
   struct gbm_msm_surface *surf = NULL;
   struct gbm_bo *bo = NULL;
   if (msm_dev == NULL) {
      errno = ENOSYS;
      return NULL;
   }

   if (width <= 0 || height <= 0) {
      errno = ENOSYS;
      return NULL;
   }

   if (count)
      assert(modifiers);

   surf = calloc(1, sizeof *surf);
   if (surf == NULL) {
      errno = ENOMEM;
      return NULL;
   }

   surf->base.gbm = gbm;
   surf->base.v0.width = width;
   surf->base.v0.height = height;
   surf->base.v0.format = gbm_core_->v0.format_canonicalize(format);
   surf->base.v0.flags = flags;
   surf->base.v0.modifiers = calloc(count, sizeof(*modifiers));
   if (count && !surf->base.v0.modifiers) {
      errno = ENOMEM;
      free(surf);
      return NULL;
   }
   surf->base.v0.count = count;
   if (count)
      memcpy(surf->base.v0.modifiers, modifiers, count * sizeof(*modifiers));

   for (uint32_t i = 0; i < NUM_BACK_BUFFERS; i++) {
      bo = gbm_msm_bo_create(gbm, width, height, format, flags, modifiers, count);
      if (bo == NULL) {
	 errno = ENOMEM;
         free(surf);
         return NULL;
      }
      surf->color_buffers[i].bo = bo;
   }

   surf->surface_get_back_bo = get_back_bo;
   surf->surface_swap_buffers = swap_buffers;

   return &surf->base;
}

static struct gbm_device *
msm_device_create(int fd, uint32_t gbm_backend_version)
{
   const char *gpu_path = "/sys/class/kgsl/kgsl-3d0/";
   if (access(gpu_path, F_OK) != 0) {
      return NULL;
   }

   struct gbm_msm_device *msm;

   assert(gbm_core_->v0.core_version == GBM_BACKEND_ABI_VERSION);
   assert(gbm_core_->v0.core_version == gbm_backend_version);

   msm = calloc(1, sizeof *msm);
   if (!msm)
      return NULL;

   init_xml_schema();

   msm->base.v0.fd = fd;
   msm->base.v0.backend_version = gbm_backend_version;
   msm->base.v0.bo_create = gbm_msm_bo_create;
   msm->base.v0.surface_create = gbm_msm_surface_create;
   msm->base.v0.surface_destroy = gbm_msm_surface_destroy;
   msm->base.v0.bo_import = gbm_msm_bo_import;
   msm->base.v0.bo_get_fd = gbm_msm_bo_get_fd;
   msm->base.v0.bo_destroy = gbm_msm_bo_destroy;
   msm->base.v0.destroy = gbm_msm_destroy;
   msm->base.v0.surface_has_free_buffers = has_free_buffers;
   msm->base.v0.surface_lock_front_buffer = lock_front_buffer;
   msm->base.v0.surface_release_buffer = release_buffer;
   msm->base.v0.bo_map = gbm_msm_bo_map;
   msm->base.v0.bo_unmap = gbm_msm_bo_unmap;
   msm->base.v0.bo_get_modifier = gbm_msm_bo_get_modifier;
   msm->base.v0.bo_get_stride = gbm_msm_bo_get_stride;
   msm->base.v0.bo_get_planes = gbm_msm_bo_get_planes;
   msm->base.v0.bo_get_offset = gbm_msm_bo_get_offset;
   msm->base.v0.bo_get_handle = gbm_msm_bo_get_handle;
   msm->base.v0.bo_write = gbm_msm_bo_write;
   msm->base.v0.bo_get_plane_fd = gbm_msm_bo_get_plane_fd;

   msm->base.v0.name = "msm";

   pthread_mutex_init(&gem_handle_mutex, NULL);

   return &msm->base;
}

struct gbm_backend gbm_msm_backend = {
   .v0.backend_version = GBM_BACKEND_ABI_VERSION,
   .v0.backend_name = "msm",
   .v0.create_device = msm_device_create,
};

__attribute__((visibility("default")))
const struct gbm_backend *gbmint_get_backend(const struct gbm_core *gbm_core) {
   gbm_core_ = gbm_core;
   return &gbm_msm_backend;
}

