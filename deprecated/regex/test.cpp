#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lua-regex.h"

std::string gsub(const char *src, const char *re, const char *replace){
    std::string result;
    LuaMatchState ms;
    int init = 0;
    while(init = str_find(&ms, src, strlen(src), re, strlen(re), init, 0)){
        std::string str;
        for(int i=0; i < ms.level; ++i){
            ptrdiff_t l = ms.capture[i].len;
            if (l == CAP_POSITION)
            //  lua_pushinteger(ms, ms->capture[i].init - ms->src_init + 1);
                printf("(pos %d:%d)\t", i, (int)(ms.capture[i].init));
            else
            {
                str.assign(ms.capture[i].init, ms.capture[i].len);
                printf("(%d:%s)\t", i, str.c_str());
            }
        }
    }
    //check for error
    if(ms.error){
        printf("%s\n", ms.error);
        ms.error = NULL;
    }
    return result;
}


int main()
{
    //str_find (MatchState *ms, const char *s, size_t ls,
    //          const char *p, , size_t lp, size_t init=1, int raw_find=0) {


    //int str_match (MatchState *ms, const char *s, size_t ls,
    //           const char *p, size_t ls, size_t init=1, bool raw_find=0) {

    LuaMatchState ms;

#define STR_SRC "1  33.34%  4   m\n2  50.00%  5   m"
//#define STR_SRC "1  m"
#define STR_PATTERN "(%d+)%s+(%d+%.%d+)%D+(%d+)%s+(%S+)"
//#define STR_PATTERN "(%d+)"
//#define STR_PATTERN "  33"
    int init = 0;
    while(init = str_find(&ms, STR_SRC, strlen(STR_SRC), STR_PATTERN, strlen(STR_PATTERN), init, 0)){
        std::string str;
        for(int i=0; i < ms.level; ++i){
            ptrdiff_t l = ms.capture[i].len;
            if (l == CAP_POSITION)
            //  lua_pushinteger(ms, ms->capture[i].init - ms->src_init + 1);
                printf("(pos %d:%d)\t", i, (int)(ms.capture[i].init));
            else
            {
                str.assign(ms.capture[i].init, ms.capture[i].len);
                printf("(%d:%s)\t", i, str.c_str());
            }
        }
    }
    //check for error
    if(ms.error){
        printf("%s\n", ms.error);
        ms.error = NULL;
    }
    printf("\nHello world!\n");

    const char src[] = "One 1 or Two 2";
    const char pat[] = "(%a+) (%d+)";
    const char *error_ptr;
    char_buffer_st *str = str_gsub(src, strlen(src), pat, strlen(pat), "%2 %1", 0, &error_ptr);
    if(str){
        printf("%s\n%s\n", src, str->buf);
        free(str);
    }
    else printf("%s", error_ptr);
    return 0;
}

