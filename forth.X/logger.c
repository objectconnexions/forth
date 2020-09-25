#include "logger.h"
#include "forth.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static char *type[5] = {"TRACE", "DEBUG", "INFO", "ERROR", "OFF"};

enum LEVEL log_level;

void log_init()
{
    log_level = TRACE;
}

static void add_message(enum LEVEL log_level, char* context, char* message, va_list argptr)
{
    char msg[256];

    memset(msg, 0, sizeof(msg));
    vsprintf(msg, message, argptr);
    
    // TODO what happens if buffer is not long enough?
    
    printf("%s - %s [%s] %s\n", type[log_level], context, current_process->name, msg);
}


void log_trace(char *context, char *message, ...)
{
    if (log_level <= TRACE && current_process->log) 
    {
        va_list argptr;
        va_start(argptr, message);
        add_message(TRACE, context, message, argptr);
        va_end(argptr);
    }
}


void log_debug(char *context, char *message, ...)
{
    if (log_level <= DEBUG && current_process->log) 
    {
        va_list argptr;
        va_start(argptr, message);
        add_message(DEBUG, context, message, argptr);
        va_end(argptr);
    }
}

void log_info(char *context, char *message, ...)
{
    if (log_level <= INFO && current_process->log) 
    {
        va_list argptr;
        va_start(argptr, message);
        add_message(INFO, context, message, argptr);
        va_end(argptr);
    }
}

void log_error(char *context, char *message, ...)
{
    if (log_level <= ERROR && current_process->log) 
    {
        va_list argptr;
        va_start(argptr, message);
        add_message(ERROR, context, message, argptr);
        va_end(argptr);
    }
}
