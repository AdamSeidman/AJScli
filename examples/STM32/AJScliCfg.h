#ifndef AJS_CLI_CFG_H
#define AJS_CLI_CFG_H

#include "project.h"

#define CLI_MAX_COMMAND_LENGTH      (256)
#define CLI_MAX_COMMAND_HISTORY     (32)
#define CLI_MAX_COMMAND_ARGS        (10)
#define CLI_MAX_COMMAND_LISTS       (32)
#define CLI_SHOW_ONLY_ASCII         (0)
#define CLI_HAS_COLOR_PRINT         (1)
#define CLI_HAS_INSERT_MODE         (1)
#define CLI_INIT_TEXT               CLI_NEWLINE
#define CLI_RTOS_TASK_DELETE        (1)

#define CLI_INIT(getChar, putChar)                  \
    do {                                            \
        vTaskDelay(5);                              \
    } while ( getChar == NULL || putChar == NULL )

#endif /* AJS_CLI_CFG_H */
