#!/usr/bin/env python3
"""
Inject version from VERSION file into src/core/core.hpp
"""
import re
from pathlib import Path

def read_version():
    """Read version from VERSION file"""
    version_file = Path(__file__).parent.parent / 'VERSION'
    try:
        content = version_file.read_text().strip()
        # Remove leading // and whitespace
        version = re.sub(r'^\/+\s*', '', content)
        return version if version else 'dev'
    except FileNotFoundError:
        print("Warning: VERSION file not found, using 'dev'")
        return 'dev'

def inject_version(version):
    """Inject version into core.hpp"""
    core_hpp = Path(__file__).parent.parent / 'src' / 'core' / 'core.hpp'

    if not core_hpp.exists():
        print(f"Error: {core_hpp} not found")
        return False

    content = core_hpp.read_text()

    # Replace VERSION_STR definition
    pattern = r'#define VERSION_STR\s+"[^"]*"'
    replacement = f'#define VERSION_STR "{version}"'

    new_content, count = re.subn(pattern, replacement, content)

    if count == 0:
        print(f"Warning: VERSION_STR not found in {core_hpp}")
        return False

    core_hpp.write_text(new_content)
    print(f"✓ Updated VERSION_STR to '{version}' in {core_hpp}")
    return True

if __name__ == '__main__':
    version = read_version()
    print(f"Version from VERSION file: {version}")
    inject_version(version)
