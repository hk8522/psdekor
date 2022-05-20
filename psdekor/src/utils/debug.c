/*
 * debug.c
 *
 * Created: 19.11.2013 13:11:54
 *  Author: huber
 */ 

#include "utils/debug.h"
#include "delay_wrapper.h"


#ifdef CONF_ENABLE_DBG_UART

char __dbg_uart_buffer[DBG_UART_BUFFER_LENGTH];

#endif