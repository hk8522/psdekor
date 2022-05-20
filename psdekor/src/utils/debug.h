/*
 * debug.h
 *
 * Created: 19.11.2013 13:07:12
 *  Author: huber
 */ 


#ifndef DEBUG_H_
#define DEBUG_H_

#include "conf_board.h"


#ifdef CONF_ENABLE_DBG_UART

# include <stdio.h>
# include "uart.h"

# ifndef DBG_UART_BUFFER_LENGTH
#  define DBG_UART_BUFFER_LENGTH 256
# endif

extern char __dbg_uart_buffer[DBG_UART_BUFFER_LENGTH];

# define DLOG(...) do {									\
		u8 *__log_buffer_ptr = (u8 *)&__dbg_uart_buffer[0];			\
		snprintf (__dbg_uart_buffer, DBG_UART_BUFFER_LENGTH,  __VA_ARGS__);	\
		__dbg_uart_buffer[DBG_UART_BUFFER_LENGTH - 1] = 0;			\
		for (;*__log_buffer_ptr != 0; ++__log_buffer_ptr) {			\
			uartTxByte (*__log_buffer_ptr);					\
		}									\
	} while (0)

#else
# define DLOG(...)
#endif


#endif /* DEBUG_H_ */