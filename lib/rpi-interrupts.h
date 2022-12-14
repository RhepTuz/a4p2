/*
    Part of the Raspberry-Pi Bare Metal Tutorials
    https://www.valvers.com/rpi/bare-metal/
    Copyright (c) 2013-2018, Brian Sidebotham

    This software is licensed under the MIT License.
    Please see the LICENSE file included with this software.

*/
/*
 * Modified by Wagner Morais on Sep 2021.
 */
#ifndef RPI_INTERRUPTS_H
#define RPI_INTERRUPTS_H

#include <stdint.h>

#include "rpi-base.h"

extern volatile int ticks;
extern void RPI_EnableARMTimerInterrupt(void);

#endif
