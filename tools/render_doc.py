#!/usr/bin/env python3
"""
Render a Markdown document from a Jinja2 template plus the data in
doc/Project.xml.

Usage:
    python3 tools/render_doc.py tools/templates/SDD.md.j2 SDD > out/SDD.md

The second argument is the document id from <metadata>/<document id="...">
(SDD, HLRs, LLRs, or STP); it picks which document's metadata gets
exposed to the template as `project.metadata`. The template controls
everything else.
"""
from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from types import SimpleNamespace
from typing import Any

import jinja2

PROJECT_XML = Path(__file__).resolve().parent.parent / "doc" / "Project.xml"


def _attrs(elem: ET.Element) -> dict[str, str]:
    return dict(elem.attrib)


def _text(elem: ET.Element | None) -> str:
    if elem is None or elem.text is None:
        return ""
    return elem.text


def _gh_slug(text: str) -> str:
    """Approximate GitHub's heading-anchor slug rule.

    GitHub lowercases the heading, strips characters other than letters,
    digits, spaces and hyphens, then replaces spaces with hyphens. Used
    by the Traceability template to link to SDD section headings.
    """
    import re
    s = text.strip().lower()
    s = re.sub(r"[^\w\s-]", "", s, flags=re.UNICODE)
    s = re.sub(r"\s+", "-", s)
    return s


# --------------------------------------------------------------------- #
# Convert each branch of the XML tree into SimpleNamespace objects so   #
# templates can use attribute access (section.title) instead of XML     #
# method calls. Templates stay readable for non-Python authors.          #
# --------------------------------------------------------------------- #
def _children_text(elem: ET.Element, tag: str) -> list[str]:
    """Collect the .text of every direct child with the given tag."""
    return [_text(c) for c in elem.findall(tag)]


def build_function(elem: ET.Element) -> SimpleNamespace:
    """A <function signature [summary]> with optional verbose children."""
    logic_elem = elem.find("logic")
    return SimpleNamespace(
        signature=elem.get("signature", ""),
        summary=elem.get("summary", "") or _text(elem.find("summary")),
        purpose=_text(elem.find("purpose")),
        pre=_text(elem.find("pre")),
        post=_text(elem.find("post")),
        returns=_text(elem.find("returns")),
        logic=_children_text(logic_elem, "step") if logic_elem is not None else [],
        notes=_text(elem.find("notes")),
    )


def build_functions(elem: ET.Element | None) -> SimpleNamespace | None:
    if elem is None:
        return None
    flat = [build_function(f) for f in elem.findall("function")]
    groups = [
        SimpleNamespace(
            name=g.get("name", ""),
            functions=[build_function(f) for f in g.findall("function")],
        )
        for g in elem.findall("group")
    ]
    return SimpleNamespace(
        intro=_text(elem.find("intro")),
        flat=flat,
        groups=groups,
    )


def build_interfaces(elem: ET.Element | None) -> SimpleNamespace | None:
    if elem is None:
        return None
    return SimpleNamespace(
        title_suffix=elem.get("title_suffix", ""),
        intro=_text(elem.find("intro")),
        prose=_text(elem.find("prose")),
        items=[
            SimpleNamespace(title=i.get("title", ""), body=_text(i))
            for i in elem.findall("interface")
        ],
    )


def build_named_body_list(parent: ET.Element | None, child_tag: str) -> list[SimpleNamespace]:
    if parent is None:
        return []
    return [
        SimpleNamespace(name=c.get("name", ""), body=_text(c))
        for c in parent.findall(child_tag)
    ]


def build_module(elem: ET.Element) -> SimpleNamespace:
    return SimpleNamespace(
        path=elem.get("path", ""),
        title=elem.get("title", ""),
        purpose=_text(elem.find("purpose")),
        responsibilities=_children_text(elem, "responsibility"),
        interfaces=build_interfaces(elem.find("interfaces")),
        data_structures=_text(elem.find("data_structures")),
        functions=build_functions(elem.find("functions")),
        algorithm=_text(elem.find("algorithm")),
        dependencies=[
            _text(d) for d in (elem.find("dependencies") or [])
        ] if elem.find("dependencies") is not None else [],
        error_handling=build_named_body_list(elem.find("error_handling"), "case"),
    )


