#!/usr/bin/env python3
"""
Generate LoRaWAN credentials (DEVEUI, APPEUI, APPKEY) for MultiGeiger.

WARNING: It's recommended to let The Things Network (TTN) generate these
credentials to ensure uniqueness and security. Use this script only if you
understand the implications.

Usage:
    python generate_lora_credentials.py
    python generate_lora_credentials.py --deveui-from-mac AA:BB:CC:DD:EE:FF
"""

import argparse
import secrets
import sys


def generate_deveui_from_mac(mac_address: str) -> str:
    """
    Generate DEVEUI from MAC address.

    Format: Converts MAC (6 bytes) to EUI-64 (8 bytes) by inserting FFFE in the middle.
    Example: AA:BB:CC:DD:EE:FF -> AABBCCFFFEFDDEEFF

    Args:
        mac_address: MAC address in format AA:BB:CC:DD:EE:FF

    Returns:
        DEVEUI as 16-character hex string (no spaces)
    """
    # Remove colons and convert to uppercase
    mac = mac_address.replace(":", "").replace("-", "").upper()

    if len(mac) != 12:
        raise ValueError(f"Invalid MAC address: {mac_address}")

    # EUI-64: Insert FFFE in the middle of MAC
    # AA BB CC DD EE FF -> AA BB CC FF FE DD EE FF
    deveui = mac[:6] + "FFFE" + mac[6:]

    return deveui


def generate_random_deveui() -> str:
    """
    Generate a random DEVEUI (8 bytes = 16 hex chars).

    WARNING: This may collide with existing devices!

    Returns:
        DEVEUI as 16-character hex string
    """
    return secrets.token_hex(8).upper()


def generate_appeui() -> str:
    """
    Generate APPEUI (JoinEUI).

    For private applications, it's common to use all zeros.
    TTN v3 allows this.

    Returns:
        APPEUI as 16-character hex string (all zeros)
    """
    return "0000000000000000"


def generate_appkey() -> str:
    """
    Generate a cryptographically secure random APPKEY (16 bytes = 32 hex chars).

    Returns:
        APPKEY as 32-character hex string
    """
    return secrets.token_hex(16).upper()


def format_with_spaces(hex_string: str) -> str:
    """
    Format hex string with spaces every 2 characters (for readability).

    Example: AABBCCDD -> AA BB CC DD
    """
    return " ".join(hex_string[i:i+2] for i in range(0, len(hex_string), 2))


def main():
    parser = argparse.ArgumentParser(
        description="Generate LoRaWAN credentials for MultiGeiger",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate random credentials
  python generate_lora_credentials.py

  # Generate DEVEUI from ESP32 MAC address
  python generate_lora_credentials.py --deveui-from-mac AA:BB:CC:DD:EE:FF

WARNING: It's strongly recommended to let TTN generate these credentials!
        """
    )

    parser.add_argument(
        "--deveui-from-mac",
        metavar="MAC",
        help="Generate DEVEUI from MAC address (format: AA:BB:CC:DD:EE:FF)"
    )

    parser.add_argument(
        "--random-deveui",
        action="store_true",
        help="Generate completely random DEVEUI (WARNING: collision risk!)"
    )

    args = parser.parse_args()

    print("=" * 70)
    print("LoRaWAN Credentials Generator for MultiGeiger")
    print("=" * 70)
    print()

    # Generate DEVEUI
    if args.deveui_from_mac:
        try:
            deveui = generate_deveui_from_mac(args.deveui_from_mac)
            print(f"ℹ️  DEVEUI derived from MAC: {args.deveui_from_mac}")
        except ValueError as e:
            print(f"❌ Error: {e}", file=sys.stderr)
            sys.exit(1)
    elif args.random_deveui:
        deveui = generate_random_deveui()
        print("⚠️  DEVEUI randomly generated (collision risk!)")
    else:
        deveui = generate_random_deveui()
        print("⚠️  DEVEUI randomly generated (collision risk!)")

    # Generate APPEUI and APPKEY
    appeui = generate_appeui()
    appkey = generate_appkey()

    print()
    print("=" * 70)
    print("Generated Credentials")
    print("=" * 70)
    print()

    # TTN format (with spaces, for manual entry in TTN console)
    print("📋 For TTN Console (copy these values):")
    print("-" * 70)
    print(f"Device EUI (DEVEUI):  {format_with_spaces(deveui)}")
    print(f"App EUI (APPEUI):     {format_with_spaces(appeui)}")
    print(f"App Key (APPKEY):     {format_with_spaces(appkey)}")
    print()

    # MultiGeiger format (no spaces, for config page)
    print("🔧 For MultiGeiger Config Page (paste these values):")
    print("-" * 70)
    print(f"DEVEUI:  {deveui}")
    print(f"APPEUI:  {appeui}")
    print(f"APPKEY:  {appkey}")
    print()

    print("=" * 70)
    print("⚠️  Important Notes:")
    print("=" * 70)
    print("1. You must manually register these credentials in TTN console")
    print("2. In TTN, select 'Enter end device specifics manually'")
    print("3. Choose: LoRaWAN 1.0.2, Europe 863-870 MHz")
    print("4. Paste the values from 'For TTN Console' above")
    print("5. Then paste 'For MultiGeiger Config Page' values into your device")
    print()
    print("🚨 RECOMMENDED: Let TTN generate credentials instead!")
    print("   This ensures uniqueness and proper security.")
    print()


if __name__ == "__main__":
    main()
