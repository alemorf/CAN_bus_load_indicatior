/* License: GPL
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "delay_cpu_cycles.h"
#include "mt12232a.h"
#include "my.h"
#include "graphics.h"
#include "fonts.h"
#include "main.h"
#include "math.h"

static Mt12232a mt12232a;
volatile uint32_t can_rx_counter = 0;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
    CAN_RxHeaderTypeDef can_message_header = {};
    uint8_t can_message_payload[8] = {};
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &can_message_header, can_message_payload) == HAL_OK) {
        /* TODO(Any): Process CAN message */
    }
}

static volatile uint32_t prev_exti_time = 0;
static volatile uint32_t can_active_time = 0;
static volatile uint32_t can_inactive_time = 0;
static const uint32_t can_playload_time = US_TO_CPU_TICKS(22);

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin_index) {
    (void)gpio_pin_index;

    const uint32_t now = DWT->CYCCNT;
    const uint32_t period = now - prev_exti_time;
    prev_exti_time = now;
    if (period < can_playload_time) {
        can_active_time += period;
    } else {
        can_inactive_time += period - can_playload_time;
        can_active_time += can_playload_time;
    }
}

static const Mt12232aConfig mt12232a_config = {/* clang-format off */
    .gpio = GPIOB,
    .db0_pin_number = 0u,
    .reset_pin_mask = GPIO_PIN_8,
    .cs_pin_mask = GPIO_PIN_9,
    .a0_pin_mask = GPIO_PIN_10,
    .rd_wr_pin_mask = GPIO_PIN_11,
    .e_pin_mask = GPIO_PIN_12
}; /* clang-format on */

#define GRAPH_Y (0u)
#define GRAPH_X (0u)
#define TEXT_WIDTH (16u + 16u)
#define GRAPH_WIDTH (MT12232A_WIDTH - TEXT_WIDTH)

static uint32_t can_active_time_prev = 0;
static uint32_t can_inactive_time_prev = 0;

void MyMain(void) {
    uint8_t* screen = NULL;
    EnableDwt();
    if (Mt12232aInit(&mt12232a, &mt12232a_config) == false) {
        Error_Handler();
    }
    screen = Mt12232aGetScreenBuffer(&mt12232a);
    assert(screen != NULL);

    GraphicsContext context;
    context.buffer = screen;
    context.bytes_per_line = MT12232A_WIDTH;
    context.width = MT12232A_WIDTH;
    context.height = MT12232A_HEIGHT;

    CAN_FilterTypeDef can_filter_config;
    can_filter_config.FilterBank = 0;
    can_filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter_config.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter_config.FilterIdHigh = 0x0000;
    can_filter_config.FilterIdLow = 0x0000;
    can_filter_config.FilterMaskIdHigh = 0x0000;
    can_filter_config.FilterMaskIdLow = 0x0000;
    can_filter_config.FilterFIFOAssignment = CAN_RX_FIFO0;
    can_filter_config.FilterActivation = ENABLE;
    can_filter_config.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan, &can_filter_config);

    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    // TODO(Any): Logo
    // DrawText(&context, &font_8x16, i % 16u, i / 16u, MT12232A_WIDTH, (i % 16u) + 1u, "Hello world!");

    for (;;) {
        /* Info */
        const uint32_t can_active_time_now = can_active_time;
        const uint32_t can_inactive_time_now = can_inactive_time;
        const uint32_t can_active_time_period = can_active_time_now - can_active_time_prev;
        const uint32_t can_inactive_time_period = can_inactive_time_now - can_inactive_time_prev;
        const uint32_t can_time_period = (can_active_time_period + can_inactive_time_period);
        can_active_time_prev = can_active_time_now;
        can_inactive_time_prev = can_inactive_time_now;
        const uint32_t value = ((can_active_time_period * 100u) + can_time_period - 1u) / can_time_period;

        /* Draw value */

        char text[3] = {};
        if (value >= 100u) {
            text[0] = ':';
            text[1] = ';';
            text[2] = '\0';
        } else {
            text[0] = (value >= 10u) ? ('0' + (value / 10u)) : ' ';
            text[1] = '0' + (value % 10u);
            text[2] = '\0';
        }
        DrawText(&context, &font_16x32, MT12232A_WIDTH - TEXT_WIDTH, 0, TEXT_WIDTH, MT12232A_HEIGHT, text);

#if 0
        char info[32];
        snprintf(info, sizeof(info), "%u %u", (unsigned)can_active_percent, can_active_time_period,
                 (unsigned)(can_active_time_period + can_inactive_time_period));
        DrawText(&context, &font_8x16, 0, 0, MT12232A_WIDTH - TEXT_WIDTH, 16, info);
#endif

        /* Draw graph */

#define GRAPH_HEIGHT (32u)
        uint8_t value32 = ((uint16_t)(value * GRAPH_HEIGHT) + 99u) / 100u;
        if (value32 > GRAPH_HEIGHT) {
            value32 = GRAPH_HEIGHT;
        }
        const uint32_t mask = (uint64_t)0xFFFFFFFFu << (32u - value32);

        uint32_t graph_address = GRAPH_X + (GRAPH_Y / MT12232A_POINTS_IN_BYTE) * MT12232A_WIDTH;
        // TODO(Any): Loop
        (void)memmove(&screen[graph_address], &screen[graph_address + 1u], GRAPH_WIDTH - 1u);
        graph_address += MT12232A_WIDTH;
        (void)memmove(&screen[graph_address], &screen[graph_address + 1u], GRAPH_WIDTH - 1u);
        graph_address += MT12232A_WIDTH;
        (void)memmove(&screen[graph_address], &screen[graph_address + 1u], GRAPH_WIDTH - 1u);
        graph_address += MT12232A_WIDTH;
        (void)memmove(&screen[graph_address], &screen[graph_address + 1u], GRAPH_WIDTH - 1u);

        // TODO(Any): Loop
        size_t screen_addr = ((GRAPH_Y / MT12232A_POINTS_IN_BYTE) * MT12232A_WIDTH) + GRAPH_X + GRAPH_WIDTH - 1u;
        screen[screen_addr] = mask;
        screen[screen_addr + MT12232A_WIDTH] = mask >> 8u;
        screen[screen_addr + (MT12232A_WIDTH * 2u)] = mask >> 16u;
        screen[screen_addr + (MT12232A_WIDTH * 3u)] = mask >> 24u;

        if (Mt12232aUpdateImage(&mt12232a) == false) {
            Error_Handler();
        }
        HAL_Delay(100u);
    }
}
