/*******************************************************************

                Adam Seidman Default CLI Include

********************************************************************

 File Name:             AJScli.h
 Compiler:              ANSI C
 Creation Date:         2024-07-03
 Author:                Adam Seidman
 License:               MIT
 Revision:              1.0

*******************************************************************/

#ifndef AJS_CLI_H__
#define AJS_CLI_H__

#include "AJScliCfg.h"

/* ===== Override Macros ===== */
#ifndef _CLI_SET_OPS
    #ifdef _WIN32
        #define _CLI_SET_OPS    0
        #define _CLI_GET_CH     1
    #else
        #define _CLI_SET_OPS    1
        #define _CLI_GET_CH     0
    #endif // _WIN32
#elif !defined(_CLI_GET_CH)
    #define _CLI_GET_CH     0
#endif // _CLI_SET_OPS

#ifndef _CLI_RTOS_TASK_DELETE
    #ifdef INC_FREERTOS_H
        #define _CLI_RTOS_TASK_DELETE   1
    #else
        #define _CLI_RTOS_TASK_DELETE   0
    #endif // INC_FREERTOS_H
#endif // _CLI_RTOS_TASK_DELETE

/* ===== CLI Types ===== */
typedef long CliType_t;

#define CLI_ERRNO_NONE              0
#define CLI_ERRNO_UNKOWN_CMD        1
#define CLI_ERRNO_UNEXPECTED        2
#define CLI_ERRNO_NOMEM             3
#define CLI_ERRNO_NULL_PTR          4
#define CLI_ERRNO_BAD_FMT           5

#define CLI_OK                      (CLI_ERRNO_NONE)
#define CLI_ERR                     (-1)

#define CLI_TRUE                    (1)
#define CLI_FALSE                   (0)

/* ===== Required CLI Parameters ===== */
#ifndef CLI_PROMPT
#define CLI_PROMPT                  ("\033[0;37m>")
#endif

#ifndef CLI_NEWLINE
#define CLI_NEWLINE                 ("\r\n")
#endif

#ifndef CLI_MAX_COMMAND_LENGTH
#define CLI_MAX_COMMAND_LENGTH      (256)
#endif

#ifndef CLI_MAX_COMMAND_HISTORY
#define CLI_MAX_COMMAND_HISTORY     (32)
#endif

#ifndef CLI_MAX_COMMAND_ARGS
#define CLI_MAX_COMMAND_ARGS        (10)
#endif

#ifndef CLI_MAX_COMMAND_LISTS
#define CLI_MAX_COMMAND_LISTS       (32)
#endif

#ifndef CLI_SHOW_ONLY_ASCII
#define CLI_SHOW_ONLY_ASCII         (0)
#endif

#ifndef CLI_HAS_COLOR_PRINT
#define CLI_HAS_COLOR_PRINT         (1)
#endif

#ifndef CLI_HAS_INSERT_MODE
#define CLI_HAS_INSERT_MODE         (1)
#endif

#ifndef CLI_INIT
#define CLI_INIT(get, put)
#endif

/* ===== CLI Constants ===== */
#define CLI_CHAR_PRINT_MIN          (0x20)
#define CLI_CHAR_PRINT_MAX          (0x7E)
#define CLI_CHAR_CTRL_S             (0x13)
#define CLI_CHAR_CTRL_C             (0x03)
#define CLI_CHAR_RETURN             (0x0D)
#define CLI_CHAR_ESCAPE             (0x1B)
#define CLI_CHAR_BACKSPACE          (0x08)
#define CLI_CHAR_NULL               (0x00)
#define CLI_CHAR_BELL               (0x07)
#define CLI_CHAR_SPACE              (' ')
#define CLI_CHAR_ESC_PREFIX         ('[')
#define CLI_CHAR_ARROW_UP           ('A')
#define CLI_CHAR_ARROW_DOWN         ('B')
#define CLI_CHAR_ARROW_RIGHT        ('C')
#define CLI_CHAR_ARROW_LEFT         ('D')
#define CLI_CHAR_INSERT             ('O')
#define CLI_CHAR_DELETE             ('P')
#define CLI_STRING_CLEAR            ("\033[1;1H\033[2J")

