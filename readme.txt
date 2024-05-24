2024-5-24
项目框架：
1、keil5 + stm32H7A3RGTx
2、芯片stm32H7A3RGTx，1.4M的ram+2M flash,主频280M
3、采用最新的freeRTOS+最新的STM hal库

功能：
1、输出两路并口信号给DAC
2、串口DMA发送+中断接收
3、SPI DMA发送+中断接收
4、ADC DMA循环写
5、芯片有两块ram，所有DMA相关的ram,都放到第块，其余所有放到第一块
6、ram采用SECTION加载机制
7、拥有完整的shell，也是采用SECTION，只不过这是在flash
8、支持cm_backtrace，hardfalut处理机制
9、不使用芯片默认的tick，因为它的中断优先级很低，当在其它中断中时，tick中断不会被响应，导致在中断中不能使用OSdelay
10、使用tim7作为systick，tim5一直在运行，可以读取并比较，然后计算出时差，用于us级别
11、支持查看freeRTOS的任务堆、栈信息PrintTaskStackHead
12、支持宏拼接PIN_NAME(val)，把val转成字符串和拼接，eg: PIN_NAME(RUN_LED)--> .Name = "RUN_LED", .Pin = RUN_LED_PIN
13、IO中断，回调并重启。EXTI15_10_IRQHandler_Config、HAL_GPIO_EXTI_Callback
