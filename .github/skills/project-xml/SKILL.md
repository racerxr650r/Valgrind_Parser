---
name: project-xml
description: "Use when editing, regenerating, or interpreting any of the project's spec documents (PVD, SDD, HLRs, LLRs, Test Plan, Traceability) or their generator inputs. doc/Project.xml is the SINGLE SOURCE OF TRUTH for the project's design, requirements, and verification — and the canonical store from which traceability (SDD→HLR→LLR→Test) is measured. doc/PVD.md is the hand-authored Product Vision Document that sits above the generated stack; the AI is expected to draft and fill in PVD content under the developer's direction, asking targeted questions when sections are thin or missing. USE FOR: any change to doc/PVD.md, doc/SDD.md, doc/HLRs.md, doc/LLRs.md, doc/STP.md, doc/Traceability.md; drafting or revising the Product Vision Document; adding/removing/renumbering HLRs or LLRs; adding new tests under test/ that need traceability annotations; updating Project.xml schema; authoring new Jinja2 templates under tools/templates/; running tools/render_doc.py. DO NOT USE FOR: edits to source code under src/ that do not change a documented behaviour, design element, or requirement; routine build/test/coverage work; non-doc tooling under tools/ that is unrelated to render_doc.py."
---

# Project.xml — Source of Truth for Design, Requirements, and Traceability

## Role

`doc/Project.xml` is the **single source of truth** for this project's
specification, design, and verification artefacts. It serves two
purposes:

1.  **Documenting the project.** The four spec markdown documents are
    *generated* from `Project.xml` by Jinja2 templates in
    [tools/templates/](../../../tools/templates/) via
    [tools/render_doc.py](../../../tools/render_doc.py). Editing the
    generated `.md` files directly is incorrect — changes will be
    overwritten on the next render.
2.  **Measuring traceability.** Every `<hlr>`, `<llr>`, and `<test>`
    element carries a `<traces>` block linking it upstream. The
    SDD→HLR→LLR→Test relations are computed from those blocks, and
    coverage gaps are reported in
    [doc/Traceability.md](../../../doc/Traceability.md).

The canonical schema reference for humans is
[tools/Project_xml_README.md](../../../tools/Project_xml_README.md).
**Read that file before performing any non-trivial edit to
`Project.xml` or any of its templates.**

