#!/usr/bin/env python3
"""
Generate LoRaWAN ABP credentials (DevAddr, NwkSKey, AppSKey) for MultiGeiger.

ABP (Activation By Personalization) is required for single-channel gateways
like Dragino LG01-N which cannot handle OTAA downlinks reliably.

WARNING: It's recommended to let The Things Network (TTN) generate these
credentials to ensure uniqueness and security. Use this script only if you
understand the implications.

Usage:
    python generate_lora_credentials.py
    python generate_lora_credentials.py --devaddr 26011D01
"""

import argparse
import secrets
import sys


def generate_devaddr() -> str:
    """
    Generate a random DevAddr (Device Address) for ABP.

    DevAddr format: 4 bytes (8 hex chars)
    For TTN, use prefix 0x26 or 0x27 (TTN address space)

    Returns:
        DevAddr as 8-character hex string
    """
    # Generate random 3 bytes and prepend 0x26 for TTN
    random_bytes = secrets.token_bytes(3)
    devaddr = "26" + random_bytes.hex().upper()
    return devaddr


def generate_nwkskey() -> str:
    """
    Generate a cryptographically secure random Network Session Key (NwkSKey).

    NwkSKey format: 16 bytes (32 hex chars)

    Returns:
        NwkSKey as 32-character hex string
    """
    return secrets.token_hex(16).upper()


def generate_appskey() -> str:
    """
    Generate a cryptographically secure random Application Session Key (AppSKey).

    AppSKey format: 16 bytes (32 hex chars)

    Returns:
        AppSKey as 32-character hex string
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
        description="Generate LoRaWAN ABP credentials for MultiGeiger",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate random ABP credentials
  python generate_lora_credentials.py

  # Use specific DevAddr
  python generate_lora_credentials.py --devaddr 26011D01

WARNING: It's strongly recommended to let TTN generate ABP credentials!
ABP is required for single-channel gateways like Dragino LG01-N.
        """
    )

    parser.add_argument(
        "--devaddr",
        metavar="ADDR",
        help="Use specific DevAddr (format: 26011D01, must start with 0x26 or 0x27 for TTN)"
    )

    args = parser.parse_args()

    print("=" * 70)
    print("LoRaWAN ABP Credentials Generator for MultiGeiger")
    print("=" * 70)
    print()

    # Generate DevAddr
    if args.devaddr:
        devaddr = args.devaddr.replace(" ", "").upper()
        if len(devaddr) != 8:
            print(f"❌ Error: DevAddr must be 8 hex characters (got {len(devaddr)})", file=sys.stderr)
            sys.exit(1)
        if not devaddr.startswith(("26", "27")):
            print("⚠️  WARNING: DevAddr should start with 0x26 or 0x27 for TTN!", file=sys.stderr)
        print(f"ℹ️  Using provided DevAddr: {devaddr}")
    else:
        devaddr = generate_devaddr()
        print("🎲 Generated random DevAddr (TTN address space: 0x26)")

    # Generate NwkSKey and AppSKey
    nwkskey = generate_nwkskey()
    appskey = generate_appskey()

    print()
    print("=" * 70)
    print("Generated ABP Credentials")
    print("=" * 70)
    print()

    # TTN format (with spaces, for manual entry in TTN console)
    print("📋 For TTN Console (ABP Mode - copy these values):")
    print("-" * 70)
    print(f"Device Address (DevAddr):  {format_with_spaces(devaddr)}")
    print(f"Network Session Key (NwkSKey):  {format_with_spaces(nwkskey)}")
    print(f"App Session Key (AppSKey):      {format_with_spaces(appskey)}")
    print()

    # MultiGeiger format (no spaces, for config page)
    print("🔧 For MultiGeiger Web Config (paste these values):")
    print("-" * 70)
    print(f"DevAddr:  {devaddr}")
    print(f"NwkSKey:  {nwkskey}")
    print(f"AppSKey:  {appskey}")
    print()

    print("=" * 70)
    print("⚠️  Important Notes:")
    print("=" * 70)
    print("1. You must manually register these ABP credentials in TTN console")
    print("2. In TTN, select 'Activation mode: Activation by personalization (ABP)'")
    print("3. LoRaWAN version: MAC V1.0.2 or V1.0.3")
    print("4. Regional Parameters: Europe 863-870 MHz (SF9 for RX2)")
    print("5. Paste the values from 'For TTN Console' above")
    print("6. Then paste 'For MultiGeiger Web Config' values into device settings")
    print("7. Enable 'Send to LoRa' in MultiGeiger config")
    print()
    print("🔧 Single-Channel Gateway Configuration:")
    print("   - Frequency: 868.1 MHz (Channel 0)")
    print("   - Spreading Factor: SF7")
    print("   - ABP ensures immediate connectivity without OTAA join")
    print()
    print("🚨 RECOMMENDED: Let TTN generate ABP credentials instead!")
    print("   This ensures uniqueness and proper security.")
    print()


if __name__ == "__main__":
    main()
