"""Validate only V2.2 Host/ARM builds; never overwrite V2.1 evidence."""
import csv
import datetime
import json
import pathlib
import shutil
import subprocess
import xml.etree.ElementTree as ET

root = pathlib.Path(__file__).resolve().parents[1]
evidence = root / "build/v22-evidence"
evidence.mkdir(parents=True, exist_ok=True)
commit = subprocess.check_output(["git", "log", "-1", "--format=%H"], cwd=root, text=True).strip()
records = []
cmake = shutil.which("cmake")
ctest = shutil.which("ctest")
compiler = shutil.which("gcc")
assert cmake and ctest and compiler

def run(name, args):
    log = evidence / (name + ".log")
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    with log.open("wb") as output:
        result = subprocess.run(args, cwd=root, stdout=output, stderr=subprocess.STDOUT)
    records.append(dict(name=name, argv=[str(x) for x in args], commit=commit,
                        started_utc=started, exit_code=result.returncode, log=log.name))
    (evidence / "commands.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    print(f"{name}: exit{result.returncode}", flush=True)
    if result.returncode:
        print("\n".join(log.read_text(encoding="utf-8", errors="replace").splitlines()[-25:]))
        raise SystemExit(result.returncode)

rows = []
for mode in ("Debug", "Release"):
    directory = root / ("build/v22-host-" + mode.lower())
    run(f"host-{mode}-configure", [cmake, "-S", "tests/host", "-B", str(directory), "-G", "Ninja",
                                 f"-DCMAKE_BUILD_TYPE={mode}", f"-DCMAKE_C_COMPILER={compiler}"])
    run(f"host-{mode}-build", [cmake, "--build", str(directory), "--parallel", "4"])
    xml = evidence / (f"host-{mode}-results.xml")
    run(f"host-{mode}-test", [ctest, "--test-dir", str(directory), "--output-on-failure", "--output-junit", str(xml)])
    cases = ET.parse(xml).getroot().findall("testcase")
    for case in cases:
        rows.append(dict(build=mode, name=case.attrib["name"], status=case.attrib.get("status"),
                         seconds=case.attrib.get("time")))
    print(f"Host {mode}: {len(cases)} tests", flush=True)
for mode in ("Debug", "Release"):
    for control in ("OFF", "ON"):
        directory = root / f"build/v22-arm-{mode.lower()}-{control.lower()}"
        run(f"arm-{mode}-{control}-configure", [cmake, "-S", ".", "-B", str(directory), "-G", "Ninja",
                                               f"-DCMAKE_BUILD_TYPE={mode}", f"-DDMF407_NO_POWER_CONTROL={control}",
                                               "-DDMF407_DWT_DIAGNOSTICS=OFF"])
        run(f"arm-{mode}-{control}-build", [cmake, "--build", str(directory), "--parallel", "4"])
with (evidence / "test-results.csv").open("w", newline="", encoding="utf-8") as output:
    writer = csv.DictWriter(output, fieldnames=("build", "name", "status", "seconds"))
    writer.writeheader()
    writer.writerows(rows)
print(f"Validation evidence: {evidence}")
