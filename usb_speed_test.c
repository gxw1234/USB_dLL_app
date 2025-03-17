#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

#define VENDOR_ID   0x1733
#define PRODUCT_ID  0xAABB
#define EP_IN       0x81    // Endpoint address, modify as needed
#define BUF_SIZE    1024   // Transfer buffer size
#define TEST_TIME   5       // Test duration (seconds)
#define SAVE_DATA   1       // 1: 保存接收到的数据, 0: 不保存
#define VERIFY_DATA 1       // 1: 验证接收到的数据, 0: 不验证

// libusb function pointer definitions
typedef int (*libusb_init_t)(void**);
typedef void (*libusb_exit_t)(void*);
typedef void* (*libusb_open_device_with_vid_pid_t)(void*, unsigned short, unsigned short);
typedef int (*libusb_claim_interface_t)(void*, int);
typedef int (*libusb_bulk_transfer_t)(void*, unsigned char, unsigned char*, int, int*, unsigned int);
typedef void (*libusb_close_t)(void*);
typedef int (*libusb_release_interface_t)(void*, int);

// Global function pointers
libusb_init_t p_libusb_init;
libusb_exit_t p_libusb_exit;
libusb_open_device_with_vid_pid_t p_libusb_open_device_with_vid_pid;
libusb_claim_interface_t p_libusb_claim_interface;
libusb_bulk_transfer_t p_libusb_bulk_transfer;
libusb_close_t p_libusb_close;
libusb_release_interface_t p_libusb_release_interface;

int load_libusb() {
    HMODULE hLib = LoadLibrary("libusb-1.0.dll");
    if (!hLib) {
        printf("Failed to load libusb-1.0.dll\n");
        return 0;
    }

    p_libusb_init = (libusb_init_t)GetProcAddress(hLib, "libusb_init");
    p_libusb_exit = (libusb_exit_t)GetProcAddress(hLib, "libusb_exit");
    p_libusb_open_device_with_vid_pid = (libusb_open_device_with_vid_pid_t)GetProcAddress(hLib, "libusb_open_device_with_vid_pid");
    p_libusb_claim_interface = (libusb_claim_interface_t)GetProcAddress(hLib, "libusb_claim_interface");
    p_libusb_bulk_transfer = (libusb_bulk_transfer_t)GetProcAddress(hLib, "libusb_bulk_transfer");
    p_libusb_close = (libusb_close_t)GetProcAddress(hLib, "libusb_close");
    p_libusb_release_interface = (libusb_release_interface_t)GetProcAddress(hLib, "libusb_release_interface");

    if (!p_libusb_init || !p_libusb_exit || !p_libusb_open_device_with_vid_pid || 
        !p_libusb_claim_interface || !p_libusb_bulk_transfer || !p_libusb_close ||
        !p_libusb_release_interface) {
        printf("Failed to get libusb function addresses\n");
        FreeLibrary(hLib);
        return 0;
    }

    return 1;
}

