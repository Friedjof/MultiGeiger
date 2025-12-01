#!/usr/bin/env python3
"""
Query and analyze MultiGeiger SQLite data.

Simple utility to view logged MultiGeiger data.
"""

import sqlite3
import argparse
import sys
from pathlib import Path


def list_tables(db_path):
    """List all tables and their record counts."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print("\n" + "=" * 60)
    print("Database Tables")
    print("=" * 60)

    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")
    tables = cursor.fetchall()

    for (table,) in tables:
        cursor.execute(f"SELECT COUNT(*) FROM {table}")
        count = cursor.fetchone()[0]
        print(f"{table:<25} {count:>10} records")

    conn.close()


def show_metric(db_path, metric, limit=10):
    """Show latest values for a specific metric."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Check if table exists
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name=?", (metric,))
    if not cursor.fetchone():
        print(f"\n✗ Table '{metric}' not found")
        conn.close()
        return

    print("\n" + "=" * 60)
    print(f"Latest {limit} values: {metric}")
    print("=" * 60)

    cursor.execute(f'''
        SELECT timestamp, value
        FROM {metric}
        ORDER BY timestamp DESC
        LIMIT {limit}
    ''')

    rows = cursor.fetchall()
    if rows:
        print(f"{'Timestamp':<25} {'Value':>15}")
        print("-" * 60)
        for timestamp, value in rows:
            print(f"{timestamp:<25} {value:>15}")
    else:
        print("No data available")

    conn.close()


def show_latest_all(db_path):
    """Show latest value from each metric table."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print("\n" + "=" * 60)
    print("Latest Values (All Metrics)")
    print("=" * 60)

    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")
    tables = cursor.fetchall()

    print(f"{'Metric':<25} {'Timestamp':<25} {'Value':>10}")
    print("-" * 60)

    for (table,) in tables:
        cursor.execute(f'''
            SELECT timestamp, value
            FROM {table}
            ORDER BY timestamp DESC
            LIMIT 1
        ''')
        row = cursor.fetchone()
        if row:
            timestamp, value = row
            print(f"{table:<25} {timestamp:<25} {str(value):>10}")

    conn.close()


def export_metric_csv(db_path, metric, output_file):
    """Export single metric to CSV."""
    import csv

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Check if table exists
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name=?", (metric,))
    if not cursor.fetchone():
        print(f"\n✗ Table '{metric}' not found")
        conn.close()
        return

    cursor.execute(f'''
        SELECT timestamp, value
        FROM {metric}
        ORDER BY timestamp
    ''')

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'value'])
        writer.writerows(cursor.fetchall())

    conn.close()
    print(f"\n✓ {metric} exported to: {output_file}")


def main():
    parser = argparse.ArgumentParser(description='Query MultiGeiger SQLite data')
    parser.add_argument('--db', default='multigeiger_data.db',
                       help='Database file (default: multigeiger_data.db)')
    parser.add_argument('--tables', action='store_true',
                       help='List all tables and record counts')
    parser.add_argument('--metric', metavar='NAME',
                       help='Show latest values for specific metric')
    parser.add_argument('--limit', type=int, default=10,
                       help='Number of records to show (default: 10)')
    parser.add_argument('--all', action='store_true',
                       help='Show latest value from all metrics')
    parser.add_argument('--export', metavar='FILE',
                       help='Export metric to CSV (requires --metric)')

    args = parser.parse_args()

    db_path = Path(args.db)
    if not db_path.exists():
        print(f"✗ Database not found: {db_path}")
        sys.exit(1)

    # If no action specified, show tables and latest all
    if not any([args.tables, args.metric, args.all, args.export]):
        list_tables(db_path)
        show_latest_all(db_path)
    else:
        if args.tables:
            list_tables(db_path)
        if args.metric:
            show_metric(db_path, args.metric, args.limit)
        if args.all:
            show_latest_all(db_path)
        if args.export:
            if not args.metric:
                print("✗ --export requires --metric")
                sys.exit(1)
            export_metric_csv(db_path, args.metric, args.export)

    print("\n" + "=" * 60 + "\n")


if __name__ == '__main__':
    main()
