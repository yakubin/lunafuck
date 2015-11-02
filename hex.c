// Copyright: see LICENSE.txt
#include <stddef.h>

#include <stdint.h>
#include <stdlib.h>

#include "addrstack.h"
#include "codelist.h"
#include "hex.h"

// appends hex instructions for moving data ptr upwards
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 6 BYTES OF ALLOCATED SPACE
//
// @param `arg`:    the amount of cells by which the data ptr should be moved
//
// @return:
// amount of bytes written to @param `dst` (6 at most)
static size_t append_cell_hex_add(uint8_t* dst, uint32_t arg) {
    uint32_t operand = 2 * arg;
    if (operand < 0x80) {
        // safe to do 8-bit subtraction
        // sub ecx, byte operand
        dst[0] = 0x83;
        dst[1] = 0xe9;
        dst[2] = operand;
        return 3;
    }

    // 32-bit subtraction
    // sub ecx, operand
    dst[0] = 0x81;
    dst[1] = 0xe9;
    dst[2] = operand & 0xff;
    dst[3] = (operand & (0xff << 8)) >> 8;
    dst[4] = (operand & (0xff << 16)) >> 16;
    dst[5] = (operand & (0xff << 24)) >> 24;
    return 6;
}

// appends hex instructions for moving data ptr downwards
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 6 BYTES OF ALLOCATED SPACE
//
// @param `arg`:    the amount of cells by which the data ptr should be moved
//
// @return:
// amount of bytes written to @param `dst` (6 at most)
static size_t append_cell_hex_sub(uint8_t* dst, uint32_t arg) {
    uint32_t operand = 2 * arg;
    if (operand < 0x80) {
        // safe to do 8-bit addition
        // add ecx, byte operand
        dst[0] = 0x83;
        dst[1] = 0xc1;
        dst[2] = operand;
        return 3;
    }

    // 32-bit addition
    // add ecx, operand
    dst[0] = 0x81;
    dst[1] = 0xc1;
    dst[2] = operand & 0xff;
    dst[3] = (operand & (0xff << 8)) >> 8;
    dst[4] = (operand & (0xff << 16)) >> 16;
    dst[5] = (operand & (0xff << 24)) >> 24;
    return 6;
}

// appends hex instructions for increasing the value of a cell
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 3 BYTES OF ALLOCATED SPACE
//
// @param `arg`:    the number by which the value of a cell should be increased
//
// @return:
// amount of bytes written to @param `dst` (3 at most)
static size_t append_ascii_hex_add(uint8_t* dst, uint8_t arg) {
    if (arg == 1) {
        // inc byte [ecx]
        dst[0] = 0xfe;
        dst[1] = 0x01;
        return 2;
    }

    // add byte [ecx], arg
    dst[0] = 0x80;
    dst[1] = 0x01;
    dst[2] = arg;
    return 3;
}

// appends hex instructions for decreasing the value of a cell
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 3 BYTES OF ALLOCATED SPACE
//
// @param `arg`:    the number by which the value of a cell should be decreased
//
// @return:
// amount of bytes written to @param `dst` (3 at most)
static size_t append_ascii_hex_sub(uint8_t* dst, uint8_t arg) {
    if (arg == 1) {
        // dec byte [ecx]
        dst[0] = 0xfe;
        dst[1] = 0x09;
        return 2;
    }

    // sub byte [ecx], arg
    dst[0] = 0x80;
    dst[1] = 0x29;
    dst[2] = arg;
    return 3;
}

// appends hex instructions for outputting the value of the current cell as an
// ASCII character
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 6 BYTES OF ALLOCATED SPACE
//
// @return:
// amount of bytes written to @param `dst` (6)
static size_t append_hex_out(uint8_t* dst) {
    // mov al, 0x04
    dst[0] = 0xb0;
    dst[1] = 0x04;

    // mov bl, 1
    dst[2] = 0xb3;
    dst[3] = 0x01;

    // int 0x80
    dst[4] = 0xcd;
    dst[5] = 0x80;

    return 6;
}

