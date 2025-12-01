#!/usr/bin/env python3
"""
Query and analyze MultiGeiger SQLite data.

Simple utility to view and analyze logged MultiGeiger data.
"""

import sqlite3
import argparse
import sys
from datetime import datetime, timedelta
from pathlib import Path


def print_stats(db_path):
    """Print database statistics."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print("\n" + "=" * 60)
    print("Database Statistics")
    print("=" * 60)

    # Count records
    cursor.execute("SELECT COUNT(*) FROM measurements")
    measurement_count = cursor.fetchone()[0]
    print(f"Total measurements: {measurement_count}")

    cursor.execute("SELECT COUNT(*) FROM environmental")
    env_count = cursor.fetchone()[0]
    print(f"Environmental records: {env_count}")

    cursor.execute("SELECT COUNT(*) FROM status")
    status_count = cursor.fetchone()[0]
    print(f"Status records: {status_count}")

    # Time range
    cursor.execute("SELECT MIN(timestamp), MAX(timestamp) FROM measurements")
    min_time, max_time = cursor.fetchone()
    if min_time:
        print(f"\nFirst record: {min_time}")
        print(f"Last record:  {max_time}")

        # Calculate duration
        start = datetime.fromisoformat(min_time)
        end = datetime.fromisoformat(max_time)
        duration = end - start
        print(f"Duration: {duration}")

    conn.close()


def print_latest(db_path, limit=10):
    """Print latest measurements."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print("\n" + "=" * 60)
    print(f"Latest {limit} Measurements")
    print("=" * 60)

    cursor.execute(f'''
        SELECT measurements.timestamp, cpm, dose_rate_uSvph, temperature
        FROM measurements
        LEFT JOIN environmental ON measurements.id = environmental.id
        ORDER BY measurements.timestamp DESC
        LIMIT {limit}
    ''')

    rows = cursor.fetchall()
    if rows:
        print(f"{'Timestamp':<20} {'CPM':>8} {'µSv/h':>10} {'Temp °C':>10}")
        print("-" * 60)
        for row in rows:
            timestamp, cpm, dose, temp = row
            cpm_str = f"{cpm}" if cpm else "N/A"
            dose_str = f"{dose:.3f}" if dose else "N/A"
            temp_str = f"{temp:.1f}" if temp else "N/A"
            print(f"{timestamp:<20} {cpm_str:>8} {dose_str:>10} {temp_str:>10}")
    else:
        print("No data available")

    conn.close()


def print_summary(db_path):
    """Print statistical summary."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print("\n" + "=" * 60)
    print("Statistical Summary")
    print("=" * 60)

    # Dose rate statistics
    cursor.execute('''
        SELECT
            AVG(dose_rate_uSvph),
            MIN(dose_rate_uSvph),
            MAX(dose_rate_uSvph),
            COUNT(dose_rate_uSvph)
        FROM measurements
        WHERE dose_rate_uSvph IS NOT NULL
    ''')
    avg_dose, min_dose, max_dose, count = cursor.fetchone()

    if count > 0:
        print(f"\nDose Rate (µSv/h):")
        print(f"  Average: {avg_dose:.3f}")
        print(f"  Minimum: {min_dose:.3f}")
        print(f"  Maximum: {max_dose:.3f}")
        print(f"  Samples: {count}")

    # CPM statistics
    cursor.execute('''
        SELECT
            AVG(cpm),
            MIN(cpm),
            MAX(cpm),
            COUNT(cpm)
        FROM measurements
        WHERE cpm IS NOT NULL
    ''')
    avg_cpm, min_cpm, max_cpm, count = cursor.fetchone()

    if count > 0:
        print(f"\nCounts Per Minute (CPM):")
        print(f"  Average: {avg_cpm:.1f}")
        print(f"  Minimum: {min_cpm}")
        print(f"  Maximum: {max_cpm}")
        print(f"  Samples: {count}")

    # Environmental statistics
    cursor.execute('''
        SELECT
            AVG(temperature),
            AVG(humidity),
            AVG(pressure),
            COUNT(temperature)
        FROM environmental
        WHERE temperature IS NOT NULL
    ''')
    avg_temp, avg_hum, avg_press, count = cursor.fetchone()

    if count > 0:
        print(f"\nEnvironmental Sensors:")
        print(f"  Avg Temperature: {avg_temp:.1f} °C")
        print(f"  Avg Humidity:    {avg_hum:.1f} %")
        print(f"  Avg Pressure:    {avg_press:.1f} hPa")
        print(f"  Samples: {count}")

    conn.close()


def export_csv(db_path, output_file):
    """Export data to CSV."""
    import csv

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute('''
        SELECT
            m.timestamp,
            m.cpm,
            m.dose_rate_uSvph,
            m.counts,
            m.hv_pulses,
            e.temperature,
            e.humidity,
            e.pressure
        FROM measurements m
        LEFT JOIN environmental e ON m.id = e.id
        ORDER BY m.timestamp
    ''')

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'cpm', 'dose_rate_uSvh', 'counts', 'hv_pulses',
                        'temperature', 'humidity', 'pressure'])
        writer.writerows(cursor.fetchall())

    conn.close()
    print(f"\n✓ Data exported to: {output_file}")


def main():
    parser = argparse.ArgumentParser(description='Query MultiGeiger SQLite data')
    parser.add_argument('--db', default='multigeiger_data.db',
                       help='Database file (default: multigeiger_data.db)')
    parser.add_argument('--stats', action='store_true',
                       help='Show database statistics')
    parser.add_argument('--latest', type=int, metavar='N',
                       help='Show N latest measurements')
    parser.add_argument('--summary', action='store_true',
                       help='Show statistical summary')
    parser.add_argument('--export', metavar='FILE',
                       help='Export data to CSV file')

    args = parser.parse_args()

    db_path = Path(args.db)
    if not db_path.exists():
        print(f"✗ Database not found: {db_path}")
        sys.exit(1)

    # If no action specified, show stats and latest
    if not any([args.stats, args.latest, args.summary, args.export]):
        print_stats(db_path)
        print_latest(db_path, limit=10)
        print_summary(db_path)
    else:
        if args.stats:
            print_stats(db_path)
        if args.latest:
            print_latest(db_path, limit=args.latest)
        if args.summary:
            print_summary(db_path)
        if args.export:
            export_csv(db_path, args.export)

    print("\n" + "=" * 60 + "\n")


if __name__ == '__main__':
    main()
