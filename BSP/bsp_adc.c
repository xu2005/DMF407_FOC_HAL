/**
  ******************************************************************************
  * @file    bsp_adc.c
  * @brief   ADC1/2/3 采样的 BSP 实现
  *
  * Legacy 出处（基线 = Legacy/Template222_A/）：
  *   SYSTEM/adc.c:62-64   ADC_Cmd(ADC1/2/3, ENABLE)
  *   SYSTEM/adc.c:70-83   ADC_GetValue()  —— 通道路由 + 无限等 EOC + 读 DR
  *   SYSTEM/adc.c:90-102  ADC_ProcessData() —— 6 次读取
  *
  * V1 约束（任务书 §9 / 本次验收标准）：
  *   不改 ADC 架构：保持 ContinuousConvMode=ENABLE、外部触发 TIM1_CC1、
  *   Falling 边沿、480 cycles 采样、12 位右对齐、无 DMA、无注入组、无 ADC 中断。
  *   不改 TIM1 控制周期；不加平均/中值滤波；不提前校准零偏与增益。
  ******************************************************************************
  */

#include "bsp_adc.h"
#include "adc.h"

#define BSP_ADC_HANDLE_BUS     (&hadc1)   /* PM1_VBUS   */
#define BSP_ADC_HANDLE_CURR_V  (&hadc2)   /* PM1_AMPV   */
#define BSP_ADC_HANDLE_CURR_W  (&hadc3)   /* PM1_AMPW   */

/**
  * @brief  单次阻塞读取，对应 Legacy SYSTEM/adc.c:70 ADC_GetValue()
  *
  * Legacy : while (ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);   ← 无限等待，无超时
  * HAL    : HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY)            ← 同为无限等待
  *
  * 已核实 Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c 中该函数的实现：
  *   - Timeout == HAL_MAX_DELAY 时整段跳过超时判断，构成真正的无限等待；
  *   - DMA + EOCS 组合的错误分支不会触发（本工程 DMA 关闭，
  *     EOCSelection = ADC_EOC_SINGLE_CONV）。
  * 故与 Legacy 的等待语义严格等价。
  *
  * V1 有意忽略返回值：不引入超时恢复、重试或错误状态机（归 V2）。
  * 差异登记见 Docs/V1_Legacy_Mapping.md 的 D-09。
  */
static uint16_t BSP_ADC_PollRead(ADC_HandleTypeDef *hadc)
{
  HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY);
  return (uint16_t)HAL_ADC_GetValue(hadc);
}

void BSP_ADC_Start(void)
{
  /* 对应 Legacy SYSTEM/adc.c:62-64  ADC_Cmd(ADC1/2/3, ENABLE)
   * 注意：外部触发为 TIM1_CC1，需 TIM1 计数器运行才会产生转换。
   *       Legacy main() 的顺序是先 ADC_Int_Init() 再 TIM1_Int_Init()，
   *       本工程保持一致。 */
  HAL_ADC_Start(BSP_ADC_HANDLE_BUS);
  HAL_ADC_Start(BSP_ADC_HANDLE_CURR_V);
  HAL_ADC_Start(BSP_ADC_HANDLE_CURR_W);
}

uint16_t BSP_ADC_ReadBus(void)
{
  return BSP_ADC_PollRead(BSP_ADC_HANDLE_BUS);
}

uint16_t BSP_ADC_ReadCurrentV(void)
{
  return BSP_ADC_PollRead(BSP_ADC_HANDLE_CURR_V);
}

uint16_t BSP_ADC_ReadCurrentW(void)
{
  return BSP_ADC_PollRead(BSP_ADC_HANDLE_CURR_W);
}

void BSP_ADC_ReadAll(uint16_t *bus, uint16_t *v_curr, uint16_t *w_curr)
{
  /* 严格复现 Legacy SYSTEM/adc.c:90-102  ADC_ProcessData() 的 6 次读取
   *
   *   :92  ADC_GetValue(ADC3, CH3);   结果丢弃
   *   :93  ADC_GetValue(ADC2, CH6);   结果丢弃
   *   :94  ADC_GetValue(ADC1, CH9);   结果丢弃
   *   :97  adc3_value = ADC_GetValue(ADC3, CH3);   → W 相
   *   :98  adc2_value = ADC_GetValue(ADC2, CH6);   → V 相
   *   :99  adc1_value = ADC_GetValue(ADC1, CH9);   → 母线
   *
   * 顺序固定 W → V → BUS → W → V → BUS，次数固定为 6。
   * 禁止改写为"每路只采一次"、三路平均、中值滤波或 DMA 批量。
   */
  (void)BSP_ADC_ReadCurrentW();   /* Legacy :92  丢弃 */
  (void)BSP_ADC_ReadCurrentV();   /* Legacy :93  丢弃 */
  (void)BSP_ADC_ReadBus();        /* Legacy :94  丢弃 */

  *w_curr = BSP_ADC_ReadCurrentW();   /* Legacy :97  adc3_value */
  *v_curr = BSP_ADC_ReadCurrentV();   /* Legacy :98  adc2_value */
  *bus    = BSP_ADC_ReadBus();        /* Legacy :99  adc1_value */
}