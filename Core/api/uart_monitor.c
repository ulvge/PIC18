#include <string.h>  
#include <stdlib.h>
#include <main.h>
#include "FreeRTOS.h"
#include "debug_print.h"
#include "uart_monitor.h"
#include "bsp_uartcomm.h"
#include "spi_communication.h"

SemaphoreHandle_t g_sem_uartResend;

void Task_uartMonitor(void *param)
{
    g_sem_uartResend = xSemaphoreCreateBinary();
    while(1)
    {
        if (xSemaphoreTake(g_sem_uartResend, portMAX_DELAY) == pdTRUE) {
            vTaskDelay(10);
            UART_sendContinue(DEBUG_UART_PERIPH);
        }
    }
}

extern UART_HandleTypeDef g_uart2Handle;
void uart_PostdMsg(bool isReSend)
{
    if (isReSend) {
        //LOG_E("When uart sends data, repeated transmission occurs\r\n");
    }
    if (vPortGetIPSR()) {
        xSemaphoreGiveFromISR(g_sem_uartResend, &xHigherPriorityTaskWoken_NO);
    }else {
        xSemaphoreGive(g_sem_uartResend);
    }
}