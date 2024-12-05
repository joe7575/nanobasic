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

#define NUM_BLOCKS(bytes)   ((bytes + k_MEM_BLOCK_SIZE + 1) / k_MEM_BLOCK_SIZE)
#define NUM_WORDS(bytes)    ((bytes + sizeof(uint32_t) + 1) / sizeof(uint32_t))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define HEADER_SIZE         2

void jbi_mem_init(t_VM *p_vm) {
    for(int i = 0; i < k_MEM_HEAP_SIZE; i += k_MEM_BLOCK_SIZE) {
        p_vm->heap[i] = k_MEM_FREE_TAG;
    }
    p_vm->str_start_addr = 0;
}

uint16_t jbi_mem_alloc(t_VM *p_vm, uint16_t bytes) {
    uint16_t num_blocks = NUM_BLOCKS(bytes);
    uint16_t num_words  = NUM_WORDS(bytes);
    uint16_t start = 0;
    uint16_t count = 0;
    uint16_t blocked = 0;

    for(int i = p_vm->str_start_addr; i < k_MEM_HEAP_SIZE; i += k_MEM_BLOCK_SIZE) {
        if(blocked > 0) {
            blocked--;
            continue;
        }
        if(p_vm->heap[i] == k_MEM_FREE_TAG) {
            if(count == 0) {
                start = i;
                count = 1;
            } else {
                count++;
            }
            if(count == num_blocks) {
                p_vm->heap[start] = num_blocks;
                p_vm->heap[start + 1] = num_words;
                p_vm->str_start_addr = start;
                return 0x8000 + start + HEADER_SIZE;
            }
        } else {
            blocked = p_vm->heap[i] - 1;
            start = 0;
            count = 0;
        }
    }
    return 0;
}

void jbi_mem_free(t_VM *p_vm, uint16_t addr) {
    addr = (addr & 0x7FFF) - HEADER_SIZE;
    if(addr < k_MEM_HEAP_SIZE) {
        uint16_t size = p_vm->heap[addr] * k_MEM_BLOCK_SIZE;
        if((addr + size) <= k_MEM_HEAP_SIZE) {
            for(uint8_t i = 0; i < size; i += k_MEM_BLOCK_SIZE) {
                p_vm->heap[addr + i] = k_MEM_FREE_TAG;
            }
            p_vm->str_start_addr = MIN(p_vm->str_start_addr, addr);
        }
    }
}

uint16_t jbi_mem_realloc(t_VM *p_vm, uint16_t addr, uint16_t bytes) {
    addr = (addr & 0x7FFF) - HEADER_SIZE;
    uint8_t num_blocks = p_vm->heap[addr];
    uint16_t new_num_blocks = NUM_BLOCKS(bytes);
    uint16_t new_num_words = NUM_WORDS(bytes);
    if(new_num_blocks == num_blocks) {
        return 0x8000 + addr + HEADER_SIZE;
    }
    if(new_num_blocks < num_blocks) {
        p_vm->heap[addr] = new_num_blocks;
        p_vm->heap[addr + 1] = new_num_words;
        uint16_t start = addr + new_num_blocks * k_MEM_BLOCK_SIZE;
        uint16_t stop = addr + num_blocks * k_MEM_BLOCK_SIZE;
        for(uint8_t i = start; i < stop; i += k_MEM_BLOCK_SIZE) {
           p_vm->heap[i] = k_MEM_FREE_TAG;
        }
        return 0x8000 + addr + HEADER_SIZE;
    }
    uint16_t new = jbi_mem_alloc(p_vm, bytes);
    if(new == 0) {
        return 0;
    }
    memcpy(&p_vm->heap[new & 0x7FFF], &p_vm->heap[addr + HEADER_SIZE], bytes);
    jbi_mem_free(p_vm, addr + HEADER_SIZE);
    return new;
}

uint16_t jbi_mem_get_blocksize(t_VM *p_vm, uint16_t addr) {
    addr = (addr & 0x7FFF) - HEADER_SIZE;
    return (p_vm->heap[addr + 1] * sizeof(uint32_t)) - HEADER_SIZE;
}

#define TEST
#ifdef TEST
void mem_dump(t_VM *p_vm) {
    uint8_t num_blocks = 0;

    printf("Memory dump:\n");
    for(int i = 0; i < 512; i += k_MEM_BLOCK_SIZE) {
        if(num_blocks == 0) {
            num_blocks = p_vm->heap[i];
            printf("%02d ", num_blocks);
            if(num_blocks > 0) {
                num_blocks--;
            }
        }
        else {
            num_blocks--;
            printf("xx ");
        }
        if((i + k_MEM_BLOCK_SIZE) % (k_MEM_BLOCK_SIZE * 32) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void test_memory(t_VM *p_vm) {
    uint16_t addr1, addr2, addr3, addr4;

    mem_dump(p_vm);
    assert((addr1 = jbi_mem_alloc(p_vm, 13)) != 0);
    assert((addr2 = jbi_mem_alloc(p_vm, 14)) != 0);
    assert((addr3 = jbi_mem_alloc(p_vm, 15)) != 0);
    assert((addr4 = jbi_mem_alloc(p_vm, 128)) != 0);

    assert(jbi_mem_get_blocksize(p_vm, addr1) == 14);
    assert(jbi_mem_get_blocksize(p_vm, addr2) == 14);
    assert(jbi_mem_get_blocksize(p_vm, addr3) == 18);
    assert(jbi_mem_get_blocksize(p_vm, addr4) == 130);

    memset(p_vm->heap + (addr1 & 0x7FFF), 0x11, 13);
    memset(p_vm->heap + (addr2 & 0x7FFF), 0x22, 14);
    memset(p_vm->heap + (addr3 & 0x7FFF), 0x33, 15);
    memset(p_vm->heap + (addr4 & 0x7FFF), 0x44, 128);

    assert((uint64_t)(p_vm->heap + (addr1 & 0x7FFF)) % 4 == 0);
    assert((uint64_t)(p_vm->heap + (addr2 & 0x7FFF)) % 4 == 0);
    assert((uint64_t)(p_vm->heap + (addr3 & 0x7FFF)) % 4 == 0);
    assert((uint64_t)(p_vm->heap + (addr4 & 0x7FFF)) % 4 == 0);

    mem_dump(p_vm);
    jbi_mem_free(p_vm, addr2);
    jbi_mem_free(p_vm, addr4);
    mem_dump(p_vm);
    jbi_mem_free(p_vm, addr1);
    jbi_mem_free(p_vm, addr3);
    mem_dump(p_vm);

    assert((addr1 = jbi_mem_alloc(p_vm, 29)) == 0x8002);
    assert((addr1 = jbi_mem_realloc(p_vm, addr1, 14)) == 0x8002);
    mem_dump(p_vm);
    assert((addr2 = jbi_mem_alloc(p_vm, 12)) == 0x8012);
    assert((addr2 = jbi_mem_realloc(p_vm, addr2, 30)) == 0x8022);
    mem_dump(p_vm);
}
#endif
