/*

Copyright 2024 Joachim Stolberg

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include "jbi.h"
#include "jbi_int.h"

#define NUM_BLOCKS(bytes)   (bytes / (k_STR_BLOCK_SIZE - 1) + (bytes % (k_STR_BLOCK_SIZE - 1) ? 1 : 0))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))

void jbi_str_init(t_VM *p_vm) {
    for(int i = 0; i < k_STR_HEAP_SIZE; i += k_STR_BLOCK_SIZE) {
        p_vm->string_heap[i] = k_STR_FREE_TAG;
    }
    p_vm->str_start_addr = 0;
}

uint16_t jbi_str_alloc(t_VM *p_vm, uint16_t bytes) {
    uint16_t num_blocks = NUM_BLOCKS(bytes);
    uint16_t start = 0;
    uint16_t count = 0;
    uint16_t blocked = 0;

    for(int i = p_vm->str_start_addr; i < k_STR_HEAP_SIZE; i += k_STR_BLOCK_SIZE) {
        if(blocked > 0) {
            blocked--;
            continue;
        }
        if(p_vm->string_heap[i] == k_STR_FREE_TAG) {
            if(count == 0) {
                start = i;
                count = 1;
            } else {
                count++;
            }
            if(count == num_blocks) {
                p_vm->string_heap[start] = num_blocks;
                p_vm->str_start_addr = start;
                return 0x8000 + start + 1;
            }
        } else {
            blocked = p_vm->string_heap[i] - 1;
            start = 0;
            count = 0;
        }
    }
    return 0;
}

void jbi_str_free(t_VM *p_vm, uint16_t addr) {
    addr = (addr & 0x7FFF) - 1;
    if(addr < k_STR_HEAP_SIZE) {
        uint16_t size = p_vm->string_heap[addr] * k_STR_BLOCK_SIZE;
        if((addr + size) <= k_STR_HEAP_SIZE) {
            for(uint8_t i = 0; i < size; i += k_STR_BLOCK_SIZE) {
                p_vm->string_heap[addr + i] = k_STR_FREE_TAG;
            }
            p_vm->str_start_addr = MIN(p_vm->str_start_addr, addr);
        }
    }
}

uint16_t jbi_str_realloc(t_VM *p_vm, uint16_t addr, uint16_t bytes) {
    addr = (addr & 0x7FFF) - 1;
    uint8_t num_blocks = p_vm->string_heap[addr];
    uint16_t new_num_blocks = NUM_BLOCKS(bytes);
    if(new_num_blocks == num_blocks) {
        return 0x8000 + addr + 1;
    }
    if(new_num_blocks < num_blocks) {
        p_vm->string_heap[addr] = new_num_blocks;
        uint16_t start = addr + new_num_blocks * k_STR_BLOCK_SIZE;
        uint16_t stop = addr + num_blocks * k_STR_BLOCK_SIZE;
        for(uint8_t i = start; i < stop; i += k_STR_BLOCK_SIZE) {
           p_vm->string_heap[i] = k_STR_FREE_TAG;
        }
        return 0x8000 + addr + 1;
    }
    uint16_t new = jbi_str_alloc(p_vm, bytes);
    if(new == 0) {
        return 0;
    }
    memcpy(&p_vm->string_heap[new & 0x7FFF], &p_vm->string_heap[addr + 1], bytes);
    jbi_str_free(p_vm, addr + 1);
    return new;
}

//#define TEST
#ifdef TEST
void mem_dump(t_VM *p_vm) {
    uint8_t num_blocks = 0;

    printf("Memory dump:\n");
    for(int i = 0; i < k_STR_HEAP_SIZE; i += k_STR_BLOCK_SIZE) {
        if(num_blocks == 0) {
            num_blocks = p_vm->string_heap[i];
            printf("%02d ", num_blocks);
            if(num_blocks > 0) {
                num_blocks--;
            }
        }
        else {
            num_blocks--;
            printf("xx ");
        }
        if((i + k_STR_BLOCK_SIZE) % (k_STR_BLOCK_SIZE * 32) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void test_memory(t_VM *p_vm) {
    uint16_t addr1, addr2, addr3, addr4;

    mem_dump(p_vm);
    assert((addr1 = jbi_str_alloc(p_vm, 14)) != 0);
    assert((addr2 = jbi_str_alloc(p_vm, 15)) != 0);
    assert((addr3 = jbi_str_alloc(p_vm, 16)) != 0);
    assert((addr4 = jbi_str_alloc(p_vm, 128)) != 0);

    memset(p_vm->string_heap + (addr1 & 0x7FFF), 0x11, 14);
    memset(p_vm->string_heap + (addr2 & 0x7FFF), 0x22, 15);
    memset(p_vm->string_heap + (addr3 & 0x7FFF), 0x33, 16);
    memset(p_vm->string_heap + (addr4 & 0x7FFF), 0x44, 128);

    assert((uint64_t)(p_vm->string_heap + (addr1 & 0x7FFF)) % 4 == 0);
    assert((uint64_t)(p_vm->string_heap + (addr2 & 0x7FFF)) % 4 == 0);
    assert((uint64_t)(p_vm->string_heap + (addr3 & 0x7FFF)) % 4 == 0);
    assert((uint64_t)(p_vm->string_heap + (addr4 & 0x7FFF)) % 4 == 0);

    mem_dump(p_vm);
    jbi_str_free(p_vm, addr2);
    jbi_str_free(p_vm, addr4);
    mem_dump(p_vm);
    jbi_str_free(p_vm, addr1);
    jbi_str_free(p_vm, addr3);
    mem_dump(p_vm);

    assert((addr1 = jbi_str_alloc(p_vm, 30)) == 0x8001);
    assert((addr1 = jbi_str_realloc(p_vm, addr1, 15)) == 0x8001);
    mem_dump(p_vm);
    assert((addr2 = jbi_str_alloc(p_vm, 12)) == 0x8011);
    assert((addr2 = jbi_str_realloc(p_vm, addr2, 30)) == 0x8021);
    mem_dump(p_vm);
}
#endif