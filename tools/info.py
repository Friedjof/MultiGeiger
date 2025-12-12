#!/usr/bin/env python3
"""Print a quick project status snapshot for MultiGeiger."""

from __future__ import annotations

import re
import subprocess
from pathlib import Path
from typing import Iterable, List, Optional, Tuple


ROOT = Path(__file__).resolve().parent.parent


def run(cmd: List[str]) -> Optional[str]:
    """Run a command and return stdout (stripped) or None on error."""
    try:
        result = subprocess.run(
            cmd, cwd=ROOT, check=False, capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            return result.stdout.strip()
    except Exception:
        return None
    return None


def read_version_file() -> Optional[str]:
    path = ROOT / "VERSION"
    if not path.exists():
        return None
    return path.read_text().strip()


def read_core_version() -> Optional[str]:
    path = ROOT / "src" / "core" / "core.hpp"
    if not path.exists():
        return None
    match = re.search(r'#define\s+VERSION_STR\s+"([^"]+)"', path.read_text())
    return match.group(1) if match else None


def git_branch() -> Optional[str]:
    return run(["git", "rev-parse", "--abbrev-ref", "HEAD"])


def git_head() -> Optional[str]:
    return run(["git", "log", "-1", "--format=%h %cs %s"])


def git_status_summary() -> str:
    out = run(["git", "status", "--porcelain"])
    if out is None:
        return "git unavailable"
    lines = [line for line in out.splitlines() if line.strip()]
    if not lines:
        return "clean"

    staged = sum(1 for line in lines if not line.startswith("??") and not line.startswith(" "))
    unstaged = sum(1 for line in lines if line.startswith(" ") and not line.startswith("??"))
    untracked = sum(1 for line in lines if line.startswith("??"))

    parts = []
    if staged:
        parts.append(f"{staged} staged")
    if unstaged:
        parts.append(f"{unstaged} unstaged")
    if untracked:
        parts.append(f"{untracked} untracked")
    return "dirty (" + ", ".join(parts) + ")"


def git_tags(limit: int = 5) -> List[Tuple[str, Optional[str]]]:
    out = run(["git", "tag", "--sort=-creatordate"])
    if out is None:
        return []
    tags = [line.strip() for line in out.splitlines() if line.strip()][:limit]
    result: List[Tuple[str, Optional[str]]] = []
    for tag in tags:
        date = run(["git", "log", "-1", "--format=%cs", tag])
        result.append((tag, date))
    return result


def commits_since_last_tag() -> Optional[Tuple[str, str]]:
    last_tag = run(["git", "describe", "--tags", "--abbrev=0"])
    if not last_tag:
        return None
    count = run(["git", "rev-list", f"{last_tag}..HEAD", "--count"])
    if count is None:
        return None
    return last_tag, count


def default_env() -> Optional[str]:
    makefile = ROOT / "Makefile"
    if not makefile.exists():
        return None
    match = re.search(r"^ENV\s*\?=\s*(\S+)", makefile.read_text(), re.MULTILINE)
    return match.group(1) if match else None


def pio_envs() -> List[str]:
    ini = ROOT / "platformio.ini"
    if not ini.exists():
        return []
    return re.findall(r"^\[env:([^\]]+)\]", ini.read_text(), re.MULTILINE)


def tool_version(cmd: List[str]) -> str:
    out = run(cmd)
    if not out:
        return "not available"
    return out.splitlines()[0]


def artifact_present(path: Path) -> str:
    return "present" if path.exists() else "absent"


def latest_changelog_heading() -> Optional[str]:
    path = ROOT / "docs" / "source" / "changes.rst"
    if not path.exists():
        return None
    for line in path.read_text().splitlines():
        if line.strip().startswith("V"):
            return line.strip()
    return None


def print_section(title: str, lines: Iterable[str]) -> None:
    print(title)
    for line in lines:
        print(f"  {line}")
    print()


def main() -> None:
    version_file = read_version_file()
    core_version = read_core_version()
    tags = git_tags()
    last_tag_info = commits_since_last_tag()
    changelog_head = latest_changelog_heading()
    env_default = default_env()
    pio_env_list = pio_envs()

    print("MultiGeiger info\n-----------------")

    version_lines = []
    version_lines.append(f"VERSION file: {version_file or 'missing'}")
    version_lines.append(f"core.hpp VERSION_STR: {core_version or 'missing'}")
    if version_file and core_version:
        version_lines.append("Version match: " + ("yes" if version_file.replace("//", "").strip() == core_version else "no"))
    if tags:
        tag_lines = ", ".join(f"{t} ({d or 'n/a'})" for t, d in tags)
        version_lines.append(f"Recent tags: {tag_lines}")
    if changelog_head:
        version_lines.append(f"Changelog head: {changelog_head}")
    print_section("Versions", version_lines)

    repo_lines = []
    branch = git_branch()
    if branch:
        repo_lines.append(f"Branch: {branch}")
    head = git_head()
    if head:
        repo_lines.append(f"HEAD: {head}")
    repo_lines.append(f"Status: {git_status_summary()}")
    if last_tag_info:
        repo_lines.append(f"Since last tag {last_tag_info[0]}: {last_tag_info[1]} commits")
    print_section("Repo", repo_lines)

    build_lines = []
    build_lines.append(f"Default ENV: {env_default or 'n/a'}")
    build_lines.append(f"PIO envs: {', '.join(pio_env_list) if pio_env_list else 'none'}")
    build_lines.append(f"pio version: {tool_version(['pio', '--version'])}")
    build_lines.append(f"python3 version: {tool_version(['python3', '--version'])}")
    build_lines.append(f"node version: {tool_version(['node', '--version'])}")
    build_lines.append(f"npm version: {tool_version(['npm', '--version'])}")
    print_section("Build/Tools", build_lines)

    artifact_lines = []
    artifact_lines.append(f"web/dist: {artifact_present(ROOT / 'web' / 'dist')}")
    artifact_lines.append(f"docs/build/html: {artifact_present(ROOT / 'docs' / 'build' / 'html')}")
    print_section("Artifacts", artifact_lines)


if __name__ == "__main__":
    main()
