![header](assets/banner.png)

# Experience the native Aero Glass interface on Windows 10+

OpenGlass restores the full glass effect to window frames, with control over blur, reflections, colorization, caption rendering, and theme integration.

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/ALTaleX531/OpenGlass)
[![CI](https://github.com/ALTaleX531/OpenGlass/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/ALTaleX531/OpenGlass/actions/workflows/build.yml)

## Supported Windows versions

| Windows version (OS build) | Status |
| --- | --- |
| Windows 10 1809–22H2 (17763–19045) | Stable |
| Windows 11 21H2–25H2 (22000–26200) | Stable |
| Windows 11 26H2 (26300) | Stable |
| Windows 11 26H1 (28000) | Experimental |
| Windows Server 2022 (20348) | Supported |

OpenGlass 3.0.2.3749 adds support for Windows 11 26H2 and fixes the inactive window border regression that followed KB5124010 ([#367](https://github.com/ALTaleX531/OpenGlass/issues/367)).

Windows 11 26H1 uses the MILComp implementation, which is still experimental because some features are not yet implemented.

Only the General Availability releases listed above are supported. Insider builds, other prerelease versions, and Windows Server versions other than 2022 are unsupported and may crash DWM. Compatibility depends on the exact build, revision, and compositor capabilities.

See [Compatibility and DWM architectures](https://github.com/ALTaleX531/OpenGlass/wiki/Compatibility-and-DWM-architectures) for the support policy and how OpenGlass selects its DWM implementation.

## Quick start

1. Download `OpenGlassSetup.exe` from [Releases](https://github.com/ALTaleX531/OpenGlass/releases).
2. Run the installer and open the OpenGlass GUI. It requests administrator rights and defaults to editing HKLM. Developers can select `--scope=hkcu` or `--scope=hklm`; the window title identifies the fixed editing layer. The [configuration reference](https://github.com/ALTaleX531/OpenGlass/wiki/Configuration-and-registry-reference) lists the registry values involved.
3. Adjust the appearance. Edits are written immediately, although user-layer precedence or restart requirements may delay a visible effect. **Save** accepts the current preview; **Revert** undoes this GUI's changes since startup or the last Save.

The installer detects your Windows build and installs the matching DWM implementation automatically.

The **Glass colors** page includes Windows Vista and Windows 7 presets plus a multicolor **Automatic** swatch. Fixed/custom colors select manual mode. **Preset library** uses a details list with Name, Accent color, Author, License and Status columns. Right-click blank space to **New preset...** or **Import...**; right-click a row to apply, update, export, inspect properties or delete it. Double-click or Enter previews the selected preset. New preset and Update require a clean checkpoint: Save or Revert pending changes first. Import validates and previews ZIP contents, then **Trust and import** saves them without changing configuration. Accepted content applies without repeated confirmation; a changed digest requires renewed trust. Updates replace the selected entry, including imported entries. Export uses saved content. Esc clears selection, F5 refreshes, and Delete confirms removal.

Presets do not select HKCU or HKLM: the editor window determines the application target. Capturing extracts the effective HKCU + HKLM configuration, preserving uncustomized states and automatic values. Explicit values applied to HKLM clear only covered HKCU values that block them; applying to HKCU preserves HKLM. Default removes only target-scope customization and allows inheritance, intentionally. The optional **Include accent color** checkbox captures the original interactive user's RGB separately. It defaults to checked when AutoColorization is disabled and unchecked when enabled. Included RGB selects manual coloring when applied; no RGB selects automatic coloring and requests a wallpaper-color refresh. Schema 3 stores uncustomized states as `{"state":"default"}`, RGB in `accent_color`, balance values as Overrides, and intensity in `GlassOpacity`. Schema 1–2 remain readable through an in-memory conversion after original digest validation; their source contents and licensing are preserved. See [the design contract](OpenGlassGUI/Configuration.design.md) for operation boundaries and [the preset format](OpenGlassGUI/Presets.md) for storage details.

Color selection has special handling: fixed/custom colors set AutoColorization=0 and call SetUserColorPreference; Automatic sets it to 1 and requests UpdateWallpaperTransition. Both remove the two color Overrides from original-user HKCU and HKLM, regardless of editing scope, so neither layer masks the Windows color. All four deletions participate in Save/Revert. Windows generates ColorizationColor and ColorizationAfterglow in the original user's HKCU even while editing HKLM; the GUI does not directly rewrite their RGB or protect alpha. New GUI presets omit their two Override variants, and legacy package entries for them are ignored rather than applied. **Save** accepts the preview; **Revert** restores net changes made by this GUI across both configuration layers and the separate color mode/manual RGB choice. Automatic rollback requests the current wallpaper color rather than restoring an old RGB; Windows-generated palette/history and other accepted API side effects are not individually undone. No-op writes stay clean, changing a value back removes its net change, and untouched external values are preserved. Neither changes saved presets. Library selection only identifies the operation target; no current-preset identity is tracked because configurations may combine across layers. Failed multi-step previews restore the state immediately before that attempt, and incomplete recovery stays pending for retry. Windows 7 style also derives three balance Overrides from `GlassOpacity`; advanced manual registry values remain supported.

Both configuration layers are valid. Ordinary runtime values resolve `HKCU → HKLM → default`; colorization values resolve `HKCU Override → HKLM Override → HKCU base → HKLM base → default`. **Compatibility change:** an HKLM Override can now take precedence over an HKCU base value. Controls show only the editing layer's values or local defaults. A yellow warning marks HKLM settings masked by HKCU; an information icon marks unconfigured HKCU settings inheriting HKLM. Controls remain editable, and ordinary non-color editing never deletes a conflicting value in the other layer; color selection has the explicit two-Override exception described above. **Default means no customization in the editing scope**, not a forced program default. Removing a local value restores the normal lookup order, so configuration in another scope can still take effect. No additional registry value controls inheritance.

Each preset has one current directory under `%ProgramData%\OpenGlass\Presets\Library`. Applied images and their source notices use fixed paths under `Configuration\Machine` or `Configuration\Users\<original SID>`; resource contents and absence join Save/Revert. Eligible protected files use hard links, with copy fallback; replacements never rewrite shared file content. Delete actually removes library files without breaking the active configuration. Pending cleanup is reported and retried. Old deployed paths remain for compatibility after one-time library migration.

Consecutive preset previews retain the original Save/Revert checkpoint. Use **Revert** to return to that configuration, or **Save** to accept the current preview. To retain the current effective configuration for later use, accept pending changes with **Save**, then choose **New preset...**; switching presets and closing do not automatically save library snapshots.

New presets use **OpenGlass Attribution v1** for configuration, RGB and layout: copying, modification and sharing are allowed with author and known-source attribution. `LICENSE` applies to image assets only. With LICENSE, Author identifies the declared rights holder; without it, Author identifies a reference source. Neither claim is verified. Legacy packages and their derivatives retain their original license scope and text; converting the schema does not grant new rights.

**Restore defaults...** is in the **More** menu at the bottom left. It removes all known OpenGlass custom settings from the editing scope, including settings outside the preset format. Windows base colors, diagnostics, unknown values and the other layer are preserved. Normal inheritance still applies; **Revert** can undo the reset preview. Some settings require a DWM restart or sign-out, which the GUI does not perform automatically.

Hold **Shift** while clicking **More** to reveal **Switch to HKCU/HKLM view**. Save or Revert pending edits and finish any symbol download first. Switching retains the original interactive user, window placement and selected tab, and does not modify configuration or accent color.

**Merge into HKLM... / Merge into HKCU...** in the bottom-left **More** menu follows the editing scope. It previews the effective configuration to write, conflicts and any deletions. HKLM merge removes corresponding valid OpenGlass user values only after copying them; HKCU merge retains all HKLM values for other users. Both preserve the five Windows base color values in both layers and never examine other users. Merge is disabled while preview changes are pending; **Save** or **Revert** them first. Like preset extraction, a clean startup also permits merging. Confirming the merge starts a preview; **Save** accepts it and **Revert** undoes it. Backups retain the exact type, bytes, and absence of affected values. Only the HKLM direction requires an elevated token; the GUI startup elevation policy and other administrative operations are unchanged.

> [!TIP]
> **Emergency Exit:** Hold <kbd>Ctrl</kbd>+<kbd>Win</kbd>+<kbd>Shift</kbd>+<kbd>Q</kbd> to terminate DWM if the system becomes unresponsive.

OpenGlass is aimed at advanced users who are comfortable troubleshooting DWM. For a simpler alternative, consider [DWMBlurGlass](https://github.com/Maplespe/DWMBlurGlass).

## Reporting issues

Report DWM crashes and other bugs in [GitHub Issues](https://github.com/ALTaleX531/OpenGlass/issues/new); reports posted in third-party communities are not tracked. Include the exact Windows build and revision, the OpenGlass version, relevant settings, reproduction steps, and screenshots or recordings.

If glass is unexpectedly opaque, check the GUI's **Diagnostics** tab. For a crash, enable full DWM dumps there and reproduce the problem once. A hang requires a dump captured manually. See [Troubleshooting and crash dumps](https://github.com/ALTaleX531/OpenGlass/wiki/Troubleshooting-and-crash-dumps) for how to collect dumps and what to include in a report.

## Building

```powershell
msbuild OpenGlass.slnx /m /restore /p:Configuration=Release /p:Platform=x64
```

GitHub Actions also builds and tests `main`. Its downloadable `v<version>-unsigned` artifact is an unsigned validation build, not a release or Git tag. See [Building OpenGlass](https://github.com/ALTaleX531/OpenGlass/wiki/Building-OpenGlass) for prerequisites, output paths, packaging, tests, CI behavior, and signing requirements.

## Credits

- [Banner for OpenGlass](https://github.com/ALTaleX531/OpenGlass/discussions/11) by [@aubymori](https://github.com/aubymori), using [metalheart jawn #2](https://www.deviantart.com/kfh83/art/metalheart-jawn-2-1068250045) by [@kfh83](https://github.com/kfh83)
- [[MS-RDPCR2]: Remote Desktop Protocol: Composited Remoting V2](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpcr2)
- [KNSoft.SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours)
- [VC-LTL](https://github.com/Chuyu-Team/VC-LTL5)
- [Windows Implementation Libraries](https://github.com/Microsoft/wil)
- [libvalinet](https://github.com/valinet/libvalinet), whose symbol download work inspired OpenGlass
- [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB), whose C++ project structure inspired OpenGlass

## Support

OpenGlass is developed in spare time and released under the GPLv3. DWM offers no official extension mechanism, so future Windows updates may break OpenGlass, and ongoing support cannot be guaranteed.

If you find OpenGlass valuable, please consider supporting it on Ko-fi. Donations are voluntary, come with no expectation of anything in return, and must be made as a natural person.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/altalex531)
