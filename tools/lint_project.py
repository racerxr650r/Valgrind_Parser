#!/usr/bin/env python3
"""
Lint doc/Project.xml: structural validation + semantic cross-reference
checks that the XSD cannot express.

Checks performed:
  STRUCTURAL
    * File parses as well-formed XML.
    * If `xmllint` is available on PATH, validate against
      tools/project.xsd. (Pure-Python XSD validation is not in the
      stdlib; if xmllint and lxml are both unavailable, structural
      validation is reduced to "well-formed" only and a note is
      printed.)

  SEMANTIC
    * <project>/@schema_version is set.
    * <metadata> contains a <document id="..."> for every standard
      document the renderer supports (SDD, HLRs, LLRs, STP, Traceability),
      and document ids are unique.
    * Every HLR id is unique and matches HLR-NNN.
    * Every LLR id is unique and matches LLR-XXX-NN.
    * Every test name is unique within its file.
    * Every <trace target="HLR" ref="..."> resolves to a known HLR.
    * Every <trace target="LLR" ref="..."> resolves to a known LLR.
    * Every <trace target="SDD" ref="..."> looks like a dotted section
      number (warn-only — SDD section numbers are template-derived, not
      data-derived).
    * Optional warnings (suppress with --no-warnings):
        - HLRs with no LLR or test linking to them.
        - LLRs with no test linking to them.

Exit codes:
  0  No errors.
  1  Errors found.
  2  Argument parsing failed (argparse default).
"""
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from collections import Counter, defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_XML = REPO_ROOT / "doc" / "Project.xml"
DEFAULT_XSD = REPO_ROOT / "tools" / "project.xsd"

STANDARD_DOCS = {"SDD", "HLRs", "LLRs", "STP", "Traceability"}

HLR_ID_RE = re.compile(r"^HLR-\d{3,}$")
LLR_ID_RE = re.compile(r"^LLR-[A-Z0-9]+-\d{2,}$")
SDD_REF_RE = re.compile(r"^[0-9]+(\.[0-9]+)*[a-zA-Z]?$")


class Findings:
    """Accumulator for lint output."""

    def __init__(self) -> None:
        self.errors: list[str] = []
        self.warnings: list[str] = []
        self.notes: list[str] = []

    def error(self, msg: str) -> None:
        self.errors.append(msg)

    def warn(self, msg: str) -> None:
        self.warnings.append(msg)

    def note(self, msg: str) -> None:
        self.notes.append(msg)

    def report(self, *, show_warnings: bool, stream=sys.stderr) -> None:
        for n in self.notes:
            stream.write(f"note:    {n}\n")
        if show_warnings:
            for w in self.warnings:
                stream.write(f"warning: {w}\n")
        for e in self.errors:
            stream.write(f"error:   {e}\n")
        n_e, n_w = len(self.errors), len(self.warnings) if show_warnings else 0
        stream.write(f"\nlint_project: {n_e} error(s), {n_w} warning(s)\n")


def validate_structure(xml_path: Path, xsd_path: Path, findings: Findings) -> ET.ElementTree | None:
    """Parse the XML and (if possible) validate against the XSD."""
    try:
        tree = ET.parse(xml_path)
    except ET.ParseError as exc:
        findings.error(f"{xml_path}: malformed XML: {exc}")
        return None

    # Try lxml first (richer error messages); fall back to xmllint; else
    # leave structural validation as well-formedness only.
    if _try_lxml_validate(xml_path, xsd_path, findings):
        return tree
    if _try_xmllint_validate(xml_path, xsd_path, findings):
        return tree
    findings.note(
        "neither python3-lxml nor xmllint is available; XSD validation "
        "skipped (well-formedness check only). Install with "
        "`apt install libxml2-utils` or `pip install lxml`."
    )
    return tree


def _try_lxml_validate(xml_path: Path, xsd_path: Path, findings: Findings) -> bool:
    try:
        from lxml import etree  # type: ignore
    except ImportError:
        return False
    try:
        with xsd_path.open("rb") as f:
            schema = etree.XMLSchema(etree.parse(f))
        with xml_path.open("rb") as f:
            doc = etree.parse(f)
        if not schema.validate(doc):
            for err in schema.error_log:
                findings.error(
                    f"{xml_path}:{err.line}: XSD: {err.message}"
                )
    except Exception as exc:  # pragma: no cover - defensive
        findings.error(f"lxml validation failed: {exc}")
    return True


