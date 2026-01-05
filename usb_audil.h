#ifndef USB_AUDIL_H
#define USB_AUDIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_application.h"

// 音频进度回调函数类型
// 参数: current_chunk - 当前已发送的块数, total_chunks - 总块数, user_data - 用户数据
typedef void (*AudioProgressCallback)(unsigned int current_chunk, unsigned int total_chunks, void* user_data);

// 注册音频进度回调（可选，不注册则不回调）
WINAPI void AudioSetProgressCallback(AudioProgressCallback callback, void* user_data);

// 音频错误码
#define AUDIO_SUCCESS               0    // 成功
#define AUDIO_ERROR_FILE_NOT_FOUND -1    // 文件未找到
#define AUDIO_ERROR_INVALID_FORMAT -2    // 无效的音频格式
#define AUDIO_ERROR_DEVICE_ERROR   -3    // 设备错误
#define AUDIO_ERROR_OTHER          -99   // 其他错误
#define AUDIO_ERROR_INVALID_PARAM   -4

// 双路音频配置结构体
typedef struct {
    const char* left_audio_path;    // 左声道音频文件路径
    const char* right_audio_path;   // 右声道音频文件路径
    float gap_duration;             // 右声道延迟时间（秒）
    int generate_file;              // 是否生成合成音频文件 (0=否, 1=是)
    const char* output_path;        // 输出合成音频文件路径（当generate_file=1时使用）
    int left_volume;                // 左声道音量 (0-任意值，100=原始音量)
    int right_volume;               // 右声道音量 (0-任意值，100=原始音量)
} DUAL_AUDIO_CONFIG;

// 简化的音频播放接口 - volume: 0-100调整音量，100=原始音量不调整
WINAPI int AudioStart(const char* target_serial, const char* wav_file_path, int volume);

// 双路音频播放接口
WINAPI int AudioStartDual(const char* target_serial, const DUAL_AUDIO_CONFIG* config);

#ifdef __cplusplus
}
#endif

#endif // USB_AUDIL_H