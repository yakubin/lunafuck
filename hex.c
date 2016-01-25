// Copyright: see LICENSE.txt
#include <stddef.h>

#include <stdint.h>
#include <stdlib.h>

#include "addrstack.h"
#include "codelist.h"
#include "hex.h"

// appends @param `num` to @param `dst` in little endian order
//
// @param `dst`:
//      * MUST NOT BE NULL
//      * MUST HAVE AT LEAST 4 BYTES OF ALLOCATED SPACE
//
// @return:
// number of bytes appended to @param `dst` (4)
static size_t append_little_endian_uint32(uint8_t* dst, uint32_t num) {
    dst[0] = num & 0xff;
    dst[1] = (num & (0xff << 8)) >> 8;
    dst[2] = (num & (0xff << 16)) >> 16;
    dst[3] = (num & (0xff << 24)) >> 24;
    return 4;
}

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
    return 2 + append_little_endian_uint32(dst+2, operand);
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
    return 2 + append_little_endian_uint32(dst+2, operand);
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

// appends hex instructions for jumping to the code after a loop
// 
// dst[5:9] should be later set by the caller to the appropriate relative 
// displacement in little endian format
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 9 BYTES OF ALLOCATED SPACE
//
// @return:
// amount of bytes written to @param `dst` (9)
static size_t append_hex_pre_loop_jump(uint8_t* dst) {
    // cmp byte [ecx], 0x00
    dst[0] = 0x80;
    dst[1] = 0x39;
    dst[2] = 0x00;

    // jz rd
    dst[3] = 0x0f;
    dst[4] = 0x84;
    // dst[5:9] left for later setting by the caller

    return 9;
}

// appends hex instructions for jumping to the beginning of a loop
//
// @param `dst`:    pointer to the place where the instructions should be
//                  written;
//                      * MUST NOT BE NULL
//                      * MUST HAVE AT LEAST 9 BYTES OF ALLOCATED SPACE
//
// @return:
// amount of bytes written to @param `dst`
static size_t append_hex_post_loop_jump(uint8_t* dst, int32_t rel_dis) {
    // cmp byte [ecx], 0x00
    dst[0] = 0x80;
    dst[1] = 0x39;
    dst[2] = 0x00;

    rel_dis -= 5;
    int32_t mask = ~0xff;
    if ((rel_dis & mask) == 0) {
        // jnz rd
        dst[3] = 0x75;
        dst[4] = rel_dis;
        return 5;
    }

    rel_dis -= 4;
    // jnz rd
    dst[3] = 0x0f;
    dst[4] = 0x85;
    return 5 + append_little_endian_uint32(dst+5, rel_dis);
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
    *dst = calloc(9 * code_list->size + 18, 1);

    // promise (*dst filled with zeroes) satisfied by calloc
    size_t curaddr = init_text_seg(*dst);
    size_t loop_beg;
    int32_t rel_dis;

    struct code_list_node* node = code_list->first;
    for (; node != NULL; node = node->next) {
        switch (node->id) {
        case OP_ASCII_ADD:
            curaddr += append_ascii_hex_add(*dst + curaddr, node->arg);
            break;
        case OP_ASCII_SUB:
            curaddr += append_ascii_hex_sub(*dst + curaddr, node->arg);
            break;
        case OP_CELL_ADD:
            curaddr += append_cell_hex_add(*dst + curaddr, node->arg);
            break;
        case OP_CELL_SUB:
            curaddr += append_cell_hex_sub(*dst + curaddr, node->arg);
            break;
        case OP_OUT:
            curaddr += append_hex_out(*dst + curaddr);
            break;
        case OP_IN:
            curaddr += append_hex_in(*dst + curaddr);
            break;
        case OP_LOOP_LABEL:
            curaddr += append_hex_pre_loop_jump(*dst + curaddr);
            addrstack_push(curaddr);
            break;
        case OP_LOOP_JUMP:
            loop_beg = addrstack_pop();
            rel_dis = loop_beg - curaddr;
            curaddr += append_hex_post_loop_jump(*dst + curaddr, rel_dis);

            // set relative displacement for the jump at the beggining of loop
            rel_dis = curaddr - loop_beg;
            (*dst)[loop_beg-4] = rel_dis & 0xff;
            (*dst)[loop_beg-3] = (rel_dis & (0xff << 8)) >> 8;
            (*dst)[loop_beg-2] = (rel_dis & (0xff << 16)) >> 16;
            (*dst)[loop_beg-1] = (rel_dis & (0xff << 24)) >> 24;
            break;
        case OP_INVALID:  // to silence the compiler
            break;
        }
    }
    curaddr += append_hex_exit(*dst + curaddr);
    return curaddr;
}
