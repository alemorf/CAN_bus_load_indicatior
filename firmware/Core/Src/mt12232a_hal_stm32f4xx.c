/* MT-13332A LCD display driver
 * License: GPL
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#include "mt12232a_hal_stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <assert.h>
#include "delay_cpu_cycles.h"

/* STM32F4 GPIO modes */

#define ALL_GPIO_INPUT (0x00000000u)
#define ALL_GPIO_OUTPUT_PP (0x55555555u)

/* Other consts */

#define MT12232A_DATA_MASK (0xFFu)
#define MT12232A_DIRECTION_MASK (0xFFFFu)

/* STM32 specific functions */

void Mt12232aInitHal(Mt12232a *self) {
    uint32_t direction_mask = 0u;

    /* Check parameters */
    assert(self != NULL);

    self->hal.gpio_value = UINT16_MAX;
    self->hal.data_mask = ((uint16_t)MT12232A_DATA_MASK) << self->config.db0_pin_number;
    self->hal.gpio_mask = self->hal.data_mask | self->config.reset_pin_mask | self->config.cs_pin_mask | self->config.a0_pin_mask |
                      self->config.rd_wr_pin_mask | self->config.e_pin_mask;
    direction_mask = ((uint32_t)MT12232A_DIRECTION_MASK) << (self->config.db0_pin_number * 2u);
    self->hal.direction_unused = ~direction_mask;
    self->hal.direction_input = ALL_GPIO_INPUT & direction_mask;
    self->hal.direction_output = ALL_GPIO_OUTPUT_PP & direction_mask;
}

static inline void EnterCriticalScetion() {
    /* TODO(Alemorf): Save IRQ state */
    __disable_irq();
}

static inline void ExitCriticalScetion() {
    __enable_irq();
}

void Mt12232aUpdateOutData(Mt12232a *self, uint16_t reset, uint16_t set, uint32_t delay) {
    /* Check parameters */
    assert(self != NULL);

    self->gpio_value &= ~reset;
    self->gpio_value |= set;

    /* TODO: SDR */
    EnterCriticalScetion();
    self->config.gpio->ODR = (self->gpio_value & self->gpio_mask) | (self->config.gpio->ODR & ~self->gpio_mask);
    ExitCriticalScetion();

    DelayCpuCycles(delay);
}

void Mt12232aSetInputDataDirection(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    EnterCriticalScetion();
    self->config.gpio->MODER = self->direction_input | (self->config.gpio->MODER & self->direction_unused);
    ExitCriticalScetion();
}

void Mt12232aSetOutputDataDirection(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    EnterCriticalScetion();
    self->config.gpio->MODER = self->direction_output | (self->config.gpio->MODER & self->direction_unused);
    ExitCriticalScetion();
}

uint8_t Mt12232aReadData(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    return self->config.gpio->IDR >> self->config.db0_pin_number;
}
