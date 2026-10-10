/**
  ******************************************************************************
  * @file    bsp_adc.h
  * @brief   ADC1/2/3 三相电流与母线电压采样的 BSP 封装
  *
  * 对应 Legacy（基线 = Legacy/Template222_A/）：
  *   SYSTEM/adc.c   — ADC_Int_Init() / ADC_GetValue() / ADC_ProcessData()
  *
  * 职责边界（任务书 §4.3）：
  *   本模块只做 ADC 外设访问与原始值读取，不含换算与任何控制算法。
  ******************************************************************************
  */
#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 通道映射（固定，来自 .ioc 与《引脚时钟配置规范》§9.1，已对照原理图核验）
 *
 *   ADC1 / PB1 / IN9 = PM1_VBUS   母线电压
 *   ADC2 / PA6 / IN6 = PM1_AMPV   V 相电流
 *   ADC3 / PA3 / IN3 = PM1_AMPW   W 相电流
 *
 *   注意：PB1 = PM1_VBUS；PB0 = PM1_AMPU（U 相电流），不是 VBUS。
 */

/* V1 待实测项 —— 实测前禁止作结论
 *
 *   (1) 采样时刻与 PWM 相位的关系：
 *       ContinuousConvMode=ENABLE 时外部触发只启动首次转换，之后 ADC 自由运行。
 *       故"连续转换 + TIM1_CC1 触发"**不等于**每个 PWM 周期严格同步采样。
 *           采样点相对 PWM 相位 = NEEDS SCOPE CONFIRM
 *
 *   (2) 轮询阻塞时长：
 *       BSP_ADC_ReadAll() 为 6 次阻塞读取，耗时量级待测。
 *           单次与合计耗时 = NEEDS MEASURE
 *       实测前不得断言其是否会影响其它实时任务。
 */

/**
  * @brief  使能三个 ADC
  * @note   对应 Legacy SYSTEM/adc.c:62-64 的 ADC_Cmd(ADC1/2/3, ENABLE)。
  *         本工程 ADC 外部触发为 TIM1_CC1，因此需要 TIM1 计数器在运行
  *         才会产生转换；启动顺序与 Legacy main() 一致（ADC 先使能，
  *         TIM1 随后启动）。
  * @retval 无
  */
void BSP_ADC_Start(void);

/**
  * @brief  阻塞读取母线电压原始值
  * @note   对应 Legacy adc1_value（ADC1 / PB1 / PM1_VBUS）
  * @retval 12 位原始 ADC 码值（0~4095），未做任何换算
  */
uint16_t BSP_ADC_ReadBus(void);

/**
  * @brief  阻塞读取 V 相电流原始值
  * @note   对应 Legacy adc2_value（ADC2 / PA6 / PM1_AMPV）
  * @retval 12 位原始 ADC 码值（0~4095），未做任何换算
  */
uint16_t BSP_ADC_ReadCurrentV(void);

/**
  * @brief  阻塞读取 W 相电流原始值
  * @note   对应 Legacy adc3_value（ADC3 / PA3 / PM1_AMPW）
  * @retval 12 位原始 ADC 码值（0~4095），未做任何换算
  */
uint16_t BSP_ADC_ReadCurrentW(void);

/**
  * @brief  组合读取三路，严格复现 Legacy ADC_ProcessData() 的读取次数与顺序
  * @note   顺序固定为 W → V → BUS → W → V → BUS（共 6 次）：
  *           前 3 次结果丢弃，后 3 次为有效值。
  *         禁止改写为"每路只采一次"、平均滤波、中值滤波或 DMA 批量。
  * @param  bus    输出母线电压原始值（对应 Legacy adc1_value）
  * @param  v_curr 输出 V 相电流原始值（对应 Legacy adc2_value）
  * @param  w_curr 输出 W 相电流原始值（对应 Legacy adc3_value）
  * @retval 无
  */
void BSP_ADC_ReadAll(uint16_t *bus, uint16_t *v_curr, uint16_t *w_curr);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADC_H */