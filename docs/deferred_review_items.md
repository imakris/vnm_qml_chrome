# Deferred Review Items

This file tracks valid review ideas that are intentionally not part of the
current corrective pass. Items here are not current blockers; they should be
revisited when planning an API, build-system, documentation, or consolidation
batch.

## Build System and QML Module

- Migrate from the manual `qmldir`/`.qrc`/runtime bootstrap list to
  `qt_add_qml_module()` when the project is ready for a build-system batch.
- Generate module metadata such as `qmldir` and `qmltypes` instead of
  maintaining parallel hand-written lists.
- Evaluate declarative C++ QML registration with `QML_ELEMENT`,
  `QML_NAMED_ELEMENT`, and `QML_SINGLETON` as part of that migration.
- Add `qmllint` and cached QML compilation targets once the module is managed by
  Qt's QML CMake machinery.

## High-Level Window Component

- Consider adding `VNM_ChromeWindow.qml` so consumers do not have to manually
  assemble the example's frameless `Window`, native-frame controller, fallback
  frame, content inset, resize layers, titlebar, and system move/resize signal
  forwarding.
- If this component is added, centralize window-state policy there: fullscreen
  behavior, maximize/restore handling, native-frame visibility, resize
  enablement, and theme propagation.
- Keep the existing lower-level primitives only if they remain useful as stable
  advanced API.

## Public API Shape

- Decide which QML files are stable public API, which are internal helpers, and
  which are example-specific conveniences.
- Revisit whether `VNM_ChromeResizeArea`, `VNM_ChromeWindowButton`,
  `VNM_ChromeSideResizeLayer`, and `VNM_ChromeBottomResizeLayer` should remain
  public or become implementation details behind a higher-level component.
- Revisit whether `VNM_DarkModeTitleButton`, `VNM_LanguageTitleButton`, and
  `VNM_AnimatedMark` belong in the core chrome module or in an example/brand
  layer.
- Consider namespacing the remaining public C++ API consistently under
  `vnm_qml_chrome` if compatibility constraints allow it.

## Titlebar API and Rendering

- Resolve `VNM_ChromeTitleBar.active`: either remove it from the public API or
  implement deliberate inactive styling. Do not add opacity dimming unless that
  becomes the chosen contract; existing tests intentionally reject child opacity
  dimming.
- Consider replacing the three repeated `Canvas` window-control icons with a
  single reusable icon component backed by `Shape`, small SVG resources, or one
  internal paint implementation.
- Preserve the synchronous move/resize contract when changing titlebar input
  handling.

## Consolidation

- Extract shared system-move threshold handling from `VNM_AnimatedMark.qml` and
  `VNM_ChromeTitleBar.qml` into an internal component or helper while preserving
  synchronous signal emission from the mouse event path.
- Consolidate top, side, and bottom resize hit areas into one internal resize
  overlay, with compatibility wrappers only if the public API requires them.
- Replace the dark-mode and language buttons with a generic two-state titlebar
  toggle component if those controls remain in the core module.
- Deduplicate the normal and alternate mark glyph geometry inside
  `VNM_AnimatedMark.qml`, and document the animation constants before changing
  the state machine.

## Native Frame

- Add Windows-specific native-frame smoke coverage if CI can provide a real
  visible `QWindow`/HWND path.
- If reliable Windows coverage is not practical, consider making the native
  frame path optional with a CMake option and documenting the manual-test scope.
- Revisit the immediate `UpdateWindow()` call during edge repaint. If it is not
  required for a known artifact, prefer invalidating and letting Windows repaint
  naturally.
- Keep native-frame window-state policy explicit: the controller draws only when
  asked through `frame_visible`; callers own fullscreen/maximized/minimized
  policy unless a higher-level component centralizes it.

## Documentation

- Add a README with Qt version requirements, C++ bootstrap, QML import usage,
  supported platforms, native-frame behavior, and license.
- Add architecture notes covering titlebar signal flow, resize-layer ownership,
  native-frame fallback rules, pixel snapping, and activity-marker layout.
- Add a public API table that distinguishes stable public types from internal
  helpers and example-specific controls.
- Add an invariants document for synchronous move/resize handling, resize-edge
  validity, pixel-snapping policy, activity-marker placeholder behavior, and
  animated-mark state assumptions.

## Tests

- Add behavioral example coverage beyond load-time validation when a practical
  way to drive native move/resize interactions is available.
- Add or update tests when deferred API decisions are made, especially around
  `VNM_ChromeTitleBar.active`, a future `VNM_ChromeWindow`, and public/private
  API boundaries.
