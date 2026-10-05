# Configuration design contract

This is the product contract for the configuration editor, color synchronization and presets. It records the final decisions from the design discussion, superseding the alternatives listed below. Format details belong in [Presets.md](Presets.md). A later explicit product decision must update this contract and its affected consumers together.

## 1. Keep five concepts separate

| Concept | Meaning |
| --- | --- |
| Editing layer | The HKCU or HKLM configuration this window displays and ordinarily edits. |
| Effective configuration | The values selected by runtime precedence across both layers, with automatic/theme/default behavior intact. |
| Windows accent state | The original interactive user's automatic/manual mode and chosen manual RGB, independent of editing layer. Windows-generated colors and companion values are outputs, not checkpoint inputs. |
| Saved preset | An explicit, reusable snapshot of effective configuration, optional accent RGB and copied resources. It has no target scope. |
| Preview checkpoint | The before-values for this GUI's net modifications since startup or the last Save. It is not a preset. |

The editor is a layer editor, not a live effective-value inspector. A preset is an effective snapshot, not a dump of the visible controls. Save accepts edits; New/Update saves a reusable preset. These meanings must not depend on which tab is open.

## 2. Layer display, inheritance and defaults

The default layer is HKLM. `--scope=hkcu` selects the original interactive user's layer; `--scope=hklm` selects the machine layer. Values are case-insensitive, conflicting duplicates and invalid values are errors, and scope cannot change during a window's lifetime.

Holding Shift while opening **More** reveals **Switch to HKCU/HKLM view**. Switching requires a clean preview and no symbol download; it never implicitly saves or reverts. It replaces the window in the same process, preserving the original user identity, session editor lock, window placement and selected tab. The new window reads its own layer without changing configuration or Windows accent color.

Ordinary controls read and write only that layer. If no valid value exists, display the local default, including existing same-layer active/inactive fallback rules. Never populate ordinary controls with the other layer's values. Accent RGB and the two color Override deletions have the explicit cross-layer exception in section 6.

| Editing layer | Registry situation | Display and notice |
| --- | --- | --- |
| HKCU | Valid user value, even one numerically equal to a default | Show that local value; no inheritance notice. |
| HKCU | No valid user value, valid machine value | Show the local default with an inheritance information icon. |
| HKLM | Valid user value takes precedence, whether or not HKLM has a value | Show the machine value/local default with a yellow warning; keep the control editable. |
| Either | Neither notice applies | Show the local value/default without a layer notice. |

**Default means no customization in this layer.** A GUI action that restores a default removes that layer's customization; runtime may inherit from elsewhere. A preset default means the same thing. This does not promise the renderer's built-in default on every target, and no extra registry marker suppresses inheritance. An explicit DWORD equal to a default remains a real stored value until removed.

Runtime reads remain independent of the editor:

- Ordinary values: HKCU → HKLM → built-in default.
- Overridable values: HKCU Override → HKLM Override → HKCU base → HKLM base → built-in default.
- Invalid types are unavailable; continue to the next candidate. Both DWM architectures share this ordering. Machine Override precedence over user base is an intentional compatibility change.

Base/Override storage is an internal compatibility detail. Ordinary controls and their tooltips do not expose alternate keys, raw source values or dedicated Override-reset controls. Technical registry documentation and explicit transaction reviews can identify exact affected keys without introducing separate editing modes.

## 3. Operation boundaries

All configuration operations target only HKLM and the original interactive user. Never inspect or clean other users' hives.

| Operation | Reads | Configuration writes | Checkpoint / saved content |
| --- | --- | --- | --- |
| Ordinary edit | Selected layer | Selected layer only | Immediate preview. |
| Choose color / Automatic | Selected-layer strength plus original-user color mode/preference | Selected-layer strength parameters; removal of both color Overrides from HKCU and HKLM; original-user AutoColorization and the corresponding Windows API | Preview of mode and manual RGB, not a raw snapshot of Windows-generated colors. |
| Adjust intensity | Selected-layer strength and glass mode | Selected-layer GlassOpacity; Win7 also writes selected-layer balance Overrides | Immediate preview; no Windows preference synchronization. |
| Apply preset to HKCU | Saved preset and current values for planning | Covered user values only; preserve HKLM | May replace an existing preview; retain its first checkpoint. |
| Apply preset to HKLM | Saved preset and both layers for planning | Covered machine values; remove only conflicting valid user values blocking explicit preset values | Same preview behavior; preserve harmless same-value user entries. |
| New / Update preset | Effective configuration, optional original-user RGB, referenced resources | None | Requires a clean preview; publishes saved preset content. |
| Merge into HKLM | Effective known configuration across both layers | Copy effective values first, then remove corresponding valid OpenGlass user values | Requires a clean preview and confirmation; remains reversible until Save. |
| Merge into HKCU | Effective known configuration across both layers | Establish user values; preserve HKLM | Same clean-preview and confirmation rules. |
| Restore defaults | Known values in selected layer | Remove that layer's known OpenGlass customizations, including internal settings outside the preset surface | May run during another preview; Revert retains the original checkpoint. |
| Save / Revert | Modification journal | Save accepts changes; Revert restores journaled registry, color and configuration-resource state | Never changes saved preset content. |

