/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


#define BUNDLED
#define PLUGINSTR "kv777"
#define PLUGNAME(NAME) kv777 ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#include "../mgw/old/sophia.h"
#define DEFINES_ONLY
#include "../common/system777.c"
#include "../KV/kv777.c"
#include "../agents/plugin777.c"
#include "../mgw/old/db777.c"
#undef DEFINES_ONLY

struct db777 *DB_msigs,*DB_NXTaccts,*DB_NXTtxids,*DB_MGW,*DB_redeems,*DB_NXTtrades;
struct db777_info SOPHIA;

char *PLUGNAME(_methods)[] = { "getPM", "getrawPM", "ping" };
char *PLUGNAME(_pubmethods)[] = { "getPM", "getrawPM", "ping" };
char *PLUGNAME(_authmethods)[] = { "getPM", "getrawPM", "ping",
#ifdef BUNDLED
    "get", "set", "object", "env", "ctl","open", "destroy", "error", "delete", "async", "drop", "cursor", "begin", "commit", "type",
#endif
};

//int32_t db777_idle(struct plugin_info *plugin) { return(kv777_idle("*")); }

// env = sp_env(void);
// ctl = sp_ctl(env): get an environment control object.
// sp_error(env): check if there any error leads to the shutdown.

// sp_open(env): create environment, open or create pre-defined databases.
// sp_open(database): create or open database.

// adatabase = sp_async(database) Get a special asynchronous object, which can be used instead of a database object. All operations executed using this object should run in parallel. Callback function is called on completion (db.name.on_complete).


// transaction = sp_begin(env): create a transaction
// During transaction, all updates are not written to the database files until a sp_commit(3) is called. All updates that were made during transaction are available through sp_get(3) or by using cursor.
// The sp_destroy(3) function is used to discard changes of a multi-statement transaction. All modifications that were made during the transaction are not written to the log file.
// No nested transactions are supported.

// sp_commit(transaction): commit a transaction
// The sp_commit(3) function is used to apply changes of a multi-statement transaction. All modifications that were made during the transaction are written to the log file in a single batch.
// If committion failed, transaction modifications are discarded.


// sp_cursor(ctl) Create cursor over all configuration values (single order only).
// sp_cursor(database, object) Create a database cursor. Object might have position key and iteration order set.
// Supported orders: ">", ">=", "<, "<=". using sp_set(object, "order", "<=");


// object = sp_object(database): create new object for a transaction on selected database.
// The sp_object function returns an object which is intended to be used in set/get operations. Object might contain a key-value pair with any additional metadata.
// sp_object(cursor): get current positioned object.


// sp_delete(database, object) Do a single-statement transaction.
// sp_delete(transaction, object) Do a key deletion as a part of multi-statement transaction.
// sp_drop(snapshot) Delete snapshot.
// sp_drop(database) Close a database (v1.2.1 equal to sp_destroy(3)).
//	int sp_destroy(transaction);

// sp_set(sp_ctl(env), "snapshot", "today");
// snapshot = sp_get(ctl, "snapshot.today");

// database, snapshot, async_database

// sp_set(ctl, "key", "value") Set configuration variable value or call a system procedure.
// sp_get(ctl, "key") Get configuration variable value object.

// sp_set(database, object) Do a single-statement transaction.
//sp_get(database, object) Do a single-statement transaction.

// sp_set(transaction, object) Do a key update as a part of multi-statement transaction.
//sp_get(transaction, object) Do a key search as a part of multi-statement transaction visibility.

// sp_set(object, "field", ptr, size) Set or update an object field.
// uint32_t size; sp_get(object, "field", &size) Get an object field and its size. Size can be NULL.


// void *sp_type(void*, ...);

/*Snapshots represent Point-in-Time read-only database view.

It is possible to do sp_get(3) or sp_cursor(3) on snapshot object. To create a snapshot, new snapshot name should be set to snapshot control namespace.

void *ctl = sp_ctl(env);
sp_set(ctl, "snapshot", "today");
void *snapshot = sp_get(ctl, "snapshot.today");
Snapshots are not persistent, therefore snapshot object must be recreated after shutdown before opening environment with latest snapshot LSN number: snapshot.name.lsn.

void *ctl = sp_ctl(env);
sp_set(ctl, "snapshot", "today");
sp_set(ctl, "snapshot.today.lsn", "12345");
To delete a snapshot, sp_drop(3) or sp_destroy(3) should be called on snapshot object.*/


