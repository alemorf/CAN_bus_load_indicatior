/* MT-13332A LCD display driver
 * License: GPL
 * Copyright (c) Aleksey Morozov aleksey.f.morozov@gmail.com
 */

#ifndef CORE_SRC_MT12232A_HAL_H_
#define CORE_SRC_MT12232A_HAL_H_

#include <stdint.h>
#include "mt12232a.h"

void Mt12232aInitHal(Mt12232a *self);
void Mt12232aUpdateOutData(Mt12232a *self, uint16_t reset, uint16_t set, uint32_t delay);
void Mt12232aSetInputDataDirection(Mt12232a *self);
void Mt12232aSetOutputDataDirection(Mt12232a *self);
uint8_t Mt12232aReadData(Mt12232a *self);

#endif /* CORE_SRC_MT12232A_HAL_H_ */
