#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern void debug_printf(const char *format, ...);

int build_protocol_frame(unsigned char** buffer, GENERIC_CMD_HEADER* cmd_header, 
                        void* param_data, size_t param_len, 
                        void* data_payload, size_t data_len) {

    // 计算total_packets：只包含协议数据部分，不包含帧头和帧尾
    cmd_header->total_packets = sizeof(GENERIC_CMD_HEADER);
    if (param_len > 0) {
        cmd_header->total_packets += sizeof(PARAM_HEADER) + param_len;
    }
    if (data_len > 0) {
        cmd_header->total_packets += data_len;
    }
    // 计算总长度：帧头 + 协议数据 + 帧尾
    int total_len = sizeof(uint32_t) + cmd_header->total_packets + sizeof(uint32_t);
    *buffer = (unsigned char*)malloc(total_len);
    if (!*buffer) {
        debug_printf("内存分配失败");
        return -1;
    }
    int pos = 0;
    // -------添加帧头标识符-------
    uint32_t frame_header = FRAME_START_MARKER;
    memcpy(*buffer + pos, &frame_header, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    // -------添加协议头-------
    memcpy(*buffer + pos, cmd_header, sizeof(GENERIC_CMD_HEADER));
    pos += sizeof(GENERIC_CMD_HEADER);
    // -------添加参数-------
    if (param_len > 0 && param_data) {
        PARAM_HEADER param_header;
        param_header.param_len = param_len;
        memcpy(*buffer + pos, &param_header, sizeof(PARAM_HEADER));
        pos += sizeof(PARAM_HEADER);
        memcpy(*buffer + pos, param_data, param_len);
        pos += param_len;
    }
    // -------添加数据-------
    if (data_len > 0 && data_payload) {
        memcpy(*buffer + pos, data_payload, data_len);
        pos += data_len;
    }

    // -------添加帧尾标识符-------
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(*buffer + pos, &end_marker, sizeof(uint32_t));
    
    // debug_printf("[PC] Protocol frame built: header=0x%08X, type=%d, cmd=%d, total_len=%d", 
    //              FRAME_START_MARKER, cmd_header->protocol_type, cmd_header->cmd_id, total_len);
    return total_len;
}