#include "foc_svpwm.h"

/* Only fields used by active SYSTEM/ipark.c:115-317. */
typedef struct {
    float Ts, U_alpha, U_beta, u1, u2, u3, X, Y, Z, t4, t6, ta, tb, tc;
    int sector;
} SVPWM_Work;

int FOC_SVPWMCalculate(const FOC_AlphaBetaVoltage *input, FOC_SVPWMResult *result)
{
    SVPWM_Work work;
    SVPWM_Work *svpwm = &work;
    float sum;
    float k_svpwm;
    float Udc = 24.0f;
    svpwm->Ts = 100.0f;
    float K = 0.8660254f / Udc * svpwm->Ts;

    svpwm->U_alpha = input->alpha;
    svpwm->U_beta = input->beta;
    svpwm->u1 = input->beta;
    svpwm->u2 = 0.8660254f * input->alpha - 0.5f * input->beta;
    svpwm->u3 = -0.8660254f * input->alpha - 0.5f * input->beta;
    svpwm->X = K * svpwm->u1;
    svpwm->Y = K * svpwm->u3;
    svpwm->Z = K * svpwm->u2;
    svpwm->sector = (svpwm->u1 > 0.0f) + ((svpwm->u2 > 0.0f) << 1) + ((svpwm->u3 > 0.0f) << 2);
    result->sector = svpwm->sector;

    switch (svpwm->sector) {
    case 5:
        svpwm->t4 = svpwm->X;
        svpwm->t6 = svpwm->Y;
        sum = svpwm->X + svpwm->Y;
        if (sum > svpwm->Ts) {
            k_svpwm = svpwm->Ts / sum;
            svpwm->t4 = k_svpwm * svpwm->t4;
            svpwm->t6 = k_svpwm * svpwm->t6;
        }
        svpwm->ta = (svpwm->Ts + svpwm->t4 + svpwm->t6) * 0.25f;
        svpwm->tb = (svpwm->Ts - svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tc = (svpwm->Ts + svpwm->t4 - svpwm->t6) * 0.25f;
        break;
    case 1:
        svpwm->t4 = -svpwm->Z;
        svpwm->t6 = -svpwm->Y;
        sum = svpwm->t4 + svpwm->t6;
        if (sum > svpwm->Ts) {
            k_svpwm = svpwm->Ts / sum;
            svpwm->t4 = k_svpwm * svpwm->t4;
            svpwm->t6 = k_svpwm * svpwm->t6;
        }
        svpwm->ta = (svpwm->Ts + svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tb = (svpwm->Ts - svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tc = (svpwm->Ts + svpwm->t4 + svpwm->t6) * 0.25f;
        break;
    case 3:
        svpwm->t4 = svpwm->Z;
        svpwm->t6 = svpwm->X;
        sum = svpwm->t4 + svpwm->t6;
        if (sum > svpwm->Ts) {
            k_svpwm = svpwm->Ts / sum;
            svpwm->t4 = k_svpwm * svpwm->t4;
            svpwm->t6 = k_svpwm * svpwm->t6;
        }
        svpwm->ta = (svpwm->Ts - svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tb = (svpwm->Ts + svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tc = (svpwm->Ts + svpwm->t4 + svpwm->t6) * 0.25f;
        break;
    case 2:
        svpwm->t4 = -svpwm->Y;
        svpwm->t6 = -svpwm->X;
        sum = svpwm->t4 + svpwm->t6;
        if (sum > svpwm->Ts) {
            k_svpwm = svpwm->Ts / sum;
            svpwm->t4 = k_svpwm * svpwm->t4;
            svpwm->t6 = k_svpwm * svpwm->t6;
        }
        svpwm->ta = (svpwm->Ts - svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tb = (svpwm->Ts + svpwm->t4 + svpwm->t6) * 0.25f;
        svpwm->tc = (svpwm->Ts + svpwm->t4 - svpwm->t6) * 0.25f;
        break;
    case 6:
        svpwm->t4 = svpwm->Y;
        svpwm->t6 = svpwm->Z;
        sum = svpwm->t4 + svpwm->t6;
        if (sum > svpwm->Ts) {
            k_svpwm = svpwm->Ts / sum;
            svpwm->t4 = k_svpwm * svpwm->t4;
            svpwm->t6 = k_svpwm * svpwm->t6;
        }
        svpwm->ta = (svpwm->Ts + svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tb = (svpwm->Ts + svpwm->t4 + svpwm->t6) * 0.25f;
        svpwm->tc = (svpwm->Ts - svpwm->t4 - svpwm->t6) * 0.25f;
        break;
    case 4:
        svpwm->t4 = -svpwm->X;
        svpwm->t6 = -svpwm->Z;
        sum = svpwm->t4 + svpwm->t6;
        if (sum > svpwm->Ts) {
            k_svpwm = svpwm->Ts / sum;
            svpwm->t4 = k_svpwm * svpwm->t4;
            svpwm->t6 = k_svpwm * svpwm->t6;
        }
        svpwm->ta = (svpwm->Ts + svpwm->t4 + svpwm->t6) * 0.25f;
        svpwm->tb = (svpwm->Ts + svpwm->t4 - svpwm->t6) * 0.25f;
        svpwm->tc = (svpwm->Ts - svpwm->t4 - svpwm->t6) * 0.25f;
        break;
    default:
        /* Do not read the original indeterminate ta/tb/tc or invent duties. */
        return 0;
    }
    result->ta = svpwm->ta;
    result->tb = svpwm->tb;
    result->tc = svpwm->tc;
    return 1;
}
