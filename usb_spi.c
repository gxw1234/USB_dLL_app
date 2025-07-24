#include "usb_spi.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 通用命令包头结构 (已移除end_marker字段)
typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型：SPI/IIC/UART等
  uint8_t cmd_id;         // 命令ID：初始化/读/写等
  uint8_t device_index;   // 设备索引
  uint8_t param_count;    // 参数数量
  uint16_t data_len;      // 数据部分长度
  uint16_t total_packets; // 整包总数
} GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;

typedef struct _PARAM_HEADER {
  uint16_t param_len;     // 参数长度
} PARAM_HEADER, *PPARAM_HEADER;

extern void debug_printf(const char *format, ...);


static int Add_Parameter(unsigned char* buffer, int pos, void* data, uint16_t len) {
    PARAM_HEADER header;
    header.param_len = len;
    memcpy(buffer + pos, &header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(buffer + pos, data, len);
    pos += len;
    
    return pos;
}


int SPI_Init(const char* target_serial, int SPIIndex, PSPI_CONFIG pConfig) {
    if (!target_serial || !pConfig) {
        debug_printf("参数无效: target_serial=%p, pConfig=%p", target_serial, pConfig);
        return SPI_ERROR_INVALID_PARAM;
    }
    if (SPIIndex < SPI1_CS0 || SPIIndex > SPI2_CS2) {
        debug_printf("SPI索引无效: %d", SPIIndex);
        return SPI_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }
    // 组包协议头和数据
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;     // SPI协议
    cmd_header.cmd_id = CMD_INIT;               // 初始化命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 1;                 // 参数数量：1个
    cmd_header.data_len = 0;                    // 数据部分长度为0

    // 计算总长度，包括命令头、参数头、SPI配置和结束符
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(SPI_CONFIG) + sizeof(uint32_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(SPI_CONFIG) + sizeof(uint32_t);
    
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    pos = Add_Parameter(send_buffer, pos, pConfig, sizeof(SPI_CONFIG));
    
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + pos, &end_marker, sizeof(uint32_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送SPI初始化命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    debug_printf("成功发送SPI初始化命令，SPI索引: %d", SPIIndex);
    return SPI_SUCCESS;
}


int SPI_WriteBytes(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial || !pWriteBuffer || WriteLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pWriteBuffer=%p, WriteLen=%d", target_serial, pWriteBuffer, WriteLen);
        return SPI_ERROR_INVALID_PARAM;
    }
    if (SPIIndex < SPI1_CS0 || SPIIndex > SPI2_CS2) {
        debug_printf("SPI索引无效: %d", SPIIndex);
        return SPI_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;    // SPI协议
    cmd_header.cmd_id = CMD_WRITE;             // 写数据命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + WriteLen + sizeof(uint32_t);  // 总大小，包含结束符
    int total_len = sizeof(GENERIC_CMD_HEADER) + WriteLen + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), pWriteBuffer, WriteLen);
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER) + WriteLen, &end_marker, sizeof(uint32_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);

//   for (size_t i = 0; i < 20; i++) {
//         debug_printf("%02X ", (unsigned char)send_buffer[i]);
//     }

//      debug_printf("-----end-----");



    free(send_buffer);
    // if (ret < 0) {
    //     debug_printf("发送SPI写数据命令失败: %d", ret);
    //     return SPI_ERROR_IO;
    // }
    // debug_printf("成功发送SPI写数据命令，SPI索引: %d, 数据长度: %d字节", SPIIndex, WriteLen);
    return SPI_SUCCESS;
}


int SPI_SlaveReadBytes(const char* target_serial, int SPIIndex, unsigned char* pReadBuffer, int ReadLen) {
    if (!target_serial || !pReadBuffer || ReadLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pReadBuffer=%p, ReadLen=%d", target_serial, pReadBuffer, ReadLen);
        return SPI_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }
    
    int actual_read = usb_middleware_read_spi_data(device_id, pReadBuffer, ReadLen);
    if (actual_read < 0) {
        debug_printf("从SPI缓冲区读取数据失败: %d", actual_read);
        return SPI_ERROR_IO;
    }
    
    if (actual_read > 0) {
        debug_printf("成功读取SPI数据，SPI索引: %d, 数据长度: %d字节", SPIIndex, actual_read);
    }
    return actual_read;
}




WINAPI int SPI_GetQueueStatus(const char* target_serial, int SPIIndex) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return SPI_ERROR_INVALID_PARAM;
    }
    if (SPIIndex < SPI1_CS0 || SPIIndex > SPI2_CS2) {
        debug_printf("SPI索引无效: %d", SPIIndex);
        return SPI_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    // 组包协议头
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;     // SPI协议
    cmd_header.cmd_id = CMD_QUEUE_STATUS;        // 队列状态查询命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                  // 无参数
    cmd_header.data_len = 0;                     // 无数据
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(uint32_t);
    
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), &end_marker, sizeof(uint32_t));
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI队列状态查询命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    debug_printf("成功发送SPI队列状态查询命令，SPI索引: %d", SPIIndex);
    
    // 等待并读取响应
    Sleep(50); // 等待STM32响应，给足够时间
    unsigned char response_buffer[1];
    int actual_read = usb_middleware_read_spi_data(device_id, response_buffer, 1);
    if (actual_read > 0) {
        debug_printf("队列状态查询成功: %d", response_buffer[0]);
        return response_buffer[0]; // 返回队列数量
    }
    
    debug_printf("队列状态查询失败，未收到响应");
    return SPI_ERROR_IO; // 读取失败
}





int SPI_StartQueue(const char* target_serial, int SPIIndex) {

    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return SPI_ERROR_INVALID_PARAM;
    }
    if (SPIIndex < SPI1_CS0 || SPIIndex > SPI2_CS2) {
        debug_printf("SPI索引无效: %d", SPIIndex);
        return SPI_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    // 组包协议头
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;     // SPI协议
    cmd_header.cmd_id = CMD_QUEUE_START;        // 队列状态查询命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                  // 无参数
    cmd_header.data_len = 0;                     // 无数据
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(uint32_t);
    
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), &end_marker, sizeof(uint32_t));
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI队列状态查询命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    return SPI_SUCCESS;
}




int SPI_StopQueue(const char* target_serial, int SPIIndex) {

    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return SPI_ERROR_INVALID_PARAM;
    }
    if (SPIIndex < SPI1_CS0 || SPIIndex > SPI2_CS2) {
        debug_printf("SPI索引无效: %d", SPIIndex);
        return SPI_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    // 组包协议头
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;     // SPI协议
    cmd_header.cmd_id = CMD_QUEUE_STOP;        // 队列状态查询命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                  // 无参数
    cmd_header.data_len = 0;                     // 无数据
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(uint32_t);
    
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), &end_marker, sizeof(uint32_t));
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI队列状态查询命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    return SPI_SUCCESS;
}