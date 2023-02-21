#ifndef __POTATO_H__
#define __POTATO_H__

#include <stdio.h>
#include <stdlib.h>

struct Potato_t {
    int holderNum;
    int remainHops;
    int current_round;
    int trace[512];

}typedef Potato;

#endif