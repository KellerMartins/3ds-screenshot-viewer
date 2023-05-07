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

#ifdef LOADBMP_IMPLEMENTATION
#define LOADBMP_API
#else
#define LOADBMP_API extern
#endif

#include <citro3d.h>

#include <string>

LOADBMP_API unsigned int loadbmp_to_texture(std::string filename, C3D_Tex *tex, unsigned int width, unsigned int height, unsigned int stride = 1);

#ifdef LOADBMP_IMPLEMENTATION

#include <stdio.h>  /* fopen(), fread(), fclose() */
#include <string.h> /* memset()*/

#include <string>
#include <vector>

LOADBMP_API unsigned int loadbmp_to_texture(std::string filename, C3D_Tex *tex, unsigned int width, unsigned int height, unsigned int stride) {
    FILE *f = fopen(filename.c_str(), "rb");

    if (!f) return LOADBMP_FILE_NOT_FOUND;

    u8 bmp_file_header[14];
    u8 bmp_info_header[40];

    thread_local std::vector<char> bmp_img;

    u32 w, h;

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

    if ((bmp_info_header[14] != 24) && (bmp_info_header[14] != 32)) {
        fclose(f);
        return LOADBMP_INVALID_BITS_PER_PIXEL;
    }

    w = (bmp_info_header[4] + (bmp_info_header[5] << 8) + (bmp_info_header[6] << 16) + (bmp_info_header[7] << 24));
    h = (bmp_info_header[8] + (bmp_info_header[9] << 8) + (bmp_info_header[10] << 16) + (bmp_info_header[11] << 24));

    bmp_img.resize(((w * 3 + 1) / 4) * 4 * h);
    fread(bmp_img.data(), 1, ((w * 3 + 1) / 4) * 4 * h, f);
    fclose(f);

    stride = stride ? stride : 1;

    u8 *buffer = reinterpret_cast<u8 *>(tex->data);
    u32 buffer_width = tex->width;

    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            u32 dst_pos = ((((y >> 3) * (buffer_width >> 3) + (x >> 3)) << 6) +
                           ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) *
                          3;
            u32 src_pos = ((h - y * stride - 1) * w + x * stride) * 3;

            buffer[dst_pos] = bmp_img[src_pos];
            buffer[dst_pos + 1] = bmp_img[src_pos + 1];
            buffer[dst_pos + 2] = bmp_img[src_pos + 2];
        }
    }

    C3D_TexFlush(tex);

    return LOADBMP_NO_ERROR;
}

#endif

#endif