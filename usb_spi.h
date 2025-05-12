#ifndef USB_SPI_H
#define USB_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_application.h"

// 定义SPI索引
#define SPI1_CS0    0
#define SPI1_CS1    1
#define SPI1_CS2    2
#define SPI2_CS0    3
#define SPI2_CS1    4
#define SPI2_CS2    5

// 定义返回值
#define SPI_SUCCESS             0    // 成功
#define SPI_ERROR_NOT_FOUND    -1    // 设备未找到
#define SPI_ERROR_ACCESS       -2    // 访问被拒绝
#define SPI_ERROR_IO           -3    // I/O错误
#define SPI_ERROR_INVALID_PARAM -4   // 参数无效
#define SPI_ERROR_OTHER        -99   // 其他错误

// SPI配置结构体
typedef struct _SPI_CONFIG {
    char   Mode;            // SPI控制方式:0-硬件控制（全双工模式）,1-硬件控制（半双工模式），2-软件控制（半双工模式）,3-单总线模式，数据线输入输出都为MOSI,4-软件控制（全双工模式）  
    char   Master;          // 主从选择控制:0-从机，1-主机  
    char   CPOL;            // 时钟极性控制:0-SCK空闲时为低电平，1-SCK空闲时为高电平  
    char   CPHA;            // 时钟相位控制:0-第一个SCK时钟采样，1-第二个SCK时钟采样  
    char   LSBFirst;        // 数据移位方式:0-MSB在前，1-LSB在前  
    char   SelPolarity;     // 片选信号极性:0-低电平选中，1-高电平选中  
    unsigned int ClockSpeedHz;    // SPI时钟频率:单位为HZ，硬件模式下最大50000000，最小390625，频率按2的倍数改变  
} SPI_CONFIG, *PSPI_CONFIG;

// USB命令协议定义
#define CMD_SPI_INIT        0x01    // SPI初始化命令
#define CMD_SPI_WRITE       0x02    // SPI写数据命令
#define CMD_SPI_READ        0x03    // SPI读数据命令
#define CMD_SPI_TRANSFER    0x04    // SPI读写数据命令

// 命令包头结构
typedef struct _CMD_HEADER {
    unsigned char cmd_id;        // 命令ID
    unsigned char spi_index;     // SPI索引
    unsigned short data_len;     // 数据长度
} CMD_HEADER, *PCMD_HEADER;

/**
 * @brief 初始化SPI
 * 
 * @param target_serial 目标设备的序列号
 * @param SPIIndex SPI索引
 * @param pConfig SPI配置结构体
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int SPI_Init(const char* target_serial, int SPIIndex, PSPI_CONFIG pConfig);

/**
 * @brief 通过SPI发送数据
 * 
 * @param target_serial 目标设备的序列号
 * @param SPIIndex SPI索引
 * @param pWriteBuffer 要发送的数据缓冲区
 * @param WriteLen 要发送的数据长度
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int SPI_WriteBytes(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen);

#ifdef __cplusplus
}
#endif

#endif // USB_SPI_H