Sitting **above** the generated stack is the hand-authored
[doc/PVD.md](../../../doc/PVD.md) — see [Product Vision
Document](#product-vision-document-pvd) below.

## Product Vision Document (PVD)

[doc/PVD.md](../../../doc/PVD.md) is the **Product Vision Document**.
Unlike the SDD/HLRs/LLRs/STP/Traceability documents, the PVD is
**hand-authored** — it is *not* generated from `Project.xml` and has
no template under `tools/templates/`. It defines *why* the product
exists, *who* it is for, and *how success is measured*, and the rest
of the specification stack must remain aligned with it.

### AI Role for the PVD

The **developer is the author** of the PVD. The **AI is the
ghostwriter**: the AI is expected to draft, expand, and revise the
actual prose of the document under the developer's direction. The
developer supplies intent, constraints, and approval; the AI supplies
the words.

When asked to work on the PVD:

1.  **Read [doc/PVD.md](../../../doc/PVD.md) first** to see what
    sections already exist and how complete each one is.
2.  **Identify thin or missing sections.** A section is "thin" if it
    is a placeholder, a single sentence where the structure expects a
    paragraph, a TODO marker, or a list with one item where multiple
    are expected.
3.  **Ask the developer targeted questions** to elicit the missing
    content before drafting it. Prefer a small number of specific,
    answerable questions over open-ended prompts. Examples (templates
    — substitute the actual section/field names from this project):
    *   "Section *<N>* (*<Section Title>*) currently lists only
        *<single item>*. Should I add other *<category>* entries, or
        is the list intentionally minimal?"
    *   "Section *<N>* (*<Section Title>*) has no *<metric / field /
        criterion>* for *<topic>*. Do you want a target value, or
        should I leave it out?"
    *   "Section *<N>* (*<Section Title>*) mentions *<broad theme>*
        — should I name a specific *<technology / standard /
        deliverable>* explicitly?"
4.  **Draft the content** once the developer has answered. Match the
    existing tone, heading depth, and table style of
    [doc/PVD.md](../../../doc/PVD.md). Keep the AI's additions
    consistent with the project facts already documented in
    [doc/SDD.md](../../../doc/SDD.md) and [README.md](../../../README.md).
5.  **Do not invent product direction.** If a question is genuinely
    open (scope, target users, success metrics, roadmap), ask — do
    not guess. The AI fills in *content*; the developer decides
    *direction*.

### Expected PVD Sections

The PVD should normally contain, in roughly this order:

*   Vision Statement
*   Problem Statement
*   Target Users (with personas)
*   Value Proposition
*   Product Principles (the tie-breakers when requirements conflict)
*   Scope (in scope / out of scope / non-goals)
*   Success Metrics
*   Roadmap Themes
*   Relationship to the rest of the spec stack
    (PVD → HLRs → LLRs → SDD → STP → Traceability)

If any of these are absent or underspecified, that is the AI's cue
to ask a clarifying question and then draft the section.

### What the PVD Is *Not*

*   It is **not** a requirements document — keep specific requirements
    in [doc/HLRs.md](../../../doc/HLRs.md) (via `Project.xml`).
*   It is **not** a design document — keep architecture and modules
    in [doc/SDD.md](../../../doc/SDD.md) (via `Project.xml`).
*   It is **not** a release plan — keep dated commitments out; the
    *Roadmap Themes* section names directions, not delivery dates.

## Generated Documents

| Generated file | Source | Template |
| -------------- | ------ | -------- |
| [doc/SDD.md](../../../doc/SDD.md) | `<sdd>` payload + `<metadata id="SDD">` | `tools/templates/SDD.md.j2` |
| [doc/HLRs.md](../../../doc/HLRs.md) | `<hlrs>` + `<metadata id="HLRs">` | `tools/templates/HLRs.md.j2` |
| [doc/LLRs.md](../../../doc/LLRs.md) | `<llrs>` + `<metadata id="LLRs">` | `tools/templates/LLRs.md.j2` |
| [doc/STP.md](../../../doc/STP.md) | `<stp>` + `<tests>` + `<llrs>` + `<metadata id="STP">` | `tools/templates/STP.md.j2` |
| [doc/Traceability.md](../../../doc/Traceability.md) | All `<traces>` + `<metadata id="Traceability">` | `tools/templates/Traceability.md.j2` |

Every generated document must have a corresponding
`<metadata><document id="..."/></metadata>` entry — the renderer
looks the document up by the second positional CLI argument.

## Decision Flow

| User intent | Action |
| ----------- | ------ |
| "Update the SDD section about X" | Edit the matching node inside `<sdd>` in `Project.xml`, then regenerate `doc/SDD.md`. |
| "Add an HLR" | Append a new `<hlr id="HLR-NNN" name="...">` inside the appropriate `<hlrs>/<section>`; add `<traces target="SDD" ref="...">` for every SDD section it implements. Regenerate `HLRs.md` and `Traceability.md`. |
| "Add an LLR" | Append a new `<llr id="LLR-XXX-NN">` inside the matching `<llrs>/<function>`; add `<traces target="HLR" ref="HLR-NNN" name="...">` for every HLR it implements. Regenerate `LLRs.md` and `Traceability.md`. |
| "Add a test" | Add the `static void test_*(void **state)` under `test/` *with a doc-comment block citing `LLR-XXX-NN` and/or `HLR-NNN`*. Then add a matching `<test name="...">` (with `<purpose>` and `<traces>`) inside the appropriate `<tests>/<file>`. Regenerate `STP.md` and `Traceability.md`. |
| "Why is HLR-NNN / LLR-XXX-NN listed as having no test?" | It has no `<test>` whose `<traces>` cite it. Either add a test, or document the gap in `Traceability.md §6.x` (the renderer does this from the data). |
| "Add a new field to a section that doesn't exist yet" | First update the schema (`tools/Project_xml_README.md` + `Project.xml` + bump `schema_version`), then update the relevant template under `tools/templates/`, then regenerate. |

## Hard Rules

*   **Never edit `doc/SDD.md`, `doc/HLRs.md`, `doc/LLRs.md`,
    `doc/STP.md`, or `doc/Traceability.md` by hand.** They are
    generated. Edit `Project.xml` (and/or the relevant template),
    then run the renderer.
*   **Never put section numbers or H2/H3 headings inside the `<sdd>`
    or `<stp>` payloads.** All scaffolding (H1 title, version block,
    section numbering, table headers) lives in the templates. Payload
    elements carry content only.
*   **HLR/LLR/test anchors are emitted by the templates.** HLRs.md,
    LLRs.md, and STP.md each emit `<a id="..."></a>` immediately
    before the bullet/row for every `HLR-NNN`, `LLR-XXX-NN`, and
    `test_*` name. The Traceability template links to those anchors;
    do not remove them when editing the templates.
*   **`HLR-NNN` and `LLR-XXX-NN` IDs are stable contracts.** Do not
    renumber existing IDs. Allocate the next free number when adding.
*   **Every `<hlr>` should have at least one `<trace target="SDD">`.**
    Every `<llr>` should have at least one `<trace target="HLR">`.
    Every `<test>` should have at least one `<trace target="LLR">` or
    `<trace target="HLR">`. Missing traces produce coverage-gap rows.
*   **Wrap any body containing backticks, angle brackets, ampersands,
    or `[markdown links]` in `<![CDATA[...]]>`.** Bare markdown bodies
    will silently corrupt the XML when they contain `<` or `&`.
*   **Bump `<project schema_version="...">`** whenever you change the
    structure of `Project.xml` in a way that existing templates or
    `tools/render_doc.py` could not consume unchanged.

## Schema Quick Reference

```xml
<project name="..." short_name="..." schema_version="1.0">
  <metadata>
    <document id="SDD|HLRs|LLRs|STP|Traceability" title="..."
              source="..." version="..." date="..." author="..."/>
    <counts><count name="..." value="..."/></counts>
  </metadata>

  <sdd>
    <kind>...</kind>
    <audience>...</audience>
    <scope>           <intro/> <file path="..."/>* <outro/> </scope>
    <overview>        <para/>* </overview>
    <definitions>     <term name="..."/>* </definitions>
    <references>      <ref/>* </references>
    <architecture>    <intro/> <component path="..."/>* <flow/> </architecture>
    <design_goals>    <goal name="..."/>* </design_goals>
    <modules>
      <module path="...">
        <purpose/> <responsibility/>*
        <interfaces title_suffix="...">
          <prose/> <interface title="..."/>*
        </interfaces>
        <data_structures/>
        <functions>
          <intro/>
          <function signature="..." summary="...">
            <purpose/> <pre/> <post/> <returns/>
            <logic><step/>*</logic>
            <notes/>
          </function>*
          <group name="..."> <function/>* </group>*
        </functions>
        <algorithm/>
        <dependencies><dep/>*</dependencies>
        <error_handling><case name="..."/>*</error_handling>
      </module>*
    </modules>
    <data_dictionary>
      <type name="..." header="..." instance="..." instance_in="..." summary="...">
        <field name="..." type="..." desc="..."/>*
      </type>*
      <constants header="...">
        <constant name="..." value="..." purpose="..."/>*
      </constants>
      <other/>
    </data_dictionary>
    <traceability><theme name="..." sections="..."/>*</traceability>
  </sdd>

  <stp>
    <introduction>
      <purpose/> <scope/>
      <related><doc/>*</related>
    </introduction>
    <strategy>
      <levels><level name="..." source="..." driver="..." style="..."/>*</levels>
      <framework/>
      <build_execution><intro/> <step/>* <outro/></build_execution>
      <pass_fail><criterion/>*</pass_fail>
      <traceability_convention/>
    </strategy>
    <integration_environment>
      <intro/>
      <fixture name="..." source="...">
        <artefact key="..." label="..." path="..."/>*
      </fixture>*
      <outro/>
    </integration_environment>
    <tooling><tool name="..." required_for="..." notes="..."/>*</tooling>
    <maintenance/>
  </stp>
  <!-- STP §3 (Test Catalogue) and §4 (LLR Coverage Matrix) are
       computed by the template from <tests> and <llrs>, not <stp>. -->

  <hlrs>
    <section number="..." title="...">
      <intro/>
      <hlr id="HLR-NNN" name="...">
        <text/>
        <traces><trace target="SDD" ref="..."/>*</traces>
      </hlr>*
    </section>*
  </hlrs>

  <llrs>
    <function number="..." title="..." name="..." source="...">
      <intro/>
      <llr id="LLR-XXX-NN">
        <text/>
        <traces><trace target="HLR" ref="HLR-NNN" name="..."/>*</traces>
      </llr>*
    </function>*
  </llrs>

  <tests>
    <file path="..." role="..." count="N">
      <header/>
      <test name="test_...">
        <purpose/>
        <traces><trace target="LLR|HLR" ref="..."/>*</traces>
      </test>*
    </file>*
  </tests>
</project>
```

The `role` attribute on `<file>` is a free-form, project-defined
string (e.g. `unit`, `integration`, `runner+integration`,
`integration-fixture`). The renderer treats it as opaque metadata;
adopt whatever vocabulary fits the project's test layout and use it
consistently.

See [tools/Project_xml_README.md](../../../tools/Project_xml_README.md)
for the full element-by-element semantics, attribute meanings, and
authoring conventions.

## Regeneration

After any edit to `Project.xml` or a template:

```bash
python3 tools/render_doc.py tools/templates/SDD.md.j2          SDD          --out doc/SDD.md
python3 tools/render_doc.py tools/templates/HLRs.md.j2         HLRs         --out doc/HLRs.md
python3 tools/render_doc.py tools/templates/LLRs.md.j2         LLRs         --out doc/LLRs.md
python3 tools/render_doc.py tools/templates/STP.md.j2          STP          --out doc/STP.md
python3 tools/render_doc.py tools/templates/Traceability.md.j2 Traceability --out doc/Traceability.md
```

### Renderer Data Surface

[tools/render_doc.py](../../../tools/render_doc.py) exposes the
following on the `project` namespace passed to every template — use
these rather than recomputing relations in Jinja:

*   **Payload roots:** `project.sdd`, `project.stp`, `project.hlrs`
    (sections), `project.llrs` (function groups), `project.tests`
    (test files), `project.metadata`, `project.counts`,
    `project.name`, `project.short_name`, `project.schema_version`.
*   **Flat lists:** `project.flat_hlrs` (each annotated with
    `section_number`/`section_title`), `project.flat_llrs` (each
    annotated with `function_name`/`function_number`),
    `project.flat_tests` (sorted by file then name, each with
    `file`, `name`, `purpose`, `traces`).
*   **ID lookups:** `project.hlr_by_id`, `project.llr_by_id`,
    `project.file_of_test`, `project.sdd_titles` (SDD section number
    → heading title, mirroring the SDD template's numbering scheme).
*   **Cross-reference indexes** (built once from every `<traces>`
    block):
    *   `project.hlrs_by_sdd[ref]` → list of HLR ids
    *   `project.llrs_by_hlr[hid]` → list of LLR ids
    *   `project.tests_by_hlr[hid]` → list of test names
    *   `project.tests_by_llr[lid]` → list of test names
*   **Coverage gaps:** `project.llrs_no_test`, `project.hlrs_no_test`
    (HLRs with neither a direct test nor any LLR-bound test).
*   **Filters:** `gh_slug` — approximates GitHub's heading-anchor
    slug rule (lowercase, strip punctuation, spaces → hyphens). Used
    by the Traceability template to link into SDD section headings.

When extending the schema with a new payload root, add the
corresponding `build_*` function in `render_doc.py` and expose it on
the returned `SimpleNamespace` so templates can reach it by attribute
access.

Validate the XML before regenerating:

```bash
python3 -c "import xml.etree.ElementTree as ET; ET.parse('doc/Project.xml')"
```

## Common Pitfalls

*   **Editing the generated `.md` instead of `Project.xml`.** Always
    grep for the prose you're about to change inside `Project.xml`
    first — if it's there, the `.md` is generated.
*   **Adding a test without a doc-comment trace block.** The test will
    not appear in `STP.md` / `Traceability.md` until a matching
    `<test>` entry is added to `Project.xml`. Both must be updated.
*   **Forgetting the `<traces>` block.** A new HLR/LLR/test with no
    traces becomes an immediate coverage gap.
*   **Putting `##` headings inside `<sdd>` bodies.** The template emits
    its own headings; duplicate or mis-numbered output is the symptom.
*   **Unescaped `<`/`&`/backticks in a body.** Wrap in `<![CDATA[...]]>`.
*   **Stale `<count>` values in `<metadata>/<counts>`.** They are
    informational; the canonical counts come from counting child
    elements at render time. Update them when convenient but do not
    rely on them.
