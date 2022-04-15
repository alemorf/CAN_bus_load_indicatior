/* MT-13332A LCD display driver
 * License: GPL
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#ifndef MT12232A_HAL_STM32F4XX_H_
#define MT12232A_HAL_STM32F4XX_H_

#include <stdint.h>

typedef struct {
    GPIO_TypeDef *gpio;
    uint16_t db0_pin_number;
    uint16_t reset_pin_mask;
    uint16_t cs_pin_mask;
    uint16_t a0_pin_mask;
    uint16_t rd_wr_pin_mask;
    uint16_t e_pin_mask;
} Mt12232aConfig;

typedef struct {
    uint16_t gpio_value;
    uint16_t gpio_mask;
    uint16_t data_mask;
    uint32_t direction_unused;
    uint32_t direction_input;
    uint32_t direction_output;
} Mt12232aHal;

#endif /* MT12232A_HAL_STM32F4XX_H_ */
