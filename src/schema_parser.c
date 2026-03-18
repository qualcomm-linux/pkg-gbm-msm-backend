/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "schema_parser.h"
#include "gbm_msm_int.h"

#ifndef LIBDIR
#define LIBDIR "/usr/lib"  // Default value if LIBDIR is not defined
#endif

int format_count = 0;
struct format_info *formats = NULL;

struct format_string_enum format_map[] = {
   {"ARGB_8888", GBM_FORMAT_ARGB8888},
   {"ABGR_8888", GBM_FORMAT_ABGR8888},
   {"RGBA_8888", GBM_FORMAT_RGBA8888},
   {"BGRA_8888", GBM_FORMAT_BGRA8888},
   {"XRGB_8888", GBM_FORMAT_XRGB8888},
   {"XBGR_8888", GBM_FORMAT_XBGR8888},
   {"RGBX_8888", GBM_FORMAT_RGBX8888},
   {"BGRX_8888", GBM_FORMAT_BGRX8888},
   {"RGB_888", GBM_FORMAT_RGB888},
   {"BGR_888", GBM_FORMAT_BGR888},
   {"RGB_565", GBM_FORMAT_RGB565},
   {"BGR_565", GBM_FORMAT_BGR565},
   {"ARGB_2101010", GBM_FORMAT_ARGB2101010},
   {"ABGR_2101010", GBM_FORMAT_ABGR2101010},
   {"RGBA_1010102", GBM_FORMAT_RGBA1010102},
   {"BGRA_1010102", GBM_FORMAT_BGRA1010102},
   {"XRGB_2101010", GBM_FORMAT_XRGB2101010},
   {"XBGR_2101010", GBM_FORMAT_XBGR2101010},
   {"RGBX_1010102", GBM_FORMAT_RGBX1010102},
   {"BGRX_1010102", GBM_FORMAT_BGRX1010102},
   {"NV_12", GBM_FORMAT_NV12},
   {"NV_21", GBM_FORMAT_NV21},
   {"XRGB_4444", GBM_FORMAT_XRGB4444},
   {"XBGR_4444", GBM_FORMAT_XBGR4444},
   {"RGBX_4444", GBM_FORMAT_RGBX4444},
   {"BGRX_4444", GBM_FORMAT_BGRX4444},
   {"ARGB_4444", GBM_FORMAT_ARGB4444},
   {"ABGR_4444", GBM_FORMAT_ABGR4444},
   {"RGBA_4444", GBM_FORMAT_RGBA4444},
   {"BGRA_4444", GBM_FORMAT_BGRA4444},
   {"XRGB_1555", GBM_FORMAT_XRGB1555},
   {"XBGR_1555", GBM_FORMAT_XBGR1555},
   {"RGBX_5551", GBM_FORMAT_RGBX5551},
   {"BGRX_5551", GBM_FORMAT_BGRX5551},
   {"ARGB_1555", GBM_FORMAT_ARGB1555},
   {"ABGR_1555", GBM_FORMAT_ABGR1555},
   {"RGBA_5551", GBM_FORMAT_RGBA5551},
   {"BGRA_5551", GBM_FORMAT_BGRA5551},
   {"XBGR_16161616", GBM_FORMAT_XBGR16161616},
   {"ABGR_16161616", GBM_FORMAT_ABGR16161616},
   {"XBGR_16161616F", GBM_FORMAT_XBGR16161616F},
   {"ABGR_16161616F", GBM_FORMAT_ABGR16161616F},
   {"R_8", GBM_FORMAT_R8},
   {"R_16", GBM_FORMAT_R16},
   {"GR_88", GBM_FORMAT_GR88},
   {"RG_1616", GBM_FORMAT_RG1616},
   {"GR_1616", GBM_FORMAT_GR1616},
   {"RGB_332", GBM_FORMAT_RGB332},
   {"BGR_233", GBM_FORMAT_BGR233},
   {"C_8", GBM_FORMAT_C8},
   {"YUYV", GBM_FORMAT_YUYV},
   {"YVYU", GBM_FORMAT_YVYU},
   {"UYVY", GBM_FORMAT_UYVY},
   {"VYUY", GBM_FORMAT_VYUY},
   {"AYUV", GBM_FORMAT_AYUV},
   {"NV_16", GBM_FORMAT_NV16},
   {"NV_61", GBM_FORMAT_NV61},
   {"YUV_410", GBM_FORMAT_YUV410},
   {"YVU_410", GBM_FORMAT_YVU410},
   {"YUV_411", GBM_FORMAT_YUV411},
   {"YVU_411", GBM_FORMAT_YVU411},
   {"YUV_420", GBM_FORMAT_YUV420},
   {"YVU_420", GBM_FORMAT_YVU420},
   {"YUV_422", GBM_FORMAT_YUV422},
   {"YVU_422", GBM_FORMAT_YVU422},
   {"YUV_444", GBM_FORMAT_YUV444},
   {"YVU_444", GBM_FORMAT_YVU444},
   {"RGBA_32323232F", GBM_FORMAT_RGBA32323232F}
};

