#include "usb_spi.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usb_log.h"

int SPI_Init(const char* target_serial, int SPIIndex, PSPI_CONFIG pConfig) {
    if (!target_serial || !pConfig) {
        debug_printf("参数无效: target_serial=%p, pConfig=%p", target_serial, pConfig);
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
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
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
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
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
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    // 使用专用状态缓冲区读取响应，支持完整协议头
    unsigned char response_buffer[16];  // 足够容纳完整状态响应
    int max_loops = 1000000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_status_data(device_id, response_buffer, sizeof(response_buffer));
        
        if (actual_read >= sizeof(GENERIC_CMD_HEADER) + 1) {  // 至少包含协议头+状态数据
            GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)response_buffer;
            if (header->protocol_type == PROTOCOL_STATUS && 
                header->cmd_id == CMD_QUEUE_WRITE ) {
                uint8_t queue_status = response_buffer[sizeof(GENERIC_CMD_HEADER)];
                return queue_status;
            }
        }
    }
    
    debug_printf("队列写入失败，未收到响应");
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
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
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
    
    // debug_printf("成功发送SPI队列状态查询命令，SPI索引: %d", SPIIndex);
    

    unsigned char response_buffer[16];  
    int max_loops = 1000000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_status_data(device_id, response_buffer, sizeof(response_buffer));
        if (actual_read >= sizeof(GENERIC_CMD_HEADER) + 1) {  // 至少包含协议头+状态数据
            GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)response_buffer;
            if (header->protocol_type == PROTOCOL_STATUS && 
                header->cmd_id == CMD_QUEUE_STATUS ) {
                uint8_t queue_status = response_buffer[sizeof(GENERIC_CMD_HEADER)];
                return queue_status;
            }
        }
    }
    
    debug_printf("队列状态查询失败，未收到响应");
    return SPI_ERROR_IO; // 读取失败
}





int SPI_StartQueue(const char* target_serial, int SPIIndex) {

    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return SPI_ERROR_INVALID_PARAM;
    }

    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;     // SPI协议
    cmd_header.cmd_id = CMD_QUEUE_START;        // 队列启动命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                  // 无参数
    cmd_header.data_len = 0;                     // 无数据

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
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
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;     // SPI协议
    cmd_header.cmd_id = CMD_QUEUE_STOP;        // 队列停止命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                  // 无参数
    cmd_header.data_len = 0;                     // 无数据
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
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