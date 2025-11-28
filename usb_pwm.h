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

// PWM定时器配置结构体
typedef struct {
    uint32_t prescaler;           // 预分频器 (0-65535)
    uint32_t period;              // 计数周期 (0-65535 for 16-bit timer)
    uint32_t counter_mode;        // 计数模式 (0=UP, 1=DOWN, 2=CENTER_ALIGNED1, etc.)
    uint32_t clock_division;      // 时钟分频 (0=DIV1, 1=DIV2, 2=DIV4)
    uint32_t auto_reload_preload; // 自动重载预装载 (0=DISABLE, 1=ENABLE)
} PWM_TimerConfig_t;

// PWM测量结果结构体
typedef struct {
    uint32_t frequency;     // 频率 (Hz)
    uint32_t duty_cycle;    // 占空比 (0-10000, 表示0.00%-100.00%)
    uint32_t period_us;     // 周期 (微秒)
    uint32_t pulse_width_us; // 脉冲宽度 (微秒)
} PWM_MeasureResult_t;

// PWM输入捕获初始化
// @param target_serial 设备序列号
// @param pwm_index PWM通道索引 (1对应PC7/TIM8_CH2)
// @param config PWM定时器配置参数
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int PWM_Init(const char* target_serial, int pwm_index, PWM_TimerConfig_t* config);

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