The preset rows describe ordinary configuration planning; the independent Windows color operation follows the cross-layer cleanup exception in section 6.

A preset default deletes only its target-layer value in **both** apply directions; it does not clear the other layer. Thus explicit values are established for the target user, while default/automatic behavior deliberately remains environment-dependent. Applying a preset is not a guarantee of byte-identical effective results on all machines.

Preset application covers only recognized public settings within that package's catalog version. Merge and Restore defaults cover known OpenGlass settings, including manual-only ones, subject to the two-color-Override exception in section 6: Merge must not copy these retired GUI inputs; reset may remove them. None of them cleans unknown/future values. The five Windows DWM bases—ColorizationColor, ColorizationAfterglow, ColorizationColorBalance, ColorizationAfterglowBalance and ColorizationBlurBalance—are excluded from configuration cleanup. Color operations separately update the two base RGB values and restore their raw backups as described in section 6; the three base balances are never directly written by the GUI. HKLM merge retains malformed user-source values rather than deleting them; either target may replace an invalid destination after recording its raw before-value.

The bottom-left **More** menu contains **Merge into HKLM/HKCU...** and **Restore defaults...**; Save and Revert remain visible on the right.

A clean preview means no pending GUI net changes, not that Save must have been clicked once. Startup and a successful Revert are clean. Extraction/update/merge must not offer an implicit Save or silently accept edits. Merging differs from applying a preset: merge consolidates the existing effective configuration and may remove same-value user copies when targeting HKLM; preset application preserves harmless copies.

## 4. One preview transaction model

Every configuration mutation uses the same checkpoint semantics. Record the original raw type, bytes and absence of each modified `(scope, setting)` only once. The Windows color checkpoint separately records automatic/manual mode, manual RGB and changed AccentColorMenu as a raw value. Missing AutoColorization is interpreted as Manual and restored as an explicit DWORD 0. The two DWM base colors have independent raw backups for Manual recovery; Automatic recovery recomputes current wallpaper RGB instead of restoring old derived colors. Palette/history and other accepted Windows side effects are not raw journal entries. No-op writes are clean; returning a GUI-modified value to its original state removes its net change.

A multi-step attempt gets an operation-before snapshot. Failure restores that attempt, preserving earlier pending edits. Revert restores only net GUI changes, not whole registry directories: first restore the Windows color choice through its mode-specific operation, then restore OpenGlass configuration values/absence, then notify. This order also restores any color Overrides removed by a color choice. Untouched external values survive. An external change to an already modified item does not revoke the GUI's original before-value. Recovery failure stays pending and retryable; it must never be reported as successful recovery.

Save explicitly accepts OpenGlass configuration and the selected color mode/manual RGB, then discards the undo baseline. It does not wait for Explorer to finish calculating colors or compare derived registry values. Normal close attempts Revert and stays open if restoration fails. Esc belongs to the focused control, not global Revert. The full preview checkpoint does not survive a process crash; only an interrupted operation has the recovery described in section 7. Asynchronous Windows writers remain independent; do not promise isolation from future external writes.

List selection identifies the target of a library operation; it does not apply settings. Presets can combine with configuration from either layer, so the GUI does not label or discover a current preset, persist its identity, or compare live settings with library entries. Save/Revert manage configuration changes and retained source notices independently of selection. Status reports only trust or load problems. Removing a library entry does not change configuration.

## 5. A small preset workflow

The local library is a report ListView with Name, Accent color, Author, License and Status columns. Single-click selects; double-click/Enter or the row context menu applies once. Row menus offer Apply, Update, Export, Delete and Properties. Blank-space menus offer New preset, Import and Refresh. Right-click targets the row under the pointer or clears selection on blank space. Keyboard menus, F5 and Delete follow those operations; Esc clears selection, never globally reverts. There is no preset button bank or permanent details pane. Merge and Restore defaults are in the bottom-left More menu; Save/Revert remain at right. Optional extraction fields start collapsed. Import/Refresh appear only in the blank-space menu; Properties follows Delete in the row menu. Import review and Properties open on a concise General page, with Changes, Contents and License available separately. Batch import uses a list with per-entry details, not expanded reports for every package.

