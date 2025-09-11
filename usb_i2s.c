#include "usb_i2s.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usb_log.h"

int I2S_Init(const char* target_serial, int I2SIndex, PI2S_CONFIG pConfig) {
    if (!target_serial || !pConfig) {
        debug_printf("参数无效: target_serial=%p, pConfig=%p", target_serial, pConfig);
        return I2S_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return I2S_ERROR_OTHER;
    }

    // 组包协议头和数据
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_AUDIO;      // 音频协议
    cmd_header.cmd_id = AUDIO_CMD_INIT;            // 初始化命令
    cmd_header.device_index = (uint8_t)I2SIndex;   // 设备索引
    cmd_header.param_count = 1;                    // 参数数量：1个
    cmd_header.data_len = 0;                       // 数据部分长度为0

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   (unsigned char*)pConfig, sizeof(I2S_CONFIG), 
                                   NULL, 0);
    if (total_len < 0) {
        return I2S_ERROR_OTHER;
    }

    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送I2S初始化命令失败: %d", ret);
        return I2S_ERROR_IO;
    }

    debug_printf("成功发送I2S初始化命令，I2S索引: %d", I2SIndex);
    return I2S_SUCCESS;
}

int I2S_Queue_WriteBytes(const char* target_serial, int I2SIndex, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial || !pWriteBuffer || WriteLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pWriteBuffer=%p, WriteLen=%d", target_serial, pWriteBuffer, WriteLen);
        return I2S_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return I2S_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_AUDIO;      // 音频协议
    cmd_header.cmd_id = AUDIO_CMD_PLAY;            // 音频播放命令
    cmd_header.device_index = (uint8_t)I2SIndex;   // 设备索引
    cmd_header.param_count = 0;                    // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;                // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return I2S_ERROR_OTHER;
    }

    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);


    
    // 使用专用状态缓冲区读取响应，支持完整协议头
    unsigned char response_buffer[16];  // 足够容纳完整状态响应
    int max_loops = 10000000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_status_data(device_id, response_buffer, sizeof(response_buffer));
        if (actual_read >= sizeof(GENERIC_CMD_HEADER) + 1) {  // 至少包含协议头+状态数据
            GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)response_buffer;
            if (header->protocol_type == PROTOCOL_STATUS && 
                header->cmd_id == AUDIO_CMD_PLAY) {
                uint8_t audio_status = response_buffer[sizeof(GENERIC_CMD_HEADER)];
                return audio_status;
            }
        }
    }
    
    debug_printf("音频队列写入失败，未收到响应");
    return I2S_ERROR_IO; // 读取失败
}

int I2S_GetQueueStatus(const char* target_serial, int I2SIndex) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return I2S_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return I2S_ERROR_OTHER;
    }

    // 组包协议头
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_AUDIO;      // 音频协议
    cmd_header.cmd_id = AUDIO_CMD_STATUS;          // 队列状态查询命令
    cmd_header.device_index = (uint8_t)I2SIndex;   // 设备索引
    cmd_header.param_count = 0;                    // 无参数
    cmd_header.data_len = 0;                       // 无数据

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   NULL, 0);
    if (total_len < 0) {
        return I2S_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送I2S队列状态查询命令失败: %d", ret);
        return I2S_ERROR_IO;
    }

    unsigned char response_buffer[16];  
    int max_loops = 1000000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_status_data(device_id, response_buffer, sizeof(response_buffer));
        if (actual_read >= sizeof(GENERIC_CMD_HEADER) + 1) {  // 至少包含协议头+状态数据
            GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)response_buffer;
            if (header->protocol_type == PROTOCOL_STATUS && 
                header->cmd_id == AUDIO_CMD_STATUS) {
                uint8_t queue_status = response_buffer[sizeof(GENERIC_CMD_HEADER)];
                return queue_status;
            }
        }
    }
    
    debug_printf("音频队列状态查询失败，未收到响应");
    return I2S_ERROR_IO; // 读取失败
}

int I2S_StartQueue(const char* target_serial, int I2SIndex) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return I2S_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return I2S_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_AUDIO;      // 音频协议
    cmd_header.cmd_id = AUDIO_CMD_START;           // 队列启动命令
    cmd_header.device_index = (uint8_t)I2SIndex;   // 设备索引
    cmd_header.param_count = 0;                    // 无参数
    cmd_header.data_len = 0;                       // 无数据

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   NULL, 0);
    if (total_len < 0) {
        return I2S_ERROR_OTHER;
    }

    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送I2S队列启动命令失败: %d", ret);
        return I2S_ERROR_IO;
    }
    
    return I2S_SUCCESS;
}

int I2S_StopQueue(const char* target_serial, int I2SIndex) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return I2S_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return I2S_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_AUDIO;      // 音频协议
    cmd_header.cmd_id = AUDIO_CMD_STOP;            // 队列停止命令
    cmd_header.device_index = (uint8_t)I2SIndex;   // 设备索引
    cmd_header.param_count = 0;                    // 无参数
    cmd_header.data_len = 0;                       // 无数据

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   NULL, 0);
    if (total_len < 0) {
        return I2S_ERROR_OTHER;
    }

    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送I2S队列停止命令失败: %d", ret);
        return I2S_ERROR_IO;
    }    

    return I2S_SUCCESS;
}

int I2S_SetVolume(const char* target_serial, int I2SIndex, unsigned char volume) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return I2S_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return I2S_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_AUDIO;      // 音频协议
    cmd_header.cmd_id = AUDIO_CMD_VOLUME;          // 音量控制命令
    cmd_header.device_index = (uint8_t)I2SIndex;   // 设备索引
    cmd_header.param_count = 1;                    // 参数数量：1个
    cmd_header.data_len = 0;                       // 数据部分长度为0

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   &volume, sizeof(unsigned char), 
                                   NULL, 0);
    if (total_len < 0) {
        return I2S_ERROR_OTHER;
    }

    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送I2S音量控制命令失败: %d", ret);
        return I2S_ERROR_IO;
    }

    return I2S_SUCCESS;
}