// appends hex instructions for inputting the value of the current cell as an
// ASCII character
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 6 BYTES OF ALLOCATED SPACE
//
// @return:
// amount of bytes written to @param `dst` (6)
static size_t append_hex_in(uint8_t* dst) {
    // mov al, 0x03
    dst[0] = 0xb0;
    dst[1] = 0x03;

    // mov bl, 0x00
    dst[2] = 0xb3;
    dst[3] = 0x00;

    // int 0x80
    dst[4] = 0xcd;
    dst[5] = 0x80;

    return 6;
}

// appends hex instructions for jumping to he beginning of a loop
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 5 BYTES OF ALLOCATED SPACE
//
// @return:
// amount of bytes written to @param `dst` (5)
static size_t append_loop_hex_jump(uint8_t* dst, int8_t rel_dis) {
    // cmp byte [ecx], 0x00
    dst[0] = 0x80;
    dst[1] = 0x39;
    dst[2] = 0x00;

    // jnz rd
    dst[3] = 0x75;
    dst[4] = rel_dis - 5;

    return 5;
}

// appends hex instructions for exiting with code 0
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 6 BYTES OF ALLOCATED SPACE
//
// @return:
// amount of bytes written to @param `dst` (6)
static size_t append_hex_exit(uint8_t* dst) {
    // mov al, 0x01
    dst[0] = 0xb0;
    dst[1] = 0x01;

    // mov bl, 0x00
    dst[2] = 0xb3;
    dst[3] = 0x00;

    // int 0x80
    dst[4] = 0xcd;
    dst[5] = 0x80;

    return 6;
}

// appends hex instructions for setting up the runtime of a brainfuck program
// these should be appended to the beginning of the text segment
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 18 BYTES OF ALLOCATED SPACE
//                      * THESE 18 BYTES SHOULD BE SET TO 0
//
// @return:
// amount of bytes written to @param `dst` (18)
static size_t init_text_seg(uint8_t* dst) {
    // first element of the array of cells
    // push word 0x00
    dst[0] = 0x66;
    dst[1] = 0x6a;
    // pushes[2] = 0x00 <- already done by promise

    // preparing the registers
    // mov ecx, esp
    dst[3] = 0x89;
    dst[4] = 0xe1;

    // mov dl, 0x01
    dst[5] = 0xb2;
    dst[6] = 0x01;

    // mov ax, 0x7fff <- 0x8000-1 remaining cells to push
    dst[7]  = 0x66;
    dst[8]  = 0xb8;
    dst[9]  = 0xff;
    dst[10] = 0x7f;

    // remaining elements of the array of cells
    // push word 0x00
    dst[11] = 0x66;
    dst[12] = 0x6a;
    // dst[13] = 0x00; <- already done by promise

    // dec ax
    dst[14] = 0x66;
    dst[15] = 0x48;

    // jnz rd
    dst[16] = 0x75;
    dst[17] = 0xf9;  // -7

    return 18;
}

size_t code_list_to_hex(uint8_t** dst, struct code_list* code_list) {
    *dst = calloc(6 * code_list->size + 18, 1);

    // promise (*dst filled with zeroes) satisfied by calloc
    size_t curaddr = init_text_seg(*dst);

    struct code_list_node* node = code_list->first;
    for (; node != NULL; node = node->next) {
        switch (node->id) {
        case OP_ASCII_ADD:
            curaddr += append_ascii_hex_add((*dst) + curaddr, node->arg);
            break;
        case OP_ASCII_SUB:
            curaddr += append_ascii_hex_sub((*dst) + curaddr, node->arg);
            break;
        case OP_CELL_ADD:
            curaddr += append_cell_hex_add((*dst) + curaddr, node->arg);
            break;
        case OP_CELL_SUB:
            curaddr += append_cell_hex_sub((*dst) + curaddr, node->arg);
            break;
        case OP_OUT:
            curaddr += append_hex_out((*dst) + curaddr);
            break;
        case OP_IN:
            curaddr += append_hex_in((*dst) + curaddr);
            break;
        case OP_LOOP_LABEL:
            addrstack_push(curaddr);
            break;
        case OP_LOOP_JUMP:
            curaddr += append_loop_hex_jump((*dst) + curaddr,
                                            addrstack_pop() - curaddr);
            break;
        case OP_INVALID:  // to silence the compiler
            break;
        }
    }
    curaddr += append_hex_exit((*dst) + curaddr);
    return curaddr;
}
