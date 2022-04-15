/* Graphics functions for a display like MT-12232A
 * MISRA
 * License: GPL
 * Version: 16-Sep-2021
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#include "graphics.h"
#include <string.h>
#include <assert.h>

/* TODO(Any): Check overflow */

#define POINTS_IN_BYTE (8u)
#define FULL_BYTE_MASK (0xFFu)

static void AndMask(uint8_t* destination, uint32_t width, uint8_t and_mask) {
    assert(destination != NULL);
    if (and_mask != FULL_BYTE_MASK) {
        uint8_t* destination_cursor = destination;
        uint32_t i = 0u;
        for (i = 0u; i < width; i++) {
            *destination_cursor &= and_mask;
            destination_cursor++;
        }
    }
}

static void XorLeftShiftAndMask(uint8_t* destination, const uint8_t* source, uint32_t width, uint8_t shift,
                                uint8_t and_mask) {
    assert(destination != NULL);
    assert(source != NULL);
    if ((and_mask != 0u) && (shift < 8u)) {
        uint8_t* destination_cursor = destination;
        const uint8_t* source_cursor = source;
        uint32_t i = 0u;
        for (i = 0u; i < width; i++) {
            *destination_cursor ^= (((*source_cursor) << shift) & and_mask);
            destination_cursor++;
            source_cursor++;
        }
    }
}

static void XorRightShiftAndMask(uint8_t* destination, const uint8_t* source, uint32_t width, uint8_t shift,
                                 uint8_t and_mask) {
    assert(destination != NULL);
    assert(source != NULL);
    uint8_t* destination_cursor = destination;
    const uint8_t* source_cursor = source;
    uint32_t i = 0u;
    for (i = 0u; i < width; i++) {
        *destination_cursor ^= (((*source_cursor) >> shift) & and_mask);
        destination_cursor++;
        source_cursor++;
    }
}

void ClearRect(GraphicsContext* context, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    /* Check parameters */
    assert(context != NULL);

    /* TODO: Clear rect */

    /* Off-screen */
    if ((x < context->width) && (y < context->height)) {
        /* Partially off-screen */
        const uint32_t limited_width = (width < (context->width - x)) ? width : (context->width - x);
        const uint32_t limited_height = (height < (context->height - y)) ? height : (context->height - y);

        /* Bit masks and bytes height */
        const uint8_t char_shift_1 = y % POINTS_IN_BYTE;
        const uint32_t byte_height = (char_shift_1 + limited_height + (POINTS_IN_BYTE - 1u)) / POINTS_IN_BYTE;
        assert(byte_height > 0u);
        static const uint8_t first_and_masks[] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
        const uint8_t first_byte_and_mask = first_and_masks[y % POINTS_IN_BYTE];
        static const uint8_t last_and_masks[] = {0x00, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};
        const uint8_t last_byte_and_mask = last_and_masks[(y + limited_height) % POINTS_IN_BYTE];

        /* Variables */
        uint8_t* const destination = context->buffer;
        uint32_t destination_position = x + ((y / POINTS_IN_BYTE) * context->bytes_per_line);

        if (byte_height == 1u) {
            AndMask(&destination[destination_position], limited_width, first_byte_and_mask | last_byte_and_mask);
        } else {
            /* Top byte of character */
            AndMask(&destination[destination_position], limited_width, first_byte_and_mask);
            destination_position += context->bytes_per_line;
            /* Middle bytes of character */
            uint32_t j = 0u;
            for (j = byte_height; j > 2u; j--) { /* 2 is the top and bottom lines */
                (void)memset(&destination[destination_position], 0, limited_width);
                destination_position += context->bytes_per_line;
            }
            /* Bottom byte of character */
            AndMask(&destination[destination_position], limited_width, last_byte_and_mask);
        }
    }
}

void DrawText(GraphicsContext* context, const Font* font, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
              const char text[]) {
    /* Check parameters */
    assert(context != NULL);
    assert(font != NULL);
    assert(text != NULL);

    /* Clear rect */
    ClearRect(context, x, y, width, height);

    /* Off-screen */
    if ((x < context->width) && (y < context->height)) {
        /* Partially off-screen */
        const uint32_t limited_width = (width < (context->width - x)) ? width : (context->width - x);
        const uint32_t limited_height = (height < (context->height - y)) ? height : (context->height - y);

        /* Char height */
        const uint32_t char_height = (limited_height < font->char_height) ? limited_height : font->char_height;
        assert(char_height > 0u);

        /* Bit masks and bytes height */
        const uint8_t char_shift_1 = y % POINTS_IN_BYTE;
        const uint8_t char_shift_2 = POINTS_IN_BYTE - char_shift_1;
        const uint32_t byte_height = (char_shift_1 + char_height + (POINTS_IN_BYTE - 1u)) / POINTS_IN_BYTE;
        assert(byte_height > 0u);
        static const uint8_t and_masks[] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
        const uint8_t last_byte_and_mask = and_masks[(y + char_height) % POINTS_IN_BYTE];

        /* Variables */
        const uint8_t* const source = font->data;
        uint8_t* const destination = context->buffer;
        uint32_t destination_position = x + ((y / POINTS_IN_BYTE) * context->bytes_per_line);

        uint32_t remain_width = limited_width;
        size_t i = 0;
        for (i = 0; text[i] != '\0'; i++) {
            /* Get char index */
            uint8_t current_char = (uint8_t)text[i];
            if ((current_char < font->first_char_code) ||
                ((current_char - font->first_char_code) >= font->chars_count)) {
                current_char = font->first_char_code;
            }
            uint32_t source_position = (uint32_t)(current_char - font->first_char_code) * font->bytes_per_char;
            const uint8_t char_width = font->data[source_position];
            source_position++;
            const uint32_t limited_char_width = (remain_width < char_width) ? remain_width : char_width;

            /* Draw character */
            if (byte_height == 1u) {
                /* 1 byte high character */
                XorLeftShiftAndMask(&destination[destination_position], &source[source_position], limited_char_width,
                                    char_shift_1, last_byte_and_mask);
            } else {
                uint32_t destination_char_position = destination_position;
                /* Top byte of character */
                XorLeftShiftAndMask(&destination[destination_char_position], &source[source_position],
                                    limited_char_width, char_shift_1, FULL_BYTE_MASK);
                destination_char_position += context->bytes_per_line;
                /* Middle bytes of character */
                uint32_t j = 0u;
                for (j = byte_height; j > 2u; j--) { /* 2 is the top and bottom lines */
                    XorRightShiftAndMask(&destination[destination_char_position], &source[source_position],
                                         limited_char_width, char_shift_2, FULL_BYTE_MASK);
                    source_position += font->char_width;
                    XorLeftShiftAndMask(&destination[destination_char_position], &source[source_position],
                                        limited_char_width, char_shift_1, FULL_BYTE_MASK);
                    destination_char_position += context->bytes_per_line;
                }
                /* Bottom byte of character */
                XorRightShiftAndMask(&destination[destination_char_position], &source[source_position],
                                     limited_char_width, char_shift_2, last_byte_and_mask);
                source_position += font->char_width;
                XorLeftShiftAndMask(&destination[destination_char_position], &source[source_position],
                                    limited_char_width, char_shift_1, last_byte_and_mask);
            }

            /* Next char position */
            destination_position += limited_char_width;
            remain_width -= limited_char_width;

            /* No more space */
            if (remain_width == 0u) {
                break;
            }
        }
    }
}
