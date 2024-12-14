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

#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "nb.h"
#include "nb_int.h"

// byte nibble vs ASCII char
#define NTOA(n)                 ((n) > 9   ? (n) + 55 : (n) + 48)
#define ATON(a)                 ((a) > '9' ? (a) - 55 : (a) - 48)

typedef struct {
    void *pv_vm;
    char *p_src;
    int src_pos;
} nb_cpu_t;

/**************************************************************************************************
** Static helper functions
**************************************************************************************************/
static char *hash_uint16(uint16_t val, char *s) {
    *s++ = 48 + (val % 64);
    val = val / 64;
    *s++ = 48 + (val % 64);
    val = val / 64;
    *s++ = 48 + val;
    return s;
}

static void *check_vm(lua_State *L) {
    void *ud = luaL_checkudata(L, 1, "nb_cpu");
    luaL_argcheck(L, ud != NULL, 1, "'NanoBasic object' expected");
    return ud;
}

static uint16_t table_to_bytes(lua_State *L, uint8_t idx, uint8_t *p_dest, uint16_t max_size) {
    if(lua_istable(L, idx)) {
        size_t num = lua_objlen(L, idx);
        num = MIN(num, max_size);
        for(size_t i = 0; i < num; i++) {
            lua_rawgeti(L, idx, i+1);
            p_dest[i] = luaL_checkinteger(L, -1);
            lua_pop(L, 1);
        }
        return num;
    }
    return 0;
}

static void bin_to_str(char *p_dst_str, uint8_t *p_src_bin, uint32_t str_size) {
    for(int i = 0; i < str_size/2; i++) {
        *p_dst_str++ = NTOA(*p_src_bin >> 4);
        *p_dst_str++ = NTOA(*p_src_bin & 0x0f);
        p_src_bin++;
    }
}

static void str_to_bin(uint8_t *p_dst_bin, char *p_src_str, uint32_t str_size) {
    for(int i = 0; i < str_size/2; i++) {
        *p_dst_bin++ = (ATON(p_src_str[0]) << 4) + ATON(p_src_str[1]);
        p_src_str += 2;
    }
}

/**************************************************************************************************
** External NanoBasic functions
**************************************************************************************************/
char *nb_get_code_line(void *fp, char *line, int max_line_len)
{
    nb_cpu_t *C = (nb_cpu_t *)fp;
    if(C->p_src == NULL) {
        return NULL;
    }
    if(C->p_src[C->src_pos] == '\0') {
        return NULL;
    }
    int len = 0;
    while(C->p_src[C->src_pos] != '\n' && C->p_src[C->src_pos] != '\0' && len < max_line_len) {
        line[len++] = C->p_src[C->src_pos++];
    }
    line[len] = '\0';
    if(C->p_src[C->src_pos] == '\n') {
        C->src_pos++;
    }
    return line;
}

void nb_print(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

/**************************************************************************************************
** Lua API functions
**************************************************************************************************/
static int version(lua_State *L) {
    lua_pushstring(L, SVERSION);
    return 1;
}

static int add_function(lua_State *L) {
    char *name = (char *)luaL_checkstring(L, 1);
    uint8_t return_type = luaL_checkinteger(L, 3);
    uint8_t types[8];
    uint8_t num = table_to_bytes(L, 2, types, sizeof(types));
    uint8_t res = nb_define_external_function(name, num, types, return_type);
    lua_pushinteger(L, res + NB_XFUNC);
    return 1;
}

static int create(lua_State *L) {   
    size_t size;
    char *p_src = (char*)lua_tolstring(L, 1, &size);
    nb_cpu_t *C = (nb_cpu_t *)lua_newuserdata(L, sizeof(nb_cpu_t));
    if(C != NULL) {
        C->pv_vm = nb_create();
        if(C->pv_vm == NULL) {
            lua_pushinteger(L, -1);
            return 2;
        }
        C->p_src = p_src;
        C->src_pos = 0;
        uint16_t errors = nb_compile(C->pv_vm, (void *)C);
        printf("errors=%d\n", errors);
        if(errors > 0) {
            lua_pushinteger(L, errors);
            return 2;
        }
        luaL_getmetatable(L, "nb_cpu");
        lua_setmetatable(L, -2);
        lua_pushinteger(L, 0);
        return 2;
    }
    lua_pop(L, 1);
    lua_pushnil(L);
    lua_pushinteger(L, -2);
    return 2;
}

static int run(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    lua_Integer cycles = luaL_checkinteger(L, 2);
    if(C != NULL) {
        int res = nb_run(C->pv_vm, cycles);
        lua_pushinteger(L, res);
        return 1;
    }
    lua_pushinteger(L, -1);
    return 1;
}

static int reset(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        nb_reset(C->pv_vm);
        lua_pushboolean(L, 1);
        return 1;
    }
    lua_pushboolean(L, 0);
    return 1;
}

static int pack_vm(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        // pack the VM into a Lua string by means of bin_to_str (binary to HEX string conversion)
        char s[sizeof(t_VM)*2];
        bin_to_str(s, (uint8_t*)C->pv_vm, sizeof(t_VM));
        lua_pushlstring(L, s, sizeof(t_VM)*2);
        return 1;
    }
    return 0;
}

