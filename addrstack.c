// Copyright: see LICENSE.txt
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "addrstack.h"

static uint32_t stack[MAXLOOPS];
static size_t len = 0;  // amount of elems on the stack

bool addrstack_push(uint32_t address) {
    if (len == MAXLOOPS) {
        return false;
    }
    stack[len++] = address;
    return true;
}

uint32_t addrstack_pop() {
    if (len == 0) {
        return 0;
    }
    return stack[--len];
}
