/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SCHEMA_PARSER_H
#define SCHEMA_PARSER_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "gbm_msm_int.h"
#include <gbm.h>

struct format_string_enum {
   const char *format_name;
   uint32_t format_enum;
};

struct meta_info {
    uint32_t block_width;
    uint32_t block_height;
    uint32_t pitch_align;
    uint32_t height_align;
    uint32_t size_align;
};

struct plane_info {
    char plane_name[50];
    uint32_t pitch_align;
    uint32_t height_align;
    struct meta_info *meta_info;
};

struct format_info {
    char format_name[50];
    uint32_t size_align;
    struct plane_info *planes;
    uint32_t plane_count;
    uint32_t bpp;
};

void init_xml_schema(void);
void parse_meta(xmlNode *node, struct meta_info *meta);
void parse_plane(xmlNode *node, struct plane_info *plane);
void parse_format(xmlNode *node, struct format_info *format);
struct format_info* parse_xml(const char *filename);
struct format_info* get_format_info(const char *format_name);
const char* find_format_name(uint32_t format_enum);

#endif // SCHEMA_PARSER_H

