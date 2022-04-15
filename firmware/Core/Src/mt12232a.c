/* MT-12232A LCD display driver from STM32
 * MISRA
 * License: GPL
 * Version: 11-Sep-2021
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "delay_cpu_cycles.h"
#include "mt12232a.h"
#include "mt12232a_hal.h"
#include "mt12232a_hal_stm32f1xx.h"

/* Macro */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

/* Commands */

#define MT12232A_COMMAND_DISPLAY_OFF (0xAEu)                             /* 10101110 */
#define MT12232A_COMMAND_DISPLAY_ON (0xAFu)                              /* 10101111 */
#define MT12232A_COMMAND_SET_DISPLAY_START_LINE(X) (0xC0u | ((X)&0x1Fu)) /* 110xxxxx, x=line 0..31 */
#define MT12232A_COMMAND_SET_PAGE(X) (0xB8u | ((X)&0x03u))               /* 101110xx, x=page 0..3 */
#define MT12232A_COMMAND_SET_ADDRESS(X) (0x00u | ((X)&0x7Fu))            /* 0xxxxxxx, x=column address 0..79 */
#define MT12232A_COMMAND_ADC_SELECT_FORWARD (0xA0u)                      /* 10100000 */
#define MT12232A_COMMAND_ADC_SELECT_BACKWARD (0xA1u)                     /* 10100001 */
#define MT12232A_COMMAND_STATIC_DRIVE_OFF (0xA4u)                        /* 10100100 */
#define MT12232A_COMMAND_STATIC_DRIVE_ON (0xA5u)                         /* 10100101 */
#define MT12232A_COMMAND_DUTY_SELECT_OFF (0xA8u)                         /* 10101000 */
#define MT12232A_COMMAND_DUTY_SELECT_ON (0xA9u)                          /* 10101001 */
#define MT12232A_COMMAND_READ_MODIFY_WRITE_ON (0xE0u)                    /* 11100000 */
#define MT12232A_COMMAND_READ_MODIFY_WRITE_OFF (0xEEu)                   /* 11101110 */
#define MT12232A_COMMAND_RESET (0xE2u)                                   /* 11100010 */

/* State */

#define MT12232A_STATUS_BUSY (0x80u)           /* 0 = ready, 1 = busy */
#define MT12232A_STATUS_ADC (0x40u)            /* 0 = backward, 1 = forward */
#define MT12232A_STATUS_DISPLAY_ON_OFF (0x20u) /* 0 = on, 1 = off */
#define MT12232A_STATUS_RESET (0x10u)          /* 0 = work, 1 = reset */

/* Delay */

/*    Read algorithm             Write algorithm
 * 1) CS = ?, A0 = ?, RW = 1     CS = ?, A0 = ?, RW = 0
 * 2) Delay Taw 100 ns           Delay Taw 100 ns
 * 3) E = 0                      DB = ?, E = 0
 * 4) Delay Tew 300 ns           Delay Tew 300 ns
 * 5) Read DB
 * 6) E = 1                      E = 1
 * 7) Delay Tw 1600 ns           Delay Tr 1600 ns
 */

#define MT12232A_DELAY_RESET_MS (10u)
#define MT12232A_DELAY_TAW_NS (100u)
#define MT12232A_DELAY_TEW_NS (300u)
#define MT12232A_DELAY_TCYC_NS (2000u)
#define MT12232A_DELAY_TEND_NS (MT12232A_DELAY_TCYC_NS - MT12232A_DELAY_TAW_NS - MT12232A_DELAY_TEW_NS)
#define MT12232A_WAIT_STATE_TIMEOUT_NS (1000000u)

#define MT12232A_DELAY_RESET_CPU_CYCLES (MS_TO_CPU_TICKS(MT12232A_DELAY_RESET_MS))
#define MT12232A_DELAY_TAW_CPU_CYCLES (NS_TO_CPU_TICKS(MT12232A_DELAY_TAW_NS))
#define MT12232A_DELAY_TEW_CPU_CYCLES (NS_TO_CPU_TICKS(MT12232A_DELAY_TEW_NS))
#define MT12232A_DELAY_TEND_CPU_CYCLES (NS_TO_CPU_TICKS(MT12232A_DELAY_TEND_NS))
#define MT12232A_WAIT_STATE_TIMEOUT_LOOPS (MT12232A_WAIT_STATE_TIMEOUT_NS / MT12232A_DELAY_TCYC_NS)

/* Internal consts */

#define MT12232A_BANK_COUNT (2u)
#define MT12232A_BANK_WIDTH (61u)
#define MT12232A_8LINE_COUNT (4u)
#define MT12232A_DATA_MASK (0xFFu)
#define MT12232A_DIRECTION_MASK (0xFFFFu)

static const uint8_t mt12232a_bank_x_offset[] = {19u, 0u};

static const uint8_t mt12232a_bank_adc[] = {MT12232A_COMMAND_ADC_SELECT_BACKWARD, MT12232A_COMMAND_ADC_SELECT_FORWARD};

/* Functions */

static uint8_t Mt12232aReadStatus(Mt12232a *self) {
    uint8_t status = 0u;

    /* Check parameters */
    assert(self != NULL);

    /* Read */
    Mt12232aSetInputDataDirection(self);
    Mt12232aUpdateOutData(self, self->config.a0_pin_mask, self->config.rd_wr_pin_mask, MT12232A_DELAY_TAW_CPU_CYCLES);
    Mt12232aUpdateOutData(self, self->config.e_pin_mask, 0, MT12232A_DELAY_TEW_CPU_CYCLES);
    status = Mt12232aReadData(self);
    Mt12232aUpdateOutData(self, 0, self->config.e_pin_mask, MT12232A_DELAY_TEND_CPU_CYCLES);
    return status;
}

