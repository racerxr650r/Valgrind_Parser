# Product Vision Document: Valgrind Parser (vgp)

**Version:** 1.0
**Date:** 4/22/2026
**Author(s):** John Anderson

## 1. Purpose

This Product Vision Document (PVD) defines *why* `vgp` exists, *who*
it is for, *what* problem it solves, and the *measurable outcomes*
that determine whether it is succeeding. It sits above the
[Software Design Document](SDD.md), [High-Level
Requirements](HLRs.md), [Low-Level Requirements](LLRs.md), and
[Software Test Plan](STP.md), and is the document the rest of the
specification stack must remain aligned with.

When in doubt about a feature, scope decision, or trade-off, this
document is the reference.

## 2. Vision Statement

> **Make Valgrind output as easy to act on as a compiler error.**
>
> A developer running their program under Valgrind should be able to
> read `vgp`'s output, jump straight to the offending line in their
> editor, and fix the bug — without learning Valgrind's log format,
> filtering library noise by hand, or correlating PIDs and stack
> frames manually.

## 3. Problem Statement

Valgrind's Memcheck is the de-facto tool for detecting memory errors
and leaks in native code, but its raw text output is hostile to fast
diagnosis:

*   Every line is prefixed with `==<PID>==`, which obscures the
    actual message.
*   Each error block contains a call stack interleaving user code
    with framework/runtime/libc frames, with no visual distinction.
*   Source-file references appear as `(file:line)` fragments scattered
    across the stack — the reader must scroll, mentally filter, and
    cross-reference to find the offending user function.
*   Leak and error summaries are printed in a separate trailing block
    that is hard to read alongside the per-error detail.
*   Multi-language projects (C, C++, Rust, Fortran, Ada) compound the
    problem because user-code recognition rules differ per language.

The cumulative effect is that developers either skim the log and
miss real bugs, run Valgrind less often than they should, or spend
disproportionate time triaging output instead of fixing code.

## 4. Target Users

| Persona | Needs from `vgp` |
| ------- | ---------------- |
| **Application developer** | Find the file, function, and line of every memory bug in a build; jump to it in their editor; ignore framework/library noise. |
| **Test/CI engineer** | Run `vgp` non-interactively in CI logs, surface a compact pass/fail summary, and produce diff-friendly output that flags regressions. |
| **Maintainer of a multi-language codebase** | Get the same quality of diagnosis whether the failing frame is C, C++, Rust, Fortran, or Ada. |
| **New contributor onboarding** | Read a Valgrind report without first learning Valgrind's output format. |

`vgp` is **not** aimed at: Valgrind tool authors, performance-profile
analysts (Cachegrind/Callgrind), or users of non-Memcheck Valgrind
tools.

## 5. Value Proposition

`vgp` turns a raw Memcheck log into a triage-ready report by doing
five things, in order:

1.  **Strips the noise** — removes `==<PID>==` prefixes and Valgrind
    bookkeeping lines that carry no diagnostic information.
2.  **Frames the errors** — emits a numbered, clearly-delimited
    `[ERROR #N]` block for every detected error.
3.  **Filters the stack** — hides framework/runtime/libc frames by
    default; surfaces only user-code frames, with bounded ellipsis
    when intervening non-user frames exist.
4.  **Quotes the source** — optionally prints the offending function's
    body with the failing line marked, located via Universal Ctags.
5.  **Summarises the run** — prints a final `--- LEAK SUMMARY ---` and
    `--- FINAL COUNTS ---` block with totals split into classified
    errors vs. possible leaks.

Every reference to a source location is emitted as `file:line`, the
format that VS Code, terminal-aware editors, and CI annotators
recognise as a clickable jump target.

## 6. Product Principles

These principles are the tie-breakers when requirements conflict.

1.  **Streaming is non-negotiable.** `vgp` processes its input
    line-by-line in bounded memory. It never builds an in-memory model
    of the entire log. This rules out features that would require
    look-ahead across the whole file.
2.  **Zero runtime configuration.** No config files, no environment
    variables. Behaviour is fully driven by command-line flags. A user
    inspecting a `vgp` invocation in a shell history can reproduce the
    output exactly.
3.  **Editor-friendly by default.** Every source reference is
    `file:line`. Output stays diff-friendly (no timestamps, no
    PIDs, no machine-specific paths beyond what Valgrind itself
    emits).
4.  **Multi-language out of the box.** First-class support for C, C++,
    Rust, Fortran, and Ada user code. Adding a language is a matter of
    extending `USER_CODE_EXTENSIONS` and (for source listing) the
    ctags language map — never a code rewrite.
