#ifndef USART_H
#define USART_H

// 是否串口1和串口2功能互换。一个负责输出debug信息，一个负责跟上位机交互
#ifdef USE_USART1_AS_OUTPUT_DEBUG
#define STDOUT_USART 1
#define STDERR_USART 2
#define STDIN_USART 2
#else
#define STDOUT_USART 2
#define STDERR_USART 1
#define STDIN_USART 1
#endif

void USART2_init(void);
void USART1_init(void);
void USART_scanf_config_EXT(void);

#endif
