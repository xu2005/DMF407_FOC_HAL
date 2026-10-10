"""Optional one-time reference execution; does not read new Control modules.

Run from repository root: python tests/host/freeze_pi_svpwm_vectors.py
Normal CTest uses the frozen header, not this program. Only defined, nonzero
SVPWM inputs are passed to Legacy. No hardware or Legacy files are modified.
"""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[2]
legacy_dir = root / "Legacy/Template222_A"


def uncomment(path):
    return re.sub(r"/\*.*?\*/|//[^\n]*", "",
                  path.read_bytes().decode("latin1"), flags=re.S)


legacy = uncomment(legacy_dir / "SYSTEM/ipark.c")


def extract(name):
    match = re.search(r"(?:void|float)\s+" + name + r"\s*\(", legacy)
    opening = legacy.index("{", match.start())
    closing, depth = opening + 1, 1
    while depth:
        depth += (legacy[closing] == "{") - (legacy[closing] == "}")
        closing += 1
    return legacy[match.start():closing]


names = ("id_Ref", "iq_Ref", "id_Kp", "id_Ki", "iq_Kp", "iq_Ki",
         "Speed_Ref", "Speed_Kp", "Speed_Ki", "Speed_Imin", "Speed_Imax",
         "Speed_actual", "current_Umin", "current_Umax", "sum_d", "sum_q",
         "sum_speed", "Position_Limit", "Position_Ref_X", "Position_Kp",
         "Position_Kff", "Position_actual")
globals_source = uncomment(legacy_dir / "USER/globals.c")
declarations = re.findall(r"^\s*(?:float|long)\s+[^;]+;", globals_source, re.M)
definitions = [line for line in declarations
               if any(re.search(r"\b" + name + r"\s*=", line) for name in names)]
functions = "\n".join(extract(name) for name in
                      ("clamp", "foc_pi_position", "foc_pi_speed",
                       "foc_pi_current", "SVPWM"))