def build_data_dictionary(elem: ET.Element | None) -> SimpleNamespace | None:
    if elem is None:
        return None
    types = []
    for t in elem.findall("type"):
        types.append(SimpleNamespace(
            name=t.get("name", ""),
            header=t.get("header", ""),
            instance=t.get("instance", ""),
            instance_in=t.get("instance_in", ""),
            summary=t.get("summary", ""),
            fields=[
                SimpleNamespace(
                    name=f.get("name", ""),
                    type=f.get("type", ""),
                    desc=f.get("desc", ""),
                )
                for f in t.findall("field")
            ],
        ))
    consts_elem = elem.find("constants")
    constants = None
    if consts_elem is not None:
        constants = SimpleNamespace(
            header=consts_elem.get("header", ""),
            items=[
                SimpleNamespace(
                    name=c.get("name", ""),
                    value=c.get("value", ""),
                    purpose=c.get("purpose", ""),
                )
                for c in consts_elem.findall("constant")
            ],
        )
    return SimpleNamespace(
        types=types,
        constants=constants,
        other=_text(elem.find("other")),
    )


def build_sdd(elem: ET.Element | None) -> SimpleNamespace | None:
    if elem is None:
        return None
    scope_elem = elem.find("scope")
    scope = None
    if scope_elem is not None:
        scope = SimpleNamespace(
            intro=_text(scope_elem.find("intro")),
            files=[
                SimpleNamespace(path=f.get("path", ""), body=_text(f))
                for f in scope_elem.findall("file")
            ],
            outro=_text(scope_elem.find("outro")),
        )
    overview = [_text(p) for p in elem.findall("overview/para")]
    definitions = [
        SimpleNamespace(name=t.get("name", ""), body=_text(t))
        for t in elem.findall("definitions/term")
    ]
    references = [_text(r) for r in elem.findall("references/ref")]
    arch_elem = elem.find("architecture")
    architecture = None
    if arch_elem is not None:
        flow_elem = arch_elem.find("flow")
        flow = None
        if flow_elem is not None:
            flow = SimpleNamespace(
                intro=_text(flow_elem.find("intro")),
                steps=_children_text(flow_elem, "step"),
            )
        architecture = SimpleNamespace(
            intro=_text(arch_elem.find("intro")),
            components=[
                SimpleNamespace(path=c.get("path", ""), body=_text(c))
                for c in arch_elem.findall("component")
            ],
            flow=flow,
        )
    design_goals = [
        SimpleNamespace(name=g.get("name", ""), body=_text(g))
        for g in elem.findall("design_goals/goal")
    ]
    modules = [build_module(m) for m in elem.findall("modules/module")]
    return SimpleNamespace(
        kind=_text(elem.find("kind")),
        audience=_text(elem.find("audience")),
        scope=scope,
        overview=overview,
        definitions=definitions,
        references=references,
        architecture=architecture,
        design_goals=design_goals,
        modules=modules,
        data_dictionary=build_data_dictionary(elem.find("data_dictionary")),
        traceability=[
            SimpleNamespace(name=t.get("name", ""), sections=t.get("sections", ""))
            for t in elem.findall("traceability/theme")
        ],
    )


def build_trace(elem: ET.Element) -> SimpleNamespace:
    return SimpleNamespace(**_attrs(elem))


def build_traces(elem: ET.Element | None) -> list[SimpleNamespace]:
    if elem is None:
        return []
    return [build_trace(t) for t in elem.findall("trace")]


