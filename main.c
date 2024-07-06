#include <stdlib.h>

// #include "AJScliCfg.h" // Include if overriding defaults
#include "AJScli.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof(x)/sizeof((x)[0]))
#endif

static void ctrlC()
{
    cli_printf_err( "Ctrl-C\r\n" );
    cli_printf( "Goodbye!\r\n\r\n" );
    exit(0);
}

// Currently testing escape sequences
static CliType_t prvCommandTest( int argc, char *argv[] )
{
    if ( argc-- < 2 ) {
        cli_printf_err( "Not enough args provided" );
        return CLI_ERRNO_BAD_FMT;
    }

    int i = 0, escCount = 0;
    while ( argc-- ) {
        if ( argv[++i][0] == '[' ) {
            cli_printf( "\033%s", argv[i] );
            ++escCount;
        }
        else {
            cli_printf( "%s%s", argv[i], (argc == 0)? "\n\r" : " " );
        }
    }

    if ( escCount == i ) {
        cli_printf( "__TEST__\n\r" );
    }

    return CLI_OK;
}

static CliType_t prvExit()
{
    cli_printf( "Goodbye!\r\n\r\n" );
    exit(0);
}

static CliType_t prvCommandPrint( int argc, char *argv[] )
{
    int i = 0;
    while  (i++ < (argc-1) ) {
        cli_printf( "%s%s", argv[i], (i == argc-1)? "\n\r" : " " );
    }

    return CLI_OK;
}

int main( int argv, char *argc[] )
{
    cli_init();

    CliCommand_t cmdList[] = {
        {
            .command = "echo",
            .fn = &prvCommandPrint,
            .help = "Do this to echo some stuff!",
            .usage = "<text>",
        },
        {
            .command = "test",
            .fn = &prvCommandTest,
            .help = "This is a test!",
            .usage = "",
        },
        {
            .command = "exit",
            .fn = &prvExit,
            .help = "Exit program.",
            .usage = ""
        },
    };

    cli_addList( cmdList, ARRAYSIZE(cmdList) );
    cli_setCtrlCOp( &ctrlC, NULL );

    if ( argv > 0 ) {
        cli_printf( "%s\n\r", argc[0] );
    }

    cli_task( NULL );

    return -1; // Should not get here
}
