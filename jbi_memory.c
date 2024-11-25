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

#define k_BLOCK_SIZE    (32)    // Must be a multiple of 4 (real size is MIN_BLOCK_SIZE - 1)
#define k_FREE_TAG      (0)     // Also used for number of blocks

typedef struct {
    uint8_t tag;
    uint8_t mem[k_BLOCK_SIZE-1];
} buff_t;

typedef struct {
    uint16_t num_blocks;
    uint8_t compensation;       // To force the alignment of buffers to 4 bytes
    buff_t buff[0];
} mem_t;

void *jbi_mem_create(uint16_t num_blocks) {
    uint16_t size = sizeof(mem_t) + num_blocks * sizeof(buff_t);
    mem_t *p_mem = (mem_t*)malloc(size);

    if(p_mem == NULL)
    {
        return NULL;
    }
    p_mem->num_blocks = num_blocks;
    for(int i = 0; i < size / k_BLOCK_SIZE; i++)  {
        p_mem->buff[i].tag = k_FREE_TAG;
    }
    return p_mem;
}

void jbi_mem_dump(void *pv_mem) {
    mem_t *p_mem = (mem_t*)pv_mem;
    uint8_t num_blocks = 0;

    printf("Memory dump:\n");
    for(uint16_t i = 0; i < p_mem->num_blocks; i++) {
        if(num_blocks == 0) {
            num_blocks = p_mem->buff[i].tag;
            printf("%02d ", num_blocks);
            if(num_blocks > 0) {
                num_blocks--;
            }
        }
        else {
            num_blocks--;
            printf("xx ");
        }
        if((i + 1) % 32 == 0) {
            printf("\n");
        }
    }
}

void jbi_mem_destroy(void *pv_mem)
{
    free(pv_mem);
}

void *jbi_mem_alloc(void *pv_mem, uint16_t bytes) {
    mem_t *p_mem = (mem_t*)pv_mem;
    uint16_t num_blocks = bytes / (k_BLOCK_SIZE - 1) + (bytes % (k_BLOCK_SIZE - 1) ? 1 : 0);
    uint16_t start = 0;
    uint16_t count = 0;
    uint16_t blocked = 0;

    for(uint16_t i = 0; i < p_mem->num_blocks; i++) {
        if(blocked > 0) {
            blocked--;
            continue;
        }
        if(p_mem->buff[i].tag == k_FREE_TAG) {
            if(count == 0) {
                start = i;
                count = 1;
            } else {
                count++;
            }
            if(count == num_blocks) {
                p_mem->buff[start].tag = num_blocks;
                return &p_mem->buff[start].mem[0];
            }
        } else {
            blocked = p_mem->buff[i].tag - 1;
            start = 0;
            count = 0;
        }
    }
    return NULL;
}

void jbi_mem_free(void *pv_mem, void *pv_ptr)
{
    mem_t *p_mem = (mem_t*)pv_mem;
    if((pv_ptr >= (void*)&p_mem->buff[0]) &&
       (pv_ptr <= (void*)&p_mem->buff[p_mem->num_blocks - 1])) {
        uint8_t *p_buff = (uint8_t*)pv_ptr;
        uint8_t num_blocks = p_buff[-1];
        for(uint8_t i = 0; i < num_blocks; i++) {
            p_buff[i * k_BLOCK_SIZE - 1] = k_FREE_TAG;
        }
    }
}

void *jbi_MemoryRealloc(void *pv_mem, void *pv_ptr, uint16_t bytes) {
    mem_t *p_mem = (mem_t*)pv_mem;
    uint8_t *p_buff = (uint8_t*)pv_ptr;
    uint8_t num_blocks = p_buff[-1];
    uint16_t new_num_blocks = bytes / (k_BLOCK_SIZE - 1) + (bytes % (k_BLOCK_SIZE - 1) ? 1 : 0);
    if(new_num_blocks == num_blocks) {
        return pv_ptr;
    }
    if(new_num_blocks < num_blocks) {
        for(uint8_t i = new_num_blocks; i < num_blocks; i++) {
            p_buff[i * k_BLOCK_SIZE - 1] = k_FREE_TAG;
        }
        return pv_ptr;
    }
    uint8_t *p_new = jbi_mem_alloc(pv_mem, bytes);
    if(p_new == NULL) {
        return NULL;
    }
    memcpy(p_new, pv_ptr, (num_blocks - 1) * k_BLOCK_SIZE);
    jbi_mem_free(pv_mem, pv_ptr);
    return p_new;
}
