#ifndef USB_SPI_H
#define USB_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_application.h"


// 定义返回值
#define SPI_SUCCESS             0    // 成功SPI_SUCCESS
#define SPI_ERROR_NOT_FOUND    -1    // 设备未找到
#define SPI_ERROR_ACCESS       -2    // 访问被拒绝
#define SPI_ERROR_IO           -3    // I/O错误
#define SPI_ERROR_INVALID_PARAM -4   // 参数无效
#define SPI_ERROR_OTHER        -99   // 其他错误



// 命令包头结构
typedef struct _CMD_HEADER {
    unsigned char cmd_id;        // 命令ID
    unsigned char spi_index;     // SPI索引
    unsigned short data_len;     // 数据长度
} CMD_HEADER, *PCMD_HEADER;


WINAPI int Bootloader_StartWrite(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen);
WINAPI int Bootloader_WriteBytes(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen);
WINAPI int Bootloader_Reset(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen);
WINAPI int Bootloader_SwitchRun(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen);
WINAPI int Bootloader_SwitchBoot(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen);




#ifdef __cplusplus
}
#endif

#endif // USB_SPI_H
