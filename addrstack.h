// Copyright: see LICENSE.txt
#ifndef ADDRSTACK_H
#define ADDRSTACK_H

// this stack implementation concerns only the handling of loops in brainfuck
// programs

#include <stdbool.h>
#include <stdint.h>

// Plato:       how many nested loops can a brainfuck program make?
// Theaetetus:  i don't know, sir.
// Plato:       it couldn't be an infinite amount of loops, could it?
// Theaetetus:  certainly, sir.
// Plato:       so it must be less than infinity; what about a half of it?
// Theaetetus:  it is still infinity, sir.
// Plato:       very apt, kid; a hundred then?
// Theaetetus:  at most, sir.
// Plato:       so be it.

#define MAXLOOPS 100

// pushes a new value to the stack of addresses in code
//
// it is supposed to be used only to implement loops and, accordingly, keep
// track of relative displacements in jnz assembly instructions, so it doesn't
// matter if the values pushed are actual addresses; only gaps between
// subsequent `addreses' matter
//
// @param `address':    address to be pushed to stack
//
// @return:
// true on success
// false on failure (not enough space - len == MAXLOOPS)
extern bool addrstack_push(uint32_t address);

// returns the popped value if len != 0 or 0 if len == 0
extern uint32_t addrstack_pop();
#endif  // ADDRSTACK_H
