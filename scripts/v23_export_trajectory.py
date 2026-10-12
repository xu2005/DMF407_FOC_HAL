"""Run the real host C simulator; validate CSV and optionally plot with matplotlib.

Only Python stdlib is mandatory. No real waiting or hardware access. Optional
--tick-hz is a chosen simulation display scale, never a measured TIM4 clock.
"""
import argparse
import csv
import json
import math
from pathlib import Path
import subprocess

SCENARIOS = ("demo", "mixed", "drop", "reject-x", "reject-y", "lag", "encoder", "long", "zero", "cancel")


def export(executable, output, tick_hz=None, plot=False):
    output.mkdir(parents=True, exist_ok=True)
    summaries = []
    for scenario in SCENARIOS:
        destination = output / (scenario + ".csv")
        result = subprocess.run([str(executable), scenario, str(destination)], check=True,
                                capture_output=True, text=True)
        with destination.open(newline="", encoding="utf-8") as stream:
            rows = list(csv.DictReader(stream))
        previous_ticks = 0
        previous_confirmed = (0, 0)
        for sequence, row in enumerate(rows, 1):
            ticks, period = int(row["virtual_ticks"]), int(row["period"])
            assert int(row["event_id"]) == sequence and period > 0
            assert ticks - previous_ticks == period
            x, y = int(row["planned_x"]), int(row["planned_y"])
            direction = int(row["direction"])
            assert direction in (-1, 1)
            expected = (previous_confirmed[0] + direction, previous_confirmed[1]) if row["axis"] == "X" else (previous_confirmed[0], previous_confirmed[1] + direction)
            assert (x, y) == expected
            confirmed = int(row["confirmed_x"]), int(row["confirmed_y"])
            assert confirmed == ((x, y) if row["accepted"] == "1" else previous_confirmed)
            for axis in ("x", "y"):
                assert math.isclose(float(row["error_" + axis]), int(row["planned_" + axis]) - float(row["feedback_" + axis]), abs_tol=1e-7)
                assert math.isclose(float(row["confirmed_error_" + axis]), int(row["confirmed_" + axis]) - float(row["feedback_" + axis]), abs_tol=1e-7)
            previous_confirmed, previous_ticks = confirmed, ticks
        last = rows[-1] if rows else {}
        error = [float(last.get("confirmed_error_" + a, 0)) for a in ("x", "y")]
        target_error = [int(last.get("target_" + a, 0)) - float(last.get("feedback_" + a, 0)) for a in ("x", "y")]
        summary = dict(scenario=scenario, events=len(rows), accepted=sum(r["accepted"] == "1" for r in rows),
                       virtual_ticks=previous_ticks, time_unit="virtual_ticks",
                       final_state=last.get("state", "COMPLETE"),
                       final_plant=[int(last.get("plant_" + a, 0)) for a in ("x", "y")],
                       final_feedback=[float(last.get("feedback_" + a, 0)) for a in ("x", "y")],
                       final_confirmed_error_steps=error,
                       final_target_error_steps=target_error,
                       endpoint_error_steps=math.hypot(*target_error),
                       max_abs_planned_error_steps=[max((abs(float(r["error_" + a])) for r in rows), default=0) for a in ("x", "y")],
                       max_abs_confirmed_error_steps=[max((abs(float(r["confirmed_error_" + a])) for r in rows), default=0) for a in ("x", "y")],
                       simulator_stdout=result.stdout.strip())
        if tick_hz is not None:
            summary.update(display_tick_hz=tick_hz, theoretical_seconds=previous_ticks/tick_hz)
        if scenario in ("demo", "mixed", "long", "zero"):
            assert error == [0, 0] and summary["final_state"] == "COMPLETE" and target_error == [0, 0]
        if scenario == "demo":
            assert len(rows) == 8192 and summary["final_plant"] == [8192, 0]
        if scenario == "long":
            assert len(rows) == 12000
        if scenario == "drop":
            assert error == [1, 0] and summary["accepted"] == len(rows)
        if scenario.startswith("reject"):
            assert summary["events"] == 3 and summary["accepted"] == 2 and summary["final_state"] == "FAULT"
        if scenario == "lag":
            assert error != [0, 0] and summary["final_plant"] == [40, 15]
        if scenario == "cancel":
            assert summary["events"] == 8 and summary["final_state"] == "CANCELLED"
        summaries.append(summary)
        print(result.stdout.strip())
        if plot and rows:
            make_plot(rows, output / (scenario + ".png"), tick_hz)
    (output / "summary.json").write_text(json.dumps(summaries, indent=2) + "\n", encoding="utf-8")
    return summaries


def make_plot(rows, destination, tick_hz):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    ticks = [int(r["virtual_ticks"]) for r in rows]
    times = [t/tick_hz for t in ticks] if tick_hz else ticks
    fig, axes = plt.subplots(1, 3, figsize=(15, 4), constrained_layout=True)
    for prefix, style in (("planned", "-"), ("confirmed", "--"), ("feedback", ":")):
        axes[0].plot([float(r[prefix + "_x"]) for r in rows], [float(r[prefix + "_y"]) for r in rows], style, label=prefix)
    axes[0].set(xlabel="X (steps)", ylabel="Y (steps)", title="Ordered single-axis commands")
    for a in ("x", "y"):
        axes[1].plot(times, [float(r["planned_" + a]) for r in rows], label="planned " + a)
        axes[1].plot(times, [float(r["feedback_" + a]) for r in rows], "--", label="feedback " + a)
        axes[2].plot(times, [float(r["confirmed_error_" + a]) for r in rows], label="confirmed-feedback " + a)
    time_label = "Theoretical seconds (chosen tick_hz)" if tick_hz else "Virtual ticks"
    axes[1].set(xlabel=time_label, ylabel="Position (steps)")
    axes[2].set(xlabel=time_label, ylabel="Error (steps)")
    for ax in axes:
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=8)
    fig.savefig(destination, dpi=150)
    plt.close(fig)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=Path("build/v23-trajectory"))
    parser.add_argument("--tick-hz", type=float)
    parser.add_argument("--plot", action="store_true", help="optional matplotlib dependency")
    args = parser.parse_args()
    if args.tick_hz is not None and (not math.isfinite(args.tick_hz) or args.tick_hz <= 0):
        parser.error("tick-hz must be finite and positive")
    if args.plot:
        try:
            import matplotlib  # noqa: F401
        except ImportError:
            print("Optional matplotlib missing: CSV and summaries will still be exported.")
            args.plot = False
    export(args.exe.resolve(), args.out.resolve(), args.tick_hz, args.plot)
