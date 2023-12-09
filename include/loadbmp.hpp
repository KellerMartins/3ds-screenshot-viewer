// Code repurposed to load bmp to citro3D texture buffer. Original header below:

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

#include <citro3d.h>

#include <functional>
#include <string>

LOADBMP_API unsigned int loadbmp(std::string filename, std::function<unsigned int(unsigned int, unsigned int, unsigned int, char *)> callback);
LOADBMP_API unsigned int loadbmp_to_texture(std::string filename, C3D_Tex *tex, u32 stride = 1);

#ifdef LOADBMP_IMPLEMENTATION

#include <stdio.h>  /* fopen(), fread(), fclose() */
#include <string.h> /* memset()*/

#include <algorithm>
#include <string>
#include <vector>

LOADBMP_API unsigned int loadbmp(std::string filename, std::function<unsigned int(unsigned int, unsigned int, unsigned int, char *)> callback) {
    FILE *f = fopen(filename.c_str(), "rb");

    if (!f) return LOADBMP_FILE_NOT_FOUND;

    u8 bmp_file_header[14];
    u8 bmp_info_header[40];

    u32 w, h, c, num_bytes;

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

    num_bytes = ((w * LOADBMP_RGB + 3) & ~0x03) * h;

    std::vector<char> bmp_img(num_bytes);
    fread(bmp_img.data(), 1, num_bytes, f);
    fclose(f);

    return callback(w, h, c, bmp_img.data());
}

LOADBMP_API unsigned int loadbmp_to_texture(std::string filename, C3D_Tex *tex, u32 img_stride) {
    return loadbmp(filename, [tex, img_stride](unsigned int w, unsigned int h, unsigned int c, char *data) {
        u8 *buffer = reinterpret_cast<u8 *>(tex->data);
        u32 buffer_width = tex->width;

        u32 height = std::min(static_cast<u16>(h), tex->height);
        u32 width = std::min(static_cast<u16>(w), tex->width);
        u32 stride = img_stride ? img_stride : 1;

        for (u32 y = 0; y < height; y++) {
            for (u32 x = 0; x < width; x++) {
                u32 dst_pos = ((((y >> 3) * (buffer_width >> 3) + (x >> 3)) << 6) +
                               ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) *
                              LOADBMP_RGB;
                u32 src_pos = (((h - 1) - y * stride) * w + x * stride) * LOADBMP_RGB;

                buffer[dst_pos] = data[src_pos];
                buffer[dst_pos + 1] = data[src_pos + 1];
                buffer[dst_pos + 2] = data[src_pos + 2];
            }
        }

        C3D_TexFlush(tex);

        return LOADBMP_NO_ERROR;
    });
}

#endif

#endif