// sp_set(ctl, "backup.run");
// sp_set(ctl, "scheduler.checkpoint");
// Database monitoring is possible by getting current dynamic statistics via sp_ctl(3) object.

// sp_set(ctl, "db.database.lockdetect", a) == 1;

#ifdef INSIDE_MGW
void *sophia_ptr(char *str)
{
    void *ptr = 0;
    if ( strlen(str) == (2 * sizeof(ptr)) )
        decode_hex((uint8_t *)&ptr,sizeof(ptr),str);
    return(ptr);
}

void *sophia_fieldptr(cJSON *json,char *fieldstr)
{
    void *ptr = 0;
    char str[MAX_JSON_FIELD];
    copy_cJSON(str,cJSON_GetObjectItem(json,fieldstr));
    if ( strlen(str) == (2 * sizeof(ptr)) )
        decode_hex((uint8_t *)&ptr,sizeof(ptr),str);
    return(ptr);
}

void sophia_retptrstr(char *retbuf,char *resultname,char *fieldname,void *ptr)
{
    char datastr[sizeof(ptr)*2 + 1];
    init_hexbytes_noT(datastr,(uint8_t *)&ptr,sizeof(ptr));
    sprintf(retbuf,"{\"result\":\"%s\",\"%s\":\"%s\"}",resultname,fieldname,datastr);
}

void sophia_retintstr(char *retbuf,char *resultname,int32_t retval)
{
    char datastr[sizeof(retval)*2 + 1];
    init_hexbytes_noT(datastr,(uint8_t *)&retval,sizeof(retval));
    sprintf(retbuf,"{\"method\":\"%s\",\"%s\":%d}",resultname,retval==0?"result":"error",retval);
}

void *sophia_funcptrcall(char *retbuf,int32_t max,cJSON *json,int32_t (*funcp)(void *,...),char *argfield)
{
    void *arg;
    char argptrstr[MAX_JSON_FIELD];
    copy_cJSON(argptrstr,cJSON_GetObjectItem(json,argfield));
    if ( (arg= sophia_ptr(argptrstr)) != 0 )
        (*funcp)(retbuf,(arg));
    return(arg);
}

void sophia_env(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"env","env",sp_env()); }

void sophia_ctl(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"ctl","ctl",sp_ctl(sophia_fieldptr(json,"env"))); }

void sophia_begin(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"begin","tx",sp_ctl(sophia_fieldptr(json,"env"))); }

void sophia_async(char *retbuf,int32_t max,cJSON *json) { sophia_retptrstr(retbuf,"async","asyncdb",sp_async(sophia_fieldptr(json,"db"))); }

void sophia_object(char *retbuf,int32_t max,cJSON *json)
{
    char *retname = "object";
    void *ptr;
    if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
        ptr = sophia_fieldptr(json,"cursor"), retname = "current";
    sophia_retptrstr(retbuf,"object",retname,sp_object(ptr));
}

void sophia_destroy(char *retbuf,int32_t max,cJSON *json) { sophia_retintstr(retbuf,"destroy",sp_destroy(sophia_fieldptr(json,"ptr"))); }

void sophia_commit(char *retbuf,int32_t max,cJSON *json) { sophia_retintstr(retbuf,"commit",sp_error(sophia_fieldptr(json,"env"))); }

void sophia_error(char *retbuf,int32_t max,cJSON *json) { sophia_retintstr(retbuf,"error",sp_error(sophia_fieldptr(json,"env"))); }

void sophia_open(char *retbuf,int32_t max,cJSON *json)
{
    void *ptr;
    if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
        ptr = sophia_fieldptr(json,"env");
    sophia_retintstr(retbuf,"open",sp_open(ptr));
}

void sophia_delete(char *retbuf,int32_t max,cJSON *json)
{
    void *ptr,*obj;
    if ( (obj= sophia_fieldptr(json,"object")) == 0 )
    {
        if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
            ptr = sophia_fieldptr(json,"tx");
        sophia_retintstr(retbuf,"open",sp_delete(ptr,obj));
    } else sprintf(retbuf,"{\"error\":\"sophia_delete cant get object\"}");
}

