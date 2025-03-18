/**
 * @file usb_log.c
 * @brief USB日志系统实现
 */

#include "usb_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// 全局变量 - 动态日志控制
static int g_log_enabled = 0; // 默认禁用日志

/**
 * @brief 设置日志状态
 * 
 * @param enable 1=开启日志，0=关闭日志
 */
void USB_SetLog(int enable) {
    g_log_enabled = enable ? 1 : 0;
    if (enable) {
        // 当启用日志时，立即写入一条记录（无论是否已定义USB_DEBUG_ENABLE）
        FILE* fp = fopen("usb_debug.log", "a");
        if (fp) {
            // 添加时间戳
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char time_str[26];
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            
            fprintf(fp, "[%s] USB日志已启用\n", time_str);
            fclose(fp);
        }
    } else {
        if (g_log_enabled) {  // 如果之前是启用的，记录一条关闭消息
            FILE* fp = fopen("usb_debug.log", "a");
            if (fp) {
                // 添加时间戳
                time_t now = time(NULL);
                struct tm* tm_info = localtime(&now);
                char time_str[26];
                strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
                
                fprintf(fp, "[%s] USB日志即将禁用\n", time_str);
                fclose(fp);
            }
        }
    }
}

/**
 * @brief 调试输出函数
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void debug_printf(const char* format, ...) {
    // 只在编译时启用日志(USB_DEBUG_ENABLE=0)或运行时启用日志(g_log_enabled=1)时写入日志
    if (USB_DEBUG_ENABLE == 0 || g_log_enabled) {
        FILE* fp = fopen("usb_debug.log", "a");
        if (fp) {
            // 添加时间戳
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char time_str[26];
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            
            // 先写入时间戳
            fprintf(fp, "[%s] ", time_str);
            
            // 然后写入用户的消息
            va_list args;
            va_start(args, format);
            vfprintf(fp, format, args);
            fprintf(fp, "\n");
            va_end(args);
            fclose(fp);
        }
    }
}
