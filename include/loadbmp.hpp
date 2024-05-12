// Code repurposed to load bmp to citro2D image. Original header below:

// Author: Christian Vallentin <vallentin.source@gmail.com>
// Website: http://vallentin.dev
// Repository: https://github.com/vallentin/LoadBMP
//
// Date Created: January 03, 2014
// Last Modified: August 13, 2016
//
// Version: 1.1.0

// Copyright (c) 2014-2016 Christian Vallentin <vallentin.source@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

// Include loadbmp.hpp as following
// to create the implementation file.
//
// #define LOADBMP_IMPLEMENTATION
// #include "loadbmp.hpp"

#ifndef LOADBMP_H
#define LOADBMP_H

// Errors
#define LOADBMP_NO_ERROR 0

#define LOADBMP_OUT_OF_MEMORY 1

#define LOADBMP_FILE_NOT_FOUND 2
#define LOADBMP_FILE_OPERATION 3

#define LOADBMP_INVALID_FILE_FORMAT 4

#define LOADBMP_INVALID_SIGNATURE 5
#define LOADBMP_INVALID_BITS_PER_PIXEL 6

#define LOADBMP_RGB 3

#ifdef LOADBMP_IMPLEMENTATION
#define LOADBMP_API
#else
#define LOADBMP_API extern
#endif

#include <citro2d.h>

#include <functional>
#include <string>

LOADBMP_API unsigned int loadbmp_to_image(std::string filename, C2D_Image img);

#ifdef LOADBMP_IMPLEMENTATION

#include <stdio.h>  /* fopen(), fread(), fclose() */
#include <string.h> /* memset()*/

#include <algorithm>
#include <string>
#include <vector>

struct bmp_buffer {
    u32 width;
    u32 height;
    u32 channels;
    u32 padding;
    char *data;
};

constexpr u32 next_multiple_of_4(u32 x) { return ((x + 3) & ~0x03); }

LOADBMP_API unsigned int loadbmp(std::string filename, std::function<unsigned int(bmp_buffer bmp)> callback) {
    FILE *f = fopen(filename.c_str(), "rb");

    if (!f) return LOADBMP_FILE_NOT_FOUND;

    u8 bmp_file_header[14];
    u8 bmp_info_header[40];

    u32 w, h, c, num_bytes, padding;

    memset(bmp_file_header, 0, sizeof(bmp_file_header));
    memset(bmp_info_header, 0, sizeof(bmp_info_header));

    if (fread(bmp_file_header, sizeof(bmp_file_header), 1, f) == 0) {
        fclose(f);
        return LOADBMP_INVALID_FILE_FORMAT;
    }

    if (fread(bmp_info_header, sizeof(bmp_info_header), 1, f) == 0) {
        fclose(f);
        return LOADBMP_INVALID_FILE_FORMAT;
    }

    if ((bmp_file_header[0] != 'B') || (bmp_file_header[1] != 'M')) {
        fclose(f);
        return LOADBMP_INVALID_SIGNATURE;
    }

    if ((bmp_info_header[14] != 24)) {
        fclose(f);
        return LOADBMP_INVALID_BITS_PER_PIXEL;
    }

    w = (bmp_info_header[4] + (bmp_info_header[5] << 8) + (bmp_info_header[6] << 16) + (bmp_info_header[7] << 24));
    h = (bmp_info_header[8] + (bmp_info_header[9] << 8) + (bmp_info_header[10] << 16) + (bmp_info_header[11] << 24));
    c = bmp_info_header[14] / 8;

    padding = next_multiple_of_4(w * LOADBMP_RGB) - w * LOADBMP_RGB;
    num_bytes = (w * LOADBMP_RGB + padding) * h;

    std::vector<char> bmp_img(num_bytes);
    fread(bmp_img.data(), 1, num_bytes, f);
    fclose(f);

    return callback((bmp_buffer){w, h, c, padding, bmp_img.data()});
}

LOADBMP_API unsigned int loadbmp_to_image(std::string filename, C2D_Image img) {
    return loadbmp(filename, [img](bmp_buffer bmp) {
        u8 *buffer = reinterpret_cast<u8 *>(img.tex->data);
        u32 buffer_width = img.tex->width;

        float stride = std::max(static_cast<float>(bmp.height) / img.subtex->height, static_cast<float>(bmp.width) / img.subtex->width);
        u16 scaledWidth = static_cast<u16>(bmp.width / stride);
        u16 scaledHeight = static_cast<u16>(bmp.height / stride);

        u32 width = img.tex->width;
        u32 height = img.tex->height;

        u32 offset_x = (img.subtex->width - scaledWidth) / 2;
        u32 offset_y = (img.subtex->height - scaledHeight) / 2;

        for (u32 y = 0; y < height; y++) {
            for (u32 x = 0; x < width; x++) {
                u32 dst_pos = ((((y >> 3) * (buffer_width >> 3) + (x >> 3)) << 6) +
                               ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) *
                              LOADBMP_RGB;

                u32 src_x = (x - offset_x) * stride;
                u32 src_y = (y - offset_y) * stride;
                u32 src_pos = (src_x * LOADBMP_RGB) + ((bmp.height - 1) - src_y) * (bmp.width * LOADBMP_RGB + bmp.padding);

                if (src_x < 0 || src_y < 0 || src_x >= bmp.width || src_y >= bmp.height) {
                    buffer[dst_pos] = 0;
                    buffer[dst_pos + 1] = 0;
                    buffer[dst_pos + 2] = 0;
                } else {
                    buffer[dst_pos] = bmp.data[src_pos];
                    buffer[dst_pos + 1] = bmp.data[src_pos + 1];
                    buffer[dst_pos + 2] = bmp.data[src_pos + 2];
                }
            }
        }

        C3D_TexFlush(img.tex);

        return LOADBMP_NO_ERROR;
    });
}

#endif

#endif