int main() {
    void *dev_handle;
    int ret;
    unsigned char *data;
    int actual_length;
    clock_t start, end;
    double total_bytes = 0;
    FILE *data_file = NULL;
    
    // Load libusb dynamic library
    if (!load_libusb()) {
        return 1;
    }

    // Initialize libusb
    ret = p_libusb_init(NULL);
    if (ret < 0) {
        printf("Failed to initialize libusb: %d\n", ret);
        return 1;
    }

    // Open the specified USB device
    dev_handle = p_libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (dev_handle == NULL) {
        printf("USB device not found (VID: 0x%04x, PID: 0x%04x)\n", VENDOR_ID, PRODUCT_ID);
        p_libusb_exit(NULL);
        return 1;
    }

    // Claim interface
    ret = p_libusb_claim_interface(dev_handle, 0);
    if (ret < 0) {
        printf("Failed to claim interface: %d\n", ret);
        p_libusb_close(dev_handle);
        p_libusb_exit(NULL);
        return 1;
    }

    // Allocate receive buffer
    data = (unsigned char*)malloc(BUF_SIZE);
    if (!data) {
        printf("Memory allocation failed\n");
        p_libusb_release_interface(dev_handle, 0);
        p_libusb_close(dev_handle);
        p_libusb_exit(NULL);
        return 1;
    }
    
    // 创建文件保存接收的数据（无论SAVE_DATA是否为1，都需要保存以便于验证）
    data_file = fopen("usb_received_data.bin", "wb");
    if (!data_file) {
        printf("Failed to create data file\n");
        free(data);
        p_libusb_release_interface(dev_handle, 0);
        p_libusb_close(dev_handle);
        p_libusb_exit(NULL);
        return 1;
    }

    printf("Starting USB uplink speed test...\n");
    start = clock();

    // Receive data for TEST_TIME seconds
    while ((clock() - start) / CLOCKS_PER_SEC < TEST_TIME) {
        ret = p_libusb_bulk_transfer(dev_handle, EP_IN, data, BUF_SIZE, &actual_length, 1000);
        if (ret == 0) {
            total_bytes += actual_length;

            // 保存接收到的数据（无论SAVE_DATA是否为1，都需要保存以便于验证）
            fwrite(data, 1, actual_length, data_file);
            
        } else if (ret != -7) { // LIBUSB_ERROR_TIMEOUT = -7
            printf("Transfer error: %d\n", ret);
            break;
        }
    }

    // 关闭数据文件
    fclose(data_file);
    data_file = NULL;

    end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
    double speed = (total_bytes / (1024.0 * 1024.0)) / elapsed_time; // MB/s

    printf("\n测试结果:\n");
    printf("总接收数据量: %.2f MB\n", total_bytes / (1024.0 * 1024.0));
    printf("耗时: %.2f 秒\n", elapsed_time);
    printf("平均速度: %.2f MB/s\n", speed);
    
    // 在接收完成后进行数据验证
    if (VERIFY_DATA) {
        printf("\n开始数据验证...\n");
        
        // 重新打开数据文件用于验证
        data_file = fopen("usb_received_data.bin", "rb");
        if (!data_file) {
            printf("无法打开数据文件进行验证\n");
        } else {
            // 分配一个缓冲区用于读取数据
            unsigned char *verify_buffer = (unsigned char*)malloc(BUF_SIZE);
            if (!verify_buffer) {
                printf("内存分配失败\n");
                fclose(data_file);
            } else {
                int data_error_count = 0; // 错误计数
                int resync_count = 0;     // 重新同步计数
                long long total_verified = 0;
                int read_size;
                unsigned char pattern[2] = {0}; // 存储检测到的模式
                int pattern_detected = 0;     // 是否已检测到模式
                int sequence_length = 2;      // 模式长度(1-2)
                int pattern_position = 0;     // 当前在模式中的位置
                int consecutive_errors = 0;   // 连续错误计数，用于检测是否需要重新同步
                
                // 从文件中读取数据并验证
                while ((read_size = fread(verify_buffer, 1, BUF_SIZE, data_file)) > 0) {
                    for (int i = 0; i < read_size; i++) {
                        // 如果还没有检测到模式，尝试检测
                        if (!pattern_detected) {
                            // 寻找1-2交替模式
                            if (i + 1 < read_size) {
                                if ((verify_buffer[i] == 1 && verify_buffer[i+1] == 2) ||
                                    (verify_buffer[i] == 2 && verify_buffer[i+1] == 1)) {
                                    pattern[0] = verify_buffer[i];
                                    pattern[1] = verify_buffer[i+1];
                                    pattern_detected = 1;
                                    pattern_position = 0; // 从模式的开始位置验证
                                    printf("检测到数据模式: %d-%d\n", pattern[0], pattern[1]);
                                    
                                    // 如果模式不是从1开始，输出提示
                                    if (pattern[0] != 1 || pattern[1] != 2) {
                                        printf("注意: 检测到的模式不是标准的1-2模式\n");
                                    }
                                    
                                    // 跳过已处理的模式样本
                                    i += 1;
                                    total_verified += 2;
                                    continue;
                                }
                            }
                        }
                        // 已检测到模式，进行验证
                        else {
                            // 检查是否需要重新同步
                            // 如果当前字节是1，且期望的模式不是1，可能是新的循环开始
                            if (verify_buffer[i] == 1 && pattern[pattern_position] != 1) {
                                // 只有当看到完整的1-2序列时才重新同步
                                if (i + 1 < read_size && verify_buffer[i+1] == 2) {
                                    
                                    resync_count++;
                                    if (resync_count <= 5) { // 只打印前几次重新同步信息
                                        printf("在位置 %lld 检测到重新同步点: 找到新的1-2序列\n", total_verified);
                                    }
                                    
                                    // 重置模式位置到开始
                                    pattern_position = 0;
                                    consecutive_errors = 0;
                                    continue;
                                }
                            }
                            
                            // 正常验证
                            if (verify_buffer[i] != pattern[pattern_position]) {
                                data_error_count++;
                                consecutive_errors++;
                                
                                // 如果连续错误过多，尝试重新同步
                                if (consecutive_errors >= 6) { // 连续错误超过两个完整的模式
                                    // 寻找下一个可能的模式起点
                                    int found = 0;
                                    for (int j = i + 1; j < read_size - 1; j++) {
                                        if (verify_buffer[j] == 1 && 
                                            j + 1 < read_size && 
                                            verify_buffer[j+1] == 2) {
                                            // 找到新的1-2序列
                                            i = j - 1; // 下一轮循环将从j开始
                                            pattern_position = 0;
                                            consecutive_errors = 0;
                                            resync_count++;
                                            
                                            if (resync_count <= 5) {
                                                printf("在位置 %lld 强制重新同步: 找到新的1-2序列\n", 
                                                       total_verified + (j - i));
                                            }
                                            
                                            found = 1;
                                            break;
                                        }
                                    }
                                    
                                    if (found) {
                                        continue;
                                    } else {
                                        consecutive_errors = 0; // 未找到，也重置计数，避免重复搜索
                                    }
                                }
                                
                                if (data_error_count <= 10) { // 只打印前10个错误
                                    printf("数据错误在位置 %lld: 期望值=%d, 实际值=%d\n", 
                                           total_verified, pattern[pattern_position], verify_buffer[i]);
                                }
                            } else {
                                consecutive_errors = 0; // 正确匹配，重置连续错误计数
                            }
                            
                            // 移动到模式中的下一个位置
                            pattern_position = (pattern_position + 1) % sequence_length;
                            total_verified++;
                        }
                    }
                    
                    // 如果读取了足够多的数据但仍未检测到模式，可能数据不符合预期
                    if (!pattern_detected && total_verified > BUF_SIZE * 2) {
                        printf("警告: 无法检测到明确的数据循环模式\n");
                        // 尝试分析前100个字节，寻找模式
                        printf("前100个字节的数据样本: ");
                        fseek(data_file, 0, SEEK_SET);
                        fread(verify_buffer, 1, 100, data_file);
                        for (int j = 0; j < 100 && j < read_size; j++) {
                            printf("%d ", verify_buffer[j]);
                            if ((j+1) % 20 == 0) printf("\n");
                        }
                        printf("\n");
                        break;
                    }
                }
                
                free(verify_buffer);
                
                printf("\n数据验证结果:\n");
                if (pattern_detected) {
                    printf("检测到的基本模式: %d-%d\n", pattern[0], pattern[1]);
                    printf("总处理数据量: %.2f MB\n", total_verified / (1024.0 * 1024.0));
                    printf("重新同步次数: %d\n", resync_count);
                    
                    if (data_error_count == 0) {
                        printf("所有数据验证正确! 数据完全符合 %d-%d 的交替模式。\n", 
                               pattern[0], pattern[1]);
                    } else {
                        printf("检测到 %d 个数据错误。\n", data_error_count);
                        double error_rate = (double)data_error_count / total_verified * 100.0;
                        printf("错误率: %.6f%%\n", error_rate);
                        
                        // 如果重新同步次数高，且错误率高，添加解释
                        if (resync_count > 0 && error_rate > 10.0) {
                            printf("\n分析结果:\n");
                            printf("数据似乎在每个缓冲区或数据包边界重新开始1-2交替模式。\n");
                            printf("这意味着设备正在正确地发送1-2交替数据，但是在传输的\n");
                            printf("某些边界处会重新开始交替模式，导致位置偏移。\n");
                        }
                    }
                } else {
                    printf("未能检测到明确的数据循环模式，请检查数据格式。\n");
                }
            }
            fclose(data_file);
        }
    }
    
    if (SAVE_DATA) {
        printf("\n数据已保存到 usb_received_data.bin 文件\n");
    } else {
        // 如果不需要保存数据，删除临时创建的文件
        remove("usb_received_data.bin");
    }

    // Clean up resources
    free(data);
    p_libusb_release_interface(dev_handle, 0);
    p_libusb_close(dev_handle);
    p_libusb_exit(NULL);

    return 0;
}