New/Update capture clean effective configuration. Update replaces any selected entry, including imports, without modifying its external ZIP. Export uses saved content. Delete actually removes library files but leaves independent current resources and rollback intact. Save, Revert, switching and closing never publish snapshots; there is no Unsaved draft or Save-a-copy command.

Every external ZIP is validated, previewed with target/differences/color/resources/conversions/ignored fields and accepted through Trust and import. Import does not apply. After confirmation, revalidate the same digest before publication. Admission is local and bound to content, not author identity. Accepted presets apply directly without repeated sensitive-setting or cross-layer confirmation; permissions, digest and capability checks remain mandatory. Changed or manually added content must be explicitly trusted. No action automatically restarts DWM.

## 6. OpenGlassGUI treats color configuration specially

The following ten registry names are **OpenGlassGUI managed settings**. This is a GUI policy group, not a new registry value or a change to runtime compatibility.

| Managed settings | GUI rule | Scope |
| --- | --- | --- |
| `ColorizationColor`, `ColorizationAfterglow` | Manual directly updates their RGB while preserving each alpha; Automatic obtains RGB through the shell refresh. | Always the original interactive user's HKCU, including HKLM editing/application. |
| `ColorizationColorOverride`, `ColorizationAfterglowOverride` | Never capture, create or apply. Color operations always delete both variants in both hives; only checkpoint recovery may restore deleted values. | Original-user HKCU and HKLM, independent of the editing scope. |
| `ColorizationColorBalance`, `ColorizationAfterglowBalance`, `ColorizationBlurBalance` | Ignore in new preset capture, schema 3 application and ordinary GUI editing. Never directly write or delete. Schema 3 fields are reported ignored and remain in the original digest; compatibility conversion is specified below. | Neither layer is edited. |
| `ColorizationColorBalanceOverride`, `ColorizationAfterglowBalanceOverride`, `ColorizationBlurBalanceOverride` | The only balance configuration recorded and written by the GUI. Capture resolves explicit Overrides across both hives; absence means uncustomized. | Ordinary editing targets the selected layer; preset application and Merge retain their ordinary scope rules. |

Compatibility exception: schema 1/2 import and explicit Merge may convert effective legacy base balances to balance Overrides when no valid explicit Override takes precedence. They write only Overrides to the selected target and preserve all base keys. This preserves old appearance settings; it does not authorize new preset capture to promote Windows-generated balances. The runtime still accepts manually configured base/Override values. Windows API side effects are separate from GUI configuration writes; the accepted base-balance updates described below remain possible. Color Override cleanup occurs during color selection/preset application, not as unsolicited startup cleanup or during an unrelated edit.

The GUI owns the user's color choice, not every value Windows generates from it. Its color checkpoint records mode/manual RGB and a separate raw Accent value only when that value is changed; directly written DWM values use independent raw backups. OpenGlass configuration continues to use the ordinary raw-value journal.

| Choice | Original interactive user's Windows operation | Color swatch / preset meaning |
| --- | --- | --- |
| Manual | Set AutoColorization=0; if permitted and different, write AccentColorMenu as an opaque COLORREF; then update both DWM RGB values directly. | A fixed/custom swatch; a preset containing accent_color RGB. |
| Automatic | Set AutoColorization=1, then synchronously send the forced wallpaper-color request used by shell32 UpdateWallpaperTransition | The multicolor Automatic swatch; a preset with absent/null accent_color. |

ColorizationColor and ColorizationAfterglow have a fixed user-state target: the original interactive user's HKCU, even when a preset is applied or the editor operates in HKLM. The editor scope never redirects these two colors into HKLM. Manual writes their RGB directly, preserving each current alpha; Automatic uses the UpdateWallpaperTransition shell request. Manual rollback restores independent raw DWM values; Automatic rollback recomputes the current wallpaper color. Do not retain the uxtheme RegCreateKeyExW/DwmpSetColorizationParameters IAT hooks or reconstruct Windows companion values afterward. ColorPrevalence writes, palette/history regeneration, Windows base balance changes and color alpha changes are accepted Windows side effects, not changes to suppress or individually undo. This acceptance does not authorize unrelated registry cleanup.

