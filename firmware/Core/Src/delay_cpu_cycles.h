/* MT-13332A LCD display driver from STM32
 * License: GPL
 * Copyright (c) Alemorf aleksey.f.morozov@gmail.com
 */

#ifndef CORE_SRC_DELAY_CPU_CYCLES_H_
#define CORE_SRC_DELAY_CPU_CYCLES_H_

#include "stm32f1xx_hal.h"

#define CPU_FREQ (64000000u)

#define NS_TO_CPU_TICKS(ns) ((uint32_t)(((((uint64_t)(ns)) * CPU_FREQ) + 999999999u) / 1000000000u))
#define US_TO_CPU_TICKS(us) ((uint32_t)(((((uint64_t)(us)) * CPU_FREQ) + 999999u) / 1000000u))
#define MS_TO_CPU_TICKS(ms) ((uint32_t)(((((uint64_t)(ms)) * CPU_FREQ) + 999u) / 1000u))

static inline void EnableDwt(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void DelayCpuCycles(uint32_t cycles) {
    if (cycles != 0u) {
        const uint32_t start = DWT->CYCCNT + cycles;
        while ((int32_t)(start - DWT->CYCCNT) > 0) {
        }
    }
}

static inline uint32_t GetCpuCycles() {
    return DWT->CYCCNT;
}

#endif /* CORE_SRC_DELAY_CPU_CYCLES_H_ */
