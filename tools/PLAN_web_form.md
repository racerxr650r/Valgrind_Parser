# Plan: Schema-Aware Web Form for Project.xml

**Status:** Proposed
**Owner:** TBD
**Target deliverable:** `tools/edit_doc/` — a self-contained FastAPI
application launched via `python3 -m tools.edit_doc` (or
`make edit-doc`) that opens a browser tab on `localhost:PORT`. It
reads and writes `doc/Project.xml`, validates against
`tools/project.xsd`, lints via `tools/lint_project.py`, and shows a
live render of the affected spec document next to the editor.

## 1. Goals

1.  **Lower the bar to contribute.** A non-engineer should be able to
    add an HLR, write its `shall` clause, link it to an SDD section,
    and see the rendered HLRs.md page update — without touching XML.
2.  **Keep `Project.xml` canonical.** The form is a thin,
    schema-driven editor over the existing file. No new database, no
    duplicated state.
3.  **Surface validation continuously.** Inline errors as the user
    types: bad trace refs, duplicate ids, missing required fields.
4.  **Live preview.** A side panel rendering the regenerated markdown
    (and Markdown→HTML preview) for the document the user is editing.
5.  **Guide, do not just expose.** The UI walks the user through the
    spec lifecycle (PVD → SDD → HLRs → LLRs → tests) and surfaces the
    next sensible action ("you have an HLR with no LLRs — add one?").

## 2. Non-Goals

*   Multi-user concurrent editing (single-user local tool).
*   Replacing `render_doc.py` — the form *uses* it for preview.
*   Editing `doc/PVD.md` as structured data (it stays free-form
    markdown; the form opens it in a markdown editor pane).
*   Hosting on a public URL. Always binds `127.0.0.1`.

## 3. Architecture

```
┌─────────────────────────┐         ┌──────────────────────────┐
│  Browser (React + RJSF) │  HTTP   │  FastAPI (uvicorn local) │
│  - tabbed form per      │ ◄─────► │  - GET  /project         │
│    payload              │  WS     │  - PUT  /project         │
│  - live preview pane    │ ◄─────► │  - GET  /preview/:doc    │
│  - validation overlay   │         │  - GET  /lint            │
│  - guided checklist     │         │  - WS   /events          │
└─────────────────────────┘         └──────────┬───────────────┘
                                               │
                                ┌──────────────┴───────────────┐
                                │  Project.xml ↔ Pydantic      │
                                │  (lxml round-trip, CDATA-    │
                                │  preserving)                 │
                                ├──────────────────────────────┤
                                │  render_doc.py + project.xsd │
                                │  + lint_project.py           │
                                └──────────────────────────────┘
```

### 3.1 Backend (Python)

*   **Framework.** FastAPI + uvicorn. Single-process, single-worker,
    bound to `127.0.0.1`.
*   **Object model.** Pydantic v2 models mirroring the XSD. Either:
    *   Hand-write Pydantic and emit JSON Schema with `model_json_schema()`, **or**
    *   Generate Pydantic from the XSD with `xsdata --output pydantic`.
    Pick (b) so the XSD remains the single source of truth — when the
    schema grows, regenerate.
*   **XML round-trip.** `lxml` for parse/serialize. Must preserve:
    *   the file header comment block,
    *   `xsi:noNamespaceSchemaLocation` attribute on `<project>`,
    *   `<![CDATA[…]]>` wrapping on markdown bodies,
    *   element order inside `<all>`-style payloads.
    Use `lxml.etree.CDATA(...)` explicitly for body fields whose model
    type is `str`. Configure `pretty_print=True` and 2-space indent to
    match the existing file style.
*   **Render-on-demand.** Re-use `tools/render_doc.py` as a library
    (refactor `main` → callable `render_to_str`). The endpoint
    `GET /preview/{doc}` returns either the rendered markdown or
    rendered HTML (markdown-it-py). Cached per Project.xml mtime.
*   **Validation pipeline** runs on every `PUT /project`:
    1.  Parse incoming JSON → Pydantic (catches structural errors).
    2.  Serialize → XML string.
    3.  XSD validate via `lxml.etree.XMLSchema`.
    4.  Run `lint_project.check_semantics` over the new tree.
    5.  Reject the write if any errors; respond with the full findings
        list. Warnings are returned but allowed.
*   **Atomic writes.** Write to `doc/Project.xml.tmp`, then `os.replace`.
*   **Backups.** Keep last N writes in `.edit_doc/backups/` keyed by
    timestamp.

### 3.2 Frontend (Browser)

