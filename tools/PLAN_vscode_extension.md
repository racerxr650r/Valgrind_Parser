# Plan: VS Code Extension for Project.xml Authoring

**Status:** Proposed
**Owner:** TBD
**Target deliverable:** `tools/vscode-project-xml/` — a VS Code
extension (publishable as a `.vsix`, optionally to the Marketplace)
that provides a structured editor, custom views, validation, and a
guided-authoring experience for `doc/Project.xml` directly inside the
editor the developer is already in.

## 1. Why a VS Code Extension Instead of (or Alongside) the Web Form

The web form (see [PLAN_web_form.md](PLAN_web_form.md)) is the right
answer for non-developer contributors. The extension is the right
answer for the developer audience this project actually has today,
because:

*   Zero context switch — the spec lives next to the code.
*   The user's existing Source Control panel handles diff/commit/
    history.
*   VS Code already has best-in-class XML support via the Red Hat
    extension; we layer on top of it instead of rebuilding.
*   Custom Tree Views, Code Lenses, Diagnostics, and Webview Panels
    cover every interaction the web form needs.
*   Distribution is `code --install-extension`; no localhost server.

The two tools share the **same backend logic** — both call
`tools/render_doc.py` and `tools/lint_project.py`. The extension is
"the IDE-native shell"; the web form is "the standalone shell".

## 2. Goals

1.  **First-class structural view** of `Project.xml` in the Explorer
    side bar (HLRs / LLRs / Tests / SDD / STP trees).
2.  **Inline diagnostics** for every error/warning surfaced by
    `lint_project.py`, with quick fixes where applicable.
3.  **Code Lenses** above each `<hlr>`, `<llr>`, and `<test>` showing
    coverage status and click-through to related items.
4.  **Form panels** (webviews) for editing complex payloads (a single
    HLR, a single test, an SDD module) without scrolling 1500-line XML.
5.  **Live preview** — render the affected spec markdown into a side
    preview pane on save.
6.  **Guided authoring** — a Walkthrough (the same UI as VS Code's
    "Get Started" tab) that steps the user through bootstrapping a new
    project and filling each payload in order.
7.  **Inline AI assistance** — a Copilot Chat participant
    (`@projectspec`) plus right-click "Draft with AI" actions on every
    payload type. The AI is constrained by the XSD and grounded in the
    PVD/SDD/upstream payloads, so generated HLRs/LLRs/tests are
    schema-valid, traceable, and consistent with what the user has
    already written.
8.  **Commands for everything** — every action discoverable via the
    Command Palette.

## 3. Non-Goals

*   Re-implement XML syntax highlighting / outline / autocomplete —
    delegate to the Red Hat XML extension (declared as `extensionDependencies`).
*   Edit `doc/PVD.md` as structured data — open it as plain markdown.
*   Bundle a Python interpreter — we shell out to the user's Python
    via `python.defaultInterpreterPath` (or a configured one).
*   Ship our own LLM client or API keys. AI features layer on top of
    the Language Model API the user has already enabled
    (`vscode.lm.*` / Copilot Chat). If those APIs are unavailable, AI
    features hide themselves; the rest of the extension still works.

## 4. Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                         VS Code Window                         │
│ ┌─────────────────────┐ ┌────────────────────────────────────┐ │
│ │  Explorer Side Bar  │ │  Editor                            │ │
│ │  ╔═══════════════╗  │ │ ┌────────────────────────────────┐ │ │
│ │  ║ Project Spec  ║  │ │ │ doc/Project.xml (text editor)  │ │ │
│ │  ║   HLRs        ║  │ │ │   - Red Hat XML diagnostics    │ │ │
│ │  ║   LLRs        ║  │ │ │   - our diagnostics + lenses   │ │ │
│ │  ║   Tests       ║  │ │ └────────────────────────────────┘ │ │
│ │  ║   Coverage    ║  │ │ ┌────────────────────────────────┐ │ │
│ │  ╚═══════════════╝  │ │ │ Webview: HLR-007 form          │ │ │
│ │                     │ │ └────────────────────────────────┘ │ │
│ └─────────────────────┘ └────────────────────────────────────┘ │
│ ┌─────────────────────┐ ┌────────────────────────────────────┐ │
│ │  Walkthrough panel  │ │  Markdown preview: HLRs.md         │ │
│ └─────────────────────┘ └────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────┘
                                │
                                ▼
                   ┌────────────────────────┐
                   │  Python sidecar tools  │
                   │  (shell-out, JSON-RPC) │
                   ├────────────────────────┤
                   │  render_doc.py         │
                   │  lint_project.py       │
                   │  (new) project_io.py   │
                   └────────────────────────┘
