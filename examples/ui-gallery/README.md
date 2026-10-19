# UI Component Gallery

A Storybook-style catalog of OES's form controls — one `NotebookPage` per control
type, built as a `Form` on the `UIComponentGallery` `DataProcessor`, inside this
folder's base. It was built live through the Designer's MCP server (an assistant
driving `metadata_create`/`form_add`/`form_bind`, not hand-authored), and is kept
here as a worked example of what that looks like, plus a record of what it found.

## Why `visualView`, not `uikit`

OES has two layers that could be called "the UI kit":

- `src/engine/frontend/uikit/` — a wxUniversal-fork, desktop-only renderer (the
  "Luna" theme). Unfinished, and the web client never calls into it at all.
- `src/engine/frontend/visualView/ctrl/` — the 19 form-control types a developer
  actually places from the Designer's palette. This is the layer a form's
  metadata is built from, and the only one with any chance of reaching the web
  client — see the gap below for how much of a chance that actually is.

A gallery built on `uikit` would showcase widgets the web client can never show
under any circumstance. This gallery catalogs `visualView` instead.

## The 19 catalogued control types

Exact registered names (`CONTROL_TYPE_REGISTER` in `visualView/ctrl/*.cpp`), one
gallery page each, plus a 20th "coming soon" page: `Button`, `Checkbox`,
`Radiobutton`, `Textbox`, `Textctrl`, `Statictext`, `Staticline`, `Slider`,
`Gauge`, `Tablebox`, `Gridbox`, `Chartbox`, `Htmlbox`, `Toolbar`, `Notebook`,
`Boxsizer`, `Gridsizer`, `Staticboxsizer`, `Wrapsizer`. Some controls (`Checkbox`,
`Textctrl`, `Tablebox`) need a bound `Attribute`/`TabularSection` to render at all
— confirmed by reading each control's own `form_control` property schema rather
than assumed; see the note on `UIComponentGallery` (metaID 1005) in the base
itself for the per-control findings.

## Known gaps

### `Choice`, `ComboBox`, the unregistered `ListBox` variant

These exist as C++ classes under `visualView/ctrl/` but are never passed through
`CONTROL_TYPE_REGISTER`, so they cannot appear in this gallery, or in any form on
either client, until someone separately verifies their rendering and registers
them. The gallery's 20th page names this gap instead of silently omitting them.

### The web client renders almost none of this gallery — a real gap, bigger than expected

The original plan for this gallery assumed the `visualView/ctrl` layer "already
reaches the web client for free" through `src/engine/frontend/web/jsonAdapter.h`.
That assumption was wrong: `jsonAdapter.h` is a 33-line `nlohmann::json` adapter
for `wxString` only, not a serializer. The actual web-rendering path is each
control's own `Create()`, conditionally compiled with `#ifdef OES_USE_WEB`
(`frontend/web/webWindow.h` holds the `ibWeb*` widget classes it instantiates).
Grepping `visualView/ctrl/*.cpp` for `new ibWeb` shows only:

**Have a web implementation:** `Button`, `Checkbox`, `Textctrl`, `Toolbar` (+
item/separator), `Statictext`, `Boxsizer`, `Staticboxsizer`, `Gridsizer`,
`Wrapsizer`, and `Tablebox` (stub only — placeholder, not a real grid).

**Have none at all** — under a web build their `Create()` still constructs a real
native wx widget, which isn't an `ibWebWindow` and can't be walked into JSON, so
the whole subtree (the control and everything inside it) is silently dropped from
what the web client ever sees: `Notebook`, `NotebookPage`, `Radiobutton`,
`Slider`, `Gauge`, `Gridbox`, `Textbox`, `Staticline`, `Htmlbox`, `Chartbox`,
`Listbox`, `Choice`, `Combobox`.

Since this gallery's root control is a `Notebook`, opening it in the web client
(`wenterprise-server` + `webClient/client.html`) currently renders nothing at
all — confirmed by inspecting the server's own `open-meta/<id>` JSON response,
which comes back as `{"children":[{"type":"boxsizer","orient":8}]}`: an empty
root sizer, no `Notebook`, no pages. Implementing the missing `ibWeb*` widgets
for the 13 types above is real new engine work, not a gallery-authoring task —
out of scope for this pass.

