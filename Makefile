PIO ?= pio
ENV ?= geiger
UV ?= uv
VENV ?= .venv
PYTHON ?= python3
SPHINXBUILD ?= $(CURDIR)/$(VENV)/bin/sphinx-build
DOCS_STAMP ?= $(VENV)/.docs-installed
WEB_ASSETS ?= src/comm/wifi/web_assets.h

.PHONY: build flash monitor run clean setup docs docs-clean docs-env erase web build-web

all: build

build-web:
	@echo "Building web frontend..."
	@$(PYTHON) tools/embed_web.py

build: build-web
	@$(PIO) run -e $(ENV)

flash: build
	@$(PIO) run -t upload -e $(ENV)

monitor:
	@$(PIO) device monitor -e $(ENV)

run: flash
	@$(PIO) device monitor -e $(ENV)

clean:
	@$(PIO) run -t clean -e $(ENV)
	@rm -f $(WEB_ASSETS)

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
