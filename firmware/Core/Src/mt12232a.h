/* MT-13332A LCD display driver
 * License: GPL
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "mt12232a_hal_stm32f1xx.h"

/* MT-13332A pinout:
 *   1  DB4   Data 4
 *   2  DB5   Data 5
 *   3  DB6   Data 6
 *   4  DB7   Data 7
 *   5  A0    Command (0) / Data (1)
 *   6  RD/WR Write (0) / Read (1)
 *   7  E     Strobe (Raise)
 *   8  DB3   Data 3
 *   9  DB2   Data 2
 *  10  DB1   Data 1
 *  11  DB0   Data 0
 *  12  GND   Ground
 *  13  NC
 *  14  VCC   5V
 *  15  LED-  Led -
 *  16  LED+  Led +
 *  17  RES   Reset (0)
 *  18  CS    Left  (1) / Right (0)
 */

/* Consts */

#define MT12232A_WIDTH (122u)
#define MT12232A_HEIGHT (32u)
#define MT12232A_POINTS_IN_BYTE (8u)

/* Driver object */
typedef struct Mt12232a {
    Mt12232aConfig config;
    Mt12232aHal hal;
    uint8_t screen[MT12232A_WIDTH * (MT12232A_HEIGHT / MT12232A_POINTS_IN_BYTE)];
    uint8_t current_screen[MT12232A_WIDTH * (MT12232A_HEIGHT / MT12232A_POINTS_IN_BYTE)];
} Mt12232a;

/* Init driver and display */
bool Mt12232aInit(Mt12232a *self, const Mt12232aConfig *config);

/* Get screen buffer */
uint8_t *Mt12232aGetScreenBuffer(Mt12232a *self);

/* Send image to display */
bool Mt12232aUpdateImage(Mt12232a *self);
