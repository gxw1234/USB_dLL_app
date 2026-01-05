#include "usb_audil.h"
#include "usb_i2s.h"
#include "usb_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include "platform_compat.h"
#endif

// WAV文件头结构体
#ifdef _WIN32
#pragma pack(push, 1)
#endif
typedef struct {
    char riff[4];           // "RIFF"
    unsigned int file_size; // 文件大小-8
    char wave[4];           // "WAVE"
}
#ifndef _WIN32
__attribute__((packed))
#endif
WAV_RIFF_HEADER;

typedef struct {
    char fmt[4];            // "fmt "
    unsigned int chunk_size; // 格式块大小
    unsigned short audio_format; // 音频格式
    unsigned short channels;     // 声道数
    unsigned int sample_rate;    // 采样率
    unsigned int byte_rate;      // 字节率
    unsigned short block_align;  // 块对齐
    unsigned short bits_per_sample; // 位深度
}
#ifndef _WIN32
__attribute__((packed))
#endif
WAV_FMT_CHUNK;

typedef struct {
    char data[4];           // "data"
    unsigned int data_size; // 数据大小
}
#ifndef _WIN32
__attribute__((packed))
#endif
WAV_DATA_HEADER;
#ifdef _WIN32
#pragma pack(pop)
#endif

// 全局音频进度回调
static AudioProgressCallback g_audio_progress_callback = NULL;
static void* g_audio_user_data = NULL;

// 注册音频进度回调
WINAPI void AudioSetProgressCallback(AudioProgressCallback callback, void* user_data) {
    g_audio_progress_callback = callback;
    g_audio_user_data = user_data;
}