void sophia_drop(char *retbuf,int32_t max,cJSON *json)
{
    void *ptr;
    if ( (ptr= sophia_fieldptr(json,"db")) == 0 )
        ptr = sophia_fieldptr(json,"snapshot");
    sophia_retintstr(retbuf,"drop",sp_drop(ptr));
}

void sophia_cursor(char *retbuf,int32_t max,cJSON *json)
{
    void **obj;
    if ( (obj= sophia_fieldptr(json,"object")) == 0 )
        sophia_retptrstr(retbuf,"cursor","cursor",sp_cursor(sophia_fieldptr(json,"db"),obj));
    else sprintf(retbuf,"{\"error\":\"sophia_delete cant get object\"}");
}

void sophia_type(char *retbuf,int32_t max,cJSON *json)
{
    
}

void sophia_setget(char *retbuf,int32_t max,cJSON *json)
{
    char *keystr,*valstr,*ctlstr,*dbstr=0,*asyncstr=0; void *ctl,*db=0,*asyncdb=0,*obj,*value,*item;
    uint32_t size; int32_t err;
    keystr = cJSON_str(cJSON_GetObjectItem(json,"key"));
    valstr = cJSON_str(cJSON_GetObjectItem(json,"value"));
    if ( keystr != 0 && keystr[0] != 0 )
    {
        if ( (ctlstr= cJSON_str(cJSON_GetObjectItem(json,"ctl"))) != 0 )
        {
            if ( ctlstr[0] != 0 && (ctl= sophia_ptr(ctlstr)) != 0 )
            {
                if ( valstr != 0 )
                {
                    if ( (err= sp_set(ctl,keystr,valstr)) == 0 )
                        sprintf(retbuf,"{\"result\":\"setctl\",\"keystr\":\"%s\"}",valstr);
                    else strcpy(retbuf,"{\"error\":\"couldnt setctl\"}");
                }
                else
                {
                    if ( (value= sp_get(ctl,keystr,NULL)) != 0 )
                    {
                        if ( strlen(value) < max-64 )
                            sprintf(retbuf,"{\"result\":\"getctl\",\"keystr\":\"%s\"}",value);
                        else strcpy(retbuf,"{\"error\":\"value size too big\"}");
                        sp_destroy(value);
                    }
                }
            }
        }
        else if ( (dbstr= cJSON_str(cJSON_GetObjectItem(json,"db"))) != 0 || (asyncstr= cJSON_str(cJSON_GetObjectItem(json,"asyncdb"))) != 0 )
        {
            if ( dbstr[0] != 0 )
                db = sophia_ptr(dbstr);
            if ( asyncstr[0] != 0 )
                asyncdb = sophia_ptr(asyncstr);
            if ( db != 0 && (obj= sp_object(db)) != 0 )
            {
                if ( (err= sp_set(obj,"key",keystr,strlen(keystr))) == 0 )
                {
                    if ( (valstr= cJSON_str(cJSON_GetObjectItem(json,"value"))) != 0 )
                    {
                        if ( (err= sp_set(obj,"value",valstr,strlen(valstr))) != 0 )
                            strcpy(retbuf,"{\"error\":\"cant set object value\"}");
                        else if ( (err= sp_set(asyncdb!=0?asyncdb:db,obj)) != 0 )
                            strcpy(retbuf,"{\"error\":\"cant save object value\"}");
                        else strcpy(retbuf,"{\"result\":\"key/value set\"}");
                    }
                    else
                    {
                        if ( (item= sp_get(db,obj)) != 0 )
                        {
                            if ( (value= sp_get(item,"value",&size)) != 0 )
                            {
                                if ( size < max-64 )
                                    sprintf(retbuf,"{\"result\":\"getkey\",\"keystr\":\"%s\"}",value);
                                else strcpy(retbuf,"{\"error\":\"value size too big\"}");
                                sp_destroy(value);
                            } else strcpy(retbuf,"{\"error\":\"cant get value from item\"}");
                            sp_destroy(item);
                        } else strcpy(retbuf,"{\"error\":\"cant find item\"}");
                    }
                } else strcpy(retbuf,"{\"error\":\"cant set object key\"}");
                sp_destroy(obj);
            } else strcpy(retbuf,"{\"error\":\"invalid db or cant allocate object\"}");
        }
        else
        {
            //sp_get(transaction, object) Do a key search as a part of multi-statement transaction visibility.
            strcpy(retbuf,"{\"error\":\"unsupported transaction or search case\"}");
        }
    }
    else strcpy(retbuf,"{\"error\":\"missing key\"}");
}

