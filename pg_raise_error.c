#include <string.h>

#include "postgres.h"
#include "fmgr.h"
#include "optimizer/planner.h"
#include "utils/builtins.h"
#include "executor/spi.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define ERROR_TRIGGER_TABLE_NAME "error_triggers"

void _PG_init(void);
void _PG_fini(void);

static PlannedStmt *custom_planner_hook(Query *parse,
                                        const char *query_string,
                                        int cursor_options,
                                        ParamListInfo bound_params);

static void do_countdown_raise_error(char *query_string);

typedef struct LevelMap
{
    char *level_string;
    int level_value;
} LevelMap;
LevelMap level_mapper[] =
    {
        {"log", LOG},
        {"info", INFO},
        {"error", ERROR},
        {"fatal", FATAL},
        {"panic", PANIC}};

char query_strings[][200] = {
    "SELECT * FROM information_schema.tables WHERE table_name = '" ERROR_TRIGGER_TABLE_NAME "'",
    "UPDATE " ERROR_TRIGGER_TABLE_NAME " SET count=count-1",
    "SELECT level FROM " ERROR_TRIGGER_TABLE_NAME " WHERE count=0",
    "DELETE FROM error_triggers WHERE count=0",
    "TRUNCATE TABLE error_triggers"};

PG_FUNCTION_INFO_V1(set_error_trigger);
Datum set_error_trigger(PG_FUNCTION_ARGS)
{
    int count = PG_GETARG_INT32(0);
    text *level = PG_GETARG_TEXT_P(1);
    char *level_c = text_to_cstring(level);

    char query_string[200];

    sprintf(query_string, "INSERT INTO %s(count, level) VALUES(%d, '%s')", ERROR_TRIGGER_TABLE_NAME, count, level_c);
    SPI_connect();
    SPI_exec(query_string, 0);
    SPI_finish();

    PG_RETURN_BOOL(true);
}

PG_FUNCTION_INFO_V1(clear_error_trigger);
Datum clear_error_trigger(PG_FUNCTION_ARGS)
{
    SPI_connect();
    SPI_exec(query_strings[4], 0);
    SPI_finish();

    PG_RETURN_BOOL(true);
}

static planner_hook_type prev_planner = NULL;

void _PG_init(void)
{
    prev_planner = planner_hook;
    planner_hook = custom_planner_hook;
}

void _PG_fini(void)
{
    planner_hook = prev_planner;
}

static PlannedStmt *custom_planner_hook(Query *parse,
                                        const char *query_string,
                                        int cursor_options,
                                        ParamListInfo bound_params)
{
    do_countdown_raise_error(query_string);
    return standard_planner(parse, query_string, cursor_options, bound_params);
}

static void do_countdown_raise_error(char *query_string)
{
    for (int i = 0; i < 5; i++)
    {
        if (strcmp(query_string, query_strings[i]) == 0)
        {
            return;
        }
    }
    if (strstr(query_string, "set_error_trigger()") != NULL || strstr(query_string, "clear_error_trigger()") != NULL)
    {
        return;
    }

    SPI_connect();
    SPI_exec(query_strings[0], 0);
    int proc;
    proc = SPI_processed;
    if (proc == 0 || SPI_tuptable == NULL)
    {
        SPI_finish();
        return;
    }
    SPI_exec(query_strings[1], 0);
    SPI_exec(query_strings[2], 0);
    proc = SPI_processed;
    if (proc != 0 && SPI_tuptable != NULL)
    {
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        SPITupleTable *tuptable = SPI_tuptable;
        SPI_exec(query_strings[3], 0);
        for (int i = 0; i < proc; i++)
        {
            HeapTuple tuple = tuptable->vals[i];
            char *level = SPI_getvalue(tuple, tupdesc, 1);
            for (int j = 0; j < 5; j++)
            {
                if (strcmp(level, level_mapper[j].level_string) == 0)
                {
                    elog(level_mapper[j].level_value, "COUNT IS ZERO!!");
                    break;
                }
            }
        }
    }
    SPI_finish();
}
