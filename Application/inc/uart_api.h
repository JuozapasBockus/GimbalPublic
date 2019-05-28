#ifndef _UART_API_
#define _UART_API_


#include <stdbool.h>


typedef enum {
    eUart_First,
    eUart_1 = eUart_First,
    eUart_2,
    eUart_3,
    eUart_Last,
} eUart_t;


void            InitializeUartMutexes       (void);
void            InitializeUartInterrupts    (void);
void            HandleUartRxIRQ             (eUart_t CurrentUart);
void            HandleUartTxIRQ             (eUart_t CurrentUart);
bool            PrintToUart                 (eUart_t OutputUart, char *Format, ...);

#endif /* _UART_API_ */