*   **Stack.** Vite + React + TypeScript. Single-page app served as a
    static bundle from FastAPI's `StaticFiles`. No backend templating.
*   **Form engine.** [`@rjsf/core`](https://github.com/rjsf-team/react-jsonschema-form)
    (React JSON Schema Form), driven by the JSON Schema FastAPI
    exposes at `GET /schema`. RJSF gives field-level validation,
    array add/remove, custom widgets.
*   **Custom widgets** (RJSF supports field overrides):
    *   `MarkdownTextarea` — CodeMirror 6 with markdown mode +
        live-rendered preview pane below.
    *   `TraceRefPicker` — typeahead populated from
        `GET /ids?kind=HLR|LLR`. Won't allow typing a non-existing id.
    *   `Reorderable` — drag-and-drop for `<hlr>`/`<llr>`/`<test>`
        ordering inside their parents.
*   **Layout.**
    *   Top-level tabs: **Metadata · SDD · STP · HLRs · LLRs · Tests**.
    *   Left rail (per tab): outline / item picker (e.g. one row per
        HLR-NNN, with status badges for "no LLRs", "no tests", "broken
        trace").
    *   Center: the form for the selected item.
    *   Right rail: live rendered preview of the matching spec
        document, scroll-locked to the edited item.
    *   Bottom bar: validation summary (e.g. "0 errors · 3 warnings ·
        last saved 14s ago").

### 3.3 Communication

*   **REST** for read/write/preview/lint/ids.
*   **WebSocket `/events`** to push file-changed notifications when
    Project.xml is edited externally (debounced inotify on the server
    side), so the form prompts before overwriting and offers to merge.

## 4. Guided Workflow ("Spec Wizard")

The UI is more than a form — it's a checklist that walks the user
through producing a complete spec stack. The wizard panel always
shows the *next sensible action* based on the current state of the
file:

| Stage | Trigger | Wizard prompt |
| ----- | ------- | ------------- |
| Bootstrap | `<sdd>` empty | "Start with the design overview: `<kind>`, `<audience>`, scope files." Opens the SDD tab pre-focused on `<scope>`. |
| Scope | `<scope>/<file>` count == 0 | "List the source files in scope." |
| Modules | `<modules>` empty after `<scope>` is filled | "Add a `<module>` for each scoped file. The wizard will pre-create stubs from the scope list." |
| HLRs | `<hlrs>` empty after first module | "Identify your high-level requirements. Group them into sections (e.g. CLI, Output, Errors)." |
| LLR drilldown | HLR with no LLR | "HLR-NNN has no LLRs implementing it — add at least one." |
| Test linkage | LLR with no test | "LLR-XXX-NN has no test verifying it — add a `<test>` row in the appropriate `<file>`." |
| Closure | All checks green | "Great — render and commit." Offers a "render all" + `git diff` view. |

Each prompt is **dismissible**, **revisitable**, and clickable — it
focuses the editor on exactly the field to fill in. The checklist
state derives from `lint_project.check_semantics` warnings; no extra
state to track.

## 5. Validation & UX details

*   **Field-level errors** (RJSF native): required fields, regex
    patterns (`HLR-NNN`).
*   **Form-level errors** from the backend `PUT /project` round-trip:
    rendered as a banner on the affected item plus an entry in the
    bottom-bar validation summary.
*   **Soft warnings** (coverage gaps, ambiguous test names) shown as
    yellow badges in the left-rail outline; never block save.
*   **Undo.** Every save creates a backup; "Undo last save" restores
    the previous backup with one click.
*   **Diff view.** Before save, show a unified-diff modal of
    `Project.xml.before` vs `Project.xml.after` (filterable to just
    the Pydantic-level changes).

## 6. Repository Layout

```
tools/edit_doc/
  __init__.py
  __main__.py            # python -m tools.edit_doc
  app.py                 # FastAPI app factory, CLI args (--port, --xml)
  api/
    project.py           # GET/PUT /project
    preview.py           # GET /preview/{doc}
    lint.py              # GET /lint, GET /ids
    schema.py            # GET /schema  (Pydantic → JSON Schema)
    events.py            # WS /events
  models/
    __init__.py          # generated by xsdata from tools/project.xsd
    extras.py            # hand-written augmentations (validators,
                         # CDATA hints, JSON-Schema overrides)
  xml_io.py              # round-trip parse/serialize, CDATA, comments
  render.py              # thin wrapper around tools.render_doc
  static/                # Vite build output (gitignored except .gitkeep)
  ui/
    package.json
    vite.config.ts
    src/
      main.tsx
      App.tsx
      tabs/
        SddTab.tsx
        StpTab.tsx
        HlrsTab.tsx
        LlrsTab.tsx
        TestsTab.tsx
        MetadataTab.tsx
      widgets/
        MarkdownTextarea.tsx
        TraceRefPicker.tsx
        Reorderable.tsx
      components/
        OutlineRail.tsx
        PreviewPane.tsx
        WizardChecklist.tsx
        ValidationBar.tsx
      api/
        client.ts
      lib/
        json-schema-utils.ts
```

## 7. Phased Delivery

### Phase 0 — Foundations (small, high-value, no UI)
1.  Refactor `tools/render_doc.py` to expose `render_to_str(template, metadata_id, xml_path) -> str`.
2.  Refactor `tools/lint_project.py` to expose `lint(tree) -> Findings` so it's importable.
3.  Add `tools/edit_doc/xml_io.py` with `load() -> ProjectModel` and `save(model) -> None`, including round-trip tests for the existing `doc/Project.xml`.
4.  Acceptance: `python -c "from tools.edit_doc.xml_io import load, save; save(load())"` produces a byte-identical (or whitespace-equivalent) file.

### Phase 1 — Read-only API + scaffolded UI
1.  FastAPI app with `GET /project`, `GET /schema`, `GET /preview/{doc}`, `GET /lint`.
2.  Generate Pydantic from XSD via `xsdata` (vendored in `tools/edit_doc/models/`).
3.  Vite/React shell with one tab (Metadata), RJSF rendering the Pydantic JSON Schema in **read-only mode**.
4.  Acceptance: `make edit-doc` opens browser; user can browse the metadata block and see live SDD preview.

### Phase 2 — Read-write for HLRs and LLRs
1.  `PUT /project` with full validation pipeline.
2.  HLRs tab and LLRs tab fully editable, with `TraceRefPicker` and `MarkdownTextarea` widgets.
3.  Backups and undo.
4.  Acceptance: Add a new HLR via the form, save, see it in `doc/HLRs.md` after running the make target. No corruption of unrelated CDATA blocks.

### Phase 3 — SDD, STP, Tests editing
1.  Three remaining tabs.
2.  SDD module editor with nested function/group widgets.
3.  STP fixture editor with dynamic `<artefact>` columns.
4.  Tests tab grouped by `<file>`.
5.  Acceptance: Every payload in the existing Project.xml is editable end-to-end; round-trip leaves it byte-equivalent.

### Phase 4 — Wizard + polish
1.  Wizard checklist driven by lint warnings.
2.  Diff modal on save.
3.  Inotify-driven external-change detection over WebSocket.
4.  Onboarding flow that runs `render_doc.py --init` from the UI for a fresh project.
5.  Acceptance: From an empty repo, a user runs `make edit-doc`, walks the wizard end-to-end, and commits a complete five-doc spec stack.

## 8. Risks & Open Questions

*   **CDATA preservation in lxml.** Confirm round-trip preserves CDATA
    on every markdown-bearing element. May require a custom serializer
    pass that walks the tree and re-wraps known fields. Spike in
    Phase 0.
*   **JSON Schema generated by xsdata** may not be ideal for RJSF
    (e.g. `xs:choice` becomes `oneOf`, which RJSF renders awkwardly).
    Likely need hand-written `uiSchema` overrides per tab.
*   **Markdown body editing UX.** A textarea with preview is enough
    for short HLR text; module bodies can be 30+ lines. Consider
    "expand to fullscreen" for long fields.
*   **Schema versioning.** When `<project>/@schema_version` bumps, the
    generated Pydantic models must be regenerated; CI should catch
    drift.
*   **Bundling node tooling** for a Python project. Decide whether to
    commit the `static/` build output (no node needed for users) or
    require `npm install` on first run. Recommendation: commit the
    build, document `npm run build` for contributors.

## 9. Estimated Effort

| Phase | Skill mix | Rough size |
| ----- | --------- | ---------- |
| 0 | Python | small |
| 1 | Python + light React | medium |
| 2 | Python + React | medium |
| 3 | React-heavy | medium-large |
| 4 | Mixed | medium |

## 10. Out-of-Scope Follow-ups (for a v2)

*   Git-aware view: load Project.xml at HEAD and show staged-vs-edited
    diff in the form itself.
*   Markdown-to-XML import for bulk seeding from existing free-form
    spec drafts.
*   AI-assisted authoring: a "draft an HLR from this PVD goal" button
    backed by an LLM.
*   Collaboration: optional locking via a `.edit_doc/lock` file, plus
    an OT/CRDT layer if multi-user becomes necessary.
