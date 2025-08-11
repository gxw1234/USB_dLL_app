

#include "usb_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>


static int g_log_enabled = 0; // 默认禁用日志

 //1=开启日志，0=关闭日志
void USB_SetLog(int enable) {
    g_log_enabled = enable ? 1 : 0;
    if (enable) {
        FILE* fp = fopen("usb_debug.log", "a");
        if (fp) {
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char time_str[26];
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            
            fprintf(fp, "[%s] USB日志已启用\n", time_str);
            fclose(fp);
        }
    } else {
        if (g_log_enabled) {  
            FILE* fp = fopen("usb_debug.log", "a");
            if (fp) {
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


void debug_printf(const char* format, ...) {
    if (USB_DEBUG_ENABLE == 0 || g_log_enabled) {
        FILE* fp = fopen("usb_debug.log", "a");
        if (fp) {

            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char time_str[26];
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

            fprintf(fp, "[%s] ", time_str);

            va_list args;
            va_start(args, format);
            vfprintf(fp, format, args);
            fprintf(fp, "\n");
            va_end(args);
            fclose(fp);
        }
    }
}
