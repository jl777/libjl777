#include "sqlite3.h"
#include "lua-regex.h"

static void sqlite3_regexp( sqlite3_context *context, int argc, sqlite3_value **argv ) {
    assert(argc == 2);
    LuaMatchState ms;
    const char *pattern = (const char *)sqlite3_value_text(argv[1]);
    const char *subject = (const char *)sqlite3_value_text(argv[0]);
    if(str_find(&ms, pattern, strlen(pattern), subject, strlen(subject), 0, 0))
        sqlite3_result_int(context, 1);
    else
    {
        if(ms.error) sqlite3_result_error(context, ms.error, strlen(ms.error));
        else sqlite3_result_int(context, 0);
    }
}

int set_sqlite3_regexp_func(sqlite3 *db)
{
    int nErr = sqlite3_create_function(db,  "regexp", 2, SQLITE_UTF8, 0, sqlite3_regexp, 0, 0);
    nErr += sqlite3_create_function(db,  "match", 2, SQLITE_UTF8, 0, sqlite3_regexp, 0, 0);
	return nErr ? SQLITE_ERROR : SQLITE_OK;
}
