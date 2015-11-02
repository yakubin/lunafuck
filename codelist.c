// Copyright: see LICENSE.txt
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "codelist.h"

static code_op char_to_code_op(char c) {
    switch (c) {
    case '>':
        return OP_CELL_ADD;
    case '<':
        return OP_CELL_SUB;
    case '+':
        return OP_ASCII_ADD;
    case '-':
        return OP_ASCII_SUB;
    case '.':
        return OP_OUT;
    case ',':
        return OP_IN;
    case '[':
        return OP_LOOP_LABEL;
    case ']':
        return OP_LOOP_JUMP;
    default:
        return OP_INVALID;
    }
}

// returns an operation antagonist to op for reducable (see reducable())
// operations; simplifies code responsible for optimizations
static code_op get_ant_op(code_op op) {
    switch (op) {
    case OP_CELL_ADD:
        return OP_CELL_SUB;
    case OP_CELL_SUB:
        return OP_CELL_ADD;
    case OP_ASCII_ADD:
        return OP_ASCII_SUB;
    case OP_ASCII_SUB:
        return OP_ASCII_ADD;
    default:
        return OP_INVALID;
    }
}

// checks if node->id is an apoeration that is potentially reducable
// if @param `node` == NULL, returns false
static bool reducable(struct code_list_node* node) {
    if (node == NULL) {
        return false;
    }

    switch (node->id) {
    case OP_CELL_ADD:
    case OP_CELL_SUB:
    case OP_ASCII_ADD:
    case OP_ASCII_SUB:
        return true;
    default:
        return false;
    }
}

// if two nodes can be reduced to a single node, it reduces them and returns
// true
// otherwise it does nothing and returns false (including a situation when
// @param `fst` == NULL || @param `snd` == NULL)
static bool reduce_nodes(struct code_list_node* fst,
                         struct code_list_node* snd) {
    // prevents 'optimizing' of subsequent e.g. output operations
    if (reducable(fst) == false || reducable(snd) == false) {
        return false;
    }

    if (fst->id == snd->id) {
        fst->arg += snd->arg;
        if (fst->id == OP_ASCII_ADD || fst->id == OP_ASCII_SUB) {
            // ascii code fits in a byte; avoid big numbers
            fst->arg %= 1 << 8;
        }
        return true;
    }

    // reduces antagonist operations
    // can result in a node, where arg == 0
    if (get_ant_op(fst->id) == snd->id) {
        if (fst->arg < snd->arg) {
            fst->id  = snd->id;
            fst->arg = snd->arg - fst->arg;
        } else {
            fst->arg -= snd->arg;
        }
        return true;
    }
    return false;
}

// creates a new code_list_node based on the input character @param `c`
// returns NULL, if @param `c` is not a brainfuck command
static bool make_code_list_node(struct code_list_node** node, char c) {
    code_op id = char_to_code_op(c);
    if (id == OP_INVALID) {
        return false;
    }

    if (*node == NULL) {
        *node = malloc(sizeof **node);
    }
    (*node)->id = id;
    (*node)->arg = 1;
    (*node)->next = NULL;
    return true;
}

// counts opening and closing braces of loops, checks for unmatched ']'
//
// @param `ol`: pointer to the counter; MUST NOT BE NULL
// @param `c`:  brainfuck command
//
// @return:
// false on valid input
// true on unmatched ']'
static bool count_loop_level(size_t* ol, char c) {
    switch (c) {
    case '[':
        ++*ol;
        return false;
    case ']':
        if (*ol == 0) {
            return true;
        }
        --*ol;
    default:
        return false;
    }
}

struct code_list* make_code_list(const char* src, size_t len) {
    struct code_list* code_list = malloc(sizeof *code_list);
    struct code_list_node** cur = &code_list->first;
    struct code_list_node* prev = NULL;
    size_t opened_loops         = 0;

    code_list->size = 0;
    size_t i = 0;
    for (*cur = NULL; i < len; ++i) {
        if (count_loop_level(&opened_loops, src[i]) == true) {
            fprintf(stderr, "error:%lu: unmatched ']'\n", i);
            free_code_list(code_list);
            return NULL;
        }

        if (make_code_list_node(cur, src[i]) &&
            reduce_nodes(prev, *cur) == false) {
            prev = *cur;
            cur  = &(*cur)->next;
            ++code_list->size;
        }
    }

    if (opened_loops > 0) {
        fprintf(stderr, "error:%lu: unmatched '['\n", i);
        free_code_list(code_list);
        return NULL;
    }

    return code_list;
}

void free_code_list(struct code_list* code_list) {
    struct code_list_node* node = code_list->first;
    struct code_list_node* next = NULL;
    while (node != NULL) {
        next = node->next;
        free(node);
        node = next;
    }
    free(code_list);
}