// 简化的音频播放接口
WINAPI int AudioStart(const char* target_serial, const char* wav_file_path, int volume) {
    FILE* file = NULL;
    unsigned char** audio_chunks = NULL;
    unsigned int chunk_count = 0;
    int ret = AUDIO_SUCCESS;

    // 打开WAV文件
    debug_printf("正在打开WAV文件: %s", wav_file_path);
    file = fopen(wav_file_path, "rb");
    if (!file) {
        debug_printf("WAV文件打开失败: %s", wav_file_path);
        return AUDIO_ERROR_FILE_NOT_FOUND;
    }

    // 读取WAV文件头
    WAV_RIFF_HEADER riff_header;
    WAV_FMT_CHUNK fmt_chunk;
    unsigned int sample_rate = 16000; // 默认采样率
    unsigned short channels = 2;      // 默认双声道

    // 读取RIFF头
    if (fread(&riff_header, sizeof(riff_header), 1, file) != 1) {
        fclose(file);
        return AUDIO_ERROR_INVALID_FORMAT;
    }

    if (memcmp(riff_header.riff, "RIFF", 4) != 0 || 
        memcmp(riff_header.wave, "WAVE", 4) != 0) {
        fclose(file);
        return AUDIO_ERROR_INVALID_FORMAT;
    }

    // 查找fmt块和data块
    char chunk_id[4];
    unsigned int chunk_size;
    unsigned int data_size = 0;
    
    while (fread(chunk_id, 4, 1, file) == 1) {
        fread(&chunk_size, 4, 1, file);
        
        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            // 读取格式信息
            if (fread(&fmt_chunk.audio_format, chunk_size, 1, file) == 1) {
                sample_rate = fmt_chunk.sample_rate;
                channels = fmt_chunk.channels;
            }
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            // 找到数据块
            data_size = chunk_size;
            break;
        } else {
            // 跳过其他块
            fseek(file, chunk_size, SEEK_CUR);
        }
    }

    if (data_size == 0) {
        fclose(file);
        return AUDIO_ERROR_INVALID_FORMAT;
    }

    debug_printf("音频格式: %d声道, 采样率=%d Hz", channels, sample_rate);

    const unsigned int CHUNK_SIZE = 1280;
    unsigned char* mono_buffer = NULL;
    unsigned int stereo_data_size = data_size;
    
    // 如果是单声道，需要转换为双声道
    if (channels == 1) {
        debug_printf("检测到单声道音频，转换为双声道...");
        stereo_data_size = data_size * 2;  // 双声道数据量翻倍
        
        // 读取全部单声道数据
        mono_buffer = (unsigned char*)malloc(data_size);
        if (!mono_buffer) {
            fclose(file);
            return AUDIO_ERROR_OTHER;
        }
        fread(mono_buffer, 1, data_size, file);
        fclose(file);
        file = NULL;
        
        // 分配双声道缓冲区
        unsigned char* stereo_buffer = (unsigned char*)malloc(stereo_data_size);
        if (!stereo_buffer) {
            free(mono_buffer);
            return AUDIO_ERROR_OTHER;
        }
        
        // 单声道转双声道：每个样本复制到左右声道
        short* mono_samples = (short*)mono_buffer;
        short* stereo_samples = (short*)stereo_buffer;
        unsigned int mono_sample_count = data_size / 2;
        for (unsigned int i = 0; i < mono_sample_count; i++) {
            stereo_samples[i * 2] = mono_samples[i];     // 左声道
            stereo_samples[i * 2 + 1] = mono_samples[i]; // 右声道
        }
        free(mono_buffer);
        mono_buffer = stereo_buffer;  // 复用指针
    }

    chunk_count = (stereo_data_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    audio_chunks = (unsigned char**)malloc(chunk_count * sizeof(unsigned char*));
    if (!audio_chunks) {
        if (file) fclose(file);
        if (channels == 1 && mono_buffer) free(mono_buffer);
        return AUDIO_ERROR_OTHER;
    }
    
    for (unsigned int i = 0; i < chunk_count; i++) {
        audio_chunks[i] = (unsigned char*)malloc(CHUNK_SIZE);
        if (!audio_chunks[i]) {
            for (unsigned int j = 0; j < i; j++) {
                free(audio_chunks[j]);
            }
            free(audio_chunks);
            if (file) fclose(file);
            if (channels == 1 && mono_buffer) free(mono_buffer);
            return AUDIO_ERROR_OTHER;
        }
        
        size_t bytes_read;
        if (channels == 1) {
            // 从转换后的双声道缓冲区复制
            unsigned int offset = i * CHUNK_SIZE;
            unsigned int copy_size = (offset + CHUNK_SIZE <= stereo_data_size) ? CHUNK_SIZE : (stereo_data_size - offset);
            memcpy(audio_chunks[i], mono_buffer + offset, copy_size);
            bytes_read = copy_size;
        } else {
            // 直接从文件读取双声道数据
            bytes_read = fread(audio_chunks[i], 1, CHUNK_SIZE, file);
        }
        
        if (bytes_read < CHUNK_SIZE) {
            memset(audio_chunks[i] + bytes_read, 0, CHUNK_SIZE - bytes_read);
        }
        
        // 应用音量（volume=100时不调整）
        if (volume != 100) {
            short* samples = (short*)audio_chunks[i];
            unsigned int sample_count = CHUNK_SIZE / 2;
            float vol_factor = volume / 100.0f;
            for (unsigned int j = 0; j < sample_count; j++) {
                samples[j] = (short)(samples[j] * vol_factor);
            }
        }
    }
    
    if (file) fclose(file);
    if (channels == 1 && mono_buffer) free(mono_buffer);

    ret = I2S_StartQueue(target_serial, 1);
    if (ret != I2S_SUCCESS) {
        debug_printf("启动I2S队列失败，错误代码: %d", ret);
        goto cleanup;
    }
    debug_printf("I2S队列启动成功");

    debug_printf("开始播放WAV文件: %s", wav_file_path);
    debug_printf("采样率: %d Hz, 音频块数: %d", sample_rate, chunk_count);

    debug_printf("开始WAV音频传输...");
    
    unsigned int initial_chunks = (chunk_count < 8) ? chunk_count : 8;
    debug_printf("预填充音频队列 (前%d个音频块)...", initial_chunks);
    for (unsigned int i = 0; i < initial_chunks; i++) {
        int write_ret = I2S_Queue_WriteBytes(target_serial, 1, audio_chunks[i], CHUNK_SIZE);
        // 调用进度回调
        if (g_audio_progress_callback) {
            g_audio_progress_callback(i + 1, chunk_count, g_audio_user_data);
        }
    }

    // 发送剩余音频块 - 动态流控制
    unsigned int remaining_chunks = chunk_count - initial_chunks;
    if (remaining_chunks > 0) {
        debug_printf("发送剩余的 %d 个音频块...", remaining_chunks);
        for (unsigned int i = initial_chunks; i < chunk_count; i++) {
            // 检查队列状态，如果大于7就等待
            while (1) {
                int queue_status = I2S_GetQueueStatus(target_serial, 1);
                if (queue_status >= 0 && queue_status <= 7) {
                    break; // 队列有空间，可以发送
                } else if (queue_status > 7) {
                    Sleep(10); // 队列满，等待10ms
                } else {
                    debug_printf("队列状态查询失败: %d", queue_status);
                    break;
                }
            }
            // 发送音频块
            int write_ret = I2S_Queue_WriteBytes(target_serial, 1, audio_chunks[i], CHUNK_SIZE);
            // 调用进度回调
            if (g_audio_progress_callback) {
                g_audio_progress_callback(i + 1, chunk_count, g_audio_user_data);
            }
            if (write_ret == 0) {
                if ((i + 1) % 50 == 0 || i == chunk_count - 1) {
                    debug_printf("成功发送第 %d 个音频块", i + 1);
                }
            } else {
                debug_printf("发送第 %d 个音频块失败，状态码: %d", i + 1, write_ret);
            }
        }
    }

    // 等待音频队列完全播放完成
    debug_printf("等待WAV音频播放完成...");
    int empty_count = 0;
    while (empty_count < 3) {
        int queue_status = I2S_GetQueueStatus(target_serial, 1);
        if (queue_status == 0) {
            empty_count++;
        } else {
            empty_count = 0;
        }
        Sleep(50); // 50ms检查一次
    }

    debug_printf("音频播放完成");

cleanup:
    // 清理资源
    if (audio_chunks) {
        for (unsigned int i = 0; i < chunk_count; i++) {
            if (audio_chunks[i]) {
                free(audio_chunks[i]);
            }
        }
        free(audio_chunks);
    }

    // 停止I2S
    I2S_StopQueue(target_serial, 1);

    return ret;
}

