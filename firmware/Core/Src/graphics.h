/* Graphics functions for a display like MT-12232A
 * MISRA
 * License: GPL
 * Version: 16-Sep-2021
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#ifndef CORE_SRC_GRAPHICS_H_
#define CORE_SRC_GRAPHICS_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* buffer;
    uint32_t bytes_per_line;
    uint32_t width;
    uint32_t height;
} GraphicsContext;

typedef struct {
    const uint8_t* data;
    uint8_t first_char_code;
    uint16_t chars_count; /* Max 0xFF */
    uint32_t bytes_per_char;
    uint32_t char_width;
    uint32_t char_height;
} Font;

void ClearRect(GraphicsContext* context, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

void DrawText(GraphicsContext* context, const Font* font, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
              const char text[]);

#endif /* CORE_SRC_GRAPHICS_H_ */