5.  **Minimal dependencies.** C standard library, POSIX
    `popen()`/`pclose()`, and an external `ctags` binary on `PATH`
    (only required for the `-s` source-listing feature). No new
    runtime dependency lands without retiring something equivalent.
6.  **Defensive, never fatal.** Malformed Valgrind output is reported
    as a non-fatal warning where possible. `vgp` always exits with a
    well-defined status code; it never aborts mid-stream on
    recoverable input.
7.  **Verifiable.** Every behaviour worth describing is captured as an
    HLR/LLR with at least one bound test. The
    [Traceability Matrix](Traceability.md) is the contract.

## 7. Scope

### 7.1 In Scope

*   Parsing and reformatting of **Valgrind Memcheck** text output.
*   Stripping of the `==<PID>==` prefix and surrounding bookkeeping.
*   Detection and numbering of error blocks for the standard Memcheck
    error classes (Invalid read/write, Invalid free, Mismatched free,
    Source/destination overlap, Use of uninitialised value).
*   User-code stack-frame classification across C, C++, Rust,
    Fortran, and Ada.
*   Optional bounded printing of the filtered call stack (`-t`).
*   Optional printing of the offending function's source body with
    the failing line marked (`-s`), using Universal Ctags to locate
    the function in the source file.
*   Optional leak-summary reformatting (`-l`).
*   Final error-count and possible-leak summary block.

### 7.2 Out of Scope

*   Output from non-Memcheck Valgrind tools (Cachegrind, Callgrind,
    Helgrind, Massif, DRD).
*   Parsing of Valgrind's XML output (`--xml=yes`).
*   Source-mutation suggestions or auto-fix.
*   Machine-readable export formats (JSON, SARIF) — may be considered
    in a future major version.
*   GUI, web frontend, or IDE plugin.
*   Network or daemon mode.

### 7.3 Non-Goals

*   Replacing Valgrind itself.
*   Becoming a general-purpose log reformatter.
*   Supporting platforms other than Linux as a primary target.

## 8. Success Metrics

`vgp` is succeeding when:

| Metric | Target |
| ------ | ------ |
| **Triage time** | A developer can locate the user-code line responsible for a Memcheck error from `vgp` output without consulting the original Valgrind log. |
| **CI signal** | A failing Valgrind run in CI produces a `vgp` report whose `--- FINAL COUNTS ---` line alone is sufficient to fail the build correctly. |
| **Coverage** | Every requirement in [HLRs.md](HLRs.md) and [LLRs.md](LLRs.md) is bound to at least one test in [STP.md](STP.md), per [Traceability.md](Traceability.md). Coverage gaps are documented, not silent. |
| **Self-cleanliness** | `vgp` running under Valgrind on every per-language fixture reports `ERROR SUMMARY: 0 errors` (the dogfood test). |
| **Output stability** | Two runs of the same `vgp` version against the same input produce byte-identical output (deterministic; no timestamps, no PIDs in output). |
| **Footprint** | The shipped binary remains a single executable with no runtime configuration files and a dependency surface no larger than: libc, POSIX, optional `ctags`. |

## 9. Roadmap Themes

These are *themes* — not committed features — that frame future
investment. Specific work items live in HLRs/LLRs as they are
adopted.

*   **Better stack filtering.** Per-project allow/deny extension
    lists; user-supplied path filters.
*   **More languages.** Go, Zig, D — gated on demand and on
    Valgrind/Memcheck producing usable frames for those toolchains.
*   **Machine-readable export.** Optional JSON or SARIF output
    alongside the human-readable form, for CI annotators.
*   **Suppression-file awareness.** Surface which Valgrind
    suppressions matched, in plain text.
*   **Performance.** Maintain bounded-memory streaming as logs grow
    into the 100 MB+ range.

Anything not listed here is not on the roadmap and would require an
explicit vision update.

## 10. Relationship to the Rest of the Spec Stack

| Document | Question it answers |
| -------- | ------------------- |
| **PVD** (this document) | *Why* does this product exist, and how do we know it is succeeding? |
| [HLRs.md](HLRs.md) | *What* must the product do to deliver on the vision? |
| [LLRs.md](LLRs.md) | *How* does each function in the implementation contribute to an HLR? |
| [SDD.md](SDD.md) | *How is the implementation structured* to satisfy the LLRs? |
| [STP.md](STP.md) | *How do we verify* that each LLR (and therefore each HLR, and therefore the vision) is actually delivered? |
| [Traceability.md](Traceability.md) | *Where are the gaps*, end-to-end? |

A change to this PVD should propagate downward (some HLRs may be
added, retired, or reworded). A change discovered during
implementation that conflicts with this PVD is a signal to update
this document — not to silently diverge.
