/*
    Part of the Real-Time Embedded Systems course at Halmstad University
    Copyright (c) 2017, Sebastian Kunze <sebastian.kunze@hh.se>
    All rights reserved.
 */
/*
 * Modified by Wagner Morais on Aug 2022.
 */
 
#include <stddef.h>
#include <stdlib.h>
#include "expstruct.h"
#include "rpi-systimer.h"

ExpStruct *iexp(int x) {

    if(x < 0){
        return NULL;
    }
    ExpStruct *e = malloc(sizeof(ExpStruct));
    if(!e){
        return NULL;
    }
    double sum = 1;
    long unsigned xTemp = 1;
    long unsigned fac = 1;
    int acc = 80;
    for (int i = acc; i > 0; i--) {
        sum = 1 + sum * x / i;
    }

    double sumTemp = sum;
    int p = 0;
    while (sumTemp > 1) {
        sumTemp /= 10;
        p++;
    }

    e->expInt = (int) sum;
    e->expFraction = (sum - (int) (sum)) * 100;
    return e;
}
