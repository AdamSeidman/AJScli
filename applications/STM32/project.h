#ifndef EXAMPLE_PROJECT_H
#define EXAMPLE_PROJECT_H

// Types

#include "FreeRTOS.h"
#include "main.h"                       /* (ST) */

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)                    (sizeof(x) / sizeof((x)[0]))
#endif

// Utilitiess

#include "ajsCli.h"
#define APPLICATION_PRINT(fmt, ...)     cli_printf_msg(("dbg: " fmt), ##__VA_ARGS__)
#define DBG_PRINTF(fmt, ...)            APPLICATION_PRINT(fmt, ##__VA_ARGS__)
#define DBG_PRINTF_V(fmt, ...)          APPLICATION_PRINT(fmt, ##__VA_ARGS__)

#endif // EXAMPLE_PROJECT_H
