#!/usr/bin/env python3
"""
Convert built web assets (Vite dist/) into a C header with gzip-compressed blobs.

Usage:
  python scripts/web_to_header.py web/dist -o lib/WebService/generated/web_files.h
"""

from __future__ import annotations

import argparse
import gzip
import mimetypes
import os
from pathlib import Path
from typing import Iterable, Tuple


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(description="Generate gzip-compressed web header from dist/")
  parser.add_argument(
      "dist_dir",
      type=Path,
      help="Path to Vite dist directory (e.g. web/dist)",
  )
  parser.add_argument(
      "-o",
      "--output",
      type=Path,
      default=Path("lib/WebService/generated/web_files.h"),
      help="Output header path (default: lib/WebService/generated/web_files.h)",
  )
  return parser.parse_args()


def collect_files(root: Path) -> Iterable[Tuple[Path, str]]:
  for file_path in sorted(root.rglob("*")):
    if not file_path.is_file():
      continue
    if file_path.suffix == ".map":
      continue  # skip sourcemaps
    rel = "/" + str(file_path.relative_to(root)).replace(os.sep, "/")
    yield file_path, rel


def sanitize_name(rel_path: str) -> str:
  safe = (
      rel_path.strip("/")
      .replace("/", "_")
      .replace(".", "_")
      .replace("-", "_")
  )
  return f"file_{safe}"


def guess_mime(path: Path) -> str:
  mime, _ = mimetypes.guess_type(str(path))
  return mime or "application/octet-stream"


def format_bytes(data: bytes) -> str:
  parts = [f"0x{b:02x}" for b in data]
  lines = []
  for i in range(0, len(parts), 12):
    lines.append("  " + ", ".join(parts[i:i + 12]))
  return ",\n".join(lines)


def generate_header(root: Path, output: Path):
  files = list(collect_files(root))
  if not files:
    raise SystemExit(f"No assets found in {root}")

  output.parent.mkdir(parents=True, exist_ok=True)

  header = [
      "/**",
      " * Auto-generated from built web assets.",
      " * DO NOT EDIT MANUALLY.",
      " */",
      "#pragma once",
      "",
      "#include <Arduino.h>",
      "#include <WebServer.h>",
      "",
      "struct WebFile {",
      "  const char* path;",
      "  const uint8_t* data;",
      "  size_t size;",
      "  const char* mime_type;",
      "};",
      "",
  ]

  table_entries = []
  for path, rel_path in files:
    name = sanitize_name(rel_path)
    mime = guess_mime(path)
    raw = path.read_bytes()
    compressed = gzip.compress(raw)

    header.append(f"// {rel_path}")
    header.append(f"static const uint8_t {name}[] PROGMEM = {{")
    header.append(format_bytes(compressed))
    header.append("};")
    header.append(f"static const size_t {name}_len = {len(compressed)};")
    header.append("")

    table_entries.append(
        f'  {{"{rel_path}", {name}, {name}_len, "{mime}"}}'
    )

  header.append("static const WebFile webFiles[] PROGMEM = {")
  header.append(",\n".join(table_entries))
  header.append("};")
  header.append("static const size_t webFilesCount = sizeof(webFiles) / sizeof(webFiles[0]);")
  header.append("")
  header.append("inline const WebFile* findWebFile(const String& path) {")
  header.append("  for (size_t i = 0; i < webFilesCount; ++i) {")
  header.append("    if (path == webFiles[i].path) return &webFiles[i];")
  header.append("  }")
  header.append("  return nullptr;")
  header.append("}")
  header.append("")
  header.append("inline void sendWebFile(WebServer& server, const WebFile* file) {")
  header.append('  server.sendHeader("Content-Encoding", "gzip");')
  header.append("  server.send_P(200, file->mime_type, reinterpret_cast<const char*>(file->data), file->size);")
  header.append("}")

  output.write_text("\n".join(header), encoding="utf-8")
  print(f"Wrote {output} ({len(files)} files)")


def main():
  args = parse_args()
  generate_header(args.dist_dir, args.output)


if __name__ == "__main__":
  main()