def build_hlr(elem: ET.Element) -> SimpleNamespace:
    return SimpleNamespace(
        id=elem.get("id", ""),
        name=elem.get("name", ""),
        text=_text(elem.find("text")),
        traces=build_traces(elem.find("traces")),
    )


def build_hlr_section(elem: ET.Element) -> SimpleNamespace:
    return SimpleNamespace(
        number=elem.get("number", ""),
        title=elem.get("title", ""),
        intro=_text(elem.find("intro")),
        hlrs=[build_hlr(h) for h in elem.findall("hlr")],
    )


def build_llr(elem: ET.Element) -> SimpleNamespace:
    return SimpleNamespace(
        id=elem.get("id", ""),
        text=_text(elem.find("text")),
        traces=build_traces(elem.find("traces")),
    )


def build_llr_group(elem: ET.Element) -> SimpleNamespace:
    """An <llrs>/<function> element groups LLRs by the function they cover."""
    return SimpleNamespace(
        number=elem.get("number", ""),
        title=elem.get("title", ""),
        name=elem.get("name", ""),
        source=elem.get("source", ""),
        intro=_text(elem.find("intro")),
        llrs=[build_llr(l) for l in elem.findall("llr")],
    )


def build_test(elem: ET.Element) -> SimpleNamespace:
    return SimpleNamespace(
        name=elem.get("name", ""),
        purpose=_text(elem.find("purpose")),
        traces=build_traces(elem.find("traces")),
    )


def build_test_file(elem: ET.Element) -> SimpleNamespace:
    return SimpleNamespace(
        path=elem.get("path", ""),
        role=elem.get("role", ""),
        count=int(elem.get("count", "0")),
        header=_text(elem.find("header")),
        tests=[build_test(t) for t in elem.findall("test")],
    )


def build_stp(elem: ET.Element | None) -> SimpleNamespace | None:
    if elem is None:
        return None

    intro_elem = elem.find("introduction")
    introduction = None
    if intro_elem is not None:
        related_elem = intro_elem.find("related")
        introduction = SimpleNamespace(
            purpose=_text(intro_elem.find("purpose")),
            scope=_text(intro_elem.find("scope")),
            related=_children_text(related_elem, "doc") if related_elem is not None else [],
        )

    strat_elem = elem.find("strategy")
    strategy = None
    if strat_elem is not None:
        levels_elem = strat_elem.find("levels")
        levels = []
        if levels_elem is not None:
            levels = [
                SimpleNamespace(
                    name=l.get("name", ""),
                    source=l.get("source", ""),
                    driver=l.get("driver", ""),
                    style=l.get("style", ""),
                )
                for l in levels_elem.findall("level")
            ]
        be_elem = strat_elem.find("build_execution")
        build_execution = None
        if be_elem is not None:
            build_execution = SimpleNamespace(
                intro=_text(be_elem.find("intro")),
                steps=_children_text(be_elem, "step"),
                outro=_text(be_elem.find("outro")),
            )
        pf_elem = strat_elem.find("pass_fail")
        pass_fail = _children_text(pf_elem, "criterion") if pf_elem is not None else []
        strategy = SimpleNamespace(
            levels=levels,
            framework=_text(strat_elem.find("framework")),
            build_execution=build_execution,
            pass_fail=pass_fail,
            traceability_convention=_text(strat_elem.find("traceability_convention")),
        )

    ie_elem = elem.find("integration_environment")
    integration_environment = None
    if ie_elem is not None:
        fixtures = []
        # Collect the union of artefact keys across fixtures (in
        # first-seen order) so the STP template can emit a stable
        # column header per artefact column.
        artefact_keys: list[str] = []
        for f in ie_elem.findall("fixture"):
            artefacts = {}
            for a in f.findall("artefact"):
                key = a.get("key", "")
                if key and key not in artefact_keys:
                    artefact_keys.append(key)
                artefacts[key] = SimpleNamespace(
                    key=key,
                    label=a.get("label", key),
                    path=a.get("path", ""),
                )
            fixtures.append(SimpleNamespace(
                name=f.get("name", ""),
                source=f.get("source", ""),
                artefacts=artefacts,
            ))
        # Resolve a per-key label using the first fixture that defines
        # it (so the column header matches the author's chosen label).
        artefact_labels: dict[str, str] = {}
        for f in fixtures:
            for k, a in f.artefacts.items():
                artefact_labels.setdefault(k, a.label)
        integration_environment = SimpleNamespace(
            intro=_text(ie_elem.find("intro")),
            fixtures=fixtures,
            artefact_keys=artefact_keys,
            artefact_labels=artefact_labels,
            outro=_text(ie_elem.find("outro")),
        )

    tooling_elem = elem.find("tooling")
    tooling = []
    if tooling_elem is not None:
        tooling = [
            SimpleNamespace(
                name=t.get("name", ""),
                required_for=t.get("required_for", ""),
                notes=t.get("notes", ""),
            )
            for t in tooling_elem.findall("tool")
        ]

    return SimpleNamespace(
        introduction=introduction,
        strategy=strategy,
        integration_environment=integration_environment,
        tooling=tooling,
        maintenance=_text(elem.find("maintenance")),
    )


