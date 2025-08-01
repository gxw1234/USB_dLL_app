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

// 定义帧头标识符
#define FRAME_START_MARKER  0x5A5A5A5A
#define CMD_END_MARKER      0xA5A5A5A5

static int Add_Parameter(unsigned char* buffer, int pos, void* data, uint16_t len) {
    PARAM_HEADER header;
    header.param_len = len;
    memcpy(buffer + pos, &header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(buffer + pos, data, len);
    pos += len;
    
    return pos;
}

// 通用的帧构建函数
static int build_spi_frame(unsigned char** buffer, GENERIC_CMD_HEADER* cmd_header, 
                          unsigned char* param_data, int param_len, 
                          unsigned char* data_payload, int data_len) {
    // total_packets只包含协议数据部分，不包含帧头和帧尾
    cmd_header->total_packets = sizeof(GENERIC_CMD_HEADER);
    if (param_len > 0) {
        cmd_header->total_packets += sizeof(PARAM_HEADER) + param_len;
    }
    if (data_len > 0) {
        cmd_header->total_packets += data_len;
    }
    
    int total_len = sizeof(uint32_t) + cmd_header->total_packets + sizeof(uint32_t);
    
    *buffer = (unsigned char*)malloc(total_len);
    if (!*buffer) {
        debug_printf("内存分配失败");
        return -1;
    }
    
    int pos = 0;
    
    // 添加帧头标识符
    uint32_t frame_header = FRAME_START_MARKER;
    memcpy(*buffer + pos, &frame_header, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    
    // 添加协议头
    memcpy(*buffer + pos, cmd_header, sizeof(GENERIC_CMD_HEADER));
    pos += sizeof(GENERIC_CMD_HEADER);
    
    // 添加参数（如果有）
    if (param_len > 0 && param_data) {
        pos = Add_Parameter(*buffer, pos, param_data, param_len);
    }
    
    // 添加数据负载（如果有）
    if (data_len > 0 && data_payload) {
        memcpy(*buffer + pos, data_payload, data_len);
        pos += data_len;
    }
    
    // 添加帧尾标识符
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(*buffer + pos, &end_marker, sizeof(uint32_t));
    
    debug_printf("[PC] SPI sending frame with header 0x%08X, protocol_type=%d, cmd_id=%d", 
                 FRAME_START_MARKER, cmd_header->protocol_type, cmd_header->cmd_id);
    
    return total_len;
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

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   (unsigned char*)pConfig, sizeof(SPI_CONFIG), 
                                   NULL, 0);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
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

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return SPI_SUCCESS;
}



int SPI_Queue_WriteBytes(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
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
    cmd_header.cmd_id = CMD_QUEUE_WRITE;             // 写数据命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);



    unsigned char response_buffer[1];
    int max_loops = 1000000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_spi_data(device_id, response_buffer, 1);
        if (actual_read > 0) {

            return response_buffer[0]; // 有数据立即返回，无数据则返回-1  读取10000次
        }
    //    Sleep(1);
    }
    

    
    debug_printf("队列状态查询失败，未收到响应");
    return SPI_ERROR_IO; // 读取失败
    
 
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

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   NULL, 0);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI队列状态查询命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    debug_printf("成功发送SPI队列状态查询命令，SPI索引: %d", SPIIndex);
    
    unsigned char response_buffer[1];
    int max_loops = 100000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_spi_data(device_id, response_buffer, 1);
        if (actual_read > 0) {
            return response_buffer[0]; // 有数据立即返回，无数据则返回-1  读取10000次
        }
    //    Sleep(1);
        
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
    cmd_header.cmd_id = CMD_QUEUE_START;        // 队列启动命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                  // 无参数
    cmd_header.data_len = 0;                     // 无数据

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   NULL, 0);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    
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
    cmd_header.cmd_id = CMD_QUEUE_STOP;        // 队列停止命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                  // 无参数
    cmd_header.data_len = 0;                     // 无数据

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   NULL, 0);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI队列状态查询命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    return SPI_SUCCESS;
}