int32_t db777_getind(struct db777 *DB)
{
    int32_t i;
    for (i=0; i<SOPHIA.numdbs; i++)
        if ( SOPHIA.DBS[i] == DB )
            return(i);
    return(-1);
}

struct db777 *db777_getDB(char *name)
{
    int32_t i;
    for (i=0; i<SOPHIA.numdbs; i++)
    {
        printf("(%s) ",SOPHIA.DBS[i]->name);
        if ( strcmp(SOPHIA.DBS[i]->name,name) == 0 )
            return(SOPHIA.DBS[i]);
    }
    return(0);
}

cJSON *db777_json(void *env,struct db777 *DB)
{
    void *cursor,*ptr,*ctl = sp_ctl(env);
    char *key,*value;
    cJSON *json = cJSON_CreateObject();
    cursor = sp_cursor(ctl);
    while ( (ptr= sp_get(cursor)) != 0 )
    {
        key = sp_get(ptr,"key",NULL), value = sp_get(ptr,"value",NULL);
        cJSON_AddItemToObject(json,key,cJSON_CreateString(value!=0?value:""));
    }
    sp_destroy(cursor);
    return(json);
}

struct db777 *_db777_restorebackup(char *specialpath,char *subdir,char *name,char *compression,char *namestr,char *backupdir,char *restoredir,char *restorelogdir,int32_t backupind)
{
    char fname[1024],_name[64],restorefname[1024],numstr[16]; int32_t iter,i,counter; FILE *fp; long fsize;
    for (iter=0; iter<2; iter++)
        for (i=1,counter=0; i<10000&&counter<100; i++,counter++)
        {
            sprintf(numstr,"%.10f",(double)i/10000000000.);
            if ( iter == 0 )
            {
                sprintf(_name,"log/%s.log",numstr+2);
                sprintf(fname,"%s/%d/%s",backupdir,backupind,_name);
                sprintf(restorefname,"%s/%s.log",restorelogdir,numstr+2);
            }
            else
            {
                sprintf(_name,"%s/%s.db",namestr,numstr+2);
                sprintf(fname,"%s/%d/%s",backupdir,backupind,_name);
                sprintf(restorefname,"%s/%s.db",restoredir,numstr+2);
            }
            if ( (fp= fopen(fname,"rb")) != 0 )
            {
                fclose(fp);
                counter = 0;
                fsize = copy_file(fname,restorefname);
                printf("FOUND.(%s) -> (%s) copied %s\n",fname,restorefname,_mbstr(fsize));
            } //else printf("skip %s (%s)\n",fname,numstr);
        }
    return(db777_create(specialpath,subdir,name,compression,1));
}

struct db777 *db777_restorebackup(struct db777 *DB,int32_t backupind)
{
    struct db777 *rDB;
    rDB = _db777_restorebackup(DB->argspecialpath,DB->argsubdir,DB->argname,DB->argcompression,DB->namestr,DB->backupdir,DB->restoredir,DB->restorelogdir,backupind);
    printf("%s backupind.%d %p\n",rDB->name,backupind,rDB);
    return(rDB);
}

void *db777_abort(struct db777 *DB)
{
    if ( DB->db != 0 )
        sp_destroy(DB->db);
    return(0);
}