def _try_xmllint_validate(xml_path: Path, xsd_path: Path, findings: Findings) -> bool:
    if shutil.which("xmllint") is None:
        return False
    proc = subprocess.run(
        ["xmllint", "--noout", "--schema", str(xsd_path), str(xml_path)],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        for line in proc.stderr.splitlines():
            line = line.strip()
            if line and not line.endswith("fails to validate") and not line.endswith("validates"):
                findings.error(f"XSD: {line}")
    return True


# --------------------------------------------------------------------- #
# Semantic checks                                                       #
# --------------------------------------------------------------------- #

def _collect_ids(root: ET.Element) -> tuple[set[str], set[str], dict[str, str]]:
    """Return (hlr_ids, llr_ids, test_name -> file_path) and report dup ids."""
    return set(), set(), {}


def check_semantics(tree: ET.ElementTree, findings: Findings) -> None:
    root = tree.getroot()

    # --- Project root -------------------------------------------------
    if root.tag != "project":
        findings.error(f"root element is <{root.tag}>, expected <project>")
        return
    if not root.get("schema_version"):
        findings.error("<project> is missing required @schema_version")

    # --- Metadata -----------------------------------------------------
    metadata = root.find("metadata")
    if metadata is None:
        findings.error("<project> is missing required <metadata>")
    else:
        doc_ids = [d.get("id", "") for d in metadata.findall("document")]
        dup = [i for i, n in Counter(doc_ids).items() if n > 1]
        for i in dup:
            findings.error(f"<metadata> has duplicate <document id=\"{i}\">")
        missing = STANDARD_DOCS - set(doc_ids)
        for m in sorted(missing):
            findings.warn(
                f"<metadata> has no <document id=\"{m}\"> "
                f"(render_doc.py will not find metadata for that document)"
            )

    # --- HLRs ---------------------------------------------------------
    hlr_ids: dict[str, ET.Element] = {}
    for hlr in root.findall("hlrs/section/hlr"):
        hid = hlr.get("id", "")
        if not HLR_ID_RE.match(hid):
            findings.error(
                f"<hlr id=\"{hid}\"> does not match HLR-NNN"
            )
        if hid in hlr_ids:
            findings.error(f"duplicate HLR id: {hid}")
        hlr_ids[hid] = hlr

    # --- LLRs ---------------------------------------------------------
    llr_ids: dict[str, ET.Element] = {}
    for llr in root.findall("llrs/function/llr"):
        lid = llr.get("id", "")
        if not LLR_ID_RE.match(lid):
            findings.error(
                f"<llr id=\"{lid}\"> does not match LLR-XXX-NN"
            )
        if lid in llr_ids:
            findings.error(f"duplicate LLR id: {lid}")
        llr_ids[lid] = llr

    # --- Tests --------------------------------------------------------
    test_owner: dict[str, str] = {}
    for tfile in root.findall("tests/file"):
        path = tfile.get("path", "<unknown file>")
        seen: set[str] = set()
        for t in tfile.findall("test"):
            name = t.get("name", "")
            if not name:
                findings.error(f"test in {path} is missing required @name")
                continue
            if name in seen:
                findings.error(
                    f"duplicate test name '{name}' within {path}"
                )
            seen.add(name)
            # Tests with the same name across files are allowed but
            # noted (they will produce ambiguous anchors).
            if name in test_owner and test_owner[name] != path:
                findings.warn(
                    f"test name '{name}' appears in both "
                    f"{test_owner[name]} and {path}; cross-file links "
                    f"will be ambiguous"
                )
            test_owner[name] = path

    # --- Trace resolution --------------------------------------------
    # HLR-side traces: <trace target="SDD" ref="X.Y">
    for hlr in root.findall("hlrs/section/hlr"):
        hid = hlr.get("id", "<?>")
        for tr in hlr.findall("traces/trace"):
            _check_trace(tr, owner=f"HLR {hid}", hlr_ids=hlr_ids,
                         llr_ids=llr_ids, findings=findings,
                         allowed_targets={"SDD"})

    # LLR-side traces: <trace target="HLR" ref="HLR-NNN">
    for llr in root.findall("llrs/function/llr"):
        lid = llr.get("id", "<?>")
        for tr in llr.findall("traces/trace"):
            _check_trace(tr, owner=f"LLR {lid}", hlr_ids=hlr_ids,
                         llr_ids=llr_ids, findings=findings,
                         allowed_targets={"HLR"})

    # Test-side traces: <trace target="LLR"|"HLR" ref="...">
    for tfile in root.findall("tests/file"):
        for t in tfile.findall("test"):
            name = t.get("name", "<?>")
            for tr in t.findall("traces/trace"):
                _check_trace(tr, owner=f"test {name}", hlr_ids=hlr_ids,
                             llr_ids=llr_ids, findings=findings,
                             allowed_targets={"HLR", "LLR"})

    # --- Coverage warnings -------------------------------------------
    # Index reverse links.
    llrs_per_hlr: dict[str, list[str]] = defaultdict(list)
    for llr in root.findall("llrs/function/llr"):
        for tr in llr.findall("traces/trace"):
            if tr.get("target") == "HLR":
                llrs_per_hlr[tr.get("ref", "")].append(llr.get("id", ""))

    tests_per_llr: dict[str, list[str]] = defaultdict(list)
    tests_per_hlr_direct: dict[str, list[str]] = defaultdict(list)
    for tfile in root.findall("tests/file"):
        for t in tfile.findall("test"):
            for tr in t.findall("traces/trace"):
                ref = tr.get("ref", "")
                if tr.get("target") == "LLR":
                    tests_per_llr[ref].append(t.get("name", ""))
                elif tr.get("target") == "HLR":
                    tests_per_hlr_direct[ref].append(t.get("name", ""))

    for lid in llr_ids:
        if not tests_per_llr.get(lid):
            findings.warn(f"LLR {lid} has no test verifying it")

    for hid in hlr_ids:
        # Covered if any test directly traces it OR any of its LLRs has a test.
        direct = bool(tests_per_hlr_direct.get(hid))
        via_llr = any(tests_per_llr.get(l) for l in llrs_per_hlr.get(hid, []))
        if not direct and not via_llr:
            findings.warn(f"HLR {hid} has no test verifying it (directly or via any LLR)")


def _check_trace(
    tr: ET.Element,
    *,
    owner: str,
    hlr_ids: dict[str, ET.Element],
    llr_ids: dict[str, ET.Element],
    findings: Findings,
    allowed_targets: set[str],
) -> None:
    target = tr.get("target", "")
    ref = tr.get("ref", "")
    if not target or not ref:
        findings.error(f"{owner}: <trace> missing target/ref")
        return
    if target not in {"SDD", "HLR", "LLR"}:
        findings.error(f"{owner}: <trace target=\"{target}\"> not in SDD|HLR|LLR")
        return
    if target not in allowed_targets:
        findings.error(
            f"{owner}: <trace target=\"{target}\"> not allowed here "
            f"(allowed: {sorted(allowed_targets)})"
        )
        return
    if target == "SDD":
        if not SDD_REF_RE.match(ref):
            findings.warn(
                f"{owner}: SDD trace ref '{ref}' does not look like a "
                f"dotted section number (e.g. '3.2.1')"
            )
    elif target == "HLR":
        if ref not in hlr_ids:
            findings.error(f"{owner}: <trace> references unknown HLR '{ref}'")
    elif target == "LLR":
        if ref not in llr_ids:
            findings.error(f"{owner}: <trace> references unknown LLR '{ref}'")


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="lint_project.py",
        description=(
            "Validate doc/Project.xml structurally (against tools/project.xsd) "
            "and semantically (cross-reference, id format, coverage)."
        ),
    )
    parser.add_argument("--xml", type=Path, default=DEFAULT_XML,
                        help="Path to Project.xml. Default: %(default)s")
    parser.add_argument("--xsd", type=Path, default=DEFAULT_XSD,
                        help="Path to project.xsd. Default: %(default)s")
    parser.add_argument("--no-warnings", action="store_true",
                        help="Suppress warnings; only fail on errors.")
    args = parser.parse_args()

    if not args.xml.exists():
        sys.stderr.write(f"lint_project: file not found: {args.xml}\n")
        return 1
    if not args.xsd.exists():
        sys.stderr.write(f"lint_project: file not found: {args.xsd}\n")
        return 1

    findings = Findings()
    tree = validate_structure(args.xml, args.xsd, findings)
    if tree is not None:
        check_semantics(tree, findings)

    findings.report(show_warnings=not args.no_warnings)
    return 1 if findings.errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