#if _CLI_GET_CH
    #define CLI_CHAR_ESCAPE_READ        (0xE0)
    #define CLI_CHAR_ARROW_UP_READ      ('H')
    #define CLI_CHAR_ARROW_DOWN_READ    ('P')
    #define CLI_CHAR_ARROW_RIGHT_READ   ('M')
    #define CLI_CHAR_ARROW_LEFT_READ    ('K')
    #define CLI_CHAR_INSERT_READ        ('R')
    #define CLI_CHAR_DELETE_READ        ('S')
    #ifndef CLI_ESC_HAS_PREFIX
        #define CLI_ESC_HAS_PREFIX      (0)
    #endif
#else
    #define CLI_CHAR_ESCAPE_READ        (CLI_CHAR_ESCAPE)
    #define CLI_CHAR_ARROW_UP_READ      (CLI_CHAR_ARROW_UP)
    #define CLI_CHAR_ARROW_DOWN_READ    (CLI_CHAR_ARROW_DOWN)
    #define CLI_CHAR_ARROW_RIGHT_READ   (CLI_CHAR_ARROW_RIGHT)
    #define CLI_CHAR_ARROW_LEFT_READ    (CLI_CHAR_ARROW_LEFT)
    #define CLI_CHAR_INSERT_READ        (CLI_CHAR_INSERT)
    #define CLI_CHAR_DELETE_READ        (CLI_CHAR_DELETE)
    #ifndef CLI_ESC_HAS_PREFIX
        #define CLI_ESC_HAS_PREFIX      (1)
    #endif
#endif // _CLI_GET_CH

#define CLI_PRINTF_BUF                  (CLI_MAX_COMMAND_LENGTH)

/* ===== CLI Utility Macros ===== */
#define CLI_COMMAND_NEXT(x)             (( ((x) + 1) > CLI_MAX_COMMAND_HISTORY )? 0 : ((x) + 1))
#define CLI_COMMAND_PREV(x)             (( ((x) - 1) == -1)? (CLI_MAX_COMMAND_HISTORY) : ((x) - 1))

/* ===== CLI Public Structures/Defines ===== */
typedef CliType_t (*CliCommandFn_t)(int argc, char *argv[]);
typedef int (*CliGetCharFn_t)(void);
typedef void (*CliPutCharFn_t)(int c);
typedef void (*CliCtrlCFn_t)(void *arg);

typedef struct {
    const char *command;
    const char *usage;
    const char *help;
    CliCommandFn_t fn;
} CliCommand_t;

typedef struct {
    CliCommand_t *commands;
    int count;
} CliCommandList_t;

/* ===== CLI Public Functions ===== */
int cli_vprintf( const char *fmt, va_list ap );
int cli_printf( const char *fmt, ... );
int cli_printf_msg( const char *fmt, ... );
CliType_t cli_addList( CliCommand_t *list, int count );
void cli_setCtrlCOp( CliCtrlCFn_t ctrlC, void *args );
CliType_t cli_init( void );
void cli_task( void *params );

#if _CLI_SET_OPS
CliType_t cli_setOps( CliGetCharFn_t getChar, CliPutCharFn_t putChar );
#endif

#if CLI_HAS_COLOR_PRINT
    #define cli_printf_err(...)             \
        do {                                \
            cli_printf( "\033[0;31m" );     \
            cli_printf( __VA_ARGS__ );      \
            cli_printf( "\033[0;37m" );     \
        } while ( 0 )
#else
    #define cli_printf_err      cli_printf
#endif

#define cli_delete()                    \
    do {                                \
        cli_setOps( NULL, NULL );       \
        cli_setCtrlCOp( NULL );         \
    } while (0)

#endif /* AJS_CLI_H__ */