struct db777 *db777_create(char *specialpath,char *subdir,char *name,char *compression,int32_t restoreflag)
{
    struct db777 *DB = calloc(1,sizeof(*DB));
    char path[1024],restorepath[1024],dbname[1024],*str,*namestr;
    int32_t err;
    cJSON *json;
    DB->flags = DB777_HDD;
    strcpy(DB->argname,name);
    if ( specialpath != 0 )
        strcpy(DB->argspecialpath,specialpath);
    if ( subdir != 0 )
        strcpy(DB->argsubdir,subdir);
    if ( compression != 0 )
        strcpy(DB->argcompression,compression);
    strcpy(dbname,name);
    if ( strlen(dbname) >= sizeof(DB->name)-1 )
        dbname[sizeof(DB->name)-1] = 0;
    strcpy(DB->name,dbname);
    DB->env = sp_env();
    DB->ctl = sp_ctl(DB->env);
    if ( SUPERNET.DBPATH[0] == '.' && (SUPERNET.DBPATH[1] == '/' || SUPERNET.DBPATH[1] == '\\') )
        strcpy(path,SUPERNET.DBPATH+2);
    else strcpy(path,SUPERNET.DBPATH);
    ensure_directory(path);
    if ( specialpath != 0 )
    {
        //printf("path.(%s) special.(%s) subdir.(%s) name.(%s)\n",path,specialpath,subdir,name);
        sprintf(path + strlen(path),"/%s",specialpath), ensure_directory(path);
        if ( subdir != 0 && subdir[0] != 0 )
            sprintf(path + strlen(path),"/%s",subdir), ensure_directory(path);
    }
    namestr = (compression != 0 && compression[0] != 0 ? compression : name), strcpy(DB->namestr,namestr);
    strcpy(restorepath,path), strcat(restorepath,"/restore"), os_compatible_path(restorepath), ensure_directory(restorepath);
    strcat(restorepath,"/"), strcat(restorepath,name), os_compatible_path(restorepath), ensure_directory(restorepath);
    strcpy(DB->restorelogdir,restorepath), strcat(DB->restorelogdir,"/log"), os_compatible_path(DB->restorelogdir), ensure_directory(DB->restorelogdir);
    strcpy(DB->restoredir,restorepath), strcat(DB->restoredir,"/"), strcat(DB->restoredir,namestr), os_compatible_path(DB->restoredir), ensure_directory(DB->restoredir);
    strcat(path,"/"), strcat(path,name), os_compatible_path(path), ensure_directory(path);
    strcpy(DB->backupdir,path), strcat(DB->backupdir,"/backups"), os_compatible_path(DB->backupdir);
    strcpy(dbname,namestr);
    printf("SOPHIA.(%s) MGW.(%s) create path.(%s).(%s) -> [%s %s].%s restore.(%s)\n",SOPHIA.PATH,MGW.PATH,name,compression,path,dbname,compression,restorepath);
    if ( (err= sp_set(DB->ctl,"sophia.path",restoreflag == 0 ? path : restorepath)) != 0 )
    {
        printf("err.%d setting path\n",err);
        return(db777_abort(DB));
    }
    if ( (err= sp_set(DB->ctl,"scheduler.threads","1")) != 0 )
    {
        printf("err.%d setting scheduler.threads\n",err);
        return(db777_abort(DB));
    }
    sprintf(DB->dbname,"db.%s",dbname);
    if ( (err= sp_set(DB->ctl,"db",dbname)) != 0 )
    {
        printf("err.%d setting name\n",err);
        return(db777_abort(DB));
    }
    if ( (err= sp_set(DB->ctl,"backup.path",DB->backupdir)) != 0 )
        printf("error.%d setting backup.path (%s)\n",err,DB->backupdir);
    if ( compression != 0 )
    {
        sprintf(dbname,"db.%s.compression",name);
        if ( strcmp(compression,"lz4") == 0 )
        {
            if ( sp_set(DB->ctl,"db.lz4.compression","lz4") != 0 )
                printf("error setting lz4.(%s)\n",dbname);
        }
        else if ( strcmp(compression,"zstd") == 0 )
        {
            if ( sp_set(DB->ctl,"db.zstd.compression","zstd") != 0 )
                printf("error setting zstd.(%s)\n",dbname);
        }
        else printf("compression.(%s) not supported\n",compression), compression = "";
    } else compression = "";
    if ( (err= sp_open(DB->env)) != 0 )
    {
        printf("err.%d setting sp_open\n",err);
        return(db777_abort(DB));
    }
    if ( (DB->db= sp_get(DB->ctl,DB->dbname)) == 0 )
    {
        if ( (err= sp_open(DB->db)) != 0 )
        {
            printf("err.%d sp_open db.(%s)\n",err,DB->dbname);
            return(db777_abort(DB));
        }
        printf("created.(%s)\n",DB->dbname);
    }
    else
    {
        DB->asyncdb = sp_async(DB->db);
        printf("opened.(%s) env.%p ctl.%p db.%p asyncdb.%p\n",DB->dbname,DB->env,DB->ctl,DB->db,DB->asyncdb);
    }
    if ( restoreflag != 0 && (json= db777_json(DB->env,DB)) != 0 )
    {
        str = cJSON_Print(json);
        printf("%s\n",str);
        free(str);
        free_json(json);
    }
    //else db777_restorebackup(specialpath,subdir,name,compression,DB->namestr,DB->backupdir,DB->restoredir,DB->restorelogdir,1);
    //SOPHIA.DBS = realloc(SOPHIA.DBS,(SOPHIA.numdbs+1) * sizeof(*DB));
    if ( SOPHIA.numdbs < sizeof(SOPHIA.DBS)/sizeof(*SOPHIA.DBS) )
        SOPHIA.DBS[SOPHIA.numdbs] = DB, SOPHIA.numdbs++;
    return(DB);
}

