// Copyright: see LICENSE.txt
#ifndef HEX_H
#define HEX_H

#include <stddef.h>
#include <stdint.h>

#include "codelist.h"

// converts a code_list to a set of instructions written in hex
//
// @param `dst`:        pointer to a pointer to which store the array of hex
//                      codes; MUST NOT BE NULL
//                      the caller is responsible for freeing *dst
//                      (with free(3))
//
// @param `code_list`:  pointer to the input code_list; MUST NOT BE NULL
//
// @return:
// length (in bytes) of *dst array
// it may not be (and usually will not be) equal to the size of allocated space
extern size_t code_list_to_hex(uint8_t** dst, struct code_list* code_list);
#endif  // HEX_H
