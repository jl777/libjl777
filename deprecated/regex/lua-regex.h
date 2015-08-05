#ifndef LUA_REGEX_H
#define LUA_REGEX_H

/******************************************************************************
* Copyright (C) 1994-2008 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
/*
Adapted by Domingo Alvarez Duarte 2012
http://code.google.com/p/lua-regex-standalone/
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stddef.h>

/*
** maximum number of captures that a pattern can do during
** pattern-matching. This limit is arbitrary.
*/
#define LUA_REGEX_MAXCAPTURES		32

#define CAP_UNFINISHED	(-1)
#define CAP_POSITION	(-2)

typedef struct char_buffer_st
{
    size_t size, used;
    char buf[1];
} char_buffer_st;

typedef struct LuaCapture
{
    const char *init;
    ptrdiff_t len;
} LuaCapture;

typedef struct LuaMatchState
{
    const char *src_init;  // init of source string
    const char *src_end;  // end ('\0') of source string
    const char *p_end;  // end ('\0') of pattern
    const char *error;
    int level;  // total number of captures (finished or unfinished)
    LuaCapture capture[LUA_REGEX_MAXCAPTURES];
} LuaMatchState;

int lua_find(LuaMatchState *ms, const char *s, size_t ls,const char *p, size_t lp, size_t init, int raw_find);
int lua_match(LuaMatchState *ms, const char *s, size_t ls,const char *p, size_t lp, size_t init, int raw_find);
char_buffer_st *lua_gsub (const char *src, size_t srcl, const char *p, size_t lp,const char *tr, size_t max_s, const char **error_ptr);

#ifdef __cplusplus
}
#endif

#endif //LUA_REGEX_H
