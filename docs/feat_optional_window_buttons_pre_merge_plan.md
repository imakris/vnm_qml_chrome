# Pre-Merge Plan: feat/optional-window-buttons

## Review Decision

The branch should not be merged unchanged because review found one standards
issue in `src/vnm_system_window.cpp`: the STL include `<cmath>` was placed
before the Windows SDK include block. Varinomics include order keeps STL
headers last.

No reviewer found a functional, packaging, test-coverage, or API blocker.

## Fix

1. Move `#include <cmath>` below the `#ifdef Q_OS_WIN` Windows header block in
   `src/vnm_system_window.cpp`.
2. Leave `include/vnm_qml_chrome/vnm_system_window.h` unchanged; its Qt include
   order is already acceptable.

## Validation

1. Run `git diff --check master...HEAD`.
2. Run the configured MSVC build and `ctest --output-on-failure`.
3. Run the configured MinGW build and `ctest --output-on-failure`.

## Merge Condition

Merge to `master` after the include-order fix is present and the validation
commands pass.
