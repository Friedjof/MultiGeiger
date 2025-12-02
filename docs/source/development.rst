.. include:: global.rst.inc
.. highlight:: bash
.. _development:

Development
===========

This chapter will get you started with |project_name| development.

|project_name| is written in C using Arduino-ESP32.


Contributions
-------------

... are welcome!

Some guidance for contributors:

- discuss about changes on github issue tracker

- make your PRs against the ``master`` branch

- do clean changesets:

  - focus on some topic, resist changing anything else.
  - do not do style changes mixed with functional changes.
  - run the automatic code formatter before committing
  - try to avoid refactorings mixed with functional changes.
  - if you need to fix something after commit/push:

    - if there are ongoing reviews: do a fixup commit you can
      merge into the bad commit later.
    - if there are no ongoing reviews or you did not push the
      bad commit yet: edit the commit to include your fix or
      merge the fixup commit before pushing.
  - have a nice, clear, typo-free commit comment
  - if you fixed an issue, refer to it in your commit comment

- make a pull request on github and check on the PR page
  what the CI system tells about the code in your PR

- wait for review by other developers


Building a development environment
----------------------------------

Requirements:

- Python 3, PlatformIO CLI (``pio``) and ``make`` in your ``PATH``.
- A serial connection to the Heltec board you want to flash.
- ``uv`` installed for Python env/dependency management (https://github.com/astral-sh/uv#installation). ``make setup`` uses it to provision ``.venv`` with Sphinx.

Quick start (default PlatformIO environment: ``geiger``):

.. code-block:: bash

   make setup    # copy src/config/config.default.hpp -> src/config/config.hpp (if missing) AND install docs deps into .venv via uv
   make build    # compile
   make flash    # upload firmware
   make monitor  # 115200 Baud serial console

Adjust ``src/config/config.hpp`` for your hardware (tube type, targets to send to, display/sound/LED toggles, alarm thresholds). LoRa hardware is auto-detected at boot; DIP switches are latched once and combine with your build-time flags.

Project layout
--------------

- ``src/app``: ``MultiGeigerController`` entry point (setup + loop coordination).
- ``src/core``: logging, data logging helpers, UTC/clock helpers, ``VERSION_STR``.
- ``src/config``: versioned defaults (``config.default.hpp``) and your local ``config.hpp``.
- ``src/drivers``: hardware abstraction (IO + DIP switches + speaker/LED ticks, GM tube ISR + HV handling + THP sensors, OLED status/values, clock, board HAL).
- ``src/comm``: WiFi config portal + HTTP uploads (sensor.community, madavi, custom), LoRa/TTN glue, BLE Heart-Rate notifications.
- ``docs``: Sphinx sources (English master, translations via Transifex).

Automatic Code Formatter
------------------------

We use astyle_ for automated code formatting / formatting checks.

Run it like this:

::

  astyle --options=.astylerc 'src/\*'

.. _astyle: http://astyle.sourceforge.net/


Documentation
-------------

Building the docs with Sphinx
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Documentation is written in English and translated to other languages from that source (initially German).

The documentation (in reStructuredText format, .rst) is in ``docs/source/``,
``index.rst`` is the starting point there.


To build the docs, you need to have Sphinx_ installed and run:

::

  make docs   # uses uv-provisioned .venv/.docs-installed stamp

Then point a web browser at ``docs/build/html/index.html``.

To clean doc build artifacts: ``make docs-clean``.

The website is updated automatically by ReadTheDocs through GitHub web hooks on the
main repository.

After changes of the (english) master docs, the translation master files (``*.pot``)
need updating (adding/removing/updating strings in there):

::

  cd docs/build/gettext
  sphinx-build -b gettext ../../source .

Then, these changes need to get pushed to transifex, so translators can comfortably
translate on the web:

Translation is organised via [transifex](https://www.transifex.com/thomaswaldmann/multigeiger/), 
you need to have an account or at least login there and fire a "join team" request. 
Then translate the missing parts and notify the developers (e.g. via issue tracker).

::

  tx push --source

Later, after translators did their part, updated translations need to get pulled
from transifex:

::

  tx pull --all --force

Now we have changes in our git workdir and we need to commit them:

::

  git add docs/locales/
  git commit -m "updated translations"
  git push

This will trigger a build of the docs and their translation(s) on readthedocs.io.

.. _Sphinx: https://www.sphinx-doc.org/


Flashing devices / creating binaries
------------------------------------

PlatformIO:

- Build: ``make build`` (or ``pio run -e geiger``)
- Flash over USB: ``make flash``
- Serial monitor: ``make monitor``
- OTA: open the device config page (``/config``) and upload ``.pio/build/geiger/firmware.bin`` produced by the build step.


.. _releasing:

Creating a new release
----------------------

Checklist:

- make sure all issues for this milestone are closed or moved to the
  next milestone
- check if there are any pending fixes for severe issues
- check whether some CA certificate (see ``ca_certs.h``) will expire soon and
  whether we already can add their next valid cert.
- find and fix any low hanging fruit left on the issue tracker
- close release milestone on Github
- update ``docs/source/changes.rst``, based on ``git log $PREVIOUS_RELEASE..``
- ``bump2version --new-version 1.23.0 release`` - this will:

  - update versions everywhere
  - auto-create a git tag
  - auto-create a git commit
- review the automatically generated changeset
- create a github release for this tag:

  - create a binary (see above) and attach to the github release
  - add a link to the relevant ``changes.rst`` section to the github release

- ``bump2version --no-tag --current-version 1.23.0 minor`` - this will:

  - update versions everywhere (now to: 1.24.0-dev)
  - not quite correctly update changes.rst, will need manual fixing afterwards
  - after fixing: git commit --amend
