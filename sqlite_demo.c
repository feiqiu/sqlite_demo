#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
  
#include "sqlite3.h"
 
#define SQL_DATABASE_FILES     "CREATE TABLE IF NOT EXISTS [files] ("\
"[path] TEXT  UNIQUE NULL PRIMARY KEY,"\
"[fps] INTEGER  NULL,"\
"[start_time] INTEGER  NULL,"\
"[end_time] INTEGER  NULL,"\
"[time_long] INTEGER  NULL,"\
"[size] INTEGER  NULL,"\
"[width] INTEGER  NULL,"\
"[height] INTEGER  NULL,"\
"[type] TEXT  NULL,"\
"[uuid] TEXT  NULL,"\
"[gps] TEXT  NULL)"
 
#define SQL_DATABASE_SYNCHRONOUS   ("PRAGMA synchronous = OFF")
 
 
#define  log_inf(fmt, ...) printf("\033[1;37m%s %04u "fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)
#define  log_imp(fmt, ...) printf("\033[0;32;32m%s %04u "fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)
#define  log_wrn(fmt, ...) printf("\033[0;35m%s %04u "fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)
#define  log_err(fmt, ...) printf("\033[0;32;31m%s %04u "fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)
 
 
typedef struct files_info_s{
    int fps;
    char path[255];
    time_t start_time;
    time_t end_time;
    time_t time_long;
    uint64_t size;
    int width;
    int height;
 
    char type[64];
 
    char uuid[64];
 
    char gps[255];
}files_info_t;
 
static volatile g_sql_mutex_inited = 0; 
static sqlite3_mutex * g_sql_mutex = NULL;
static sqlite3 * g_db = NULL; 
 
static sqlite3_mutex * sql_mutex_init(){
    sqlite3_mutex * mutex = NULL;
     
    if (g_sql_mutex_inited){
        log_wrn("sql mutex have inited\n");
        return g_sql_mutex;
    }
     
    mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_RECURSIVE);
    if (!mutex){
        log_err("mutex alloc failed\n");
        return NULL;
    }
     
    g_sql_mutex = mutex;
    g_sql_mutex_inited = 1;
     
    return g_sql_mutex;
} 
 
 
static void sql_mutex_free(){
    if (g_sql_mutex){
        sqlite3_mutex_free(g_sql_mutex);
        g_sql_mutex = NULL;
    }
     
    g_sql_mutex_inited = 0;
}
 
static void sql_mutex_enter(){
    if (g_sql_mutex){
            sqlite3_mutex_enter(g_sql_mutex);
    }
}
 
static void sql_mutex_leave(){
    if (g_sql_mutex){
            sqlite3_mutex_leave(g_sql_mutex);
    }
}
 
static int sql_mutex_try(){
    if (g_sql_mutex){
        return sqlite3_mutex_try(g_sql_mutex);
    }
     
    return -1;
}
 
 
static int sql_init(){
    if (!g_sql_mutex_inited){
        sql_mutex_init();
    }
     
    sql_mutex_enter();
     
    int ret = SQLITE_ERROR;
    //char * database = ":memory:";
    char * database = "./db.db";
     
    ret = sqlite3_open(database, &g_db);
    if (ret != SQLITE_OK){
        log_err("open database failed\n");
        goto end;
    }
     
    ret = sqlite3_exec(g_db, SQL_DATABASE_FILES, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        log_err("sqlite3_exec failed, create table [%s], error=[%s]\n", SQL_DATABASE_FILES, sqlite3_errmsg(g_db));
        goto end;
    }
     
    ret = sqlite3_exec(g_db, SQL_DATABASE_SYNCHRONOUS, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        log_err("sqlite3_exec failed, [%s]\n", SQL_DATABASE_SYNCHRONOUS);
        goto end;
    }
 
    ret = SQLITE_OK;
    log_imp("create database[%s] success\n", database);
 
end:;
 
    sql_mutex_leave();
    return ret;
}
 
static void sql_release(){
    if (!g_sql_mutex_inited){
        sql_mutex_init();
    }
     
    sql_mutex_enter();
     
    if (g_db){
        sqlite3_close(g_db);    
    }
     
    sql_mutex_leave();
}
 
