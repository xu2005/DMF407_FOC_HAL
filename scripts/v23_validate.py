"""One final Host Debug/Release regression with concise, reproducible evidence.
ARM matrix is intentionally skipped: root firmware link graph is unchanged.
"""
import datetime
import json
from pathlib import Path
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
evidence = root / "build/v23-evidence"
evidence.mkdir(parents=True, exist_ok=True)
commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
records = []


def run(name, args):
    log = evidence / (name + ".log")
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    with log.open("wb") as stream:
        result = subprocess.run([str(a) for a in args], cwd=root, stdout=stream, stderr=subprocess.STDOUT)
    records.append(dict(name=name, argv=[str(a) for a in args], commit=commit,
                        started_utc=started, exit_code=result.returncode, log=log.name))
    (evidence / "commands.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    print(f"{name}: exit {result.returncode}", flush=True)
    if result.returncode:
        print(log.read_text(encoding="utf-8", errors="replace")[-4000:])
        raise SystemExit(result.returncode)


cmake, ctest, compiler = (shutil.which(t) for t in ("cmake", "ctest", "gcc"))
if not all((cmake, ctest, compiler)):
    raise SystemExit("missing cmake/ctest/gcc")
baseline = root / "Docs/审核证据/v22_20261011/host-Debug-results.xml"
baseline_names = {case.attrib["name"] for case in ET.parse(baseline).getroot().findall("testcase")}
assert len(baseline_names) == 104
for mode in ("Debug", "Release"):
    build = root / ("build/v23-host-" + mode.lower())
    run(mode + "-configure", [cmake, "-S", "tests/host", "-B", build, "-G", "Ninja",
                              "-DCMAKE_BUILD_TYPE=" + mode, "-DCMAKE_C_COMPILER=" + compiler])
    run(mode + "-build", [cmake, "--build", build, "--parallel", "4"])
    listed = json.loads(subprocess.check_output([ctest, "--test-dir", str(build), "--show-only=json-v1"], text=True))
    assert baseline_names.issubset({t["name"] for t in listed["tests"]})
    def labeled(label):
        return sum(any(p["name"] == "LABELS" and label in p["value"] for p in t["properties"]) for t in listed["tests"])
    assert labeled("KNOWN_RISK") == 3 and labeled("V22_MOTION") == 18 and labeled("V23_SIM") == 14
    run(mode + "-test", [ctest, "--test-dir", build, "--output-on-failure", "--output-junit", evidence / (mode + "-results.xml")])
    cases = ET.parse(evidence / (mode + "-results.xml")).getroot().findall("testcase")
    print(f"Host {mode}: {len(cases)}/{len(listed['tests'])}; baseline 104 retained, V23_SIM 14, KNOWN_RISK 3", flush=True)
    exe = build / ("motion_v23_demo.exe" if sys.platform == "win32" else "motion_v23_demo")
    run(mode + "-trajectory", [sys.executable, "scripts/v23_export_trajectory.py", "--exe", exe,
                               "--out", root / ("build/v23-trajectory-" + mode.lower()), "--plot"])
debug = json.loads((root / "build/v23-trajectory-debug/summary.json").read_text())
release = json.loads((root / "build/v23-trajectory-release/summary.json").read_text())
assert debug == release, "Debug and Release simulation summaries differ"
for scenario in debug:
    name = scenario["scenario"] + ".csv"
    assert (root / "build/v23-trajectory-debug" / name).read_bytes() == (root / "build/v23-trajectory-release" / name).read_bytes()
(evidence / "summary.json").write_text(json.dumps(dict(tested_source_commit=commit,
    host_tests_per_mode=118, new_v23_tests=14, known_risks=3, simulations=debug,
    debug_release_csv_identical=True, arm="NOT RUN: no firmware source/link/configuration changes",
    L4="NEEDS HARDWARE", L5="POWER-STAGE ENABLE BLOCKED"), indent=2) + "\n", encoding="utf-8")
print("PASS: Host matrix, 10 scenarios, identical Debug/Release CSV; L4 NEEDS HARDWARE; L5 BLOCKED")
