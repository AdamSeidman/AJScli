/*******************************************************************

                Adam Seidman Default CLI Source

********************************************************************

 File Name:             AJScli.c
 Compiler:              ANSI C
 Creation Date:         2024-07-03
 Author:                Adam Seidman
 License:               MIT
 Revision:              1.0

*******************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#endif

#include "AJScli.h"

static char commandHistory[CLI_MAX_COMMAND_HISTORY + 1][CLI_MAX_COMMAND_LENGTH];
static char currentCommand[CLI_MAX_COMMAND_LENGTH];
static char printfBuf[CLI_PRINTF_BUF];

static CliGetCharFn_t getCharFn = NULL;
static CliPutCharFn_t putCharFn = NULL;
static CliCtrlCFn_t ctrlCFn = NULL;
static void *ctrlCArgs = NULL;

static CliCommandList_t commandLists[CLI_MAX_COMMAND_LISTS];

static CliType_t prvCommandHelp(int argc, char *argv[]);
static CliType_t prvCommandHistory(int argc, char *argv[]);
static CliType_t prvClearScreen(int argc, char *argv[]);

static CliCommand_t defaultCommands[] = {
    {   .command    = "help",
        .usage      = "<command>",
        .help       = "Run help without <command> to see available commands\r\n    Use <command> to get detailed help on a specific command",
        .fn         = &prvCommandHelp,
    },
    {   .command    = "history",
        .usage      = "",
        .help       = "Show command history",
        .fn         = &prvCommandHistory,
    },
    {
        .command    = "cls",
        .usage      = "",
        .help       = "Clear screen",
        .fn         = &prvClearScreen,
    },
};

static int historyCommand;
static int currentCommandIdx;
static struct Flags_s {
    unsigned screenCleared   : 1;
    unsigned insertMode      : 1;
} flags = {0};

// Repeatable call for Ctrl-C, if registered
static void prvCallCtrlC( void )
{
    if ( ctrlCFn != NULL ) {
        ctrlCFn( ctrlCArgs );
    }
}

// Wrapper for getChar that checks for NULL
static int prvGetChar( void )
{
    if ( getCharFn == NULL ) {
        printf( "FATAL: getChar occurred with NULL pointer!\r\n" );
        prvCallCtrlC();
    }

    return getCharFn();
}

// Wrapper for putChar that checks for NULL
static void prvPutChar( char c )
{
    if ( putCharFn == NULL ) {
        printf( "FATAL: putChar occurred with NULL pointer!\r\n" );
        prvCallCtrlC();
    }
    else {
        putCharFn(c);
    }
}

// Get a command based on the strings we know of
static CliCommand_t *prvGetCommand( char *command )
{
    int i, j;
    for ( i = 0; i < CLI_MAX_COMMAND_LISTS; i++ ) {
        for ( j = 0; j < commandLists[i].count; j++ ) {
            if ( 0 == strncmp(command, commandLists[i].commands[j].command, CLI_MAX_COMMAND_LENGTH) ) {
                return &commandLists[i].commands[j];
            }
        }
    }

    return NULL;
}

// Default command to print CLI help information
static CliType_t prvCommandHelp( int argc, char *argv[] )
{
    int i, j;

    if ( argc == 1 ) {
        // Generic help message
        cli_printf( "%sCommand\t\tUsage%s===================================%s",
            CLI_NEWLINE, CLI_NEWLINE, CLI_NEWLINE );
        for ( i = 0; i < CLI_MAX_COMMAND_LISTS; i++ ) {
            for ( j = 0; j < commandLists[i].count; j++ ) {
                cli_printf("%s\t\t%s %s%s",
                    commandLists[i].commands[j].command,
                    commandLists[i].commands[j].command,
                    commandLists[i].commands[j].usage,
                    CLI_NEWLINE );
            }
        }
    }
    else {
        // Requested help about specific command
        CliCommand_t *cmd = prvGetCommand( argv[1] );
        if ( cmd == NULL ) {
            cli_printf_err( "Could not find \"%s\"%s", argv[1], CLI_NEWLINE );
            return CLI_ERRNO_UNKOWN_CMD;
        }
        else {
            cli_printf( "%s%s %s%s    %s%s",
                CLI_NEWLINE,
                cmd->command,
                cmd->usage,
                CLI_NEWLINE,
                cmd->help,
                CLI_NEWLINE );
        }
    }

    return CLI_OK;
}

// Clear out terminal
static CliType_t prvClearScreen( int argc, char *argv[] )
{
    (void) argc;
    (void) argv;
    cli_printf( CLI_STRING_CLEAR );
    flags.screenCleared = CLI_TRUE;
    return CLI_OK;
}

// Default command to print history of CLI
static CliType_t prvCommandHistory( int argc, char *argv[] )
{
    int i = CLI_COMMAND_NEXT( historyCommand );
    int j = 1;

    while ( i != currentCommandIdx ) {
        if ( strnlen(commandHistory[i], CLI_MAX_COMMAND_LENGTH) > 0 ) {
            cli_printf( "\t%d %s", j++, commandHistory[i] );
            cli_printf( CLI_NEWLINE );
        }
        i = CLI_COMMAND_NEXT(i);
    }

    return CLI_OK;
}

// Find command within lists and call its function
static void prvCallCommand( char *command )
{
    // Terminate, if necessary
    int i = strnlen(command, CLI_MAX_COMMAND_LENGTH - 1);
    command[i] = CLI_CHAR_NULL; 

    // Remove any trailing spaces
    while ( command[--i] == CLI_CHAR_SPACE ) {
        command[i] = CLI_CHAR_NULL;
    }

    char *argv[CLI_MAX_COMMAND_ARGS] = {0};
    int argc = 0;

    argv[ argc++ ] = command;

    // Split arguments
    while ( *command != CLI_CHAR_NULL ) {
        if ( argc == CLI_MAX_COMMAND_ARGS ) {
            break;
        }
        else if ( *command == CLI_CHAR_SPACE ) {
            *(command++) = CLI_CHAR_NULL;
            argv[ argc++ ] = command;
        }
        else {
            ++command;
        }
    }

    // Find Command
    CliCommand_t *cmd = prvGetCommand( argv[0] );
    if ( cmd == NULL ) {
        cli_printf_err( "Could not find \"%s\"%s", argv[0], CLI_NEWLINE );
        return;
    }

    // Execute Command
    int err = cmd->fn( argc, argv );
    if ( err != CLI_OK ) {
        cli_printf_err("%sCommand \"%s\" returned error code: %d%s",
            CLI_NEWLINE,
            cmd->command,
            err, CLI_NEWLINE );
    }
}

/* ===== Public Functions ===== */
int cli_printf( const char *fmt, ... )
{
    va_list ap;
    va_start( ap, fmt );
    int r = vsnprintf( printfBuf, CLI_PRINTF_BUF, fmt, ap );
    va_end(ap);
    printfBuf[ CLI_PRINTF_BUF - 1 ] = CLI_CHAR_NULL;

    int i = 0;
    while ( printfBuf[i] != CLI_CHAR_NULL ) {
        prvPutChar( printfBuf[i++] );
    }

    return r;
}

