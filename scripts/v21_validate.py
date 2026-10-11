"""Execute V2.1 host/ARM matrix and retain real commands, output and exit codes.

Run from any directory with Python 3 and CMake/Ninja/GCC/ARM GCC on PATH.
No hardware access. Build artifacts stay under ignored build/.
"""
import csv
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Docs/审核证据/v21_20261011"
OUT.mkdir(parents=True, exist_ok=True)
SHA = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
records = []

def run(name, args):
    started = datetime.now(timezone.utc).isoformat()
    result = subprocess.run(args, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (OUT / (name + ".log")).write_bytes(result.stdout)
    records.append(dict(name=name, argv=args, commit=SHA, started_utc=started,
                        exit_code=result.returncode, log=name + ".log"))
    (OUT / "commands.json").write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"{name}: exit {result.returncode}", flush=True)
    if result.returncode:
        print(result.stdout.decode("utf-8", errors="replace")[-6000:])
        raise SystemExit(result.returncode)
    return result.stdout

tools = {}
for tool in ["cmake", "ninja", "gcc", "arm-none-eabi-gcc", "arm-none-eabi-size", "arm-none-eabi-nm", "ctest"]:
    resolved = shutil.which(tool)
    if not resolved:
        raise SystemExit(f"BLOCKED: missing {tool} on PATH")
    tools[tool] = resolved
    run("version-" + tool, [resolved, "--version"])
run("python-version", [sys.executable, "--version"])
(OUT / "tested-source-commit.txt").write_text(SHA + "\n", encoding="utf-8")
tracked = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT).decode("utf-8").split("\0")
source_paths = [ROOT / p for p in tracked if p and not p.startswith("Docs/")]
source_paths += list((ROOT / "Drivers").rglob("*.h")) + list((ROOT / "Drivers").rglob("*.c"))
with (OUT / "source-sha256.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream); writer.writerow(["path", "sha256"])
    for path in sorted(set(source_paths)):
        if path.is_file():
            writer.writerow([path.relative_to(ROOT).as_posix(), hashlib.sha256(path.read_bytes()).hexdigest()])
baseline_log = ROOT / "Docs/审核证据/audit_20261011/final-ctest.log"
baseline_names = set(re.findall(r"Test\s+#\d+:\s+(\S+)", baseline_log.read_text(encoding="utf-8")))
if len(baseline_names) != 70:
    raise SystemExit(f"BLOCKED: baseline has {len(baseline_names)}, expected 70")
test_rows = []
artifacts = []
for config in ["Debug", "Release"]:
    tag = "host-" + config.lower()
    build = "build/v21-final-" + tag
    run(tag + "-configure", [tools["cmake"], "-S", "tests/host", "-B", build, "-G", "Ninja",
                              "-DCMAKE_BUILD_TYPE=" + config, "-DCMAKE_C_COMPILER=" + tools["gcc"]])
    run(tag + "-build", [tools["cmake"], "--build", build, "--parallel"])
    listed = run(tag + "-test-list", [tools["ctest"], "--test-dir", build, "--show-only=json-v1"])
    tests = json.loads(listed)["tests"]
    names = {test["name"] for test in tests}
    if not baseline_names.issubset(names):
        raise SystemExit("BLOCKED: removed baseline tests: " + str(baseline_names - names))
    run(tag + "-ctest", [tools["ctest"], "--test-dir", build, "-V", "--output-junit", "results.xml"])
    shutil.copyfile(ROOT / build / "Testing/Temporary/LastTest.log", OUT / (tag + "-raw.log"))
    shutil.copyfile(ROOT / build / "results.xml", OUT / (tag + "-results.xml"))
    for test in tests:
        labels = next((p["value"] for p in test["properties"] if p["name"] == "LABELS"), [])
        test_rows.append([config, test["name"], "baseline" if test["name"] in baseline_names else "V2.1",
                          "KNOWN_RISK" if "KNOWN_RISK" in labels else "PASS"])
for config, mode, dwt in [(c,m,"OFF") for c in ["Debug","Release"] for m in ["OFF","ON"]] + [("Debug","ON","ON")]:
    tag = "arm-" + config.lower() + "-" + mode.lower() + ("-dwt" if dwt == "ON" else "")
    build = "build/v21-final-" + tag
    run(tag + "-configure", [tools["cmake"], "-S", ".", "-B", build, "-G", "Ninja",
                              "-DCMAKE_BUILD_TYPE=" + config, "-DDMF407_NO_POWER_CONTROL=" + mode,
                              "-DDMF407_DWT_DIAGNOSTICS=" + dwt])
    run(tag + "-build", [tools["cmake"], "--build", build, "--parallel"])
    run(tag + "-size", [tools["arm-none-eabi-size"], build + "/DMF407_FOC_HAL.elf"])
    run(tag + "-symbols", [tools["arm-none-eabi-nm"], build + "/DMF407_FOC_HAL.elf"])
    for suffix in ["elf", "map", "bin", "hex"]:
        path = ROOT / build / ("DMF407_FOC_HAL." + suffix)
        artifacts.append([tag, path.relative_to(ROOT).as_posix(), path.stat().st_size,
                          hashlib.sha256(path.read_bytes()).hexdigest()])
for filename, header, rows in [("test-results.csv", ["config","test","origin","status"], test_rows),
                               ("artifact-sha256.csv", ["matrix","path","bytes","sha256"], artifacts)]:
    with (OUT / filename).open("w", newline="", encoding="utf-8") as stream:
        writer=csv.writer(stream); writer.writerow(header); writer.writerows(rows)
print("PASS host Debug/Release + 4 ARM OFF/ON builds + optional DWT build. L4 NEEDS HARDWARE; L5 BLOCKED.")
