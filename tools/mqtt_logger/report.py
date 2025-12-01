#!/usr/bin/env python3
"""
MultiGeiger Report Generator

Generate visual reports from logged data.
Shows line charts for important metrics over 24 hours or 7 days.
"""

import sqlite3
import argparse
import sys
from datetime import datetime, timedelta
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
except ImportError:
    print("Error: matplotlib not installed. Install with: uv sync")
    sys.exit(1)


def fetch_metric_data(db_path, metric, hours):
    """Fetch metric data for the last N hours."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Check if table exists
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name=?", (metric,))
    if not cursor.fetchone():
        conn.close()
        return [], []

    # Get data from last N hours
    cutoff_time = datetime.now() - timedelta(hours=hours)

    cursor.execute(f'''
        SELECT timestamp, value
        FROM {metric}
        WHERE timestamp >= ?
        ORDER BY timestamp
    ''', (cutoff_time.isoformat(),))

    rows = cursor.fetchall()
    conn.close()

    if not rows:
        return [], []

    # Parse timestamps and values
    timestamps = [datetime.fromisoformat(row[0]) for row in rows]
    values = [row[1] for row in rows]

    return timestamps, values


def create_report(db_path, hours=24):
    """Create visual report with line charts."""

    # Define metrics to plot: (metric_name, display_name, unit)
    metrics = [
        ('cpm', 'Counts Per Minute', 'CPM'),
        ('dose_rate_uSvph', 'Dose Rate', 'µSv/h'),
        ('temperature', 'Temperature', '°C'),
        ('humidity', 'Humidity', '%'),
        ('pressure', 'Pressure', 'hPa'),
    ]

    # Create figure with subplots
    fig, axes = plt.subplots(len(metrics), 1, figsize=(12, 10), sharex=True)
    fig.suptitle(f'MultiGeiger Report - Last {hours} Hours', fontsize=16, fontweight='bold')

    # Plot each metric
    for idx, (metric, display_name, unit) in enumerate(metrics):
        ax = axes[idx]

        timestamps, values = fetch_metric_data(db_path, metric, hours)

        if timestamps and values:
            ax.plot(timestamps, values, linewidth=1.5, color='#2E86AB')
            ax.fill_between(timestamps, values, alpha=0.3, color='#2E86AB')

            # Calculate statistics
            avg_val = sum(values) / len(values)
            min_val = min(values)
            max_val = max(values)

            # Add statistics text
            stats_text = f'Avg: {avg_val:.2f} | Min: {min_val:.2f} | Max: {max_val:.2f}'
            ax.text(0.02, 0.95, stats_text, transform=ax.transAxes,
                   fontsize=9, verticalalignment='top',
                   bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))
        else:
            ax.text(0.5, 0.5, 'No data available',
                   horizontalalignment='center', verticalalignment='center',
                   transform=ax.transAxes, fontsize=12, color='gray')

        # Formatting
        ax.set_ylabel(f'{display_name}\n({unit})', fontsize=10, fontweight='bold')
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.set_facecolor('#F7F7F7')

    # Format x-axis
    axes[-1].xaxis.set_major_formatter(mdates.DateFormatter('%d.%m\n%H:%M'))
    axes[-1].set_xlabel('Time', fontsize=11, fontweight='bold')

    # Rotate x-axis labels for better readability
    plt.setp(axes[-1].xaxis.get_majorticklabels(), rotation=0, ha='center')

    # Adjust layout
    plt.tight_layout()

    return fig


def main():
    parser = argparse.ArgumentParser(description='Generate MultiGeiger visual report')
    parser.add_argument('--db', default='multigeiger_data.db',
                       help='Database file (default: multigeiger_data.db)')
    parser.add_argument('--hours', type=int, default=24,
                       help='Hours to look back (default: 24)')
    parser.add_argument('--days', type=int,
                       help='Days to look back (shortcut for --hours)')
    parser.add_argument('--save', metavar='FILE',
                       help='Save plot to file instead of showing')

    args = parser.parse_args()

    # Convert days to hours if specified
    hours = args.days * 24 if args.days else args.hours

    db_path = Path(args.db)
    if not db_path.exists():
        print(f"✗ Database not found: {db_path}")
        sys.exit(1)

    print(f"Generating report for last {hours} hours...")

    # Create report
    fig = create_report(db_path, hours)

    # Save or show
    if args.save:
        fig.savefig(args.save, dpi=150, bbox_inches='tight')
        print(f"✓ Report saved to: {args.save}")
    else:
        print("✓ Showing report (close window to exit)")
        plt.show()


if __name__ == '__main__':
    main()
