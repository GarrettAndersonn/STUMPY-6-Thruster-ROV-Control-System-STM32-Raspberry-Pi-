import sys
from pathlib import Path
from datetime import datetime
import pandas as pd
import matplotlib.pyplot as plt


def find_latest_csv(logs_root: Path):
    if not logs_root.exists():
        return None
    csv_files = sorted(logs_root.rglob("*.csv"), key=lambda p: p.stat().st_mtime, reverse=True)
    return csv_files[0] if csv_files else None


def to_numeric(df, columns):
    for col in columns:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
    return df


def prepare_plots_folder(script_dir: Path):
    date_folder = datetime.now().strftime("%Y-%m-%d")
    plots_dir = script_dir / "plots" / date_folder
    plots_dir.mkdir(parents=True, exist_ok=True)
    return plots_dir


def save_plot(fig_name: str, plots_dir: Path):
    timestamp = datetime.now().strftime("%H%M%S")
    filename = plots_dir / f"{fig_name}_{timestamp}.png"
    plt.savefig(filename)
    print(f"Saved plot: {filename}")
    plt.close()


def main():
    script_dir = Path(__file__).resolve().parent
    logs_root = script_dir / "logs"

    if len(sys.argv) > 1:
        csv_path = Path(sys.argv[1]).expanduser().resolve()
    else:
        csv_path = find_latest_csv(logs_root)

    if csv_path is None or not csv_path.exists():
        print("No telemetry CSV found.")
        return

    print(f"Using CSV: {csv_path}")

    plots_dir = prepare_plots_folder(script_dir)

    df = pd.read_csv(csv_path)

    numeric_columns = [
        "SEQ", "ARM",
        "T1", "T2", "T3", "T4", "T5", "T6",
        "O1", "O2", "O3", "O4", "O5", "O6",
        "IMU", "WHO",
        "AX", "AY", "AZ",
        "GX", "GY", "GZ",
        "TP", "R", "P",
        "BX", "BY", "BZ",
    ]

    df = to_numeric(df, numeric_columns)

    if "SEQ" in df.columns and df["SEQ"].notna().any():
        x = df["SEQ"]
        x_label = "Telemetry Sequence"
    else:
        x = range(len(df))
        x_label = "Sample Index"

    # Roll
    if "R" in df.columns:
        plt.figure(figsize=(10, 5))
        plt.plot(x, df["R"], label="Roll")
        plt.xlabel(x_label)
        plt.ylabel("Degrees")
        plt.title("Roll Over Time")
        plt.legend()
        plt.tight_layout()
        save_plot("roll", plots_dir)

    # Pitch
    if "P" in df.columns:
        plt.figure(figsize=(10, 5))
        plt.plot(x, df["P"], label="Pitch")
        plt.xlabel(x_label)
        plt.ylabel("Degrees")
        plt.title("Pitch Over Time")
        plt.legend()
        plt.tight_layout()
        save_plot("pitch", plots_dir)

    # Thruster outputs
    thruster_cols = ["O1", "O2", "O3", "O4", "O5", "O6"]
    available_thrusters = [c for c in thruster_cols if c in df.columns]

    if available_thrusters:
        plt.figure(figsize=(12, 6))
        for col in available_thrusters:
            plt.plot(x, df[col], label=col)
        plt.xlabel(x_label)
        plt.ylabel("PWM Output")
        plt.title("Thruster Outputs")
        plt.legend()
        plt.tight_layout()
        save_plot("thrusters", plots_dir)

    # Gyro drift
    gyro_cols = ["GX", "GY", "GZ"]
    available_gyro = [c for c in gyro_cols if c in df.columns]

    if available_gyro:
        plt.figure(figsize=(12, 6))
        for col in available_gyro:
            plt.plot(x, df[col], label=col)
        plt.xlabel(x_label)
        plt.ylabel("deg/s")
        plt.title("Gyro Drift / Motion")
        plt.legend()
        plt.tight_layout()
        save_plot("gyro", plots_dir)

    # Accelerometer
    accel_cols = ["AX", "AY", "AZ"]
    available_accel = [c for c in accel_cols if c in df.columns]

    if available_accel:
        plt.figure(figsize=(12, 6))
        for col in available_accel:
            plt.plot(x, df[col], label=col)
        plt.xlabel(x_label)
        plt.ylabel("mg")
        plt.title("Accelerometer Data")
        plt.legend()
        plt.tight_layout()
        save_plot("accel", plots_dir)

    # Bias
    bias_cols = ["BX", "BY", "BZ"]
    available_bias = [c for c in bias_cols if c in df.columns]

    if available_bias:
        plt.figure(figsize=(12, 6))
        for col in available_bias:
            plt.plot(x, df[col], label=col)
        plt.xlabel(x_label)
        plt.ylabel("deg/s")
        plt.title("Gyro Bias")
        plt.legend()
        plt.tight_layout()
        save_plot("bias", plots_dir)

    print("\nAll plots saved successfully.")


if __name__ == "__main__":
    main()