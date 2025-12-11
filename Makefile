PIO ?= pio
ENV ?= geiger
UV ?= uv
VENV ?= .venv
PYTHON ?= python3
SPHINXBUILD ?= $(CURDIR)/$(VENV)/bin/sphinx-build
DOCS_STAMP ?= $(VENV)/.docs-installed
WEB_ASSETS ?= lib/WebService/generated/web_files.h

.PHONY: build flash monitor run clean setup docs docs-clean docs-env erase web build-web release

all: build

build-web:
	@echo "Building web frontend (Vite -> header)..."
	@cd web && npm install
	@cd web && npm run build
	@$(PYTHON) scripts/web_to_header.py web/dist -o lib/WebService/generated/web_files.h

web: build-web

build: build-web
	@echo "Injecting version from VERSION file..."
	@$(PYTHON) scripts/inject_version.py
	@$(PIO) run -e $(ENV)

flash: build
	@$(PIO) run -t upload -e $(ENV)

monitor:
	@$(PIO) device monitor -e $(ENV)

run: flash
	@$(PIO) device monitor -e $(ENV)

clean:
	@$(PIO) run -t clean -e $(ENV)

setup:
	@test -f src/config/config.hpp || cp src/config/config.default.hpp src/config/config.hpp
	@$(MAKE) docs-env

erase:
	@$(PIO) run -t erase -e $(ENV)

docs: docs-env
	@$(MAKE) -C docs html SPHINXBUILD="$(SPHINXBUILD)"

docs-clean: docs-env
	@$(MAKE) -C docs clean SPHINXBUILD="$(SPHINXBUILD)"
	@rm -f $(DOCS_STAMP)

docs-env: $(DOCS_STAMP)

$(VENV)/bin/python:
	@command -v $(UV) >/dev/null || { echo "uv not found: install from https://github.com/astral-sh/uv#installation" >&2; exit 1; }
	@$(UV) venv $(VENV)

$(DOCS_STAMP): docs/requirements.txt $(VENV)/bin/python
	@command -v $(UV) >/dev/null || { echo "uv not found: install from https://github.com/astral-sh/uv#installation" >&2; exit 1; }
	@$(UV) pip install --python $(VENV)/bin/python -r docs/requirements.txt
	@touch $(DOCS_STAMP)

release:
ifndef v
	@echo "Error: version required. Usage: make release v=X.Y.Z"
	@exit 1
endif
	@echo "Creating release v$(v)..."
	@echo "// v$(v)" > VERSION
	@echo "Version file updated to v$(v)"
	@$(MAKE) build-web
	@git add VERSION $(WEB_ASSETS)
	@git commit -m "Release v$(v)"
	@git tag v$(v)
	@echo "Release v$(v) created successfully!"
	@echo "To push: git push && git push --tags"
