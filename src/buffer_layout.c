/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include "schema_parser.h"
#include "buffer_layout.h"
#include "drm_fourcc.h"

#define ALIGN(x, align) (((x) + ((align)-1)) & ~((align)-1))

int get_num_planes(struct gbm_bufdesc *descriptor, uint32_t *num_planes) {
   if (!descriptor)
      return false;

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      fprintf(stderr,"Unsupported format\n");
      return -1;
   }
   struct format_info *info = get_format_info(format_name);
   if (!info)
      return -1;

   *num_planes = info->plane_count;
   return 0;
}

bool ubwc_enabled(struct gbm_bufdesc *descriptor) {
   if (!descriptor)
      return false;

   if ((descriptor->modifiers & DRM_FORMAT_MOD_QCOM_COMPRESSED) == DRM_FORMAT_MOD_QCOM_COMPRESSED)
      return true;

   return false;
}

bool ubwc_supported(struct gbm_bufdesc *descriptor) {
   if (!descriptor)
      return false;

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      return false;
   }

   struct format_info *info = get_format_info(format_name);
   if (!info)
      return false;

   if (!info->planes[0].meta_info) {
      return false;
   }

   return true;
}

int get_best_layout(const unsigned int count, const uint64_t *modifiers, struct gbm_bufdesc *descriptor) {
   if (count == 0) {
      descriptor->modifiers = 0;
      return 0;
   }

   for (int m = 0; m < count; m++) {
      if (modifiers[m] == DRM_FORMAT_MOD_QCOM_COMPRESSED && ubwc_supported(descriptor)) {
         descriptor->modifiers = modifiers[m];
         return 0;
      }
   }

   for (int m = 0; m < count; m++) {
      if (modifiers[m] == DRM_FORMAT_MOD_LINEAR) {
         descriptor->modifiers = modifiers[m];
         return 0;
      }
   }

   for (int m = 0; m < count; m++) {
      if (modifiers[m] == DRM_FORMAT_MOD_INVALID) {
         descriptor->modifiers = modifiers[m];
         return 0;
      }
   }

   return -1;
}

void get_plane_width_and_height(uint32_t format, int plane, uint32_t *plane_width, uint32_t *plane_height) {
   if (plane == 0)
      return;

   switch(format) {
   case GBM_FORMAT_NV12:
   case GBM_FORMAT_NV21:
   case GBM_FORMAT_NV16:
   case GBM_FORMAT_NV61:
      *plane_height /= 2;
      break;
   case GBM_FORMAT_YUV410:
   case GBM_FORMAT_YVU410:
   case GBM_FORMAT_YUV411:
   case GBM_FORMAT_YVU411:
      *plane_width /= 4;
      *plane_height /= 4;
      break;
   case GBM_FORMAT_YUV420:
   case GBM_FORMAT_YVU420:
      *plane_width /= 2;
      *plane_height /= 2;
      break;
   case GBM_FORMAT_YUV422:
   case GBM_FORMAT_YVU422:
      *plane_width /= 2;
      break;
   case GBM_FORMAT_YUV444:
   case GBM_FORMAT_YVU444:
      break;

   default:
      break;
   }
}

int get_aligned_width_and_height(struct gbm_bufdesc *descriptor, int plane,
                                 uint32_t *alignedw, uint32_t *alignedh)
{
   if (!descriptor || !alignedw || !alignedh)
      return -1;

   uint32_t width = descriptor->width;
   uint32_t height = descriptor->height;
   get_plane_width_and_height(descriptor->format, plane, &width, &height);

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      fprintf(stderr,"Unsupported format\n");
      return -1;
   }

   struct format_info *info = get_format_info(format_name);
   if (!info)
      return -1;

   uint32_t pitch_align_factor = info->planes[plane].pitch_align;
   uint32_t height_align_factor = info->planes[plane].height_align;

   if (ubwc_enabled(descriptor)) {
      if (!info->planes[plane].meta_info) {
	fprintf(stderr,"UBWC not supported for this format \n");
	return -1;
      }

      pitch_align_factor = info->planes[plane].meta_info->pitch_align;
      height_align_factor = info->planes[plane].meta_info->height_align;
   }

   *alignedw = ALIGN(width, pitch_align_factor);
   *alignedh = ALIGN(height, height_align_factor);

   return 0;
}

