# Project.xml — Schema Reference

`doc/Project.xml` is the **single source of truth** for the project's
specification, design, and verification artefacts. This document
describes its schema. The companion document for AI agents is
[.github/skills/project-xml/SKILL.md](../.github/skills/project-xml/SKILL.md).

## Role

`Project.xml` exists for two reasons:

1.  **To document** the project. The five spec markdown documents in
    [doc/](../doc/) are *generated* from `Project.xml` via Jinja2
    templates in [templates/](templates/) and the renderer
    [render_doc.py](render_doc.py).
2.  **To measure traceability** across the four layers of the stack —
    SDD → HLRs → LLRs → Tests. Every relation lives in a `<traces>`
    block on the originating element, so a renderer can compose a
    forward or reverse traceability matrix without any other input.

The five generated documents are:

*   [doc/SDD.md](../doc/SDD.md) — Software Design Document
*   [doc/HLRs.md](../doc/HLRs.md) — High-Level Requirements
*   [doc/LLRs.md](../doc/LLRs.md) — Low-Level Requirements
*   [doc/STP.md](../doc/STP.md) — Software Test Plan
*   [doc/Traceability.md](../doc/Traceability.md) — Traceability Matrix

Sitting **above** the generated stack is the hand-authored
[doc/PVD.md](../doc/PVD.md) (Product Vision Document). It is *not*
generated; a starter template is provided at
[templates/PVD.md.template](templates/PVD.md.template).

Together with the per-test annotations under [test/](../test/), the
file holds enough structured information to regenerate any of the five
generated documents from a single edit point.

## 1. Root Element

```xml
<project name="Valgrind Parser" short_name="vgp" schema_version="1.1">
  <metadata>...</metadata>
  <sdd>...</sdd>
  <stp>...</stp>
  <hlrs>...</hlrs>
  <llrs>...</llrs>
  <tests>...</tests>
</project>
```

| Attribute | Description |
| --------- | ----------- |
| `name` | Full project name. |
| `short_name` | Binary / package name. |
| `schema_version` | Version of *this* schema. Bump when the structure changes incompatibly. The current schema is `1.1`. |

Children may appear in any order; the renderer looks them up by tag.

## 2. `<metadata>`

Document-level metadata for each generated spec, plus derived counts.

```xml
<metadata>
  <document id="SDD"          title="..." source="doc/SDD.md"          version="..." date="..." author="..."/>
  <document id="HLRs"         title="..." source="doc/HLRs.md"         version="..." date="..." author="..."/>
  <document id="LLRs"         title="..." source="doc/LLRs.md"         version="..." date="..." author="..."/>
  <document id="STP"          title="..." source="doc/STP.md"          version="..." date="..." author="..."/>
  <document id="Traceability" title="..." source="doc/Traceability.md" version="..." date="..." author="..."/>
  <counts>
    <count name="hlrs"       value="45"/>
    <count name="llrs"       value="124"/>
    <count name="tests"      value="120"/>
    <count name="test_files" value="6"/>
  </counts>
</metadata>
```

The renderer selects which `<document>` block populates
`project.metadata` based on its command-line `METADATA_ID` argument
(`SDD`, `HLRs`, `LLRs`, `STP`, or `Traceability`). Every generated
document must have a matching `<document id="...">` entry.

`<count>` values are derived (informational); the canonical counts come
from counting the corresponding child elements at render time.

## 3. `<sdd>` — Software Design Document Payload