Choosing a fixed/custom color **or Automatic must delete ColorizationColorOverride and ColorizationAfterglowOverride from both the original interactive user's HKCU and HKLM, regardless of editing scope**. Even an HKLM editor changes Windows color preference and its generated ColorizationColor/ColorizationAfterglow in that user's HKCU; an Override in either hive would mask those colors. This is an explicit color-operation exception to layer-only editing, not a general permission to clean the other layer. Each of the four possible deletions is a separate (scope, setting) journal entry with its raw before-value/absence, restored on Revert or failed-attempt rollback. Check access to both layers before changing mode or colors; do not silently perform only half the cleanup. No other user hive is inspected. There is no explicit-preset exception: new GUI capture/update/application and Merge must not create or copy either color Override. Legacy package entries for them are ignored, even when explicit. Restoring values deleted by this GUI during Revert/failed-attempt recovery is the sole write-back exception; it restores a checkpoint, not a preset setting.

Intensity remains in the selected layer's GlassOpacity in both glass modes. Win7 intensity editing derives that layer's three balance Overrides. New presets capture only explicit balance Overrides across the two layers; missing Overrides stay uncustomized and are never filled from Windows base balances. Schema 3 ignores base balance fields. Schema 1/2 import and explicit Merge retain the compatibility conversion described above. GUI configuration operations never write or delete the three base balances; Windows API side effects remain governed by the preceding paragraph. Changing intensity does not change automatic/manual mode, and selecting Automatic retains the configured intensity. In automatic mode, the Custom swatch reads the original user's current accent RGB on color/settings notifications and window reactivation. Coalesce these into a deferred, read-only UI refresh; preserve the last swatch on read failure and never change registry values, saved presets or preview checkpoints. Alpha is neither new slider storage nor an integrity condition. Advanced manual values remain supported without Detailed Colorization Settings controls.

AutoColorization and all color operations target the original interactive user, including HKLM editing and alternate-credential elevation. Swatch selection reflects the actual mode. Include accent color starts checked when that user's AutoColorization is off (missing means off); invalid types/read failures must be reported. Present RGB in a preset means manual, not merely an optional RGB write; absent/null means automatic, not leave-unchanged. A legacy package follows the mode implied by its converted main RGB. Presets without RGB use the multicolor appearance.

Color rollback restores the choice rather than a byte-for-byte Windows snapshot:

- Restore a changed Accent value from its independent raw backup, regardless of current policy. Restore Manual by writing AutoColorization=0 and the checkpoint RGB, then restore the two directly journaled base colors to their distinct original values.
- Restore Automatic by restoring its mode and requesting a fresh color from the **current** wallpaper. Do not reapply the old automatic RGB.
- Missing AutoColorization means Manual on read. Disabling automatic coloring and restoring Manual always write an explicit 0, including when the baseline value was absent; they never delete the value.
- Save accepts the choice. Revert, close and failed attempts use these same mode-specific operations alongside the existing OpenGlass configuration transaction. Capability or recovery failures must be explicit and remain retryable.

The GUI does not monitor later Explorer RGB/palette writes or add them to a deferred journal; it backs up values involved in its explicit color operation. Do not introduce registry polling or late corrective writes to simulate cross-process isolation. Direct Manual writes are part of the synchronous GUI operation. Automatic selection must actively request the current wallpaper color; consuming an already pending result is insufficient after a manual choice. Deliver the verified forced shell request synchronously, with a bounded wait, so a successfully completed GUI request does not remain queued behind a newer Manual choice. Allow sent-message dispatch during the wait for Explorer broadcasts. Report delivery failure through the preview recovery path; timeout does not promise cancellation of work already executing in Explorer. Exact message parameters and evidence belong in the binary audit. Do not claim receiver completion proves successful image extraction on every Windows build.

### Color-selection policies

These checks respect policy restrictions on synchronizing the Windows accent-color preference (AccentColorMenu) when choosing a new color. They do not disable OpenGlass color editing: Manual still writes AutoColorization=0 and both DWM RGB values, so this is not a claim of full compliance with Windows color-selection policies. Recovery is a separate operation and restores only this GUI's recorded changes, including a changed Accent value even if policy has since changed.

Manual always writes AutoColorization=0 and updates the original user's ColorizationColor and ColorizationAfterglow RGB, preserving each alpha. Both existing DWM values must be readable DWORDs and writable before mutation. Before the DWM writes, synchronize AccentColorMenu only when neither machine Personalization policy blocks it: PersonalColors_Background must not succeed as REG_SZ with uxtheme's 16-byte buffer, and NoChangingStartMenuBackground must not be a nonzero REG_DWORD. An access-denied policy read fails preparation. Policies themselves are never modified.