void db777_path(char *path,char *coinstr,char *subdir,int32_t useramdisk)
{
    if ( useramdisk == 0 || SOPHIA.RAMDISK[0] == 0 )
    {
        if ( SOPHIA.PATH[0] == '.' && SOPHIA.PATH[1] == '/' )
            strcpy(path,SOPHIA.PATH+2);
        else strcpy(path,SOPHIA.PATH);
    } else strcpy(path,SOPHIA.RAMDISK);
    ensure_directory(path);
    if ( coinstr[0] != 0 )
        strcat(path,"/"), strcat(path,coinstr), ensure_directory(path);
    if ( subdir[0] != 0 )
        strcat(path,"/"), strcat(path,subdir), ensure_directory(path);
   // printf("db777_path.(%s)\n",path);
}

struct db777 *db777_open(int32_t dispflag,struct env777 *DBs,char *name,char *compression,int32_t flags,int32_t valuesize)
{
    char path[1024],bdir[1024],compname[512]; int32_t err; struct db777 *DB = 0;
    if ( DBs->env == 0 && (flags & DB777_HDD) != 0 )
    {
        DBs->env = sp_env();
        DBs->ctl = sp_ctl(DBs->env);
        db777_path(path,DBs->coinstr,DBs->subdir,flags & DB777_RAMDISK);
        if ( (err= sp_set(DBs->ctl,"sophia.path",path)) != 0 )
            printf("err.%d setting path (%s)\n",err,path);
        if ( SOPHIA.RAMDISK[0] != 0 )
             db777_path(path,DBs->coinstr,DBs->subdir,0);
        strcpy(bdir,path), strcat(bdir,"/backups"), ensure_directory(bdir);
        if ( (err= sp_set(DBs->ctl,"backup.path",bdir)) != 0 )
            printf("error.%d settingB backup.path (%s)\n",err,bdir);
        else printf("set backup path to.(%s)\n",bdir);
        if ( (err= sp_set(DBs->ctl,"scheduler.threads","1")) != 0 )
            printf("err.%d setting scheduler.threads\n",err);
    }
    if ( DBs->env != 0 && DBs->numdbs < (int32_t)(sizeof(DBs->dbs)/sizeof(*DBs->dbs)) )
    {
        DB = &DBs->dbs[DBs->numdbs];
        memset(DB,0,sizeof(*DB));
        safecopy(DB->coinstr,DBs->coinstr,sizeof(DB->coinstr));
        safecopy(DB->name,name,sizeof(DB->name));
        portable_mutex_init(&DB->mutex);
        DB->flags = flags, DB->valuesize = valuesize;
        if ( (DB->flags & DB777_HDD) == 0 )
            return(DB);
        if ( compression != 0 )
            safecopy(DB->compression,compression,sizeof(DB->compression));
        sprintf(DB->dbname,"db.%s",name);
        if ( (err= sp_set(DBs->ctl,"db",name)) != 0 )
            printf("err.%d setting name\n",err);
        else
        {
            printf("path.(%s) name.(%s)\n",path,name);
            if ( compression != 0 )
            {
                sprintf(compname,"db.%s.compression",name);
                if ( sp_set(DBs->ctl,compname,compression) != 0 )
                    printf("error setting (%s).%s\n",compname,compression);
            }
            DBs->numdbs++;
            return(DB);
        }
    }
    return(0);
}