def load_project(xml_path: Path, metadata_for: str) -> SimpleNamespace:
    root = ET.parse(xml_path).getroot()
    if root.tag != "project":
        raise SystemExit(f"Root element is <{root.tag}>, expected <project>")

    # Pull the document metadata block requested by the caller.
    metadata = None
    for d in root.findall("metadata/document"):
        if d.get("id") == metadata_for:
            metadata = SimpleNamespace(**_attrs(d))
            break
    if metadata is None:
        raise SystemExit(
            f"<document id={metadata_for!r}> not found in metadata block"
        )

    sdd = build_sdd(root.find("sdd"))
    stp = build_stp(root.find("stp"))
    hlrs = [build_hlr_section(s) for s in root.findall("hlrs/section")]
    llrs = [build_llr_group(f) for f in root.findall("llrs/function")]
    tests = [build_test_file(f) for f in root.findall("tests/file")]

    # Counts pass-through so templates can show "X tests across Y files".
    counts = {c.get("name"): c.get("value")
              for c in root.findall("metadata/counts/count")}

    # Cross-reference maps consumed by the LLR coverage matrix in the
    # STP template. Each maps an upstream identifier to the list of
    # test names that carry a <trace target="..."> citing it.
    tests_by_llr: dict[str, list[str]] = {}
    tests_by_hlr: dict[str, list[str]] = {}
    for tf in tests:
        for t in tf.tests:
            for tr in t.traces:
                target = getattr(tr, "target", "")
                ref = getattr(tr, "ref", "")
                if not ref:
                    continue
                if target == "LLR":
                    tests_by_llr.setdefault(ref, []).append(t.name)
                elif target == "HLR":
                    tests_by_hlr.setdefault(ref, []).append(t.name)

    # Flat list of every LLR (in declaration order) annotated with the
    # owning function group, for the coverage-matrix loop.
    flat_llrs: list[SimpleNamespace] = []
    for grp in llrs:
        for llr in grp.llrs:
            flat_llrs.append(SimpleNamespace(
                id=llr.id,
                text=llr.text,
                traces=llr.traces,
                function_name=grp.name or grp.title,
                function_number=grp.number,
            ))

    # Flat list of every HLR (in declaration order) annotated with its
    # owning section number/title. Used by the Traceability template.
    flat_hlrs: list[SimpleNamespace] = []
    for sec in hlrs:
        for hlr in sec.hlrs:
            flat_hlrs.append(SimpleNamespace(
                id=hlr.id,
                name=hlr.name,
                text=hlr.text,
                traces=hlr.traces,
                section_number=sec.number,
                section_title=sec.title,
            ))

    # ID lookup tables.
    hlr_by_id = {h.id: h for h in flat_hlrs}
    llr_by_id = {l.id: l for l in flat_llrs}

    # LLRs grouped by HLR they implement (from each LLR's <traces>).
    llrs_by_hlr: dict[str, list[str]] = {}
    for llr in flat_llrs:
        for tr in llr.traces:
            if getattr(tr, "target", "") == "HLR" and tr.ref:
                llrs_by_hlr.setdefault(tr.ref, []).append(llr.id)

    # HLRs grouped by SDD section they implement.
    hlrs_by_sdd: dict[str, list[str]] = {}
    for hlr in flat_hlrs:
        for tr in hlr.traces:
            if getattr(tr, "target", "") == "SDD" and tr.ref:
                hlrs_by_sdd.setdefault(tr.ref, []).append(hlr.id)

    # File-of-test lookup so the test-side matrix can show source paths.
    file_of_test: dict[str, str] = {}
    for tf in tests:
        for t in tf.tests:
            file_of_test[t.name] = tf.path

    # Flat alphabetical test list for the §5 reverse matrix.
    flat_tests: list[SimpleNamespace] = []
    for tf in tests:
        for t in tf.tests:
            flat_tests.append(SimpleNamespace(
                name=t.name,
                purpose=t.purpose,
                traces=t.traces,
                file=tf.path,
            ))
    flat_tests.sort(key=lambda t: (t.file, t.name))

    # IDs that have no direct binding in the verification chain.
    llrs_no_test = [l.id for l in flat_llrs if l.id not in tests_by_llr]
    hlrs_no_test = [
        h.id for h in flat_hlrs
        if h.id not in tests_by_hlr
        and not any(lid in tests_by_llr for lid in llrs_by_hlr.get(h.id, []))
    ]

    # SDD section number -> heading title. Mirrors the numbering scheme
    # used by SDD.md.j2 so the Traceability template can resolve refs
    # like "3.2.1" or "4.3.2" to a human-readable section name.
    sdd_titles: dict[str, str] = {}
    if sdd is not None:
        sdd_titles.update({
            "1": "Introduction",
            "1.1": "Purpose of the Document",
            "1.2": "Scope of the Document",
            "1.3": "Project Overview",
            "1.4": "Definitions, Acronyms, and Abbreviations",
            "1.5": "References",
            "1.6": "Document Overview",
            "2": "System Overview",
            "2.1": "System Architecture",
            "2.2": "Design Goals and Constraints",
        })
        for i, m in enumerate(sdd.modules):
            sec = i + 3
            mod_label = m.path or m.title
            sdd_titles[f"{sec}"] = f"Detailed Design ({mod_label})"
            sdd_titles[f"{sec}.1"] = f"Purpose and Responsibilities ({mod_label})"
            if m.interfaces is not None:
                base = "External Interfaces"
                if getattr(m.interfaces, "title_suffix", ""):
                    base = f"{base} {m.interfaces.title_suffix}"
                sdd_titles[f"{sec}.2"] = f"{base} ({mod_label})"
                for j, iface in enumerate(m.interfaces.items, start=1):
                    sdd_titles[f"{sec}.2.{j}"] = iface.title
            sdd_titles[f"{sec}.3"] = f"Internal Structure ({mod_label})"
            sub_n = 0
            if m.data_structures:
                sub_n += 1
                sdd_titles[f"{sec}.3.{sub_n}"] = "Key Data Structures"
            if m.functions is not None:
                sub_n += 1
                sdd_titles[f"{sec}.3.{sub_n}"] = f"Key Functions ({mod_label})"
            if m.algorithm:
                sub_n += 1
                sdd_titles[f"{sec}.3.{sub_n}"] = "Parsing Strategy / Algorithm"
            if m.dependencies:
                sdd_titles[f"{sec}.4"] = f"Dependencies ({mod_label})"
            if m.error_handling:
                sdd_titles[f"{sec}.5"] = f"Error Handling and Logging ({mod_label})"
        dd_sec = len(sdd.modules) + 3
        sdd_titles[f"{dd_sec}"] = "Data Dictionary"

    return SimpleNamespace(
        name=root.get("name", ""),
        short_name=root.get("short_name", ""),
        schema_version=root.get("schema_version", ""),
        metadata=metadata,
        counts=counts,
        sdd=sdd,
        sdd_titles=sdd_titles,
        stp=stp,
        hlrs=hlrs,
        llrs=llrs,
        flat_hlrs=flat_hlrs,
        flat_llrs=flat_llrs,
        hlr_by_id=hlr_by_id,
        llr_by_id=llr_by_id,
        llrs_by_hlr=llrs_by_hlr,
        hlrs_by_sdd=hlrs_by_sdd,
        tests=tests,
        flat_tests=flat_tests,
        file_of_test=file_of_test,
        tests_by_llr=tests_by_llr,
        tests_by_hlr=tests_by_hlr,
        llrs_no_test=llrs_no_test,
        hlrs_no_test=hlrs_no_test,
    )


