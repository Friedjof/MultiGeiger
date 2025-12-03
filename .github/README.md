# GitHub Actions CI/CD

This directory contains GitHub Actions workflows for automated building and testing of the MultiGeiger firmware.

## Workflows

### Build Workflow (`build.yml`)

Automatically builds the firmware on every push and pull request to ensure code quality and prevent build failures.

**Triggers:**
- Push to `master`, `main`, or `develop` branches
- Pull requests to `master`, `main`, or `develop` branches
- Manual workflow dispatch

**Steps:**
1. Checkout repository
2. Set up Python 3.11
3. Cache PlatformIO dependencies
4. Install PlatformIO
5. Build web assets (HTML, CSS, JS)
6. Verify web assets generation
7. Build firmware for `geiger` environment
8. Check firmware size
9. Upload firmware artifacts (`.bin` and `.elf`)
10. Generate build summary

**Artifacts:**
- Firmware binaries are stored for 30 days
- Accessible in the GitHub Actions run summary

## Build Badge

Add this badge to your README.md to show the build status:

```markdown
![Build Status](https://github.com/YOUR_USERNAME/MultiGeiger/actions/workflows/build.yml/badge.svg)
```

Replace `YOUR_USERNAME` with your actual GitHub username.

## Local Development

To ensure your code passes CI before pushing:

```bash
# Build web assets
make web

# Build firmware
make build

# Run full build pipeline (equivalent to CI)
make clean && make build
```

## Troubleshooting

If the CI build fails:

1. Check the workflow logs in the GitHub Actions tab
2. Verify that all web files in `web/` directory are valid
3. Ensure `platformio.ini` is properly configured
4. Test the build locally with `make build`

## Caching

PlatformIO dependencies are cached to speed up builds:
- Cache key: Based on `platformio.ini` hash
- Cache location: `~/.platformio` and `.pio/`
- Cache is automatically invalidated when dependencies change
