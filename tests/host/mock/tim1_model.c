#include "tim1_model.h"
void Tim1_ModelReset(Tim1_Model *m, TIM_TypeDef *regs)
{
    *m = (Tim1_Model){regs,1,0};
    regs->BDTR = 0;
}
void Tim1_ModelWriteBDTR(Tim1_Model *m, uint32_t value)
{
    uint32_t old = m->regs->BDTR;
    uint32_t protected_bits = m->first_write ? 0 : TIM_BDTR_LOCK;
    /* LOCK1 freezes DTG, BKE/BKP/AOE. Level2 also OSSI/OSSR.
     * CR2/CCER/CCMR higher-level locks are outside this BDTR-only model. */
    if ((old & TIM_BDTR_LOCK) >= TIM_LOCKLEVEL_1) { protected_bits |= 0x70ffU; }
    if ((old & TIM_BDTR_LOCK) >= 0x200U) { protected_bits |= 0xc00U; }
    m->regs->BDTR = (value & ~protected_bits) | (old & protected_bits);
    m->first_write = 0;
    if (m->break_active && (m->regs->BDTR & 0x1000U)) { m->regs->BDTR &= ~TIM_BDTR_MOE; }
}
void Tim1_ModelBreak(Tim1_Model *m, unsigned active)
{
    m->break_active = active;
    if (active && (m->regs->BDTR & 0x1000U)) { m->regs->BDTR &= ~TIM_BDTR_MOE; }
}
void Tim1_ModelUpdate(Tim1_Model *m)
{
    /* Explicit UEV may also be software UG with CEN=0. */
    m->regs->SR |= TIM_FLAG_UPDATE;
    if ((m->regs->BDTR & TIM_BDTR_AOE) && !m->break_active) {
        m->regs->BDTR |= TIM_BDTR_MOE;
    }
}