PVD_TEMPLATE = Path(__file__).resolve().parent / "templates" / "PVD.md.template"


SKELETON_PROJECT_XML = """\
<?xml version="1.0" encoding="UTF-8"?>
<!--
  Project.xml — single source of truth for this project's spec stack.
  See tools/Project_xml_README.md for the schema reference.

  This file was created by `render_doc.py` in init mode. Fill in the
  payload sections (sdd, stp, hlrs, llrs, tests) as the project takes
  shape, then regenerate the markdown specs with `render_doc.py`.
-->
<project name="{name}" short_name="{short_name}" schema_version="1.1"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:noNamespaceSchemaLocation="../tools/project.xsd">
  <metadata>
    <document id="SDD"          title="Software Design Document"     source="doc/SDD.md"          version="0.1" date="{date}" author="{author}"/>
    <document id="HLRs"         title="High-Level Requirements"      source="doc/HLRs.md"         version="0.1" date="{date}" author="{author}"/>
    <document id="LLRs"         title="Low-Level Requirements"       source="doc/LLRs.md"         version="0.1" date="{date}" author="{author}"/>
    <document id="STP"          title="Software Test Plan"           source="doc/STP.md"          version="0.1" date="{date}" author="{author}"/>
    <document id="Traceability" title="Traceability Matrix"          source="doc/Traceability.md" version="0.1" date="{date}" author="{author}"/>
    <counts>
      <count name="hlrs"       value="0"/>
      <count name="llrs"       value="0"/>
      <count name="tests"      value="0"/>
      <count name="test_files" value="0"/>
    </counts>
  </metadata>

  <!-- Software Design Document payload (see tools/Project_xml_README.md §3). -->
  <sdd>
  </sdd>

  <!-- Software Test Plan payload (see tools/Project_xml_README.md §4). -->
  <stp>
  </stp>

  <!-- High-Level Requirements (see tools/Project_xml_README.md §5). -->
  <hlrs>
  </hlrs>

  <!-- Low-Level Requirements (see tools/Project_xml_README.md §6). -->
  <llrs>
  </llrs>

  <!-- Test sources (see tools/Project_xml_README.md §7). -->
  <tests>
  </tests>
</project>
"""


