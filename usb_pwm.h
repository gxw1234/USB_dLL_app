#ifndef USB_PWM_H
#define USB_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// 跨平台宏定义
#ifndef WINAPI
#ifdef _WIN32
#define WINAPI __stdcall
#else
#define WINAPI
#endif
#endif

// PWM测量结果结构体
typedef struct {
    uint32_t frequency;     // 频率 (Hz)
    uint32_t duty_cycle;    // 占空比 (0-10000, 表示0.00%-100.00%)
    uint32_t period_us;     // 周期 (微秒)
    uint32_t pulse_width_us; // 脉冲宽度 (微秒)
} PWM_MeasureResult_t;

// PWM输入捕获初始化
// @param target_serial 设备序列号
// @param pwm_index PWM通道索引 (1-4对应TIM2的CH1-CH4)
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int PWM_Init(const char* target_serial, int pwm_index);

// 开始PWM测量
// @param target_serial 设备序列号
// @param pwm_index PWM通道索引
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int PWM_StartMeasure(const char* target_serial, int pwm_index);

// 停止PWM测量
// @param target_serial 设备序列号
// @param pwm_index PWM通道索引
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int PWM_StopMeasure(const char* target_serial, int pwm_index);

// 获取PWM测量结果
// @param target_serial 设备序列号
// @param pwm_index PWM通道索引
// @param result 测量结果指针
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int PWM_GetResult(const char* target_serial, int pwm_index, PWM_MeasureResult_t* result);

#ifdef __cplusplus
}
#endif

#endif // USB_PWM_H