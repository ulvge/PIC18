
#include <main.h>
#include <stdlib.h>
#include <string.h>
#include <Types.h>
#include "output_wave.h"
#include "debug_print.h"
#include "spi_communication.h"
#include "bsp_gpio.h"
#include "bsp_spi1_slave.h"

SemaphoreHandle_t g_sem_recvedWaveData;
SemaphoreHandle_t g_sem_isSending;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t oldStamp, nowStamp;
    oldStamp = Get_dealyTimer_cnt();
    vTaskSuspendAll();
    if (GPIO_Pin == GPIO_PIN_13) {
        while(GPIO_Get_MCLR()){
            nowStamp = Get_dealyTimer_cnt();
            if ((nowStamp - oldStamp) >= OUTPUT_DELAY_4US) {
                HAL_NVIC_SystemReset();
            }
        }
    }
    xTaskResumeAll();
}
// htim5 run clok: 280M
// Period = count 280 = 1us
static void delay_us(uint32_t dly)
{
    static uint32_t oldStamp, nowStamp;

    oldStamp = Get_dealyTimer_cnt();
    do{
        nowStamp = Get_dealyTimer_cnt();
    } while ((nowStamp - oldStamp) < dly);
}
inline static void output_setDirection(uint16_t val)
{
    GPIO_Set_DIRECTION((GPIO_PinState)((val & BIT(12)) == 0));
}
inline static void output_setSpotMode(uint16_t val)
{
    GPIO_Set_SPOT((GPIO_PinState)((val & BIT(13)) == 0));
}

inline static void output_waitMasterBeReady(void)
{
    while (GPIO_Get_GLITCH_SHUTDOWN()) {
        GPIO_Set_PIC_LED(GPIO_PIN_RESET);
    }
}
inline static void output_waitMasterMatch(void)
{
    while (GPIO_Get_MATCH()) {
    }
}
void Task_outputWave(void *argument)
{
    bool isFirst = true;
    uint16_t reSendCount;
    uint16_t slp;
    g_sem_recvedWaveData = xSemaphoreCreateBinary();
    g_sem_isSending = xSemaphoreCreateMutex();

    while (1) {
        if (xSemaphoreTake(g_sem_recvedWaveData, portMAX_DELAY) == pdTRUE) {
            bsp_spi_DiagSendStart();
            vTaskSuspendAll();
            GPIO_Set_BUSY(GPIO_PIN_SET);
            reSendCount = 0;

            isFirst = true;

            while (g_protocolCmd.reSendTimes == 0 || reSendCount++ < g_protocolCmd.reSendTimes) {
                for (size_t i = 0; i < g_protocolData.recvedGroupCount; i++) {
                    // wait master ready
                    output_waitMasterBeReady();     
                    GPIO_Set_PIC_LED(GPIO_PIN_SET); // run normal

                    slp = g_protocolData.data[i].slope;
                    output_setSpotMode(slp);    // send slope mode
                    if (isFirst) {
                        // send position val
                        GPIO_SetDAC(g_protocolData.data[i].position);
                        GPIO_Set_LD_POS(GPIO_PIN_SET);
                        delay_us(OUTPUT_DELAY_0U1S);
                        GPIO_Set_LD_POS(GPIO_PIN_RESET);
                        isFirst = false;
                    }
                    // send slope val
                    output_setDirection(slp);   // send Direction
                    GPIO_SetDAC(slp);
                    GPIO_Set_LD_SLOPE(GPIO_PIN_SET);
                    delay_us(OUTPUT_DELAY_0U1S);
                    GPIO_Set_LD_SLOPE(GPIO_PIN_RESET);
                    // wait master match
                    delay_us(g_protocolCmd.sleepUsWave);
                    output_waitMasterMatch();
                }

                delay_us(g_protocolCmd.sleepUsGroupData);
            }


            GPIO_Set_BUSY(GPIO_PIN_RESET);

            GPIO_Set_INTRPT(GPIO_PIN_SET);
            delay_us(OUTPUT_DELAY_1U5S);
            GPIO_Set_INTRPT(GPIO_PIN_RESET);

            bsp_spi_DiagSendFinished(reSendCount);

            if (xSemaphoreTake(g_sem_isSending, 0) == pdTRUE) {// is sending
                g_protocolData.isSending = false;
                xSemaphoreGive(g_sem_isSending);
            }
            if (!xTaskResumeAll()) {
                taskYIELD();
            }
        }
    }
}