int32_t db777_dbopen(void *ctl,struct db777 *DB)
{
    int32_t err = -1;
    if ( (DB->db= sp_get(ctl,DB->dbname)) != 0 )
    {
        if ( (err= sp_open(DB->db)) != 0 )
        {
            //printf("err.%d sp_open will error if already exists\n",err);
        }
        DB->asyncdb = 0;//sp_async(DB->db);
        //printf("DB->db.%p for %s\n",DB->db,DB->dbname);
        return(0);
    }
    return(err);
}

int32_t env777_start(int32_t dispflag,struct env777 *DBs,uint32_t RTblocknum)
{
    uint32_t matrixkey; int32_t allocsize,err,i,j; struct db777 *DB; cJSON *json; char *str; void *ptr;
    fprintf(stderr,"Open environment\n");
    if ( (err= sp_open(DBs->env)) != 0 )
        printf("err.%d setting sp_open for DBs->env %p\n",err,DBs->env);
    DBs->start_RTblocknum = RTblocknum;
    DBs->matrixentries = ((RTblocknum * 10) / DB777_MATRIXROW) + 16;
    for (i=0; i<DBs->numdbs; i++)
    {
        DB = &DBs->dbs[i];
        DB->start_RTblocknum = RTblocknum;
        DB->reqsock = RELAYS.lbclient;
        if ( db777_dbopen(DBs->ctl,DB) == 0 )
        {
            if ( db777_matrixalloc(DB) != 0 )
            {
                DB->dirty = calloc(DBs->matrixentries,sizeof(*DB->dirty));
                DB->matrix = calloc(DBs->matrixentries,sizeof(*DB->matrix));
                DB->matrixentries = DBs->matrixentries;
                for (j=0; j<DB->matrixentries; j++)
                {
                    matrixkey = (j * DB777_MATRIXROW);
                    DB->matrix[j] = calloc(DB->valuesize,DB777_MATRIXROW);
                    allocsize = DB->valuesize * DB777_MATRIXROW;
                   // fprintf(stderr,"%s allocsize.%d read\n",DB->name,allocsize);
                    if ( (ptr= db777_read(DB->matrix[j],&allocsize,0,DB,&matrixkey,sizeof(matrixkey),1)) != 0 && allocsize == DB->valuesize * DB777_MATRIXROW )
                        fprintf(stderr,"+[%d] ",matrixkey);
                    else
                    {
                        printf("got allocsize.%d vs %d | ptr.%p vs matrix[] %p\n",allocsize,DB->valuesize * DB777_MATRIXROW,ptr,DB->matrix[j]);
                        break;
                    }
                }
                fprintf(stderr,"loaded matrix.%d from DB\n",j);
            }
            if ( dispflag != 0 && (json= db777_json(DBs->env,DB)) != 0 )
            {
                str = cJSON_Print(json);
                printf("%s\n",str);
                free(str);
                free_json(json);
            }
            printf("opened %s.(%s/%s) env.%p ctl.%p db.%p asyncdb.%p matrixalloc.%d | RAM.%d KEY32.%d valuesize.%d\n",DBs->coinstr,DBs->subdir,DB->name,DBs->env,DBs->ctl,DB->db,DB->asyncdb,DB->matrixentries,DB->flags & DB777_RAM,DB->flags & DB777_KEY32,DB->valuesize);
        }
        else
        {
            printf("error opening (%s)\n",DB->dbname);
            if ( DB->db != 0 )
                sp_destroy(DB->db), DB->db = DB->asyncdb = 0;
        }
    }
    return(0);
}