int get_stride(struct gbm_bufdesc *descriptor, int plane, uint32_t *stride)
{
   if (!descriptor || !stride)
      return -1;

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      fprintf(stderr,"Unsupported format\n");
      return -1;
   }

   struct format_info *info = get_format_info(format_name);
   if (!info)
      return -1;

   uint32_t alignedw = 0, alignedh = 0;

   if (get_aligned_width_and_height(descriptor, plane, &alignedw, &alignedh) != 0)
      return -1;

   uint32_t alignment = info->planes[plane].pitch_align;
   if (ubwc_enabled(descriptor)) {
      if (!info->planes[plane].meta_info) {
        fprintf(stderr,"UBWC not supported for this format \n");
        return -1;
      }
      alignment = info->planes[plane].meta_info->pitch_align;
   }

   uint32_t bpp = info->bpp;
   *stride = ALIGN(alignedw * bpp, alignment);

   return 0;
}

int get_data_plane_size(struct gbm_bufdesc *descriptor, int plane, uint32_t *plane_size) {
   *plane_size = 0;

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      fprintf(stderr,"Unsupported format\n");
      return -1;
   }

   struct format_info *info = get_format_info(format_name);
   if (!info)
      return -1;
    
   uint32_t alignedw = 0, alignedh = 0;
   if (get_aligned_width_and_height(descriptor, plane, &alignedw, &alignedh) != 0)
      return -1;

   uint32_t stride = alignedw;
   if (get_stride(descriptor, plane, &stride) != 0)
      return -1;

   if (ubwc_enabled(descriptor)) {
      if (!info->planes[plane].meta_info) {
         fprintf(stderr,"UBWC not supported for this format \n");
         return -1;
      }
   }

   uint32_t alignment = (ubwc_enabled(descriptor)) ? info->planes[plane].meta_info->size_align : info->size_align;

   *plane_size = ALIGN(stride * alignedh, alignment);
   return 0;
}

int get_meta_buffer_size(struct gbm_bufdesc *descriptor, int plane, uint32_t *metabuffer_size)
{
   if (!descriptor || !metabuffer_size)
      return -1;

   uint32_t width = descriptor->width;
   uint32_t height = descriptor->height;
   get_plane_width_and_height(descriptor->format, plane, &width, &height);

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      fprintf(stderr,"Unsupported format\n");
      return -1;
   }

   struct format_info *info = get_format_info(format_name);
   if (!info)
      return -1;

   if (!ubwc_enabled(descriptor)) {
      *metabuffer_size = 0;
      return 0;
   }
   if (!info->planes[plane].meta_info) {
      fprintf(stderr,"UBWC not supported for this format \n");
      return -1;
   }

   uint32_t block_width = info->planes[plane].meta_info->block_width;
   uint32_t block_height = info->planes[plane].meta_info->block_height;

   int meta_height = ALIGN(((height + block_height - 1) / block_height), 16);
   int meta_width = ALIGN(((width + block_width - 1) / block_width), 64);
   *metabuffer_size = (unsigned int)ALIGN((meta_width * meta_height), 4096);

   return 0;
}

int get_size(struct gbm_bufdesc *descriptor, uint32_t *buffer_size)
{
   if (!descriptor || !buffer_size)
      return -1;

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      fprintf(stderr,"Unsupported format\n");
      return -1;
   }

   struct format_info *info = get_format_info(format_name);
   if (!info)
      return -1;

   uint32_t plane_count = info->plane_count;

   uint32_t size = 0;
   for (uint32_t i = 0; i < plane_count; i++) {
      uint32_t data_size = 0, ubwc_size = 0;
      if (get_data_plane_size(descriptor, i, &data_size) != 0)
         return -1;
      if (get_meta_buffer_size(descriptor, i, &ubwc_size) != 0)
         return -1;

      size += data_size + ubwc_size;
   }

   *buffer_size = size;
   return 0;
}

int get_plane_offset(struct gbm_bufdesc *descriptor, int plane, uint32_t *plane_offset)
{
   if (!descriptor || !plane_offset)
      return -1;

   const char *format_name = find_format_name(descriptor->format);
   if (!format_name) {
      fprintf(stderr,"Unsupported format\n");
      return -1;
   }

   struct format_info *info = get_format_info(format_name);
   if (!info || (plane > info->plane_count - 1))
      return -1;

   uint32_t offset = 0;
   for (uint32_t i = 0; i < plane; i++) {
      uint32_t data_size = 0, ubwc_size = 0;
      if (get_data_plane_size(descriptor, i, &data_size) != 0)
         return -1;
      if (get_meta_buffer_size(descriptor, i, &ubwc_size) != 0)
         return -1;

      offset += data_size + ubwc_size;
   }

   *plane_offset = offset;
   return 0;
}