source = '#include "ipark.h"\n#include <stdio.h>\n' + "\n".join(definitions)
source += "\nlong id_show=0, iq_show=0;\nfloat tcm1=0, tcm2=0, tcm3=0;\n"
source += functions + r'''
int main(void) {
    if (sizeof(long) != 4) { fputs("Reference needs 32-bit long\n", stderr); return 1; }
    DQ_Def out;
    DQ_currents measured;
    const float speeds[][2] = {{100,0},{100,0},{20000,0},{0,0},
                              {-20000,0},{-20000,0},{0,0},{50,25}};
    sum_speed=0;
    for (unsigned i=0; i<sizeof(speeds)/sizeof(speeds[0]); ++i) {
        Speed_Ref=speeds[i][0]; Speed_actual=speeds[i][1];
        float before=sum_speed, output=foc_pi_speed();
        printf("speed %.9g %.9g %.9g %.9g %.9g\n", Speed_Ref, Speed_actual,
               before, sum_speed, output);
    }
    const float currents[][4] = {{0,0,-2,-3},{0,0,-2,-3},{0,0,-100,-100},
                                {0,0,100,100},{0,0,0,0},{2,4,1,2}};
    sum_d=0; sum_q=0;
    for (unsigned i=0; i<sizeof(currents)/sizeof(currents[0]); ++i) {
        id_Ref=currents[i][0]; iq_Ref=currents[i][1];
        measured.I_d=currents[i][2]; measured.I_q=currents[i][3];
        float before_d=sum_d, before_q=sum_q;
        foc_pi_current(&measured,&out);
        printf("current %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",
               id_Ref, iq_Ref, measured.I_d, measured.I_q, before_d, before_q,
               sum_d, sum_q, out.d, out.q);
    }
    sum_speed=0; sum_d=0; sum_q=0; id_Ref=0; iq_Ref=0;
    Speed_Ref=100; Speed_actual=0; measured.I_d=0; measured.I_q=0;
    for (int tick=1; tick<=20; ++tick) {
        float before_speed=sum_speed, before_d=sum_d, before_q=sum_q;
        if (tick%10==0) foc_pi_speed();
        foc_pi_current(&measured,&out);
        printf("schedule %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",
               before_speed, before_d, before_q, sum_speed, sum_d, sum_q,
               iq_Ref, out.d, out.q);
    }
    const long positions[][3] = {{100,80,3},{200,0,0},{-200,0,0},{0,0,0}};
    for (unsigned i=0; i<sizeof(positions)/sizeof(positions[0]); ++i) {
        Position_Ref_X=positions[i][0]; Position_actual=positions[i][1];
        foc_pi_position(999,positions[i][2]); /* Legacy ignores first argument. */
        printf("position %ld %ld %ld %.9g\n", Position_Ref_X, Position_actual,
               positions[i][2], Speed_Ref);
    }
    const float voltages[][2] = {{0,2},{2,-1},{2,1},{-2,-1},{-2,1},{0,-2},
                                {0,48},{48,-24},{48,24},{-48,-24},{-48,24},
                                {0,-48},{2,0},{-2,0}};
    for (unsigned i=0; i<sizeof(voltages)/sizeof(voltages[0]); ++i) {
        AlphaBeta_Def input={voltages[i][0],voltages[i][1]};
        SVPWM_Def result; /* All compared fields are assigned on sectors 1-6. */
        SVPWM(&input,&result);
        if (result.sector<1 || result.sector>6) return 2;
        printf("svpwm %.9g %.9g %d %.9g %.9g %.9g\n", input.alpha, input.beta,
               result.sector, result.ta, result.tb, result.tc);
    }
    return 0;
}
'''
build = root / "build/host/reference"
build.mkdir(parents=True, exist_ok=True)
reference = build / "legacy_pi_svpwm_vectors.c"
reference.write_text(source, encoding="ascii")
executable = build / "legacy_pi_svpwm_vectors.exe"
subprocess.run(["gcc", "-std=c11", "-O0", "-I", str(legacy_dir / "USER"),
                str(reference), "-o", str(executable)], check=True)
output = subprocess.check_output([str(executable)], text=True)
layouts = {
    "speed": ("SpeedVector", "ref, actual, before, after, output", ()),
    "current": ("CurrentVector", "id_ref, iq_ref, id_actual, iq_actual, before_d, before_q, after_d, after_q, d, q", ()),
    "schedule": ("ScheduleVector", "before_speed, before_d, before_q, after_speed, after_d, after_q, iq_ref, d, q", ()),
    "position": ("PositionVector", "ref, actual, feed, output", (0, 1, 2)),
    "svpwm": ("SVPWMVector", "alpha, beta, sector, ta, tb, tc", (2,)),
}
header = ["/* Frozen from active Legacy SYSTEM/ipark.c; never from new Control/. */",
          "#ifndef PI_SVPWM_VECTORS_H", "#define PI_SVPWM_VECTORS_H"]
for group, (typename, fields, integers) in layouts.items():
    header.append("typedef struct {")
    for index, field in enumerate(fields.split(", ")):
        header.append(f"    {'int' if index in integers else 'float'} {field};")
    header.append(f"}} {typename};")
    header.append(f"static const {typename} {group}_vectors[] = {{")
    for line in output.splitlines():
        tokens = line.split()
        if tokens[0] != group:
            continue
        values = [token if i in integers else
                  token + ("f" if "." in token or "e" in token.lower() else ".0f")
                  for i, token in enumerate(tokens[1:])]
        header.append("    {" + ", ".join(values) + "},")
    header.append("};")
header.append("#endif\n")
(root / "tests/host/pi_svpwm_vectors.h").write_text("\n".join(header), encoding="ascii")
print("Frozen: speed 8, current 6, schedule 20, position 4, nonzero SVPWM 14; no zero Legacy execution.")