static bool Mt12232aWaitForReady(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    uint32_t tries = MT12232A_WAIT_STATE_TIMEOUT_LOOPS;
    for (;;) {
        const uint8_t status = Mt12232aReadStatus(self);
        if ((status & (MT12232A_STATUS_BUSY | MT12232A_STATUS_RESET)) == 0u) {
            return true;
        }

        tries--;
        if (tries == 0u) {
            return false;
        }
    }
}

static bool Mt12232aWrite(Mt12232a *self, uint8_t a0, uint8_t command) {
    /* Check parameters */
    assert(self != NULL);

    /* Wait for ready */
    if (Mt12232aWaitForReady(self) == false) {
        return false;
    }

    /* Write */
    Mt12232aSetOutputDataDirection(self);
    if (a0 == 0u) {
        Mt12232aUpdateOutData(self, self->config.a0_pin_mask | self->config.rd_wr_pin_mask, 0u,
                              MT12232A_DELAY_TAW_CPU_CYCLES);
    } else {
        Mt12232aUpdateOutData(self, self->config.rd_wr_pin_mask, self->config.a0_pin_mask,
                              MT12232A_DELAY_TAW_CPU_CYCLES);
    }
    Mt12232aUpdateOutData(self, self->config.e_pin_mask | self->hal.data_mask,
                          ((uint16_t)command) << self->config.db0_pin_number, MT12232A_DELAY_TEW_CPU_CYCLES);
    Mt12232aUpdateOutData(self, 0, self->config.e_pin_mask, MT12232A_DELAY_TEND_CPU_CYCLES);
    return true;
}

static inline void Mt12232aSelectBank(Mt12232a *self, uint8_t bank_number) {
    /* Check parameters */
    assert(self != NULL);
    assert(bank_number < MT12232A_BANK_COUNT);

    if (bank_number == 0u) {
        Mt12232aUpdateOutData(self, 0, self->config.cs_pin_mask, 0);
    } else {
        Mt12232aUpdateOutData(self, self->config.cs_pin_mask, 0, 0);
    }
}

bool Mt12232aUpdateImage(Mt12232a *self) {
    uint8_t bank_number = 0u;

    /* Check parameters */
    assert(self != NULL);

    for (bank_number = 0; bank_number < MT12232A_BANK_COUNT; bank_number++) {
        const uint8_t x_offset = mt12232a_bank_x_offset[bank_number];
        uint8_t y = 0u;
        Mt12232aSelectBank(self, bank_number);
        for (y = 0; y < MT12232A_8LINE_COUNT; y++) {
            uint8_t x = 0u;
            uint16_t address = (y * MT12232A_WIDTH) + (bank_number * MT12232A_BANK_WIDTH);
            if (Mt12232aWrite(self, 0u, MT12232A_COMMAND_SET_PAGE(y)) == false) {
                return false;
            }
            for (x = 0; x < MT12232A_BANK_WIDTH; x++) {
                const uint8_t byte = self->screen[address];
                if (self->current_screen[address] != byte) {
                    self->current_screen[address] = byte;
                    if (Mt12232aWrite(self, 0u, MT12232A_COMMAND_SET_ADDRESS(x_offset + x)) == false) {
                        return false;
                    }
                    if (Mt12232aWrite(self, 1u, byte) == false) {
                        return false;
                    }
                }
                address++;
            }
        }
    }
    return true;
}

bool Mt12232aInit(Mt12232a *self, const Mt12232aConfig *config) {
    uint8_t bank_number = 0u;
    bool done = true;

    /* Check parameters */
    assert(self != NULL);
    assert(config != NULL);

    /* Init variables */
    (void)memset(self->current_screen, UINT8_MAX, sizeof(self->current_screen));
    (void)memset(self->screen, 0, sizeof(self->screen));
    self->config = *config;
    Mt12232aInitHal(self);

    /* Reset display */
    Mt12232aUpdateOutData(self, config->reset_pin_mask, config->e_pin_mask, MT12232A_DELAY_RESET_CPU_CYCLES);
    Mt12232aUpdateOutData(self, 0u, config->reset_pin_mask, MT12232A_DELAY_RESET_CPU_CYCLES);

    /* Configure controllers */
    for (bank_number = 0u; bank_number < MT12232A_BANK_COUNT; bank_number++) {
        Mt12232aSelectBank(self, bank_number);
        done = Mt12232aWrite(self, 0u, MT12232A_COMMAND_READ_MODIFY_WRITE_OFF);
        if (done != false) {
            done = Mt12232aWrite(self, 0u, MT12232A_COMMAND_STATIC_DRIVE_OFF);
        }
        if (done != false) {
            done = Mt12232aWrite(self, 0u, MT12232A_COMMAND_DUTY_SELECT_ON);
        }
        if (done != false) {
            done = Mt12232aWrite(self, 0u, MT12232A_COMMAND_DISPLAY_ON);
        }
        if (done != false) {
            done = Mt12232aWrite(self, 0u, mt12232a_bank_adc[bank_number]);
        }
        if (done != false) {
            done = Mt12232aWrite(self, 0u, MT12232A_COMMAND_RESET);
        }
        if (done != false) {
            done = Mt12232aWrite(self, 0u, MT12232A_COMMAND_SET_DISPLAY_START_LINE(0u));
        }
        if (done == false) {
            break;
        }
    }

    /* Clear screen */
    if (done != false) {
        done = Mt12232aUpdateImage(self);
    }

    return done;
}

uint8_t *Mt12232aGetScreenBuffer(Mt12232a *self) {
    /* Check parameters */
    assert(self != NULL);

    return self->screen;
}