AccentColorMenu records the matching Windows preference so later synchronization does not restore a different RGB; watcher propagation is not required to perform the GUI's DWM write. Compare its raw DWORD with the requested opaque COLORREF and skip an equal value entirely, including its rollback entry. A changed Accent value receives its own first-before-value backup, including raw type, bytes and absence. DWM values have independent raw backups. Recovery restores recorded targets regardless of later policy changes, and preserves untouched Accent state. Manual capture reads ColorizationColor RGB. Automatic retains its existing shell request; restoring Automatic recomputes from the current wallpaper instead of restoring old derived DWM colors.

The inspected 14393.0 and 26100.9549 uxtheme implementations cache NoChangingStartMenuBackground and update that cache only on a successful policy read. Deleting the value can therefore leave a running process's old nonzero cache intact. Direct DWM writes do not depend on that process propagating Accent changes. They still cannot prevent later external or policy-driven writes, and preserving alpha during the GUI write does not protect it from Windows callbacks.

Changing either policy while the desktop session is running can leave the registry and background processes' cached policy states out of sync. If a policy is added after those processes initialized, the GUI may stop updating AccentColorMenu while a process still permits propagation; a later notification can then copy the old Accent RGB over the newly written DWM RGB. Skipping the Accent write does not itself notify that key's watchers, but it cannot prevent other triggers. Removing a policy can produce the opposite mismatch, including the cached NoChangingStartMenuBackground behavior above. This is an uncommon live-policy-transition scenario, not a guarantee that the current registry policy reflects every process's behavior. The GUI neither forces cross-process policy-cache refresh nor guarantees RGB persistence during that transition. Successful GUI writes alone do not establish convergence between those processes.

### Why Manual does not use SetUserColorPreference

The observed implementation of SetUserColorPreference has a serious cross-process consistency defect: it publishes palette/history changes before completing the persistent preference, writes StartColorMenu and AccentColorMenu separately, and allows a registry-watcher read to publish an obsolete preference back to shared memory and DWM. An atomic shared-memory exchange does not make this multi-step operation atomic. This is a race condition in the cross-process read/modify/write protocol, not a GUI selection or rendering failure.

On the examined Windows 10.0.26100.9549 binaries, ProcMon captured both failure modes: consumers read new StartColorMenu with old AccentColorMenu between the GUI's writes; another consumer read a complete old preference, then wrote the old DWM RGB after the GUI had successfully written the new RGB. Its stack ran from a Windows.UI.Immersive registry-watcher callback through GetImmersiveUserColorSetPreference, SetColorPreferenceHelper, _UpdateColorPreferenceForDWM and DwmpSetColorizationParameters. Even the consumer's DWM Get read the new color before its stale request replaced that RGB. Thus successful registry writes and API return values do not establish a consistent final color. The helper's same-preference optimization can also skip subsequent DWM synchronization; forcing a broadcast does not force that write. This is an observed API design flaw, not a claim about Microsoft's intent or every Windows revision.

OpenGlassGUI sets manual mode, optionally publishes the requested AccentColorMenu when policy permits and it differs, then writes both DWM RGB values directly. It does not call SetUserColorPreference, SetImmersiveColorPreferenceInSharedMemory or DwmpSetColorizationParameters, regenerate AccentPalette/StartColorMenu/history, or add a forced manual broadcast. This avoids the early palette notification and split preference publication caused by the previous GUI path, while keeping the persistent accent source used by later Windows refreshes. The direct DWM writes no longer depend on watcher propagation; Accent records the matching source for later Windows refreshes.

### Known limits

This is not a cross-process transaction or a guarantee against arbitrary late writers. Manual persistence is written synchronously, but a successful registry write does not prove that DWM has rendered it. Later external writers remain independent. The GUI does not regenerate Start color, palette or history, promise that all Windows surfaces adopt the same RGB, or protect alpha against subsequent Windows writes; its own direct writes preserve each current alpha. No polling, delay/retry loop, cross-process lock or hook is added.

This design does not patch system DLLs, install COM hijacks or add Host RGB monitoring. The separately discussed uxtheme patcher is a different project, not a dependency of GUI color selection.

## 7. Format, resources and simplicity