static int unpack_vm(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if((C != NULL) && (lua_isstring(L, 2))) {
        size_t size;
        const char *s = lua_tolstring(L, 2, &size);
        if(size == sizeof(t_VM)*2) {
            // unpack the VM from a Lua string by means of str_to_bin (HEX string to binary conversion)
            str_to_bin((uint8_t*)C->pv_vm, (char*)s, sizeof(t_VM)*2);
            lua_pushboolean(L, 1);
            return 1;
        }
    }
    lua_pushboolean(L, 0);
    return 1;
}

static int destroy(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        nb_destroy(C->pv_vm);
        C->pv_vm = NULL; 
    }
    return 0;
}

static int dump_code(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        nb_dump_code(C->pv_vm);
        return 0;
    }
    return 0;
}

static int output_symbol_table(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        nb_output_symbol_table(C->pv_vm);
        return 0;
    }
    return 0;
}

static int get_label_address(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        char *name = (char *)luaL_checkstring(L, 2);
        uint16_t res = nb_get_label_address(C->pv_vm, name);
        lua_pushinteger(L, res);
        return 1;
    }
    return 0;
}

static int set_pc(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        uint16_t addr = luaL_checkinteger(L, 2);
        nb_set_pc(C->pv_vm, addr);
        return 0;
    }
    return 0;
}

static int stack_depth(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        uint8_t res = nb_stack_depth(C->pv_vm);
        lua_pushinteger(L, res);
        return 1;
    }
    return 0;
}

static int pop_num(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        uint32_t res = nb_pop_num(C->pv_vm);
        lua_pushinteger(L, res);
        return 1;
    }
    return 0;
}

static int push_num(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        uint32_t value = luaL_checkinteger(L, 2);
        nb_push_num(C->pv_vm, value);
        return 0;
    }
    return 0;
}

static int push_str(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        char *str = (char *)luaL_checkstring(L, 2);
        nb_push_str(C->pv_vm, str);
        return 0;
    }
    return 0;
}

static int pop_str(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        char str[256];
        char *res = nb_pop_str(C->pv_vm, str, (uint8_t)sizeof(str));
        if(res != NULL) {
            lua_pushstring(L, res);
            return 1;
        }
    }
    return 0;
}

static int get_var_num(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        char *name = (char *)luaL_checkstring(L, 2);
        uint16_t res = nb_get_var_num(C->pv_vm, name);
        lua_pushinteger(L, res);
        return 1;
    }
    return 0;
}

static int read_arr(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        uint8_t var = luaL_checkinteger(L, 2);
        uint8_t arr[256];
        uint16_t bytes = nb_read_arr(C->pv_vm, var, arr, sizeof(arr));
        if(bytes > 0) {
            lua_newtable(L);
            for(uint16_t i = 0; i < bytes; i++) {
                lua_pushinteger(L, arr[i]);
                lua_rawseti(L, -2, i+1);
            }
            return 1;
        }
    }
    return 0;
}

static int write_arr(lua_State *L) {
    nb_cpu_t *C = check_vm(L);
    if(C != NULL) {
        uint8_t var = luaL_checkinteger(L, 2);
        uint8_t arr[256];
        uint16_t num = table_to_bytes(L, 3, arr, sizeof(arr));
        uint16_t res = nb_write_arr(C->pv_vm, var, arr, num);
        lua_pushinteger(L, res);
        return 1;
    }
    return 0;
}

static int hash_node_position(lua_State *L) {
    int16_t x, y, z;
    char s[12];
    char *ptr = s;
    
    if(lua_istable(L, 1)) {
        lua_getfield(L, 1, "x");
        lua_getfield(L, 1, "y");
        lua_getfield(L, 1, "z");
        x = luaL_checkint(L, -3);
        y = luaL_checkint(L, -2);
        z = luaL_checkint(L, -1);
        lua_settop(L, 0);

        ptr = hash_uint16(x + 32768, ptr);
        ptr = hash_uint16(y + 32768, ptr);
        ptr = hash_uint16(z + 32768, ptr);
        *ptr = '\0';

        lua_pushlstring(L, s, 9);
        return 1;
    }
    return 0;
}

/* msleep(): Sleep for the requested number of milliseconds. */
static int msleep(lua_State *L) {
    uint32_t msec = luaL_checkinteger(L, 1);
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
    lua_pushinteger(L, res);
    return 1;
}

static const luaL_Reg R[] = {
    {"version",                 version},
    {"add_function",            add_function},
    {"create",                  create},
    {"reset",                   reset},
    {"destroy",                 destroy},
    {"pack_vm",                 pack_vm},
    {"unpack_vm",               unpack_vm},
    {"run",                     run},
    {"dump_code",               dump_code},
    {"output_symbol_table",     output_symbol_table},
    {"get_label_address",       get_label_address},
    {"set_pc",                  set_pc},
    {"stack_depth",             stack_depth},
    {"pop_num",                 pop_num},
    {"push_num",                push_num},
    {"pop_str",                 pop_str},
    {"push_str",                push_str},
    {"get_var_num",             get_var_num},
    {"read_arr",                read_arr},
    {"write_arr",               write_arr},
    {"hash_node_position",      hash_node_position},
    {"msleep",                  msleep},
    {NULL, NULL}
};

/* }====================================================== */
LUALIB_API int luaopen_nanobasiclib(lua_State *L) {
    nb_init();
    luaL_newmetatable(L, "nb_cpu");
    luaL_register(L, NULL, R);
    return 1;
}