```

### 4.1 Extension (TypeScript)

*   **Stack.** TypeScript, `@vscode/extension` API, esbuild for
    bundling, mocha + `@vscode/test-electron` for tests.
*   **Activation events:** `onLanguage:xml` (filtered to files named
    `Project.xml`), `workspaceContains:doc/Project.xml`,
    `onCommand:projectXml.*`.
*   **Extension dependencies:** `redhat.vscode-xml` (autocomplete,
    formatting, schema-driven editing — already triggered by our
    `xsi:noNamespaceSchemaLocation`).

### 4.2 Python Sidecar

The extension shells out to Python for all schema-aware work. To
avoid spawning per-call:

*   New tool `tools/project_io.py` exposes a tiny stdio JSON-RPC server.
*   Methods: `lint`, `render`, `parse_to_json`, `apply_edit(json_patch)`,
    `init_project(name, short_name, author)`, `subscribe_changes`.
*   Started lazily on first command invocation; killed on extension
    deactivate.
*   Re-uses `lint_project.lint(tree)` and a refactored
    `render_doc.render_to_str()`.
*   The extension never parses `Project.xml` itself — it always asks
    the sidecar.

### 4.3 Data Model

The sidecar returns a JSON projection of the tree. The extension
treats it as immutable; edits are sent as **JSON Patch** ops that the
sidecar applies and serializes back to XML (preserving CDATA, comments,
whitespace via `lxml`).

## 5. Surfaces

### 5.1 Tree View (Activity Bar)

A new Activity Bar icon "Project Spec" with these tree nodes:

```
Project Spec
├── 📘 SDD
│   ├── Scope (3 files)
│   ├── Architecture (2 components, 4 flow steps)
│   ├── Modules
│   │   ├── src/main.c
│   │   └── src/vgp.c
│   └── Data Dictionary
├── 📗 STP
│   ├── Strategy
│   ├── Integration Environment (5 fixtures)
│   └── Tooling
├── 🟦 HLRs (45)
│   ├── §2 Command-Line Interface (5)
│   │   ├── HLR-002 Argument Parsing            ⚠ no LLRs
│   │   └── HLR-003 Usage and Help Display
│   └── ...
├── 🟪 LLRs (124)
│   └── parse_command_line (12)
│       ├── LLR-PCL-01           ❌ broken trace
│       └── LLR-PCL-02           ⚠ no test
├── 🧪 Tests (120, 6 files)
└── 🎯 Coverage Gaps (5 HLRs, 39 LLRs)
```

*   Each node has commands on its context menu: **Edit**, **Reveal in XML**,
    **Add LLR…**, **Delete**, **Render this section**.
*   Status badges (`⚠`, `❌`) come straight from the lint findings.
*   Clicking a leaf reveals it in `doc/Project.xml` (DocumentSymbol-based).

### 5.2 Diagnostics

Every error and warning from `lint_project.py` becomes a VS Code
`Diagnostic` attached to `doc/Project.xml` at the right line. The
extension watches the file (and the JSON projection) and refreshes
diagnostics on save / on JSON edit.

Examples:

| Lint finding | Diagnostic | Severity | Quick fix |
| ------------ | ---------- | -------- | --------- |
| `<trace> references unknown HLR 'HLR-999'` | underlines the `ref` attribute value | Error | "Replace with…" picker |
| `LLR LLR-FOO-01 has no test verifying it` | underlines the `<llr id="…">` | Warning | "Create stub test entry" |
| `<hlr id="HLR-2">` (bad format) | underlines the id | Error | "Renumber as HLR-002" |
| `<metadata> has no <document id="Traceability">` | underlines `<metadata>` | Warning | "Insert standard `<document>` row" |

### 5.3 Code Lenses

Above each `<hlr>`, `<llr>`, and `<test>` element:

*   `LLRs (3) · Tests (4) · ✏ Edit · 👁 Preview HLR section`
*   Click "LLRs (3)" → quick-open list of the LLR ids that trace this HLR.
*   Click "Edit" → opens the form webview for this item.

### 5.4 Form Webview

For complex items (a single HLR, LLR, test, or SDD module), the user
clicks "Edit" and we open a webview containing a small RJSF-rendered
form. This webview is functionally identical to a single tab of the
web-form plan, but:

*   It edits one item at a time, not the whole tree.
*   It posts a JSON Patch back to the sidecar; the sidecar writes the
    XML; the extension picks up the file change and refreshes.
*   The form's JSON Schema comes from the sidecar's `parse_to_json`
    response (which derives it from `tools/project.xsd` plus per-field
    UI hints).

### 5.5 Markdown Preview

`Project Spec: Render & Preview` command:

1.  Calls sidecar `render(template_id)`.
2.  Writes the result to a temp file (or in-memory).
3.  Opens it in VS Code's built-in Markdown Preview pane to the side.

A setting `projectXml.previewOnSave` triggers this automatically when
`Project.xml` is saved, scoped to the document affected by the edit.

### 5.6 Walkthrough (Guided Authoring)

VS Code's `walkthroughs` contribution renders an interactive Get
Started tab:

```
Project Spec — Get Started
1. ✅  Bootstrap your Project.xml
2. ✅  Fill in the Product Vision Document
3. ⏵  Sketch your design (SDD)
4. ⏵  Identify high-level requirements (HLRs)
5. ⏵  Drill down to low-level requirements (LLRs)
6. ⏵  Add tests and link them
7. ⏵  Render & commit
```

Each step has Markdown content with a **"Run command"** link that
fires the relevant extension command (e.g. step 1 runs
`projectXml.initProject`). Steps mark themselves complete based on
file state (e.g. step 1 = "Project.xml exists").

This is the IDE-native equivalent of the web form's Wizard panel and
serves the same "guide, don't just expose" goal.

### 5.7 Commands

Every interaction is a Command Palette entry under the
`Project Spec:` prefix:

| Command | Description |
| ------- | ----------- |
| `projectXml.initProject` | Run `render_doc.py --init` with prompted args. |
| `projectXml.lint` | Run linter; show findings in Problems panel. |
| `projectXml.renderAll` | Regenerate all five spec docs. |
| `projectXml.render` | Pick one spec doc, render & preview. |
| `projectXml.addHlr` | Open form to create a new `<hlr>`. |
| `projectXml.addLlr` | Open form to create a new `<llr>`. |
| `projectXml.addTest` | Open form to create a new `<test>` entry. |
| `projectXml.addModule` | Open form to create a new SDD `<module>`. |
| `projectXml.revealInXml` | Reveal current selection in `Project.xml`. |
| `projectXml.openSchemaReference` | Open `tools/Project_xml_README.md`. |
| `projectXml.openWebForm` | (Bridge) launch `make edit-doc` if installed. |
| `projectXml.ai.draft` | Draft a new payload item (HLR/LLR/test/module) with AI. |
| `projectXml.ai.expand` | Expand the selected item (e.g. add LLRs under an HLR). |
| `projectXml.ai.review` | Critique the selected item against PVD/SDD. |
| `projectXml.ai.suggestTraces` | Propose `<trace>` links for the selected item. |
| `projectXml.ai.gapFill` | Walk current lint warnings and propose fixes for each. |

### 5.8 Inline AI Assistance

The extension contributes a Copilot Chat participant **`@projectspec`**
plus context-menu "Draft with AI" / "Expand with AI" / "Review with AI"
actions on every payload type. The intent is that AI is a *co-author*
that already understands the schema, the existing payloads, and the
upstream documents — not a free-form text generator.

#### 5.8.1 Why a chat participant *and* commands?

*   **Commands** (right-click in tree view, code-lens links) are the
    fast path: one click, structured output, written straight to the
    XML through the same JSON-Patch pipeline used by the form
    webviews.
*   **Chat (`@projectspec`)** is the conversational path: iterate on
    wording, ask "why is HLR-007 not covered?", request a batch
    operation ("draft LLRs for every HLR in §3 that has none"), or
    triage the linter output.

Both share the same backend: a sidecar method
`project_io.ai_request(intent, context, target)` that

1.  Assembles the **grounding bundle** (see 5.8.2).
2.  Calls VS Code's Language Model API (`vscode.lm.selectChatModels`
    / `sendRequest`) — never the network directly.
3.  Receives a **typed JSON response** (see 5.8.3) constrained by a
    JSON Schema derived from `tools/project.xsd`.
4.  Validates the response against the XSD + lint, retries up to N
    times on failure (with the validator output fed back as a
    correction prompt), then either applies the JSON Patch or surfaces
    the failure to the user.

#### 5.8.2 Grounding bundle (what every prompt receives)

Every AI call is grounded in the project's own data so output is
consistent and traceable. The bundle is assembled by
`project_io.build_ai_context(target)`:

| Section | Always included | Conditionally included |
| ------- | --------------- | ---------------------- |
| Schema reference | The relevant subtree of `tools/project.xsd` | — |
| Author guidance | The relevant subtree of `tools/Project_xml_README.md` | — |
| PVD | First N kb of `doc/PVD.md` | — |
| SDD context | `<sdd>/<kind>`, `<audience>`, `<scope>` | Full `<module>` if drafting LLRs / tests for it |
| HLR context | List of `HLR-NNN: name` pairs (slim) | Full `<hlr>` if drafting LLRs / tests under it |
| LLR context | List of `LLR-XXX-NN: text` (slim) | Full `<llr>` if drafting tests for it |
| Existing items | Sibling items in the same scope (style anchor) | — |
| Lint state | Current findings on the target | — |
| User intent | The user's chat message or command argument | — |

Token budget is enforced by a deterministic packer that drops the
largest optional sections first, never the schema or the user intent.

#### 5.8.3 Typed responses (no free-form XML in chat)

The AI never emits XML directly. Each intent has a JSON response
schema and a deterministic translator that maps the JSON back to a
JSON Patch the sidecar can apply.

| Intent | Response schema (sketch) | Translator output |
| ------ | ------------------------ | ----------------- |
| `draft.hlr` | `{ section_number, name, text, traces:[{target,ref}] }` | `add` op into `<hlrs>/<section>/hlr` |
| `draft.llr` | `{ function_name, id_suffix, text, traces:[…] }` | `add` op into `<llrs>/<function>/llr` |
| `draft.test` | `{ file_path, name, purpose, traces:[…] }` | `add` op into `<tests>/<file>` |
| `draft.module` | `{ path, purpose, responsibilities:[…], functions:[…] }` | `add` op into `<sdd>/<modules>` |
| `expand.hlr_to_llrs` | `{ items:[ draft.llr, … ] }` | batched `add` ops |
| `review.item` | `{ severity, summary, suggestions:[ {field, rationale, replacement} ] }` | rendered as a Webview review pane; user clicks Apply per suggestion |
| `suggest.traces` | `{ traces:[ {target, ref, rationale} ] }` | adds `<trace>` rows; rationale shown in tooltip |
| `gap.fix` | `{ findings:[ {finding_id, action, payload} ] }` | one Quick Fix per finding |

Validation pipeline on every AI response:

1.  Parse JSON against the intent's response schema (Pydantic). On
    failure → automatic retry with the parser error fed back.
2.  Translate to JSON Patch.
3.  Apply patch to a *copy* of the tree.
4.  Run XSD + `lint_project.lint(tree)` on the result.
5.  If new errors appear that did not exist before, retry with the
    findings as feedback.
6.  After max retries, surface the original suggestion + the
    validation failures to the user instead of writing it.

This makes "the AI broke my Project.xml" structurally impossible —
the worst case is a rejected suggestion.

#### 5.8.4 Surfaces

*   **Right-click in the Project Spec tree view.**
    *   On `HLRs` node → "Draft new HLR with AI…"
    *   On an `<hlr>` leaf → "Expand: add LLRs", "Review against SDD",
        "Suggest tests"
    *   On `Coverage Gaps` node → "Walk gaps with AI" (opens chat with
        every gap pre-listed)
*   **Code Lens line above each item.**
    Adds an `🪄 AI` link → quick-pick of intents applicable to that
    item.
*   **Quick Fixes on lint diagnostics.**
    "Broken trace" diagnostic gets an extra "Suggest correct ref with
    AI" fix; "no test" gets "Draft a test with AI".
*   **Form webview footer.**
    A "Draft this field with AI" button next to long markdown fields
    (e.g. `<text>`, `<purpose>`, module bodies). Insert into the
    field, do not auto-save.
*   **Chat: `@projectspec` participant.** Examples:
    *   `@projectspec /draft-hlr we need to support reading from stdin`
    *   `@projectspec why does LLR-PCL-04 fail lint?`
    *   `@projectspec /expand HLR-007`
    *   `@projectspec /review SDD module src/vgp.c`
    *   `@projectspec /gap-fill --apply` (offers to walk every warning)
    The participant streams responses, supports follow-ups, and
    references items by id with clickable links into the editor.
*   **Walkthrough integration.** Each step's "Run command" link can
    optionally be "Run command with AI", which pre-drafts the section
    rather than opening an empty form.

#### 5.8.5 Trust, transparency, and undo

*   **Always preview before write.** AI-applied patches open a diff
    in the side, and a `Apply` / `Discard` lens. Optional setting
    `projectXml.ai.autoApplyValidated: false` (default) lets power
    users skip the diff for fully-validated suggestions.
*   **Provenance.** Each AI-authored element gets a tracked-changes
    marker stored in `.edit_doc/ai_history.jsonl` (timestamp, intent,
    model, prompt hash, accepted/rejected) so the team can audit
    AI-generated content. The XML itself is never tagged — Project.xml
    stays clean.
*   **Reversible.** AI writes go through the same backup mechanism as
    manual writes; "Undo last AI edit" is one click.
*   **Model selection.** Defer to whatever the user's Copilot Chat
    setup offers (`vscode.lm.selectChatModels({ vendor: 'copilot' })`).
    Settings expose `projectXml.ai.preferredModel` and
    `projectXml.ai.maxRetries`.
*   **Privacy.** Grounding bundles never include `.git`, `node_modules`,
    or any file outside `doc/` and `tools/templates/` by default. A
    setting `projectXml.ai.contextAllowList` lets users widen this.
*   **Disabled by default in untrusted workspaces.** The extension's
    `untrustedWorkspaces.supported` is `limited`; AI features off
    until the workspace is trusted.

## 6. Settings

```jsonc
{
  "projectXml.xmlPath":         "doc/Project.xml",
  "projectXml.xsdPath":         "tools/project.xsd",
  "projectXml.toolsDir":        "tools",
  "projectXml.pythonPath":      "",          // "" → workspace default
  "projectXml.previewOnSave":   true,
  "projectXml.warningsAsErrors": false,
  "projectXml.showCoverageBadges": true,
  "projectXml.autoLintOnChange": true,

  // --- AI assistance (Section 5.8) ---
  "projectXml.ai.enabled":           true,    // master switch
  "projectXml.ai.preferredModel":    "",      // "" → user's default
  "projectXml.ai.maxRetries":        2,       // validator-feedback retries
  "projectXml.ai.autoApplyValidated": false,  // diff-preview by default
  "projectXml.ai.maxContextTokens":  16000,   // grounding-bundle cap
  "projectXml.ai.contextAllowList":  ["doc/**", "tools/templates/**"],
  "projectXml.ai.logHistory":        true     // .edit_doc/ai_history.jsonl
}
```

## 7. Repository Layout

```
tools/vscode-project-xml/
  package.json                 # extension manifest (contributes points)
  README.md                    # marketplace listing
  CHANGELOG.md
  tsconfig.json
  esbuild.config.js
  src/
    extension.ts               # activate/deactivate, command registration
    sidecar.ts                 # JSON-RPC client to project_io.py
    treeView/
      ProjectSpecProvider.ts
      nodes.ts
    diagnostics/
      LintDiagnosticsProvider.ts
      QuickFixProvider.ts
    codeLens/
      ItemLensProvider.ts
    webview/
      ItemFormPanel.ts         # single-item form host
      formAssets/              # built RJSF bundle (Vite output)
    commands/
      init.ts
      lint.ts
      render.ts
      addHlr.ts
      ...
    ai/
      participant.ts          # @projectspec chat participant
      intents.ts              # intent registry + JSON Schemas
      grounding.ts            # context-bundle assembler (TS side)
      patchPipeline.ts        # validate → retry → apply → backup
      reviewPanel.ts          # webview for /review responses
    walkthrough/
      walkthroughs.ts
      content/
        01-bootstrap.md
        02-pvd.md
        ...
    util/
      paths.ts
      reveal.ts
  test/
    suite/
      activation.test.ts
      diagnostics.test.ts
      sidecar.test.ts
    fixtures/
      empty-project/
      vgp-project/
  .vscodeignore

