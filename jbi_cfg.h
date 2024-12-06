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

#define cfg_LINE_NUMBERS      // enable line numbers (or use labels)
//#define cfg_STRING_SUPPORT     // enable string support
//#define cfg_BYTE_ACCESS        // enable byte access to arrays

#define cfg_MAX_FOR_LOOPS     (4)   // nested FOR loops (2 values per FOR loop on the datastack)
#define cfg_DATASTACK_SIZE    (16)  // for expression evaluation and FOR loop variables
#define cfg_STACK_SIZE        (8)   // for function calls and external parameters
#define cfg_NUM_VARS          (255) // in the range 8..256
#define cfg_MEM_HEAP_SIZE     (1024 * 16) // in 1k steps
#define cfg_MAX_CODE_LEN      (32000)

#ifdef cfg_LINE_NUMBERS
    #define cfg_MAX_NUM_SYM     (4000) // for line numbers
#else
    #define cfg_MAX_NUM_SYM     (256) // for labels only
#endif