// 双路音频播放接口
WINAPI int AudioStartDual(const char* target_serial, const DUAL_AUDIO_CONFIG* config) {
    FILE* left_file = NULL;
    FILE* right_file = NULL;
    FILE* output_file = NULL;
    unsigned char** audio_chunks = NULL;
    unsigned int chunk_count = 0;
    int ret = AUDIO_SUCCESS;

    // 参数验证
    if (!target_serial || !config || !config->left_audio_path || !config->right_audio_path) {
        debug_printf("双路音频参数无效");
        return AUDIO_ERROR_INVALID_PARAM;
    }

    debug_printf("开始双路音频播放");
    debug_printf("左声道文件: %s", config->left_audio_path);
    debug_printf("右声道文件: %s", config->right_audio_path);
    debug_printf("延迟时间: %.2f秒", config->gap_duration);
    debug_printf("生成文件: %s", config->generate_file ? "是" : "否");
    debug_printf("左声道音量: %d%%", config->left_volume);
    debug_printf("右声道音量: %d%%", config->right_volume);

    // 打开左声道文件
    left_file = fopen(config->left_audio_path, "rb");
    if (!left_file) {
        debug_printf("左声道文件打开失败: %s", config->left_audio_path);
        return AUDIO_ERROR_FILE_NOT_FOUND;
    }

    // 打开右声道文件
    right_file = fopen(config->right_audio_path, "rb");
    if (!right_file) {
        debug_printf("右声道文件打开失败: %s", config->right_audio_path);
        fclose(left_file);
        return AUDIO_ERROR_FILE_NOT_FOUND;
    }

    // 读取左声道WAV文件头
    WAV_RIFF_HEADER left_riff, right_riff;
    WAV_FMT_CHUNK left_fmt, right_fmt;
    unsigned int left_sample_rate = 16000, right_sample_rate = 16000;
    unsigned int left_data_size = 0, right_data_size = 0;

    // 解析左声道文件
    if (fread(&left_riff, sizeof(left_riff), 1, left_file) != 1 ||
        memcmp(left_riff.riff, "RIFF", 4) != 0 || memcmp(left_riff.wave, "WAVE", 4) != 0) {
        debug_printf("左声道文件格式无效");
        fclose(left_file);
        fclose(right_file);
        return AUDIO_ERROR_INVALID_FORMAT;
    }

    // 解析右声道文件
    if (fread(&right_riff, sizeof(right_riff), 1, right_file) != 1 ||
        memcmp(right_riff.riff, "RIFF", 4) != 0 || memcmp(right_riff.wave, "WAVE", 4) != 0) {
        debug_printf("右声道文件格式无效");
        fclose(left_file);
        fclose(right_file);
        return AUDIO_ERROR_INVALID_FORMAT;
    }

    // 查找左声道fmt和data块
    char chunk_id[4];
    unsigned int chunk_size;
    while (fread(chunk_id, 4, 1, left_file) == 1) {
        fread(&chunk_size, 4, 1, left_file);
        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            if (fread(&left_fmt.audio_format, chunk_size, 1, left_file) == 1) {
                left_sample_rate = left_fmt.sample_rate;
            }
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            left_data_size = chunk_size;
            break;
        } else {
            fseek(left_file, chunk_size, SEEK_CUR);
        }
    }

    // 查找右声道fmt和data块
    while (fread(chunk_id, 4, 1, right_file) == 1) {
        fread(&chunk_size, 4, 1, right_file);
        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            if (fread(&right_fmt.audio_format, chunk_size, 1, right_file) == 1) {
                right_sample_rate = right_fmt.sample_rate;
            }
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            right_data_size = chunk_size;
            break;
        } else {
            fseek(right_file, chunk_size, SEEK_CUR);
        }
    }

    // 检查采样率是否一致
    if (left_sample_rate != right_sample_rate) {
        debug_printf("两个音频的采样率不一致: 左=%d, 右=%d", left_sample_rate, right_sample_rate);
        fclose(left_file);
        fclose(right_file);
        return AUDIO_ERROR_INVALID_FORMAT;
    }

    if (left_data_size == 0 || right_data_size == 0) {
        debug_printf("音频数据块无效: 左=%d, 右=%d", left_data_size, right_data_size);
        fclose(left_file);
        fclose(right_file);
        return AUDIO_ERROR_INVALID_FORMAT;
    }

    debug_printf("音频信息: 采样率=%d Hz, 左声道=%d字节, 右声道=%d字节", 
                 left_sample_rate, left_data_size, right_data_size);

    // 读取左右声道数据
    unsigned char* left_data = (unsigned char*)malloc(left_data_size);
    unsigned char* right_data = (unsigned char*)malloc(right_data_size);
    
    if (!left_data || !right_data) {
        debug_printf("内存分配失败");
        if (left_data) free(left_data);
        if (right_data) free(right_data);
        fclose(left_file);
        fclose(right_file);
        return AUDIO_ERROR_OTHER;
    }

    fread(left_data, 1, left_data_size, left_file);
    fread(right_data, 1, right_data_size, right_file);
    fclose(left_file);
    fclose(right_file);

    // 计算合成音频参数
    unsigned int gap_samples = (unsigned int)(config->gap_duration * left_sample_rate * 2); // 16位立体声，每样本2字节
    unsigned int left_samples = left_data_size / 2;  // 16位单声道
    unsigned int right_samples = right_data_size / 2;
    unsigned int total_samples = (left_samples > (gap_samples/2 + right_samples)) ? left_samples : (gap_samples/2 + right_samples);
    unsigned int total_stereo_size = total_samples * 4; // 16位立体声，每样本4字节

    debug_printf("合成参数: 延迟样本=%d, 左样本=%d, 右样本=%d, 总样本=%d", 
                 gap_samples/2, left_samples, right_samples, total_samples);

    // 分配立体声合成缓冲区
    unsigned char* stereo_data = (unsigned char*)calloc(total_stereo_size, 1);
    if (!stereo_data) {
        debug_printf("立体声缓冲区分配失败");
        free(left_data);
        free(right_data);
        return AUDIO_ERROR_OTHER;
    }

    // 合成立体声音频：左声道数据填充到左声道，右声道数据延迟后填充到右声道
    short* stereo_samples = (short*)stereo_data;
    short* left_samples_ptr = (short*)left_data;
    short* right_samples_ptr = (short*)right_data;

    // 计算音量系数 (0-100 -> 0.0-1.0)
    float left_volume_factor = config->left_volume / 100.0f;
    float right_volume_factor = config->right_volume / 100.0f;
    
    // 填充左声道（应用音量）
    for (unsigned int i = 0; i < left_samples; i++) {
        stereo_samples[i * 2] = (short)(left_samples_ptr[i] * left_volume_factor); // 左声道
    }

    // 填充右声道（带延迟和音量）
    unsigned int right_start_sample = gap_samples / 2;
    for (unsigned int i = 0; i < right_samples && (right_start_sample + i) < total_samples; i++) {
        stereo_samples[(right_start_sample + i) * 2 + 1] = (short)(right_samples_ptr[i] * right_volume_factor); // 右声道
    }

    free(left_data);
    free(right_data);

    // 如果需要生成文件
    if (config->generate_file && config->output_path) {
        debug_printf("生成合成音频文件: %s", config->output_path);
        output_file = fopen(config->output_path, "wb");
        if (output_file) {
            // 写入WAV文件头
            WAV_RIFF_HEADER out_riff = {"RIFF", total_stereo_size + 36, "WAVE"};
            WAV_FMT_CHUNK out_fmt = {"fmt ", 16, 1, 2, left_sample_rate, left_sample_rate * 4, 4, 16};
            WAV_DATA_HEADER out_data = {"data", total_stereo_size};
            
            fwrite(&out_riff, sizeof(out_riff), 1, output_file);
            fwrite(&out_fmt, sizeof(out_fmt), 1, output_file);
            fwrite(&out_data, sizeof(out_data), 1, output_file);
            fwrite(stereo_data, 1, total_stereo_size, output_file);
            fclose(output_file);
            debug_printf("合成音频文件生成完成");
        } else {
            debug_printf("合成音频文件创建失败: %s", config->output_path);
        }
    }
    // 使用合成的立体声数据进行播放
    const unsigned int CHUNK_SIZE = 16000;
    chunk_count = (total_stereo_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    audio_chunks = (unsigned char**)malloc(chunk_count * sizeof(unsigned char*));
    if (!audio_chunks) {
        free(stereo_data);
        return AUDIO_ERROR_OTHER;
    }
    // 分块处理合成音频
    for (unsigned int i = 0; i < chunk_count; i++) {
        audio_chunks[i] = (unsigned char*)malloc(CHUNK_SIZE);
        if (!audio_chunks[i]) {
            for (unsigned int j = 0; j < i; j++) {
                free(audio_chunks[j]);
            }
            free(audio_chunks);
            free(stereo_data);
            return AUDIO_ERROR_OTHER;
        }
        unsigned int offset = i * CHUNK_SIZE;
        unsigned int copy_size = (offset + CHUNK_SIZE <= total_stereo_size) ? CHUNK_SIZE : (total_stereo_size - offset);
        memcpy(audio_chunks[i], stereo_data + offset, copy_size);
        if (copy_size < CHUNK_SIZE) {
            memset(audio_chunks[i] + copy_size, 0, CHUNK_SIZE - copy_size);
        }
    }
    free(stereo_data);



    // I2S_SetVolume(target_serial, 1, 40);
    // debug_printf("设置音量: 40%%");

    ret = I2S_StartQueue(target_serial, 1);
    if (ret != I2S_SUCCESS) {
        debug_printf("启动I2S队列失败，错误代码: %d", ret);
        goto cleanup_dual;
    }
    debug_printf("I2S队列启动成功");
    debug_printf("开始播放双路合成音频");
    debug_printf("采样率: %d Hz, 音频块数: %d", left_sample_rate, chunk_count);

    unsigned int initial_chunks = (chunk_count < 8) ? chunk_count : 8;
    debug_printf("预填充音频队列 (前%d个音频块)...", initial_chunks);
    for (unsigned int i = 0; i < initial_chunks; i++) {
        I2S_Queue_WriteBytes(target_serial, 1, audio_chunks[i], CHUNK_SIZE);
    }
    unsigned int remaining_chunks = chunk_count - initial_chunks;
    if (remaining_chunks > 0) {
        debug_printf("发送剩余的 %d 个音频块...", remaining_chunks);
        for (unsigned int i = initial_chunks; i < chunk_count; i++) {
            while (1) {
                int queue_status = I2S_GetQueueStatus(target_serial, 1);
                if (queue_status >= 0 && queue_status <= 7) {
                    break;
                } else if (queue_status > 7) {
                    Sleep(10);
                } else {
                    debug_printf("队列状态查询失败: %d", queue_status);
                    break;
                }
            }
            
            int write_ret = I2S_Queue_WriteBytes(target_serial, 1, audio_chunks[i], CHUNK_SIZE);
            if (write_ret == 0) {
                if ((i + 1) % 50 == 0 || i == chunk_count - 1) {
                    debug_printf("成功发送第 %d 个音频块", i + 1);
                }
            } else {
                debug_printf("发送第 %d 个音频块失败，状态码: %d", i + 1, write_ret);
            }
        }
    }
    debug_printf("等待双路音频播放完成...");
    int empty_count = 0;
    while (empty_count < 3) {
        int queue_status = I2S_GetQueueStatus(target_serial, 1);
        if (queue_status == 0) {
            empty_count++;
        } else {
            empty_count = 0;
        }
        Sleep(50);
    }
    debug_printf("双路音频播放完成");
cleanup_dual:
    if (audio_chunks) {
        for (unsigned int i = 0; i < chunk_count; i++) {
            if (audio_chunks[i]) {
                free(audio_chunks[i]);
            }
        }
        free(audio_chunks);
    }

    I2S_StopQueue(target_serial, 1);
    return ret;
}