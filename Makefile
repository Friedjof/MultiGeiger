PIO ?= pio
ENV ?= geiger
UV ?= uv
VENV ?= .venv
PYTHON ?= python3
SPHINXBUILD ?= $(CURDIR)/$(VENV)/bin/sphinx-build
DOCS_STAMP ?= $(VENV)/.docs-installed

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
	@echo "Error: version required. Usage: make release v=X.Y.Z or v=vX.Y.Z"
	@exit 1
endif
	$(eval VERSION_CLEAN := $(shell echo "$(v)" | sed 's/^v//'))
	@echo "Creating release v$(VERSION_CLEAN)..."
	@echo "// v$(VERSION_CLEAN)" > VERSION
	@echo "Version file updated to v$(VERSION_CLEAN)"
	@$(MAKE) build-web
	@echo "Preparing release commit with recent changes..."
	@echo "Release v$(VERSION_CLEAN)" > /tmp/release_msg.txt
	@echo "" >> /tmp/release_msg.txt
	@last_tag=$$(git describe --tags --abbrev=0 2>/dev/null || echo ""); \
	if [ -n "$$last_tag" ]; then \
		commit_count=$$(git rev-list $$last_tag..HEAD --count); \
		if [ $$commit_count -eq 0 ]; then \
			echo "No changes since last release ($$last_tag)" >> /tmp/release_msg.txt; \
		else \
			echo "Changes since $$last_tag:" >> /tmp/release_msg.txt; \
			git log $$last_tag..HEAD --format="- %s" -20 >> /tmp/release_msg.txt; \
		fi; \
	else \
		echo "Recent changes:" >> /tmp/release_msg.txt; \
		git log -5 --format="- %s" >> /tmp/release_msg.txt; \
	fi
	@git add VERSION
	@git commit -F /tmp/release_msg.txt
	@rm /tmp/release_msg.txt
	@git tag v$(VERSION_CLEAN)
	@echo ""
	@echo "✅ Release v$(VERSION_CLEAN) created successfully!"
	@echo "📦 Pushing to origin..."
	@git push origin HEAD
	@git push origin v$(VERSION_CLEAN)
	@echo ""
	@echo "🎉 Release v$(VERSION_CLEAN) published!"
	@echo "View at: $$(git remote get-url origin | sed 's/\.git$$//')/releases/tag/v$(VERSION_CLEAN)"
