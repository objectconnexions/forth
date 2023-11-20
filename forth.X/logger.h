
#ifndef _LOGGER_H    /* Guard against multiple inclusion */
#define _LOGGER_H

#include <stdint.h>
#include <stdbool.h>   


#define NL '\n'
#define SPACE ' '

enum LEVEL {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    OFF
};

void log_set_level(uint8_t level);

bool log_is_trace(void);

/*
 * Initialise the logging system before it is used.
 */
void log_init(void);

/*
 * Write a log entry (as described for log_debug()) if the logging level is TRACE
 * or greater.
 */
void log_trace(char *context, char *message, ...);

/*
 * Write a log entry (as described for log_debug()) if the logging level is DEBUG
 * or greater.
 */
void log_debug(char *context, char *message, ...);

/*
 * Write out the specified context name and log message to the logging system if
 * the logging level is INFO or greater. Template markers in the message are 
 * replaced by the parameters following the message (if any). The templates are:-
 * 
 *      %S - character string
 *      %I - 32 bit integer, displayed as a decimal
 *      %X - integer displayed as a 2 digit hex number (values 0-255 -> 00~FF)
 *      %Y - integer displayed as a 4 digit hex number (values 0000~FFFF)
 *      %X - integer displayed as a 8 digit hex number (values 00000000~FFFFFFFF)
 * 
 */
void log_info(char *context, char *message, ...);

/*
 * Write a log entry (as described for log_debug()) if the logging level is WARN
 * or ERROR.
 */
void log_warn(char *context, char *message, ...);

/*
 * Write a log entry (as described for log_debug()) if the logging level is ERROR
 */
void log_error(char *context, char *message, ...);

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