### Desktop: a page-selection bug, found and fixed; a rendering bug, found but not fixed

Opening this gallery in `enterprise.exe` (or the Designer's own form editor) used
to show only one `NotebookPage` at a time, picking a different, seemingly
arbitrary page on every fresh run — with the visible tab's own label sometimes
not even matching what was drawn under it. Two distinct bugs were involved, both
in `src/engine/frontend/visualView/ctrl/notebook.cpp`:

1. **Selection — fixed.** A notebook built wholesale in one pass (every
   `NotebookPage::OnCreated` calls `AddPage(..., select=false)`, since no page
   has a reason to prefer itself over a sibling) never explicitly selects a
   page — but wx's standard "focusing a descendant switches its notebook to
   that page" behaviour fires anyway, for whichever control happens to receive
   initial keyboard focus, landing on an arbitrary page. Confirmed by tracing:
   `OnCreated` still reads the correct initial selection; the very next
   `OnUpdated` pass already reads a different one. Fixed by pinning the
   selection to page 0 exactly once, in `ibValueNotebook::OnUpdated`, guarded by
   a new `m_selectionSettled` member so a tab the user later switches to is
   never stolen back. Verified with a pristine-vs-fixed rebuild comparison: the
   unmodified code consistently mis-selects (confirmed across multiple cold
   restarts); the fixed build consistently selects page 0.
2. **Rendering — not fixed, pre-existing, deeper.** Even with the correct page
   now selected, only one tab button paints in the strip (not 20, with no
   overflow/scroll arrows despite `wxAUI_NB_SCROLL_BUTTONS` being set), and that
   page's own content does not render, despite `GetPageCount()` correctly
   reporting 20 throughout. Ruled out, each confirmed to make no difference:
   `Refresh()`, `SendSizeEvent()`, an explicit `Layout()` on every page,
   `ChangeSelection` vs `SetSelection` (to avoid `OnPageChanged`'s
   `g_visualHostContext`/`RefreshForm()` side effects), and `wxAUI_NB_TAB_SPLIT`.
   Confirmed pre-existing (not a regression from the selection fix above) via a
   stashed-vs-restored rebuild: the untouched original code shows the exact same
   "one tab, wrong/stale content" symptom, just with the wrong page's content
   bleeding through instead of nothing. This looks like a genuine
   `wxAuiNotebook`/`wxAuiTabCtrl` internal desync specific to building many pages
   in one scripted pass rather than through normal interactive one-at-a-time use
   — the exact mechanism (likely in how `wxAuiTabCtrl`'s own, separate
   `m_pages`/`active` bookkeeping gets populated across repeated `InsertPage`
   calls) was not found. Left for a session that can attach a debugger to
   `enterprise.exe` and step through `wxAuiTabContainer::DoShowHide` /
   `wxAuiNotebook::DoModifySelection` (`src/3rdparty/wxWidgets/src/aui/auibook.cpp`)
   live, rather than the trace-log-and-rebuild cycle this pass used.

### Protocol divergence

`wenterprise-server`'s web protocol is a bespoke REST + JSON + SSE API over the
control tree (`src/engine/wenterprise-server/main.cpp`), not the documented
unified JSON-RPC protocol (`docs/public/client-protocol.md`). Aligning the two is
a separate, larger task, independent of the gaps above.

## The shared Luna palette

Both renderers read the same colours from one file:
`src/engine/frontend/uikit/theme/lunaPalette.json` (shape documented in
`src/engine/core/graphics/lunaPalette.h`). The desktop Luna theme
(`uikit/theme/lunaTheme.cpp`) loads it as `wxColour`; `wenterprise-server` loads
the same file and injects it as CSS custom properties into
`webClient/client.html` (the `<!--OES_THEME-->` marker, the same mechanism the
existing `<!--OES_I18N-->` caption marker already uses).

## Opening it

- Desktop: `designer.exe --file=examples/ui-gallery` /
  `enterprise.exe --file=examples/ui-gallery`, then Operations → Data processors
  → UI Component Gallery.
- Web (`-DOES_BUILD_WEB=ON` build only — off by default, CI never turns it on):
  `wenterprise-server --file=examples/ui-gallery`, open the served
  `webClient/client.html`. Per the gap above, expect an empty form.
