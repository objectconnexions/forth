
#ifndef _LOGGER_H    /* Guard against multiple inclusion */
#define _LOGGER_H


#define NL '\n'
#define SPACE ' '

enum LEVEL {
    TRACE,
    DEBUG,
    INFO,
    ERROR,
    OFF
};

extern enum LEVEL log_level;

void log_init(void);

void log_trace(char *context, char *message, ...);

void log_debug(char *context, char *message, ...);

void log_info(char *context, char *message, ...);

void log_warn(char *context, char *message, ...);

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