def init_project(
    *,
    name: str,
    short_name: str,
    author: str,
    xml_path: Path,
    pvd_path: Path,
    pvd_template: Path,
    force: bool,
) -> int:
    """Bootstrap a new project: write skeleton Project.xml and PVD.md.

    Returns 0 on success, non-zero on refusal to overwrite an existing
    file when --force was not supplied.
    """
    from datetime import date as _date

    today = _date.today().isoformat()

    targets = [xml_path, pvd_path]
    if not force:
        existing = [str(p) for p in targets if p.exists()]
        if existing:
            sys.stderr.write(
                "render_doc.py --init: refusing to overwrite existing "
                "file(s):\n"
            )
            for p in existing:
                sys.stderr.write(f"  {p}\n")
            sys.stderr.write("Re-run with --force to overwrite.\n")
            return 1

    if not pvd_template.exists():
        sys.stderr.write(
            f"render_doc.py --init: PVD template not found: {pvd_template}\n"
        )
        return 1

    xml_path.parent.mkdir(parents=True, exist_ok=True)
    pvd_path.parent.mkdir(parents=True, exist_ok=True)

    xml_path.write_text(
        SKELETON_PROJECT_XML.format(
            name=name,
            short_name=short_name,
            date=today,
            author=author,
        )
    )

    # Substitute the obvious header placeholders in the PVD template.
    # Body placeholders (e.g. <Persona 1>, <Capability 1>) are left for
    # the human author to fill in.
    pvd_text = pvd_template.read_text()
    pvd_text = pvd_text.replace("<Product Name>", name)
    pvd_text = pvd_text.replace("<short_name>", short_name)
    pvd_text = pvd_text.replace("<YYYY-MM-DD>", today)
    pvd_text = pvd_text.replace("<Your name(s)>", author)
    pvd_path.write_text(pvd_text)

    sys.stderr.write(f"Wrote skeleton {xml_path}\n")
    sys.stderr.write(f"Wrote {pvd_path}\n")
    sys.stderr.write(
        "Next steps: edit doc/PVD.md, then populate doc/Project.xml "
        "and regenerate the spec documents with render_doc.py.\n"
    )
    return 0


