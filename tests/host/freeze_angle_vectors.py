"""Freeze independent expectations from the Legacy function, never from Motor/.

Optional reference derivation, run at repository root with Python and host gcc:
  python tests/host/freeze_angle_vectors.py
The normal CMake/CTest run uses the checked-in vectors and needs no Python.
"""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[2]
# Byte-preserving decode: comments may have mixed encodings; C tokens are ASCII.
legacy = (root / "Legacy/Template222_A/USER/angle.c").read_bytes().decode("latin1")
legacy = re.sub(r"/\*.*?\*/|//[^\n]*", "", legacy, flags=re.S)
start = legacy.index("void calculate_motor_parameters(")
opening = legacy.index("{", start)
depth = 1
closing = opening + 1
while depth:
    depth += (legacy[closing] == "{") - (legacy[closing] == "}")
    closing += 1
function = legacy[start:closing]
# Only rename and expose the original local result; arithmetic stays untouched.
function = function.replace("calculate_motor_parameters", "legacy_calculate")
function = function.replace("*electrical_angle = mechanical_angle * POLE_PAIRS;",
                            "reference_mechanical = mechanical_angle;\n"
                            "*electrical_angle = mechanical_angle * POLE_PAIRS;")
config = (root / "Legacy/Template222_A/USER/drv_config.h").read_bytes().decode("latin1")
definitions = []
for name in ("ENCODER_RESOLUTION", "PI", "POLE_PAIRS"):
    definitions.append(re.search(r"^#define\s+" + name + r"\s+[^\r\n/]+", config, re.M).group())

cases = [
    ("zero", 0, 0, 0),
    ("positive", 0, 400, 400),
    ("reverse", 400, 399, 399),
    ("sample_1", 0, 100, 100),
    ("sample_2", 100, 101, 101),
    ("upper_wrap", 65535, 0, 65536),
    ("lower_wrap", 0, 65535, -1),
    ("half_positive_limit", 0, 32767, 32767),
    ("half_positive_wrap", 0, 32768, 32768),
    ("half_negative_limit", 32768, 0, 0),
    ("r01_upper_bad_total", 65535, 0, 0),
    ("r01_lower_bad_total", 0, 65535, 65535),
]
calls = []
for name, prev, raw, total in cases:
    calls.append(f'Previous_Count={prev}; reference_raw={raw}; reference_total={total}; '
                 f'legacy_calculate(&angle, &speed); '
                 f'printf("{name} %.9g %.9g %.9g\\n", reference_mechanical, angle, speed);')
source = """#include <stdint.h>
#include <stdio.h>
#include <math.h>
typedef uint32_t u32;
volatile int32_t Previous_Count;
float mechanical_angle_offset = 0.546637118;
int g_timx_encode_count;
uint32_t reference_raw;
int32_t reference_total;
float reference_mechanical;
#define TIM3 0
uint32_t TIM_GetCounter(int ignored) { (void)ignored; return reference_raw; }
int32_t gtim_get_encode(void) { return reference_total; }
""" + "\n".join(definitions) + "\n" + function + """
int main(void) {
 if (sizeof(long) != 4) { fputs("Reference requires ARM-like 32-bit long\\n", stderr); return 1; }
 float angle, speed;
""" + "\n".join(calls) + "\nreturn 0;\n}\n"
build = root / "build/host/reference"
build.mkdir(parents=True, exist_ok=True)
(build / "legacy_angle_vectors.c").write_text(source, encoding="utf-8")
executable = build / "legacy_angle_vectors.exe"
subprocess.run(["gcc", "-std=c11", "-O0", str(build / "legacy_angle_vectors.c"),
                "-o", str(executable), "-lm"], check=True)
rows = subprocess.check_output([str(executable)], text=True).splitlines()
header = ["/* Frozen from unchanged Legacy angle.c by freeze_angle_vectors.py.",
          " * MinGW GCC, 32-bit long; DUT is not consulted. */",
          "static const struct angle_vector {", "    const char *name;",
          "    int32_t previous; uint32_t raw; int32_t total;",
          "    float mechanical, electrical, rpm;", "} angle_vectors[] = {"]
for case, line in zip(cases, rows):
    name, prev, raw, total = case
    fields = line.split()
    assert fields[0] == name
    values = ", ".join(repr(float(x)) + "f" for x in fields[1:])
    header.append(f'    {{"{name}", {prev}, {raw}U, {total}, {values}}},')
assert len(rows) == len(cases)
header.append("};\n")
(root / "tests/host/math_angle_vectors.h").write_text("\n".join(header), encoding="utf-8")
print("\n".join(rows))