The current unpublished format is schema 3, with no scope field and no `system_color` alias. Published schemas 1/2 validate their original digest before conversion in memory. Legacy main base RGB becomes optional accent RGB; legacy base balances convert to Overrides only during schema 1/2 import, with valid explicit Overrides taking precedence. ColorizationColorOverride and ColorizationAfterglowOverride are excluded from new snapshots and application models in every schema. Validate old content and its digest first, then list these entries as ignored; do not use their RGB or alpha as conversion fallbacks. An independent legacy afterglow cannot be preserved by synthesizing an Override and is reported as ignored; the new GUI uses one Windows accent choice for both base colors. Legacy Win7 main base alpha may fill missing intensity only when valid GlassOpacity is absent. No usable main color means automatic coloring, without inventing an RGB. Original packages stay unchanged; ignored fields remain covered by their original content digest. Removing these two settings from the GUI/preset surface does not remove runtime/manual-registry compatibility, and does not retire the three balance Overrides.

Snapshots retain legal automatic/theme sentinels and defaults rather than baking them into temporary output values. Unknown or out-of-catalog settings participate in the original digest but are not written or used to clean configuration. Runtime support, preset surface/catalog version and GUI controls are separate responsibilities.

Keep one current self-contained directory per library entry, a stable local ID and a digest-bound local acceptance record. Do not maintain an authoritative library index, permanent revisions or resource ownership history. Stage and verify updates, replace under the library lock, and remove temporary recovery content after success. Delete performs actual cleanup; failures remain visible. ZIP is transport only. Legacy deployments are imported once without moving old live paths or touching registry state.

Apply/Merge resources use fixed Machine and Users/<original SID> configuration slots independent of the library. Eligible protected files are hard-linked before falling back to copies; writable external sources are copied. Never overwrite a shared file inode or alter its ACL/attributes through another link. Resource bytes/absence and source notices join the preview journal, including same-path changes, operation rollback and first-baseline Revert. A writer lock serializes GUI previews across sessions. Incomplete operations have a durable recovery record; completed operations do not become persistent session history. Directory, manifest and source-notice files are never hard-linked. Send a Theme refresh for changed contents even when the registry path is identical. Notification delivery does not prove compositor completion.

New Author metadata declares the LICENSE rights holder when LICENSE is supplied, otherwise a reference source. Legacy authorship/terms retain their original scope. Known notices and external terms remain flat metadata, not an editing lock or a legal authority. New configuration/RGB/layout uses OpenGlass Attribution v1; LICENSE covers referenced images. Uninstall keeps library cleanup independent of machine/current-user configuration resources and preserves other users.

Prefer a clear rule and a disclosed limitation to a new state machine for a speculative edge case. Simplification must not remove transactional restoration, identity checks, source notices or hostile-input validation. Do not turn resource provenance into the editor's organizing principle or repeatedly increment an unpublished schema during exploration.

## 8. Privileges and acceptance evidence

The installer's optional completion-page launch uses runasoriginaluser, so the GUI starts under the installing interactive user's pre-UAC identity and performs its own authenticated elevation. Do not use runascurrentuser here: alternate administrator credentials would otherwise become the GUI's configuration user. If Setup was explicitly started already elevated, Inno Setup cannot recover a pre-elevation identity; launch the GUI from the intended user's desktop or Start menu instead.

The GUI still elevates at startup with an asInvoker manifest and authenticated original-user handoff; one editor per interactive session prevents competing color previews. Scope is not privilege. System-tab configuration follows scope; services, symbol downloads and WER retain their administrative targets. Symbol downloads and WER mutations check elevation at entry as well as disabling their controls; read-only diagnostics remain available. Library storage is protected. On-demand elevation for unelevated HKCU use is not implemented or promised.

Uninstall policy is independent of editor scope: known current-user plus HKLM configuration and preset packages have separate choices. Never enumerate other user profiles or delete the Windows DWM key wholesale. Preserve user-selected custom dump folders.

## Superseded alternatives

Do not reintroduce mandatory startup migration, scope-bound presets, effective values in ordinary controls, registry default-suppression markers, whole-directory rollback, an Unsaved draft, automatic Save after Merge, a global Esc Revert shortcut, per-balance GUI sliders, color-alpha intensity storage, or automatic deletion of the other layer during ordinary non-color editing. The two color Overrides are the explicit cross-layer exception in section 6. Also superseded: GUI uxtheme color IAT hooks and raw/deferred rollback of Windows-generated color companions. COM/WinRT hijacking, system-DLL patch templates and Host RGB watching were exploratory alternatives, not dependencies of this GUI design.