static int sql_bind_info(sqlite3_stmt * stmt, files_info_t *file){
    sql_mutex_enter();
     
    if (!stmt){
        log_err("stmt is not null\n");
        goto end;
    }
     
    if (!file){
        log_err("out file is null\n");
        goto end;
    }
     
    int index = 0, ret = SQLITE_ERROR;
     
    index = sqlite3_bind_parameter_index(stmt, "@path");
    if (sqlite3_bind_text(stmt, index, file->path, strlen(file->path), SQLITE_TRANSIENT) != SQLITE_OK){
        log_err("bind path failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@fps");
    if (sqlite3_bind_int64(stmt, index, file->fps) != SQLITE_OK){
        log_err("bind fps failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@start_time");
    if (sqlite3_bind_int64(stmt, index, file->start_time) != SQLITE_OK){
        log_err("bind start_time failed\n");
        goto end;
    }
     
     
    index = sqlite3_bind_parameter_index(stmt, "@end_time");
    if (sqlite3_bind_int64(stmt, index, file->end_time) != SQLITE_OK){
        log_err("bind end_time failed\n");
        goto end;
    }
     
     
    index = sqlite3_bind_parameter_index(stmt, "@time_long");
    if (sqlite3_bind_int64(stmt, index, file->time_long) != SQLITE_OK){
        log_err("bind time_long failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@size");
    if (sqlite3_bind_int64(stmt, index, file->size) != SQLITE_OK){
        log_err("bind size failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@width");
    if (sqlite3_bind_int64(stmt, index, file->width) != SQLITE_OK){
        log_err("bind width failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@height");
    if (sqlite3_bind_int64(stmt, index, file->height) != SQLITE_OK){
        log_err("bind height failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@type");
    if (sqlite3_bind_text(stmt, index, file->type, strlen(file->type), SQLITE_TRANSIENT) != SQLITE_OK){
        log_err("bind type failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@uuid");
    if (sqlite3_bind_text(stmt, index, file->uuid, strlen(file->uuid), SQLITE_TRANSIENT) != SQLITE_OK){
        log_err("bind uuid failed\n");
        goto end;
    }
     
    index = sqlite3_bind_parameter_index(stmt, "@gps");
    if (sqlite3_bind_text(stmt, index, file->gps, strlen(file->gps), SQLITE_TRANSIENT) != SQLITE_OK){
        log_err("bind gps failed\n");
        goto end;
    }
     
    ret = SQLITE_OK;
end:;
 
    sql_mutex_leave();
    return ret;
}
 
static int sql_insert_prepare(sqlite3 *db, sqlite3_stmt ** stmt){
    sql_mutex_enter();
 
    int ret = SQLITE_ERROR;
     
    if (!db){
        log_err("db is not open\n");
        goto end;
    }
     
    if (!stmt){
        log_err("out stmt is null\n");
        goto end;
    }
     
    char * sql = "insert into files values ("
    "@path,"
    "@fps,"
    "@start_time,"
    "@end_time,"
    "@time_long,"
    "@size,"
    "@width,"
    "@height,"
    "@type,"
    "@uuid,"
    "@gps"
    ")";
     
    if (sqlite3_prepare(db, sql, strlen(sql), stmt, NULL) != SQLITE_OK){
        log_err("insert prepare failed\n");
        goto end;
    }
     
    ret = SQLITE_OK;
end:;
 
    sql_mutex_leave();
    return ret;
}
 
 
 
static int sql_insert_file(sqlite3 * db, files_info_t *file){
    sql_mutex_enter();
     
    int ret = SQLITE_ERROR;
    sqlite3_stmt * stmt = NULL;
     
    if (!file){
        log_err("file is null\n");
        goto end;
    }
     
    if (!db){
        log_err("db is null\n");
        goto end;
    }
     
    if (sql_insert_prepare(db, &stmt) != SQLITE_OK){
        log_err("insert prepare failed\n");
        goto end;
    }
     
    if (sql_bind_info(stmt, file) != SQLITE_OK){
        log_err("bind info failed\n");
        goto end;
    }
     
    if (sqlite3_step(stmt) != SQLITE_DONE){
        log_err("sqlite3_step failed\n");
        goto end;
    }
     
    ret = SQLITE_OK;
     
end:;
 
    if (stmt){
        sqlite3_finalize(stmt);
    }
    sql_mutex_leave();
    return ret;
}
 
 
extern int sql_add_file(files_info_t *file){
    sql_mutex_enter();
     
    if (!file){
        log_err("file is null\n");
        goto end;
    }
     
    int ret = SQLITE_ERROR;
     
    ret = sql_insert_file(g_db, file);   
end:;   
 
    sql_mutex_leave();
    return ret;
}
 
/*
static volatile value = 0;
 
static void * insert_thread(void * arg){
     
    while (1){
        sql_mutex_enter();
        value = 1;
        printf("thread-------------------value =[%d]\n", value);
        sql_mutex_leave();
        sleep(1);
    }
    return NULL;
}
     
*/
 
 
 
 
int main(int argc , char ** argv){
    sql_init();
     
    files_info_t file;
     
    bzero(&file, sizeof(file));
     
    strncpy(file.path, "test", sizeof(file.path));
    file.fps = 25;
    file.start_time = time(NULL);
    file.end_time = time(NULL);
    file.time_long = 0;
    file.size = 1000;
    file.width = 1080;
    file.height = 720;
     
    strncpy(file.type, "MP4", sizeof(file.path));
    strncpy(file.uuid, "-uuid-", sizeof(file.path));
    strncpy(file.gps, "gps info", sizeof(file.path));
     
    sql_add_file(&file);
     
    sql_release();
    /*
    pthread_t pid;
    pthread_create(&pid, NULL, insert_thread, NULL);
     
    while (1){
        sql_mutex_enter();
        value = 2;
        printf("main-------------------value =[%d]\n", value);
        sql_mutex_leave();
        sleep(1);
    }
    */
    return 0;
}