const char* find_format_name(uint32_t format_enum)
{
   size_t map_size = sizeof(format_map) / sizeof(format_map[0]);
   for (size_t i = 0; i < map_size; ++i) {
      if (format_map[i].format_enum == format_enum) {
         return format_map[i].format_name;
      }
   }
   return NULL;
}

void parse_meta(xmlNode *node, struct meta_info *meta)
{
   xmlNode *cur_node = NULL;
   for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if (cur_node->type == XML_ELEMENT_NODE) {
         if (strcmp((char *)cur_node->name, "block_width") == 0) {
            meta->block_width = atoi((char *)xmlNodeGetContent(cur_node));
         } else if (strcmp((char *)cur_node->name, "block_height") == 0) {
            meta->block_height = atoi((char *)xmlNodeGetContent(cur_node));
         } else if (strcmp((char *)cur_node->name, "pitch_align") == 0) {
            meta->pitch_align = atoi((char *)xmlNodeGetContent(cur_node));
         } else if (strcmp((char *)cur_node->name, "height_align") == 0) {
            meta->height_align = atoi((char *)xmlNodeGetContent(cur_node));
         } else if (strcmp((char *)cur_node->name, "size_align") == 0) {
            meta->size_align = atoi((char *)xmlNodeGetContent(cur_node));
         }
      }
   }
}

void parse_plane(xmlNode *node, struct plane_info *plane)
{
   xmlNode *cur_node = NULL;
   plane->meta_info = NULL;

   for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if (cur_node->type == XML_ELEMENT_NODE) {
         if (strcmp((char *)cur_node->name, "plane_name") == 0) {
            strlcpy(plane->plane_name, (char *)xmlNodeGetContent(cur_node), sizeof(plane->plane_name));
         } else if (strcmp((char *)cur_node->name, "pitch_align") == 0) {
            plane->pitch_align = atoi((char *)xmlNodeGetContent(cur_node));
         } else if (strcmp((char *)cur_node->name, "height_align") == 0) {
            plane->height_align = atoi((char *)xmlNodeGetContent(cur_node));
         } else if (strcmp((char *)cur_node->name, "meta_info") == 0) {
            plane->meta_info = calloc(1, sizeof(struct meta_info));
            parse_meta(cur_node, plane->meta_info);
         }
      }
   }
}

void parse_format(xmlNode *node, struct format_info *format) {
    xmlNode *cur_node = NULL;
    format->plane_count = 0;
    format->planes = NULL;

    for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if (strcmp((char *)cur_node->name, "format_name") == 0) {
                strlcpy(format->format_name, (char *)xmlNodeGetContent(cur_node), sizeof(format->format_name));
            } else if (strcmp((char *)cur_node->name, "constraints") == 0) {
                for (xmlNode *constraint_node = cur_node->children; constraint_node;
		     constraint_node = constraint_node->next) {
                    if (constraint_node->type == XML_ELEMENT_NODE) {
                        if (strcmp((char *)constraint_node->name, "size_align") == 0) {
                            format->size_align = atoi((char *)xmlNodeGetContent(constraint_node));
                        } else if (strcmp((char *)constraint_node->name, "bpp") == 0) {
                            format->bpp = atoi((char *)xmlNodeGetContent(constraint_node));
                        } else if (strcmp((char *)constraint_node->name, "planes") == 0) {
                            for (xmlNode *plane_node = constraint_node->children; plane_node;
			         plane_node = plane_node->next) {
                                if (plane_node->type == XML_ELEMENT_NODE &&
				    strcmp((char *)plane_node->name, "plane") == 0) {
                                    format->planes = realloc(format->planes, 
				                (format->plane_count + 1) * sizeof(struct plane_info));
                                    parse_plane(plane_node, &format->planes[format->plane_count]);
                                    format->plane_count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

struct format_info* parse_xml(const char *filename)
{
   xmlDoc *doc = NULL;
   xmlNode *root_element = NULL;

   doc = xmlReadFile(filename, NULL, 0);
   if (doc == NULL) {
      printf("Could not parse the XML file\n");
      return NULL;
   }

   root_element = xmlDocGetRootElement(doc);
   xmlNode *cur_node = NULL;
   struct format_info *formats = NULL;
   format_count = 0;

   for (cur_node = root_element->children; cur_node; cur_node = cur_node->next) {
      if (cur_node->type == XML_ELEMENT_NODE && strcmp((char *)cur_node->name, "format") == 0) {
         formats = realloc(formats, (format_count + 1) * sizeof(struct format_info));
         parse_format(cur_node, &formats[format_count]);
         format_count++;
      }
   }

   xmlFreeDoc(doc);
   xmlCleanupParser();

   return formats;
}

void init_xml_schema(void)
{
   char filename[256];
   // @todo: decide the filename to be parsed based on the SoC variant
   snprintf(filename, sizeof(filename), "%s/gbm/default_fmt_alignment.xml", LIBDIR);
   formats = parse_xml(filename);
}

struct format_info* get_format_info(const char *format_name) {
   struct format_info *info = NULL;
   for (int i = 0; i < format_count; i++) {
      if (strcmp(formats[i].format_name, format_name) == 0) {
         info = &formats[i];
      }
   }
   return info;
}