def render(template_path: Path, project: SimpleNamespace) -> str:
    env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(template_path.parent),
        trim_blocks=True,
        lstrip_blocks=False,
        keep_trailing_newline=True,
        undefined=jinja2.StrictUndefined,
    )
    env.filters["gh_slug"] = _gh_slug
    template = env.get_template(template_path.name)
    return template.render(project=project)


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="render_doc.py",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=(
            "Render a Markdown specification document from a Jinja2 "
            "template combined with the data in doc/Project.xml.\n"
            "\n"
            "The renderer loads Project.xml, builds a `project` namespace "
            "exposing the parsed payload (sdd, stp, hlrs, llrs, tests) "
            "plus cross-reference indexes (tests_by_llr, tests_by_hlr, "
            "llrs_by_hlr, hlrs_by_sdd, etc.), then renders the named "
            "template against it. The output is written to stdout by "
            "default, or to the path given by --out."
        ),
        epilog=(
            "EXAMPLES\n"
            "  Bootstrap a new project (writes doc/Project.xml and\n"
            "  doc/PVD.md from the skeletons):\n"
            "    python3 tools/render_doc.py --init \\\n"
            "        --name \"My Product\" --short-name myprod \\\n"
            "        --author \"Jane Doe\"\n"
            "\n"
            "  Render the SDD to stdout:\n"
            "    python3 tools/render_doc.py tools/templates/SDD.md.j2 SDD\n"
            "\n"
            "  Render the HLRs document to a file:\n"
            "    python3 tools/render_doc.py tools/templates/HLRs.md.j2 \\\n"
            "        HLRs --out doc/HLRs.md\n"
            "\n"
            "  Use a Project.xml from a different location:\n"
            "    python3 tools/render_doc.py tools/templates/STP.md.j2 \\\n"
            "        STP --xml /path/to/Project.xml --out doc/STP.md\n"
            "\n"
            "  Regenerate every spec document in this project:\n"
            "    for d in SDD HLRs LLRs STP Traceability; do \\\n"
            "      python3 tools/render_doc.py \\\n"
            "        tools/templates/$d.md.j2 $d --out doc/$d.md; \\\n"
            "    done\n"
            "\n"
            "EXIT STATUS\n"
            "  0  Render (or --init) succeeded.\n"
            "  1  --init refused to overwrite an existing file (use --force).\n"
            "  2  Argument parsing failed (argparse default).\n"
            "  Other non-zero values are raised by the underlying XML "
            "parser, Jinja2, or filesystem operations."
        ),
    )
    parser.add_argument(
        "template",
        type=Path,
        nargs="?",
        metavar="TEMPLATE",
        help=(
            "Path to the Jinja2 template to render (e.g. "
            "tools/templates/SDD.md.j2). The template's parent directory "
            "becomes the loader root, so {%% include %%} / {%% import %%} "
            "directives may reference sibling templates by relative path. "
            "Required unless --init is given."
        ),
    )
    parser.add_argument(
        "metadata_id",
        nargs="?",
        metavar="METADATA_ID",
        help=(
            "Identifier of the <metadata>/<document id=\"...\"> block in "
            "Project.xml whose title/version/date/author values should be "
            "exposed to the template as `project.metadata`. Must match "
            "exactly one <document id=\"...\"> entry. Standard values in "
            "this project: SDD, HLRs, LLRs, STP, Traceability. "
            "Required unless --init is given."
        ),
    )
    parser.add_argument(
        "--xml",
        type=Path,
        default=PROJECT_XML,
        metavar="PATH",
        help=(
            "Path to the Project.xml file to load as the data source. "
            "Defaults to %(default)s (relative to the repository root)."
        ),
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=None,
        metavar="PATH",
        help=(
            "Write the rendered document to this file instead of stdout. "
            "Output is normalised to exactly one trailing newline so "
            "round-tripping a document does not accumulate blank lines."
        ),
    )
    init_group = parser.add_argument_group(
        "project bootstrap (--init)",
        "Create a skeleton Project.xml plus a substituted PVD.md as the "
        "first step of a new project. When --init is given, TEMPLATE and "
        "METADATA_ID are not required.",
    )
    init_group.add_argument(
        "--init",
        action="store_true",
        help=(
            "Create a skeleton Project.xml at --xml (default doc/Project.xml) "
            "and a substituted PVD.md at --pvd-out (default doc/PVD.md). "
            "Refuses to overwrite existing files unless --force is also given."
        ),
    )
    init_group.add_argument(
        "--name",
        default=None,
        metavar="NAME",
        help="Full project name (used as Project.xml @name and in PVD title).",
    )
    init_group.add_argument(
        "--short-name",
        default=None,
        metavar="SHORT",
        help="Short / package name (used as Project.xml @short_name).",
    )
    init_group.add_argument(
        "--author",
        default="TBD",
        metavar="AUTHOR",
        help="Author string for metadata blocks and PVD header. Default: TBD.",
    )
    init_group.add_argument(
        "--pvd-out",
        type=Path,
        default=Path(__file__).resolve().parent.parent / "doc" / "PVD.md",
        metavar="PATH",
        help="Output path for the generated PVD. Default: %(default)s.",
    )
    init_group.add_argument(
        "--pvd-template",
        type=Path,
        default=PVD_TEMPLATE,
        metavar="PATH",
        help="Source PVD template. Default: %(default)s.",
    )
    init_group.add_argument(
        "--force",
        action="store_true",
        help="With --init, overwrite existing Project.xml / PVD.md.",
    )
    args = parser.parse_args()

    if args.init:
        if not args.name or not args.short_name:
            parser.error("--init requires --name and --short-name")
        return init_project(
            name=args.name,
            short_name=args.short_name,
            author=args.author,
            xml_path=args.xml,
            pvd_path=args.pvd_out,
            pvd_template=args.pvd_template,
            force=args.force,
        )

    if args.template is None or args.metadata_id is None:
        parser.error("TEMPLATE and METADATA_ID are required unless --init is given")

    project = load_project(args.xml, args.metadata_id)
    output = render(args.template, project)
    # Normalize to exactly one trailing newline so round-tripping a doc
    # does not accumulate blank lines at EOF.
    output = output.rstrip("\n") + "\n"

    if args.out:
        args.out.write_text(output)
    else:
        sys.stdout.write(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
