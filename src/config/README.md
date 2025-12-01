# Config files

- `config.default.hpp` — versioned defaults. Copy this to `config.hpp` and adapt to your hardware.
- `config.hpp` — your local settings (ignored by git). The build includes this file via `#include "config/config.hpp"`.