// Add a list of commands to our command list array
CliType_t cli_addList( CliCommand_t *list, int count )
{
    int i = 0;
    while ( i < CLI_MAX_COMMAND_LISTS ) {
        // Find empty spot to add list
        if ( commandLists[i].commands == NULL ) {
            commandLists[i].commands = list;
            commandLists[i].count = count;
            return CLI_OK;
        }
        else ++i;
    }

    return CLI_ERRNO_NOMEM;
}

// Set operation to occur on Ctrl-C
void cli_setCtrlCOp( CliCtrlCFn_t ctrlC, void *args )
{
    ctrlCFn = ctrlC;
    ctrlCArgs = args;
}

#if _CLI_SET_OPS
// Set getChar and putChar functions for CLI
CliType_t cli_setOps( CliGetCharFn_t getChar, CliPutCharFn_t putChar )
{
    if ( getChar == NULL || putChar == NULL ) return CLI_ERRNO_NULL_PTR;

    getCharFn = getChar;
    putCharFn = putChar;

    return CLI_OK;
}

CliType_t cli_init( void )
{
#else   // Default character functions for Windows
static void prvWinPutChar( int c )
{
    printf( "%c", ((char) c) );
}

static int prvWinGetChar( void )
{
    return (int) getch();
}

CliType_t cli_init( void )
{
    getCharFn = &prvWinGetChar;
    putCharFn = &prvWinPutChar;
#endif /* _CLI_SET_OPS */

    flags.insertMode = 0;
    flags.screenCleared = 0;
    memset( commandLists, 0, sizeof(commandLists) );
    int err = cli_addList( defaultCommands, (sizeof(defaultCommands) / sizeof(defaultCommands[0])) );
    if ( err != CLI_OK ) {
        printf( "Could not add CLI commands: %s:%d%s", __FILE__, __LINE__, CLI_NEWLINE );
    }
    return err;
}

// Quick macro for left arrow key press
#define _CLI_CURSOR_LEFT()  cli_printf( "%c%c%c", CLI_CHAR_ESCAPE, CLI_CHAR_ESC_PREFIX, CLI_CHAR_ARROW_LEFT )

// Blocking task to process input. Should not return.
void cli_task( void *params )
{
#ifdef UNUSED
    UNUSED( params );
#else
    (void) params;
#endif

    // Set all task variables to default states
    cli_printf( "%s ", CLI_PROMPT );
    int currentCommandLength = 0;
    int currentCursorPosition = 0;
    historyCommand = 0;
    currentCommandIdx = 0;
    memset( commandHistory, 0, sizeof(commandHistory) );
    memset( currentCommand, 0, sizeof(currentCommand) );

    int c, i, t;

    while ( getCharFn != NULL && putCharFn != NULL ) {
        c = prvGetChar();
        if ( c < 0 ) continue;

#if CLI_ONLY_SHOW_ASCII
        cli_printf( "0x%02x%s", c, CLI_NEWLINE );
        prvPutChar( CLI_CHAR_BELL );
#else
        /* Standard Characters */
        if (c >= CLI_CHAR_PRINT_MIN && c <= CLI_CHAR_PRINT_MAX && currentCommandLength < CLI_MAX_COMMAND_LENGTH ) {
            if ( currentCursorPosition < currentCommandLength && flags.insertMode ) {
                // Insert mode
                prvPutChar(c);
                currentCommand[ currentCursorPosition++ ] = c;
            } else if ( currentCursorPosition < currentCommandLength ){
                // Non-insert mode
                i = currentCursorPosition;
                ++currentCommandLength;
                while ( i < currentCommandLength ) {
                    prvPutChar(c);
                    // Save character at current location
                    t = currentCommand[i];
                    // Insert new character
                    currentCommand[i++] = (char) c;
                    // Make pushed character new character and repeat
                    c = t;
                }
                ++currentCursorPosition;
                // Move cursor back
                while ( i-- > currentCursorPosition ) {
                    _CLI_CURSOR_LEFT();
                }
            }
            else {
                prvPutChar(c);
                currentCommand[ currentCommandLength++ ] = (char) c;
                ++currentCursorPosition;
            }
        }
        /* Control-C */
        else if ( c == CLI_CHAR_CTRL_C ) {
            prvCallCtrlC();
        }
        /* Show History */
        else if ( c == CLI_CHAR_CTRL_S ) {
            cli_printf( "%s%s%s", CLI_NEWLINE, CLI_NEWLINE, CLI_NEWLINE );
            for ( i = 0; i <= CLI_MAX_COMMAND_HISTORY; i++ ) {
                cli_printf( "%s(%d) %s%s", (i == historyCommand)? CLI_PROMPT : " ", i, commandHistory[i], CLI_NEWLINE );
            }
            cli_printf( "%s%s%s ", CLI_NEWLINE, CLI_NEWLINE, CLI_PROMPT );
        }
        /* Return Character */
        else if ( c == CLI_CHAR_RETURN ) {
            // Adjust for overflow
            currentCommandLength = (currentCommandLength < CLI_MAX_COMMAND_LENGTH)? currentCommandLength : (CLI_MAX_COMMAND_LENGTH - 1);
            currentCommand[currentCommandLength] = CLI_CHAR_NULL;
            if ( currentCommandLength > 0 ) {
                cli_printf( CLI_NEWLINE );
                // Only add if it wasn't last command executed
                if ( 0 != strncmp( currentCommand, commandHistory[CLI_COMMAND_PREV(currentCommandIdx)], CLI_MAX_COMMAND_LENGTH ) ) {
                    memcpy( commandHistory[currentCommandIdx], currentCommand, CLI_MAX_COMMAND_LENGTH );
                    currentCommandIdx = CLI_COMMAND_NEXT( currentCommandIdx );
                }

                historyCommand = currentCommandIdx;
                prvCallCommand( currentCommand );
                memset( currentCommand, 0, CLI_MAX_COMMAND_LENGTH );
            }
            currentCommandLength = 0;
            currentCursorPosition = 0;
            cli_printf( "%s%s ", flags.screenCleared? "" : CLI_NEWLINE, CLI_PROMPT );
            flags.screenCleared = CLI_FALSE;
            flags.insertMode = CLI_FALSE;
        }
        /* Backspace */
        else if ( c == CLI_CHAR_BACKSPACE && currentCommandLength > 0 && currentCursorPosition > 0 ) {
            // Cursor left
            _CLI_CURSOR_LEFT();
            if ( currentCursorPosition < currentCommandLength ) {
                // We are not at the end of the word
                i = currentCursorPosition;
                while ( i < currentCommandLength ) {
                    prvPutChar( currentCommand[i++] );
                }
                prvPutChar( CLI_CHAR_SPACE );
                memmove( currentCommand + currentCursorPosition - 1, currentCommand + currentCursorPosition, currentCommandLength - currentCursorPosition );
                currentCommand[ currentCommandLength - 1 ] = 0;
                for ( i = 0; i <= currentCommandLength - currentCursorPosition; i++ ) {
                    // Move back to where we were
                    _CLI_CURSOR_LEFT();
                }
                --currentCommandLength;
            }
            // Normal backspace
            else {
                // Erase Character
                prvPutChar( CLI_CHAR_SPACE );
                // Cursor left
                _CLI_CURSOR_LEFT();
                currentCommand[ currentCommandLength-- ] = 0;
            }
            --currentCursorPosition;
        }
        /* Arrow keys (and others) are implemented as escape sequeneces (ex.)
         * 'ESC'[A - Up
         * 'ESC'[B - Down
         * 'ESC'[C - Right
         * 'ESC'[D - Left
         */
        else if ( c == CLI_CHAR_ESCAPE_READ ) {
#if CLI_ESC_HAS_PREFIX
            if ( prvGetChar() != CLI_CHAR_ESC_PREFIX ) continue;
#endif
            c = prvGetChar();
            /* Delete */
            if ( c == CLI_CHAR_DELETE_READ && currentCommandLength > 0 && currentCursorPosition < currentCommandLength ) {
                for ( i = currentCursorPosition + 1; i < currentCommandLength; i++ ) {
                    prvPutChar( currentCommand[i] );
                }
                prvPutChar( CLI_CHAR_SPACE );
                for ( i = 1; i <= currentCommandLength - currentCursorPosition; i++ ) {
                    // Move back to where we were
                    _CLI_CURSOR_LEFT();
                }
                memmove( currentCommand + currentCursorPosition, currentCommand + currentCursorPosition + 1, (currentCommandLength - currentCursorPosition) + 1 );
                currentCommand[ --currentCommandLength ] = 0;
            }
            /* Insert */
            else if ( c == CLI_CHAR_INSERT_READ ) {
#if CLI_HAS_INSERT_MODE
                flags.insertMode = !flags.insertMode;
                if ( !flags.insertMode ) {
                    // If leaving mode, clean up next character
                    prvPutChar( (currentCursorPosition == currentCommandLength)? CLI_CHAR_SPACE : currentCommand[ currentCursorPosition ] );
                    _CLI_CURSOR_LEFT();
                }
#else
                // We are ignoring this input
                flags.insertMode = CLI_FALSE;
#endif // CLI_HAS_INSERT_MODE
            }
            else if ( c == CLI_CHAR_ARROW_UP_READ || c == CLI_CHAR_ARROW_DOWN_READ ) {
                /* Up */
                if ( c == CLI_CHAR_ARROW_UP_READ && CLI_COMMAND_PREV(historyCommand) != currentCommandIdx ) {
                    if ( historyCommand == currentCommandIdx ) {
                        memcpy( commandHistory[currentCommandIdx], currentCommand, CLI_MAX_COMMAND_LENGTH );
                    }
                    if ( commandHistory[ CLI_COMMAND_PREV(historyCommand) ][0] == 0 ) {
                        prvPutChar( CLI_CHAR_BELL );
                    }
                    else {
                        historyCommand = CLI_COMMAND_PREV( historyCommand );
                    }
                }
                /* Down */
                else if ( c == CLI_CHAR_ARROW_DOWN_READ && historyCommand != currentCommandIdx ) {
                    if ( commandHistory[ CLI_COMMAND_NEXT(historyCommand) ][0] == 0 ) {
                        prvPutChar( CLI_CHAR_BELL );
                    }
                    else {
                        historyCommand = CLI_COMMAND_NEXT( historyCommand );
                    }
                }

                memcpy( currentCommand, commandHistory[historyCommand], CLI_MAX_COMMAND_LENGTH );
                cli_printf( "%c%c%c%c%s %s", CLI_CHAR_ESCAPE, CLI_CHAR_ESC_PREFIX, 'M', CLI_CHAR_RETURN, CLI_PROMPT, currentCommand );
                currentCursorPosition = strnlen( currentCommand, CLI_MAX_COMMAND_LENGTH );
                currentCommandLength = currentCursorPosition;
            }
            /* Right */
            else if ( c == CLI_CHAR_ARROW_RIGHT_READ && currentCursorPosition < currentCommandLength ) {
                prvPutChar( currentCommand[currentCursorPosition++] );
            }
            /* Left */
            else if ( c == CLI_CHAR_ARROW_LEFT_READ && currentCursorPosition != 0 ) {
                if ( flags.insertMode ) {
                    prvPutChar( (currentCursorPosition == currentCommandLength)? CLI_CHAR_SPACE : currentCommand[currentCursorPosition] );
                    _CLI_CURSOR_LEFT();
                }
                --currentCursorPosition;
                _CLI_CURSOR_LEFT();
            }
        }
        /* Unkown Input */
        else {
            // Ring bell
            prvPutChar( CLI_CHAR_BELL );
            //cli_printf( "\n\r0x%02x\n\r%d\n\r", ((char) c), c );
        }

        // If in insert mode, display cursor properly
        if ( flags.insertMode ) {
            cli_printf( "\033[7m%c%c%c%c\033[0;37m",
                (currentCursorPosition == currentCommandLength)? CLI_CHAR_SPACE : currentCommand[currentCursorPosition],
                CLI_CHAR_ESCAPE, CLI_CHAR_ESC_PREFIX, CLI_CHAR_ARROW_LEFT );
        }

#endif /* CLI_ONLY_SHOW_ASCII */
    }

#if _CLI_RTOS_TASK_DELETE
    // In case of other error, prevent FreeRTOS configASSERT
    vTaskDelete( NULL );
#endif
}

#undef _CLI_CURSOR_LEFT
