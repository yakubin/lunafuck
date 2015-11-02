// Copyright: see LICENSE.txt
#ifndef CODELIST_H
#define CODELIST_H

#include <stddef.h>
#include <stdint.h>

// types of operations performed by brainfuck programs
typedef enum {
    OP_INVALID,     // for error reporting
    OP_CELL_ADD,    // '>'
    OP_CELL_SUB,    // '<'
    OP_ASCII_ADD,   // '+'
    OP_ASCII_SUB,   // '-'
    OP_OUT,         // '.'
    OP_IN,          // ','
    OP_LOOP_LABEL,  // '['
    OP_LOOP_JUMP    // ']'
} code_op;

// a node in code_list
//
// @elem `id`:  type of operation
//
// @elem `arg`: needed to optimize OP_CELL_ADD, OP_CELL_SUB, OP_ASCII_ADD,
//              OP_ASCII_SUB operations, ex:
//
//                  '++' should reduce to a node, where:
//                      id = OP_ASCII_ADD,
//                      arg = 2
//
//                  it would later be changed to a single assembly sub
//                  instruction
//
//                  '+-' should reduce to nothing
//
//                  etc.
//
// @elem next:  pointer to the next node of code_list
struct code_list_node {
    code_op id;
    uint32_t arg;
    struct code_list_node* next;
};

// singly-linked list of operations performed by a brainfuck program
struct code_list {
    struct code_list_node* first;
    size_t size;
};

// converts brainfuck source code to a code_list
//
// @param `src`:    pointer to an array of chars containing brainfuck source
//                  code; not necessarily a C-string; MUST NOT BE NULL
//
// @param `len`:    length of @param `src`
//
// @return:
// NULL in case of failure, e.g. unmatched '[', ']'...
// otherwise a pointer to a code_list corresponding to brainfuck source code in
// @param `src`; all optimizations should have been performed by now
extern struct code_list* make_code_list(const char* src, size_t len);

// frees all the resources allocated by a code_list
// use it instead of a basic free(3)
extern void free_code_list(struct code_list* code_list);
#endif