The `<sdd>` element is a **data-only payload** consumed by
[templates/SDD.md.j2](templates/SDD.md.j2). All standard SDD scaffolding
(section numbers, standard headings such as "Purpose of the Document",
boilerplate lead-in sentences, and the auto-generated "Document
Overview") lives in the template, *not* in the data. This lets the same
template render an SDD for any project that supplies a similarly-shaped
payload.

```xml
<sdd>
  <kind>command-line application</kind>
  <audience>developers, testers, and maintainers</audience>
  <scope>...</scope>
  <overview>...</overview>
  <definitions>...</definitions>
  <references>...</references>
  <architecture>...</architecture>
  <design_goals>...</design_goals>
  <modules>...</modules>
  <data_dictionary>...</data_dictionary>
  <traceability>...</traceability>
</sdd>
```

All children are optional unless noted. Every text body marked as
**markdown** below is emitted verbatim by the template; wrap it in
`<![CDATA[...]]>` whenever it contains backticks, angle brackets, or
ampersands.

### 3.1 `<kind>` and `<audience>`

Plain-text strings used in the §1.1 "Purpose of the Document" sentence
("This document provides a detailed design for the … *kind*. It is
intended for *audience* of the … software.").

### 3.2 `<scope>` — §1.2

```xml
<scope>
  <intro>describes the design of the source modules ...</intro>
  <file path="src/main.c">Application entry point ...</file>
  <file path="src/vgp.c">Streaming parser ...</file>
  <outro><![CDATA[It does not exhaustively describe ...]]></outro>
</scope>
```

| Element / Attribute | Description |
| ------------------- | ----------- |
| `<intro>` | Lead-in sentence ("This document `<intro>`:"). |
| `<file path>` | One bullet per in-scope file. Body is markdown. |
| `<outro>` | Optional trailing paragraph after the file list. |

### 3.3 `<overview>` — §1.3

```xml
<overview>
  <para><![CDATA[Markdown paragraph...]]></para>
  <para><![CDATA[Another paragraph...]]></para>
</overview>
```

### 3.4 `<definitions>` — §1.4

```xml
<definitions>
  <term name="SDD">Software Design Document</term>
  <term name="ctags"><![CDATA[Universal Ctags — used at runtime ...]]></term>
</definitions>
```

### 3.5 `<references>` — §1.5

```xml
<references>
  <ref><![CDATA[Valgrind User Manual: <https://...>]]></ref>
</references>
```

Each `<ref>` body is one markdown bullet.

### 3.6 `<architecture>` — §2.1

```xml
<architecture>
  <intro><![CDATA[`vgp` is a single executable composed of ...]]></intro>
  <component path="src/main.c"><![CDATA[Acts as the controller ...]]></component>
  <component path="src/vgp.c"><![CDATA[Implements the streaming parser ...]]></component>
  <flow>
    <intro>The runtime data flow is:</intro>
    <step><![CDATA[`main()` parses argv ...]]></step>
    <step><![CDATA[...]]></step>
  </flow>
</architecture>
```

| Element | Description |
| ------- | ----------- |
| `<intro>` | Optional lead-in for §2.1. |
| `<component path>` | One bulleted component description. Body is markdown. |
| `<flow>` | Optional ordered numbered list. Contains `<intro>` and one or more `<step>` children. |

### 3.7 `<design_goals>` — §2.2

```xml
<design_goals>
  <goal name="Streaming"><![CDATA[Process logs line-by-line ...]]></goal>
</design_goals>
```

### 3.8 `<modules>` — §3..N

One `<module>` per design unit. Each renders as its own top-level
section ("Detailed Design for *path-or-title*").

```xml
<modules>
  <module path="src/main.c">
    <purpose><![CDATA[is the entry point for the `vgp` executable. ...]]></purpose>
    <responsibility><![CDATA[Define `main()`.]]></responsibility>
    <responsibility>...</responsibility>
    <interfaces title_suffix="(optional appended title text)">
      <prose><![CDATA[Optional free-form markdown above the interface list.]]></prose>
      <interface title="Command-Line Arguments"><![CDATA[markdown body]]></interface>
      <interface title="File System">...</interface>
    </interfaces>
    <data_structures><![CDATA[markdown body]]></data_structures>
    <functions>
      <intro>Optional lead-in.</intro>
      <function signature="void parse_command_line(int argc, char *argv[])"
                summary="optional one-line summary attribute">
        <purpose>Walk argv and populate the global app_config.</purpose>
        <pre>...</pre>
        <post>...</post>
        <returns>...</returns>
        <logic>
          <step><![CDATA[Iterate `argv[1..argc-1]`.]]></step>
        </logic>
        <notes>...</notes>
      </function>
      <group name="Helpers">
        <function signature="...">...</function>
      </group>
    </functions>
    <algorithm><![CDATA[markdown body]]></algorithm>
    <dependencies>
      <dep>...markdown bullet...</dep>
    </dependencies>
    <error_handling>
      <case name="Unknown option"><![CDATA[markdown body]]></case>
    </error_handling>
  </module>
</modules>
```

| `<module>` child | Renders as |
| ---------------- | ---------- |
| `<purpose>` | Lead sentence in §N.1 ("`<module>` *purpose-text*"). |
| `<responsibility>` (repeatable) | Bullet in §N.1. |
| `<interfaces>` | §N.2 "External Interfaces". Optional `title_suffix` attribute appends to the heading. Optional `<prose>` precedes the per-interface subsections. Each `<interface title>` becomes §N.2.k with the given title and verbatim markdown body. |
| `<data_structures>` | §N.3.1 "Key Data Structures". Body is markdown. |
| `<functions>` | §N.3.2 "Key Functions". Use bare `<function>` children for ungrouped entries; use `<group name="...">` to introduce a labelled subsection of related functions. |
| `<function>` verbose form | When `<purpose>` is present, the template emits a nested *Purpose / Pre-condition / Post-condition / Return Value / Logic (numbered) / Notes* bullet block. |
| `<function>` compact form | When `<purpose>` is absent, the template emits one bullet: "**`signature`** — *summary*", optional inline numbered logic steps, optional notes paragraph. The `summary` may be supplied as either an attribute or a `<summary>` child element. |
| `<algorithm>` | §N.3.3 "Parsing Strategy / Algorithm". |
| `<dependencies>` | §N.4 "Dependencies". Each `<dep>` child is one markdown bullet. |
| `<error_handling>` | §N.5 "Error Handling and Logging". Each `<case name>` becomes one bullet. |

A `<module>` with a `path` attribute renders its heading as
"Detailed Design for [path](../relpath)". A `<module>` with only a
`title` attribute renders as "Detailed Design for *title*".

### 3.9 `<data_dictionary>` — §N+1

```xml
<data_dictionary>
  <type name="AppConfig"
        header="inc/vgp.h"
        instance="app_config"
        instance_in="src/vgp.c"
        summary="Application-wide configuration ...">
    <field name="verbose"      type="bool" desc="-v flag"/>
    <field name="print_source" type="bool" desc="-s flag"/>
  </type>
  <constants header="inc/vgp.h">
    <constant name="MAX_LINE_LENGTH" value="4096" purpose="Bounded line buffer ..."/>
  </constants>
  <other><![CDATA[Optional trailing markdown for additional dictionary content.]]></other>
</data_dictionary>
```

`instance` and `instance_in` are optional; when present, the template
adds "instantiated as the global `<instance>` in [`<instance_in>`]".

### 3.10 `<traceability>` — §N+2

```xml
<traceability>
  <theme name="CLI parsing"               sections="§3.2.1, §3.3.2"/>
  <theme name="Streaming parser dispatch" sections="§4.4"/>
</traceability>
```

This drives the SDD's own narrative traceability table. The full
matrix used by [doc/Traceability.md](../doc/Traceability.md) is built
from the `<traces>` blocks on each `<hlr>`, `<llr>`, and `<test>`
element (see §8).

## 4. `<stp>` — Software Test Plan Payload

The `<stp>` element is the data-only payload consumed by
[templates/STP.md.j2](templates/STP.md.j2). It supplies the prose for
STP §1, §2, §5, §6, and §7. STP §3 (Test Catalogue) and §4 (LLR
Coverage Matrix) are computed by the template directly from `<tests>`
and `<llrs>`, not from `<stp>`.

```xml
<stp>
  <introduction>
    <purpose><![CDATA[markdown]]></purpose>
    <scope><![CDATA[markdown]]></scope>
    <related>
      <doc><![CDATA[[SDD.md](SDD.md) — design]]></doc>
      <doc>...</doc>
    </related>
  </introduction>

  <strategy>
    <levels>
      <level name="Unit"
             source="[test/unit/](../test/unit/)"
             driver="cmocka group runners"
             style="White-box, per-function"/>
      <level name="Integration"
             source="..."
             driver="..."
             style="..."/>
    </levels>
    <framework><![CDATA[markdown]]></framework>
    <build_execution>
      <intro><![CDATA[All tests are built and executed by `make test`. The target:]]></intro>
      <step><![CDATA[Step 1 markdown.]]></step>
      <step><![CDATA[Step 2 markdown.]]></step>
      <outro><![CDATA[Optional trailing paragraph.]]></outro>
    </build_execution>
    <pass_fail>
      <criterion><![CDATA[markdown bullet 1]]></criterion>
      <criterion><![CDATA[markdown bullet 2]]></criterion>
    </pass_fail>
    <traceability_convention><![CDATA[markdown]]></traceability_convention>
  </strategy>

  <integration_environment>
    <intro><![CDATA[markdown]]></intro>
    <fixture name="`c_error_generator`"
             source="[test/integration/c_error_generator.c](../test/integration/c_error_generator.c)">
      <artefact key="valgrind_log" label="Generated Valgrind Log"
                path="`build/.../valgrind.log`"/>
      <artefact key="vgp_output"   label="Generated vgp Output"
                path="`build/.../vgp_output.log`"/>
    </fixture>
    <fixture name="..." source="...">
      <artefact key="..." label="..." path="..."/>*
    </fixture>
    <outro><![CDATA[Optional trailing paragraph.]]></outro>
  </integration_environment>

  <tooling>
    <tool name="`gcc`"
          required_for="Build of `vgp` and unit tests"
          notes="C99, with -fanalyzer, ..."/>
    <tool name="..." required_for="..." notes="..."/>
  </tooling>

  <maintenance><![CDATA[markdown]]></maintenance>
</stp>
```

| Element | Renders as |
| ------- | ---------- |
| `<introduction>` | §1 — `<purpose>` (§1.1), `<scope>` (§1.2), `<related>` (§1.3 list of `<doc>` markdown links). |
| `<strategy>` | §2 — `<levels>` table, `<framework>` paragraph, `<build_execution>` numbered list (`<intro>` + `<step>`* + optional `<outro>`), `<pass_fail>` bulleted list, and `<traceability_convention>` paragraph. |
| `<integration_environment>` | §5 — `<intro>`, fixture table, `<outro>`. |
| `<tooling>` | §6 — one row per `<tool>`. |
| `<maintenance>` | §7 — verbatim markdown. |

### 4.1 Fixture Artefacts (schema 1.1+)

Each `<fixture>` carries a `name` and `source` plus zero or more
`<artefact key="..." label="..." path="..."/>` children. The renderer
collects the **union of artefact keys across all fixtures** in
first-seen order; the STP template emits one column per key with the
header taken from the first fixture that defined that key's `label`.
A fixture missing a given key renders an em-dash in that column.

This makes the fixture table extensible per-project: add new `key`s
freely without touching the template or renderer.

## 5. `<hlrs>` — High-Level Requirements

```xml
<hlrs>
  <section number="2" title="Command-Line Interface and Application Lifecycle">
    <intro><![CDATA[Optional intro paragraph that appears before the first HLR in the section.]]></intro>
    <hlr id="HLR-002" name="Argument Parsing and Validation">
      <text><![CDATA[The application shall parse `argv` and shall: ...]]></text>
      <traces>
        <trace target="SDD" ref="3.2.1"/>
        <trace target="SDD" ref="3.3.2"/>
      </traces>
    </hlr>
  </section>
</hlrs>
```

| Element / Attribute | Description |
| ------------------- | ----------- |
| `<section number title>` | One per H2 in `HLRs.md`. |
| `<intro>` | Optional markdown paragraph between the section heading and its first HLR. |
| `<hlr id name>` | One per HLR. `id` is `HLR-NNN`; `name` is the short title. |
| `<text>` | The HLR `shall` clause, as markdown. |
| `<trace target="SDD" ref="3.2.1"/>` | Each link from this HLR to an SDD section (dotted id). Repeatable. |

The HLRs template emits `<a id="HLR-NNN"></a>` immediately before the
bullet for every HLR so other documents (notably
[doc/Traceability.md](../doc/Traceability.md)) can link to it.

## 6. `<llrs>` — Low-Level Requirements

```xml
<llrs>
  <function number="4"
            title="`parse_command_line` ([src/main.c](../src/main.c))"
            name="parse_command_line"
            source="src/main.c">
    <intro><![CDATA[Optional intro paragraph.]]></intro>
    <llr id="LLR-PCL-04">
      <text><![CDATA[`parse_command_line` shall handle the `-h` flag ...]]></text>
      <traces>
        <trace target="HLR" ref="HLR-003" name="Usage and Help Information Display"/>
        <trace target="HLR" ref="HLR-009" name="Application Exit Status"/>
      </traces>
    </llr>
  </function>
</llrs>
```

| Element / Attribute | Description |
| ------------------- | ----------- |
| `<function number title name source>` | One per H2 in `LLRs.md`. `name` is the bare function name; `source` is the file the function lives in; `title` may be markdown (function name + linked source path). |
| `<intro>` | Optional markdown paragraph before the first LLR in the function's section. |
| `<llr id>` | One per LLR. `id` is `LLR-XXX-NN` (function prefix + 2-digit sequence). The XXX mnemonic style is a project convention, not a renderer requirement. |
| `<text>` | Body of the LLR `shall` clause. |
| `<trace target="HLR" ref="HLR-NNN" name="..."/>` | Each link to an HLR; `name` is the HLR's short name copied through for convenience. |

The LLRs template emits `<a id="LLR-XXX-NN"></a>` immediately before
the bullet for every LLR.

## 7. `<tests>` — Test Sources

```xml
<tests>
  <file path="test/unit/vgp_core.c" role="unit" count="79">
    <header><![CDATA[Optional file-level comment block, verbatim.]]></header>
    <test name="test_initialize_parse_state_normal">
      <purpose><![CDATA[Verifies LLR-IPS-03: every ParseState field is reset...]]></purpose>
      <traces>
        <trace target="LLR" ref="LLR-IPS-03"/>
        <trace target="HLR" ref="HLR-038"/>
      </traces>
    </test>
  </file>
</tests>
```

| Element / Attribute | Description |
| ------------------- | ----------- |
| `<file path role count>` | One per `*.c` file under `test/`. `role` is a **project-defined free-form string** (e.g. `unit`, `runner+integration`, `integration-fixture`); the renderer treats it as opaque metadata. `count` is the number of `<test>` children — informational. |
| `<header>` | Optional file-level doc-comment, verbatim. |
| `<test name>` | One per `static void test_*(void **state)` in the file. |
| `<purpose>` | Doc-comment block immediately above the test, normalised to plain text. |
| `<trace target="LLR"\|"HLR" ref="..."/>` | Each link the test claims, taken from the IDs cited in its doc-comment. |

The STP template emits `<a id="<test_name>"></a>` in the catalogue row
for every test so other documents can link to individual tests.

## 8. Traceability Discovery

A renderer building the traceability matrix does **not** need any
input besides the `<traces>` blocks:

*   **SDD → HLR** — for each `<hlr>`, its `<trace target="SDD">`
    children name the SDD sections it implements.
*   **HLR → LLR** — for each `<llr>`, its `<trace target="HLR">`
    children name the HLRs it implements.
*   **LLR → Test** — for each `<test>`, its `<trace target="LLR">`
    children name the LLRs it verifies.
*   **HLR → Test (direct)** — `<test>`'s `<trace target="HLR">`
    children are direct HLR claims (used when a test verifies an HLR
    that has no specific bound LLR).

`render_doc.py` builds these relations once at load time and exposes
them on the `project` namespace so templates do not have to recompute
them. See **§9 Renderer Data Surface** below.

A complete forward matrix is built by composing the four relations. A
reverse matrix is built by inverting them. Coverage gaps (LLRs / HLRs
without any verifying test) are inferred from the same data and
reported in
[doc/Traceability.md §6](../doc/Traceability.md#6-coverage-gaps).

## 9. Renderer Data Surface

Templates receive a single `project` namespace. The values below are
all populated by [render_doc.py](render_doc.py); use them directly
rather than recomputing relations in Jinja.

| Name | Description |
| ---- | ----------- |
| `project.name`, `project.short_name`, `project.schema_version` | From the `<project>` root element. |
| `project.metadata` | The `<document>` block selected by the CLI `METADATA_ID` argument. |
| `project.counts` | Dict of `<count name=…>` values from `<metadata>`. |
| `project.sdd` | `<sdd>` payload as a nested namespace. |
| `project.stp` | `<stp>` payload as a nested namespace. |
| `project.hlrs` | List of `<section>` namespaces, each with `.hlrs[]`. |
| `project.llrs` | List of `<function>` group namespaces, each with `.llrs[]`. |
| `project.tests` | List of `<file>` namespaces, each with `.tests[]`. |
| `project.flat_hlrs` | Every HLR in declaration order, annotated with `section_number` and `section_title`. |
| `project.flat_llrs` | Every LLR in declaration order, annotated with `function_name` and `function_number`. |
| `project.flat_tests` | Every test, sorted by (file, name), each with `.file`, `.name`, `.purpose`, `.traces`. |
| `project.hlr_by_id`, `project.llr_by_id` | ID → namespace lookup. |
| `project.file_of_test` | Test name → owning source-file path. |
| `project.sdd_titles` | SDD section number → heading title; mirrors the SDD template's numbering scheme (used for SDD-anchor links). |
| `project.hlrs_by_sdd[ref]` | List of HLR ids that cite the SDD section `ref`. |
| `project.llrs_by_hlr[hid]` | List of LLR ids that implement HLR `hid`. |
| `project.tests_by_llr[lid]` | List of test names that verify LLR `lid`. |
| `project.tests_by_hlr[hid]` | List of test names that verify HLR `hid` directly. |
| `project.llrs_no_test` | LLRs with no `<test>` citing them. |
| `project.hlrs_no_test` | HLRs with neither a direct test nor any LLR-bound test in their chain. |

Custom Jinja filters registered by the renderer:

| Filter | Purpose |
| ------ | ------- |
| `gh_slug` | Approximates GitHub's heading-anchor slug rule (lowercase, strip punctuation, spaces → hyphens). Used by the Traceability template to link to SDD section headings. |

When extending the schema with a new payload root, add a corresponding
`build_*` function in `render_doc.py` and expose it on the returned
`SimpleNamespace` so templates can reach it by attribute access.

## 10. Regeneration

The XML is the canonical source. After modifying any input, regenerate
the dependent markdown documents:

```bash
python3 tools/render_doc.py tools/templates/SDD.md.j2          SDD          --out doc/SDD.md
python3 tools/render_doc.py tools/templates/HLRs.md.j2         HLRs         --out doc/HLRs.md
python3 tools/render_doc.py tools/templates/LLRs.md.j2         LLRs         --out doc/LLRs.md
python3 tools/render_doc.py tools/templates/STP.md.j2          STP          --out doc/STP.md
python3 tools/render_doc.py tools/templates/Traceability.md.j2 Traceability --out doc/Traceability.md
```

Run `python3 tools/render_doc.py --help` for the full command-line
reference.

### 10.1 Bootstrapping a new project

For a brand-new project the renderer can create the initial
`doc/Project.xml` skeleton plus a substituted `doc/PVD.md`:

```bash
python3 tools/render_doc.py --init \
    --name "My Product" --short-name myprod --author "Jane Doe"
```

This writes `doc/Project.xml` (a valid schema-1.1 skeleton with empty
`<sdd>`, `<stp>`, `<hlrs>`, `<llrs>`, `<tests>` payloads and a fully
populated `<metadata>` block) and `doc/PVD.md` (the
[templates/PVD.md.template](templates/PVD.md.template) with the title,
short name, date, and author placeholders substituted). Both files are
refused if they already exist; pass `--force` to overwrite. From there
the workflow is: edit `doc/PVD.md`, then start populating
`doc/Project.xml` and regenerating the five spec documents above.

### 10.2 Validation

Two validators ship alongside the schema:

*   **[tools/project.xsd](project.xsd)** — XML Schema describing the
    structural shape (element nesting, attribute names and required-ness,
    `HLR-NNN` / `LLR-XXX-NN` id formats, trace target enum). The
    canonical `doc/Project.xml` references it via
    `xsi:noNamespaceSchemaLocation="../tools/project.xsd"`, so any
    XSD-aware editor (VS Code's Red Hat XML extension, IntelliJ,
    oXygen) provides autocomplete and inline error squigglies for free.
*   **[tools/lint_project.py](lint_project.py)** — semantic checker
    that resolves every `<trace>` against the actual HLR/LLR/test ids,
    flags duplicates, warns about coverage gaps (LLRs/HLRs with no
    verifying test), and validates against the XSD when `lxml` or
    `xmllint` is available.

Run both via the Make target:

```bash
make validate-xml
```

Or directly:

```bash
python3 tools/lint_project.py                # all checks, warnings on
python3 tools/lint_project.py --no-warnings  # errors only (for CI)
```

Exit status is non-zero only on **errors** (broken trace refs, bad id
formats, duplicate ids, malformed XML, XSD violations). Coverage gaps
are warnings. The simplest round-trip parse check remains:

```python
import xml.etree.ElementTree as ET
ET.parse("doc/Project.xml")  # raises on malformed XML
```

## 11. Editing and Authoring Notes

*   **CDATA content is markdown.** Templates that emit markdown pass it
    through verbatim. Templates that emit other formats (HTML, PDF,
    DOCX) should run the CDATA through a markdown renderer.
*   **Use CDATA defensively.** Any body containing backticks, angle
    brackets, ampersands, or square brackets that *might* look like
    XML or entity markup should be wrapped in `<![CDATA[...]]>`.
*   **Indentation is meaningful** in the markdown payloads of `<text>`
    and `<intro>` only insofar as markdown itself uses indentation
    (for sub-bullets, code blocks, etc.). The renderer strips the
    four-space requirement-list indentation that `HLRs.md` and
    `LLRs.md` use, so a template can re-add that indentation when
    emitting bullet items.
*   **IDs are stable.** `HLR-NNN` and `LLR-XXX-NN` identifiers are
    contracts; do not renumber them when re-rendering.
*   **Order is preserved.** `<section>` / `<hlr>` / `<llr>` / `<test>`
    children appear in the same order as their source files. Templates
    iterate in document order.
*   **Headings come from the template, never the data.** Do not put
    section numbers, `##` headings, or "Document Overview" content into
    the `<sdd>` or `<stp>` payloads. Adding either to the data will
    produce duplicate or mis-numbered headings in the rendered
    document.
*   **Anchors come from the template, never the data.** The HLRs,
    LLRs, and STP templates emit `<a id="...">` for every requirement
    and test name. The Traceability template links to those anchors;
    do not remove them when editing the templates.
*   **Bump `schema_version`** on the `<project>` root whenever you
    change the structure of `Project.xml` in a way the existing
    templates and `tools/render_doc.py` could not consume unchanged.
