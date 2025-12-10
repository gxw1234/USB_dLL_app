#ifndef USB_UART_H
#define USB_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// 跨平台宏定义
#ifndef WINAPI
#ifdef _WIN32
#define WINAPI __stdcall
#else
#define WINAPI
#endif
#endif

// UART配置结构体
typedef struct {
    unsigned int  BaudRate;         // 波特率
    unsigned char WordLength;       // 数据位宽，0-8bit,1-9bit
    unsigned char StopBits;         // 停止位宽，0-1bit,1-0.5bit,2-2bit,3-1.5bit
    unsigned char Parity;           // 奇偶校验，0-No,4-Even,6-Odd
    unsigned char TEPolarity;       // TE输出控制，0x80-输出TE信号且低电平有效，0x81-输出TE信号且高电平有效，0x00不输出TE信号
} UART_CONFIG, *PUART_CONFIG;

// UART初始化
// @param target_serial 设备序列号
// @param uart_index UART索引 (1对应USART3/PD8/PD9)
// @param config UART配置参数
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int UART_Init(const char* target_serial, int uart_index, UART_CONFIG* config);

// UART读取数据
// @param target_serial 设备序列号
// @param uart_index UART索引
// @param pReadBuffer 读取缓冲区
// @param ReadLen 读取长度
// @return 成功返回实际读取字节数，失败返回负数错误码
WINAPI int UART_ReadBytes(const char* target_serial, int uart_index, unsigned char* pReadBuffer, int ReadLen);

// UART发送数据
// @param target_serial 设备序列号
// @param uart_index UART索引 (1对应USART3/PD8/PD9)
// @param pWriteBuffer 要发送的数据指针
// @param WriteLen 数据长度
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int UART_WriteBytes(const char* target_serial, int uart_index, unsigned char* pWriteBuffer, int WriteLen);

// UART清除缓冲区数据
// @param target_serial 设备序列号
// @param uart_index UART索引 (1对应USART3/PD8/PD9)
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int UART_ClearData(const char* target_serial, int uart_index);

#ifdef __cplusplus
}
#endif

#endif // USB_UART_H