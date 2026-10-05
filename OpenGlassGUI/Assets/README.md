# OpenGlass GUI icon

The application icon is adapted from **Window Apps (48 Filled)** in Microsoft's [Fluent UI System Icons](https://github.com/microsoft/fluentui-system-icons).

- Original source: [`ic_fluent_window_apps_48_filled.svg`](https://github.com/microsoft/fluentui-system-icons/blob/main/assets/Window%20Apps/SVG/ic_fluent_window_apps_48_filled.svg).
- Copyright (c) 2020 Microsoft Corporation.
- License: [MIT License](FluentIcons.LICENSE.txt), retained in full alongside the artwork.

## Files and modifications

- [`window-apps-filled.original.svg`](window-apps-filled.original.svg) is the unmodified upstream SVG.
- [`OpenGlass.svg`](OpenGlass.svg) preserves the original Filled geometry and negative space, replaces the solid fill with opaque blue/cyan gradients, and fits the artwork to a transparent 256×256 canvas without outer padding.
- [`OpenGlass.png`](OpenGlass.png) is the 256×256 RGBA rendering used for ICO conversion; its pixels match the ICO's 256 pixel image.
- [`OpenGlass.ico`](OpenGlass.ico) is the application icon derived from the modified artwork. It contains 16, 24, 32, 48, 64, 72, 96, 128, and 256 pixel images, all stored as uncompressed 32-bit DIBs with alpha transparency. The artwork fills the canvas without outer padding.

The ICO is embedded in `OpenGlassGUI.exe` and used for the main window. The full Microsoft copyright notice and MIT license are also embedded in the executable as the `IDR_FLUENT_ICONS_LICENSE` resource. Retain [FluentIcons.LICENSE.txt](FluentIcons.LICENSE.txt) when redistributing the standalone icon assets or derivatives.