int32_t env777_close(struct env777 *DBs,int32_t reopenflag)
{
    int32_t i,errs = 0;
    if ( DBs->env != 0 )
    {
        for (i=0; i<DBs->numdbs; i++)
            if ( DBs->dbs[i].db != 0 )
                errs += (sp_destroy(DBs->dbs[i].db) != 0), DBs->dbs[i].db = DBs->dbs[i].asyncdb = 0;
        errs += (sp_destroy(DBs->env) != 0 );
        if ( errs == 0 && reopenflag != 0 )
        {
            for (i=0; i<DBs->numdbs; i++)
                errs += (db777_dbopen(DBs->ctl,&DBs->dbs[i]) != 0);
        }
        if ( errs != 0 )
            printf("env777_close reopenflag.%d errs.%d env (%s/%s)\n",reopenflag,errs,DBs->coinstr,DBs->subdir);
    }
    return(errs);
}
#endif

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *method,*resultstr,*path,*subdir,*pmstr,*retstr = 0; int32_t ind,len;
    //struct db777 *DB;
    //int32_t len,offset;
    retbuf[0] = 0;
    //printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        ensure_directory(SOPHIA.PATH);
        if ( SUPERNET.DBPATH[0] == 0 )
            strcpy(SUPERNET.DBPATH,"DB"), ensure_directory(SUPERNET.DBPATH);
        set_KV777_globals(&SUPERNET.relays,SUPERNET.transport,SUPERNET.NXTADDR,(int32_t)strlen(SUPERNET.NXTADDR),SUPERNET.SERVICENXT,SUPERNET.relayendpoint);
        strcpy(retbuf,"{\"result\":\"initflag > 0\"}");
        KV777.readyflag = 1;
        plugin->allowremote = 1;
        //Debuglevel = 3;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        method = cJSON_str(cJSON_GetObjectItem(json,"method"));
        path = cJSON_str(cJSON_GetObjectItem(json,"path"));
        subdir = cJSON_str(cJSON_GetObjectItem(json,"subdir"));
        if ( method == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        //printf("kv777.(%s)\n",method);
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(method,"ping") == 0 )
        {
            char *dKV777_processping(cJSON *json,char *jsonstr,char *sender,char *tokenstr);
            retstr = dKV777_processping(json,jsonstr,sender,tokenstr);
        }
        else if ( strcmp(method,"getPM") == 0 )
        {
            if ( (ind= juint(json,"ind")) == 0 )
                ind = -1;
            if ( SUPERNET.PM != 0 )
            {
                sprintf(retbuf,"{\"result\":\"success\",\"numkeys\":%d}",SUPERNET.PM->numkeys);
                if ( ind >= 0 )
                {
                    if ( (pmstr= kv777_read(SUPERNET.PM,(void *)&ind,sizeof(ind),0,&len,0)) != 0 )
                        sprintf(retbuf + strlen(retbuf) - 1,",\"ind\":%u,\"PM\":\"%s\",\"len\":%d}",ind,pmstr,len);
                }
            } else sprintf(retbuf,"{\"error\":\"no PM database\"}");
            //printf("retbuf.(%s)\n",retbuf);
        }
        else if ( strcmp(method,"getrawPM") == 0 )
        {
            if ( (ind= juint(json,"ind")) == 0 )
                ind = -1;
            if ( SUPERNET.rawPM != 0 )
            {
                sprintf(retbuf,"{\"result\":\"success\",\"numkeys\":%d}",SUPERNET.rawPM->numkeys);
                if ( ind >= 0 )
                {
                    if ( (pmstr= kv777_read(SUPERNET.rawPM,(void *)&ind,sizeof(ind),0,&len,0)) != 0 )
                        sprintf(retbuf + strlen(retbuf) - 1,",\"ind\":%u,\"rawPM\":\"%s\",\"len\":%d}",ind,pmstr,len);
                }
            } else sprintf(retbuf,"{\"error\":\"no rawPM database\"}");
            //printf("retbuf.(%s)\n",retbuf);
        }
        else sprintf(retbuf,"{\"error\":\"invalid kv777 method\",\"method\":\"%s\",\"tag\":\"%llu\"}",method,(long long)tag);
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            free(retstr);
        }
        if ( retbuf[0] == 0 )
            sprintf(retbuf,"{\"error\":\"null return\",\"method\":\"%s\",\"tag\":\"%llu\"}",method,(long long)tag);
        else sprintf(retbuf + strlen(retbuf) - 1,",\"tag\":\"%llu\"}",(long long)tag);
    }
    return(plugin_copyretstr(retbuf,maxlen,0));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
#ifdef INSIDE_MGW
    env777_close(&SUPERNET.DBs,0);
#endif
    
    return(retcode);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

#include "../agents/plugin777.c"