tools/project_io.py            # new — JSON-RPC sidecar
tools/ai/                      # new — grounding + intent prompts
  __init__.py
  intents/
    draft_hlr.md               # system prompt + few-shot, per intent
    draft_llr.md
    draft_test.md
    draft_module.md
    expand_hlr_to_llrs.md
    review_item.md
    suggest_traces.md
    gap_fix.md
  schemas/                     # JSON Schema for each intent's response
    draft_hlr.json
    ...
  context.py                   # build_ai_context(target) -> bundle
  pipeline.py                  # validate → retry loop
```

## 8. Phased Delivery

### Phase 0 — Sidecar foundations
1.  Refactor `render_doc.py` and `lint_project.py` to expose
    importable functions (same prerequisite as the web-form plan).
2.  Build `tools/project_io.py` with `lint`, `render`,
    `parse_to_json`, `init_project` over stdio JSON-RPC.
3.  Acceptance: `echo '{"method":"lint"}' | python3 tools/project_io.py`
    returns the same findings as `tools/lint_project.py`.

### Phase 1 — Read-only extension
1.  Generate the manifest, declare `extensionDependencies` on the
    Red Hat XML extension.
2.  Tree View showing HLRs / LLRs / Tests with counts.
3.  Diagnostics provider wired to `projectXml.lint` command.
4.  Reveal-in-XML for each tree node.
5.  Acceptance: install extension in a VS Code instance, open this
    repo, see the Project Spec view populated and lint findings in
    the Problems panel.

### Phase 2 — Code Lenses + Render
1.  CodeLensProvider over `Project.xml`.
2.  `Render & Preview` command with markdown preview pane.
3.  `Render All` command.
4.  Acceptance: clicking a Code Lens link jumps to the correct LLR;
    saving the file (with `previewOnSave: true`) updates the preview.

### Phase 3 — Form webview for HLRs and LLRs
1.  Single-item form panel with RJSF.
2.  JSON Patch round-trip via sidecar.
3.  Add-new commands for HLR and LLR.
4.  Quick fixes for "broken trace" and "id format" diagnostics.
5.  Acceptance: add a new HLR via the form, see it appear in the
    XML, in the tree, and in the rendered HLRs.md preview.

### Phase 4 — SDD/STP/Test forms + Walkthrough
1.  Module/test/fixture form variants.
2.  Walkthrough contribution with all seven steps.
3.  Init-project command flow that drives `render_doc.py --init`.
4.  Acceptance: from an empty repo, a user can install the
    extension, run the Walkthrough, and end up with a populated
    Project.xml plus all five rendered spec docs.

### Phase 5 — Inline AI assistance
1.  Build `tools/ai/context.py` (grounding bundle assembler) and
    `tools/ai/pipeline.py` (validate → retry loop). Re-uses
    `lint_project.lint` and the XSD already in tree.
2.  Author per-intent prompt files in `tools/ai/intents/` and JSON
    Schemas in `tools/ai/schemas/`.
3.  Extend `project_io.py` with `ai_request(intent, context, target)`
    plus deterministic translators for each intent's response →
    JSON Patch.
4.  TypeScript side: `ai/participant.ts` registers the `@projectspec`
    chat participant via `vscode.chat.createChatParticipant`. Wire the
    `/draft-hlr`, `/expand`, `/review`, `/gap-fill` slash commands.
5.  Right-click "Draft / Expand / Review with AI" entries on tree
    nodes and code lenses. Quick Fix "Suggest correct ref with AI"
    on broken-trace diagnostics.
6.  Diff-preview-and-apply flow with backup + provenance log
    (`.edit_doc/ai_history.jsonl`).
7.  Settings UI for the `projectXml.ai.*` block; graceful disablement
    when no language model is available.
8.  Acceptance:
    *   `@projectspec /draft-hlr support reading from stdin` produces a
        schema-valid `<hlr>` with non-empty `<text>` and at least one
        plausible SDD trace; user accepts via diff and the entry
        appears in the tree, the rendered HLRs.md, and the lint passes.
    *   `/gap-fill --apply` walks every existing warning and proposes
        per-finding fixes that are validated before being offered.
    *   Disabling `projectXml.ai.enabled` removes every AI surface
        cleanly.

### Phase 6 — Polish
1.  Settings UI, status bar item showing `n errors / m warnings`.
2.  Marketplace listing assets (icon, screenshots, animated GIF).
3.  CI to publish `.vsix` on tag.

## 9. Risks & Open Questions

*   **Python discovery.** Need a robust strategy to find Python on
    Windows/macOS/Linux: defer to the Microsoft Python extension's
    interpreter API if installed, otherwise fall back to
    `python3`/`python`.
*   **XML editing is text-based.** When the form panel writes via
    sidecar while the XML is open in a dirty text editor, we must
    handle the conflict (prompt user, refuse save, or merge). VS Code
    has APIs for this; needs explicit design.
*   **Webview ↔ extension messaging.** Standard pattern; bundle
    overhead can be significant. Reuse the same RJSF bundle as the
    web-form plan to avoid double work.
*   **Schema regeneration.** The extension caches a JSON Schema
    snapshot derived from `tools/project.xsd`; need a version check so
    users don't see stale fields after a schema bump.
*   **Marketplace publishing.** Optional. The `.vsix` install path is
    sufficient for in-team distribution.
*   **AI: model availability.** `vscode.lm.selectChatModels` may return
    no models (no Copilot, no compatible extension). All AI surfaces
    must hide themselves cleanly and the rest of the extension must
    remain fully functional.
*   **AI: hallucinated trace refs.** The validator-feedback retry loop
    catches most cases, but a model that *consistently* invents ids
    can burn the retry budget. Mitigation: the typed-response schema
    asks for refs from a closed set passed in the context, and the
    pipeline rejects unknown refs before validation.
*   **AI: prompt size vs. cost/latency.** The deterministic packer
    enforces `projectXml.ai.maxContextTokens`. For huge projects we
    summarise lists ("45 HLRs, ids HLR-001..HLR-045, sample 5 below")
    rather than enumerating everything.
*   **AI: provenance and audit.** `.edit_doc/ai_history.jsonl` is
    intentionally separate from Project.xml so the spec stays clean,
    but it is the only record of which content originated from a
    model. Decide whether to gitignore or commit it (recommend: commit
    in regulated environments, gitignore otherwise).
*   **AI: privacy.** Grounding bundles are sent to whatever model the
    user selected. The `contextAllowList` setting and trusted-workspace
    gating exist so users can't accidentally exfiltrate `.env` files
    or unrelated source code through this path.

## 10. Estimated Effort

| Phase | Skill mix | Rough size |
| ----- | --------- | ---------- |
| 0 | Python | small |
| 1 | TypeScript + VS Code API | medium |
| 2 | TypeScript | small-medium |
| 3 | TS + React (webview) | medium |
| 4 | TS + Markdown content | medium |
| 5 | Python (prompts/schemas) + TS (chat API) | medium-large |
| 6 | Polish | small |

## 11. Out-of-Scope Follow-ups

*   **Codelens "Show in Traceability"** that opens
    `doc/Traceability.md` scrolled to the right anchor.
*   **CI integration.** A `Project Spec: Validate Pre-Push` task that
    fails the commit if `lint_project.py` reports errors. Already
    available via `make validate-xml`; the extension would just wire
    it to a Git hook.
*   **Bridge to the web form.** A command that boots the FastAPI
    server and opens it in the Simple Browser pane, for users who
    want the side-by-side preview without leaving VS Code.
