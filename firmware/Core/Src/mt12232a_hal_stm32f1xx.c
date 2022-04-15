/* MT-13332A LCD display driver
 * License: GPL
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#include <assert.h>
#include "mt12232a_hal.h"
#include "mt12232a_hal_stm32f1xx.h"
#include "delay_cpu_cycles.h"

/* STM32F1 GPIO modes */

#define ALL_GPIO_INPUT (0x88888888u)     /* Input with pull-up / pull-down */
#define ALL_GPIO_OUTPUT_PP (0x33333333u) /* General purpose output push-pull, 50 MHz */

/* Other consts */

#define MT12232A_DATA_MASK (0xFFu)
#define MT12232A_DIRECTION_MASK (0xFFFFFFFFu)
#define BITS_IN_UINT32 (32u)

/* STM32 specific functions */

void Mt12232aInitHal(Mt12232a *self) {
    uint64_t direction_mask = 0u;

    /* Check parameters */
    assert(self != NULL);

    self->hal.gpio_value = UINT16_MAX;
    self->hal.data_mask = ((uint16_t)MT12232A_DATA_MASK) << self->config.db0_pin_number;
    self->hal.gpio_mask = self->hal.data_mask | self->config.reset_pin_mask | self->config.cs_pin_mask |
                          self->config.a0_pin_mask | self->config.rd_wr_pin_mask | self->config.e_pin_mask;
    direction_mask = ((uint64_t)MT12232A_DIRECTION_MASK) << (self->config.db0_pin_number * 4u);
    self->hal.direction_unused_l = ~(uint32_t)direction_mask;
    self->hal.direction_unused_h = ~(uint32_t)(direction_mask >> BITS_IN_UINT32);
    self->hal.direction_input_l = ALL_GPIO_INPUT & (uint32_t)direction_mask;
    self->hal.direction_input_h = ALL_GPIO_INPUT & (uint32_t)(direction_mask >> BITS_IN_UINT32);
    self->hal.direction_output_l = ALL_GPIO_OUTPUT_PP & (uint32_t)direction_mask;
    self->hal.direction_output_h = ALL_GPIO_OUTPUT_PP & (uint32_t)(direction_mask >> BITS_IN_UINT32);
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

    self->hal.gpio_value &= ~reset;
    self->hal.gpio_value |= set;

    /* TODO: SDR */
    EnterCriticalScetion();
    self->config.gpio->ODR =
        (self->hal.gpio_value & self->hal.gpio_mask) | (self->config.gpio->ODR & ~self->hal.gpio_mask);
    ExitCriticalScetion();

    DelayCpuCycles(delay);
}

void Mt12232aSetInputDataDirection(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    EnterCriticalScetion();
    self->config.gpio->CRL = self->hal.direction_input_l | (self->config.gpio->CRL & self->hal.direction_unused_l);
    self->config.gpio->CRH = self->hal.direction_input_h | (self->config.gpio->CRH & self->hal.direction_unused_h);
    ExitCriticalScetion();
}

void Mt12232aSetOutputDataDirection(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    EnterCriticalScetion();
    self->config.gpio->CRL = self->hal.direction_output_l | (self->config.gpio->CRL & self->hal.direction_unused_l);
    self->config.gpio->CRH = self->hal.direction_output_h | (self->config.gpio->CRH & self->hal.direction_unused_h);
    ExitCriticalScetion();
}

uint8_t Mt12232aReadData(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    return self->config.gpio->IDR >> self->config.db0_pin_number;
}
