/*
 * shell.c
 * Author: aseidman
 */

#include "project.h"
#include "shell.h"
#include "usbd_cdc_if.h"
#include "ajsCli.h"

extern QueueHandle_t rxCharsQueueHandle;
extern USBD_HandleTypeDef hUsbDeviceFS;

static void sendByte( int byte );
static int recvByte( void );
static void ctrlC( void *arg );

static CliType_t pongCommand( int argc, char *argv[] );

static CliCommand_t defaultCommands[] = {
        {
                .command = "ping",
                .usage = "",
                .help = "Test command that replies with pong",
                .fn = pongCommand,
        },
};

void shell_init( void ) {
    cli_init();
    cli_setCtrlCOp( ctrlC, NULL );
    cli_addList( defaultCommands, ARRAYSIZE(defaultCommands) );
    cli_setOps( recvByte, sendByte );
}

static void sendByte( int byte ) {
    uint8_t val = (uint8_t) byte;
    while ( hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED ||
            CDC_Transmit_FS( &val, 1 ) != USBD_OK ) {
        vTaskDelay(1);
    }
}

static int recvByte( void ) {
    uint8_t val;
    xQueueReceive( rxCharsQueueHandle, &val, portMAX_DELAY );
    return (int) val;
}

static void ctrlC( void *arg ) {
    PROJ_UNUSED( arg );
    cli_printf("\r\033[2K<CTRL-C> Rebooting.... ");
    vTaskDelay(50);
    NVIC_SystemReset();
}

static CliType_t pongCommand( int argc, char *argv[] ) {
    cli_printf( "PONG!\r\n" );
    return CLI_OK;
}
