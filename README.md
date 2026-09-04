# markus

Vibe coded single-header C++20 Markdown parser that converts Markdown 
to HTML. Implements the [CommonMark specification](https://spec.commonmark.org/) 
with zero external dependencies.

_Vibe coded with Claude Opus 4.5 and Opencode (qwen 3.6 35b)_

> [!CAUTION]
> DO NOT USE IN PRODUCTION. This is vibe coded and has not been reviewed by a
> human. Memory safety has been checked with fuzzing (AddressSanitizer +
> UndefinedBehaviorSanitizer under both libFuzzer and AFL++; see
> [Fuzzing](#fuzzing)), which found no issues — but fuzzing finds bugs, it does
> not prove their absence.

## Features

- **Single Header**: Just include `markus.h` - no linking required
- **Zero Dependencies**: Uses only the C++20 standard library
- **CommonMark Compliant**: Passes the full CommonMark spec test suite (655 tests)
- **GitHub Flavored Markdown (GFM)**: Optional extensions for tables, autolinks, strikethrough, task lists, and tag filtering
- **High Performance**: Efficient parsing with lookup tables and inline optimizations
- **Full Unicode Support**: UTF-8 encoding/decoding, case folding, punctuation detection
- **AST Access**: Parse to an Abstract Syntax Tree for inspection or custom rendering
- **Streaming**: Feed Markdown in chunks as it arrives and emit finished blocks incrementally
- **Google Style**: Clean, readable codebase following Google C++ Style Guide

## Quick Start

### Basic Usage

```cpp
#include "markus.h"

int main() {
    std::string markdown = "# Hello, World!\n\nThis is **bold** and *italic*.";
    std::cout << markus::MarkdownToHtml(markdown);
    return 0;
}
```

Output:
```html
<h1>Hello, World!</h1>
<p>This is <strong>bold</strong> and <em>italic</em>.</p>
```

### AST Access

```cpp
#include "markus.h"

int main() {
    std::string markdown = "# Title\n\nParagraph with [a link](https://example.com).";

    // Parse to AST
    markus::Document doc = markus::Parse(markdown);

    // Inspect the AST
    std::cout << markus::DebugAst(doc);

    // Render to HTML
    std::cout << markus::RenderHtml(doc);

    return 0;
}
```

### GFM Extensions

GitHub Flavored Markdown features are opt-in via `markus::Options`. Enable any
combination with the CLI (`-e`) or programmatically:

```cpp
#include "markus.h"

int main() {
    std::string markdown =
        "| A | B |\n|---|---|\n| 1 | 2 |\n\n"
        "~~struck through~~\n\n- [x] done\n- [ ] todo\n\n"
        "Visit https://example.com for details.";

    markus::Options options;
    options.enable_tables = true;
    options.enable_strikethrough = true;
    options.enable_tasklist = true;
    options.enable_autolink = true;
    options.enable_tagfilter = true;

    std::cout << markus::MarkdownToHtml(markdown, options);
    return 0;
}
```

Available extensions:

| Extension | CLI flag | Option field | Notes |
|-----------|----------|--------------|-------|
| Tables | `-e table` | `enable_tables` | Pipe tables with alignment |
| Autolink | `-e autolink` | `enable_autolink` | Bare URLs/emails autolinked |
| Strikethrough | `-e strikethrough` | `enable_strikethrough` | `~~text~~` → `<del>` |
| Task list | `-e tasklist` | `enable_tasklist` | `- [x]` checkboxes |
| Tag filter | `-e tagfilter` | `enable_tagfilter` | Drops disallowed raw HTML tags |

### Streaming

Feed Markdown incrementally (e.g. as it arrives over a network) and emit
finished blocks through a callback as soon as they can no longer change:

```cpp
#include "markus.h"

int main() {
    markus::StreamingMarkdownParser parser;
    parser.setOutputCallback([](std::string_view html) {
        std::cout << html;  // e.g. send to a client
    });
    parser.Feed("# Hello\n\nThis arrives ");
    parser.Feed("in chunks.\n");
    parser.Flush();  // emit anything still buffered at end of input
    return 0;
}
```

Semantics:

- `Feed(chunk)` appends the chunk and emits every block that is now final.
  The trailing block that may still grow (open paragraph, list, code block,
  ...) is held back until its terminator arrives or `Flush()` is called.
- Each block's HTML is emitted exactly once, in order, and is identical to
  the output of `markus::MarkdownToHtml` for the same input.
- Link reference definitions are remembered across chunks, so a reference
  defined in an earlier chunk resolves in a later one. A reference used
  before its definition arrives renders literally (inherent to one-pass
  streaming).

GFM extensions are supported the same way as in `MarkdownToHtml` (pass a
`markus::Options` to the constructor).

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `markus::MarkdownToHtml(input)` | Convert Markdown string to HTML |
| `markus::MarkdownToHtml(input, options)` | Convert to HTML with GFM extensions enabled |
| `markus::Parse(input)` | Parse Markdown to AST (returns `Document`) |
| `markus::Parse(input, options)` | Parse to AST with GFM extensions enabled |
| `markus::RenderHtml(doc)` | Render AST to HTML |
| `markus::DebugAst(doc)` | Get a debug string representation of the AST |
| `markus::StreamingMarkdownParser` | Feed Markdown in chunks; emit completed blocks via callback (`Feed`, `Flush`, `Reset`) |
| `markus::StreamMarkdownToHtml(input, callback)` | Convenience: feed whole input, then flush |

`markus::Options` controls GFM extensions (`enable_tables`, `enable_autolink`,
`enable_strikethrough`, `enable_tasklist`, `enable_tagfilter`).

### AST Node Types

#### Block Nodes
| Type | Description |
|------|-------------|
| `Document` | Root node containing all blocks |
| `Paragraph` | Text paragraph |
| `Heading` | ATX heading (levels 1-6) |
| `ThematicBreak` | Horizontal rule (`---`, `***`, `___`) |
| `CodeBlock` | Fenced or indented code block |
| `HtmlBlock` | Raw HTML block |
| `BlockQuote` | Block quotation |
| `List` | Ordered or unordered list |
| `ListItem` | Item within a list |

#### Inline Nodes
| Type | Description |
|------|-------------|
| `Text` | Plain text content |
| `SoftBreak` | Line break within a paragraph |
| `HardBreak` | Explicit line break (`<br />`) |
| `Code` | Inline code span |
| `Emphasis` | Emphasized text (`*text*` or `_text_`) |
| `Strong` | Strong emphasis (`**text**` or `__text__`) |
| `Link` | Hyperlink |
| `Image` | Image |
| `HtmlInline` | Raw inline HTML |

## Building

### With CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build
```

Run tests:

```bash
ctest --test-dir build
```

Run the CLI tool:

```bash
echo "# Hello" | ./build/main
```

### With Other Build Systems

Since markus is a header-only library, simply add `markus.h` to your include 
path and ensure you're compiling with C++20 support:

```bash
# GCC/Clang
g++ -std=c++20 -O2 your_program.cc -o your_program

# MSVC
cl /std:c++20 /O2 your_program.cc
```

## Command-Line Tool

The included `main.cc` provides a simple CLI for converting Markdown:

```bash
# Convert Markdown from stdin to HTML
echo "**bold** text" | ./build/main
# Output: <p><strong>bold</strong> text</p>

# Print the AST instead of HTML
echo "# Title" | ./build/main --ast
# Output:
# Document
#   Heading (level 1)
#     Text: "Title"

# Enable GFM extensions (tables, strikethrough, task lists, ...)
printf '| A | B |\n|---|---|\n| 1 | 2 |\n\n~~gone~~\n- [x] done\n' \
  | ./build/main -e table -e strikethrough -e tasklist

# Stream HTML output as blocks complete (progressive rendering)
echo "**bold** text" | ./build/main --stream
```

## Testing

The test suite uses the official CommonMark spec tests:

```bash
# Run all tests (655 CommonMark spec tests + streaming API tests)
./run_tests.sh
```

GFM extension compliance is verified against cmark-gfm's GFM spec (tables,
autolinks, strikethrough, task lists, and disallowed raw HTML). It is opt-in so
it stays out of the default CommonMark run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMARKUS_BUILD_GFM_TESTS=ON
cmake --build build --target test_gfm
 ctest --test-dir build -R 'markus\.gfm\.' --output-on-failure
```

## Fuzzing

`fuzz/` contains a coverage-guided fuzzing harness (modeled on cmark-gfm's
`fuzz/`) that feeds a config-prefixed, quadratically-amplified Markdown body
through both the batch path (`Parse`/`RenderHtml`/`DebugAst`) and the streaming
path (`StreamingMarkdownParser`), with AddressSanitizer +
UndefinedBehaviorSanitizer instrumentation. Memory safety has been checked with
this harness under both libFuzzer and AFL++; no issues were found.

- `fuzz_markus_standalone` — standalone / AFL++ driver (builds with any compiler)
- `fuzz_markus` — libFuzzer harness (needs a clang that ships the fuzzer
  runtime; the default Apple clang does not)

```bash
# Replay the seed corpus (quick smoke test for crashes on known inputs)
cmake -S . -B build -DBUILD_TESTING=OFF -DMARKUS_BUILD_FUZZER=ON
cmake --build build --target fuzz_markus_standalone
./fuzz/run_fuzz.sh replay

# AFL++ (recommended locally): build with afl-clang-fast for edge-guided
# coverage, then run (see run_fuzz.sh afl for the dumb-mode shortcut)
cmake -S . -B build-afl -DBUILD_TESTING=OFF \
      -DCMAKE_CXX_COMPILER="$(which afl-clang-fast++)" \
      -DCMAKE_C_COMPILER="$(which afl-clang-fast)" \
      -DMARKUS_BUILD_FUZZER=ON
cmake --build build-afl --target fuzz_markus_standalone
afl-fuzz -m 2000 -x fuzz/fuzzing_dictionary -i fuzz/corpus -o fuzz/afl_out \
      ./build-afl/fuzz_markus_standalone @@

# libFuzzer (best in CI / Linux)
cmake -S . -B build-fuzz -DBUILD_TESTING=OFF \
      -DCMAKE_CXX_COMPILER=/path/to/fuzzer-capable/clang++ \
      -DMARKUS_BUILD_FUZZER=ON -DMARKUS_BUILD_FUZZER_LIBFUZZER=ON
cmake --build build-fuzz --target fuzz_markus
BUILDDIR=build-fuzz ./fuzz/run_fuzz.sh libfuzzer
```

`fuzz/corpus/` holds the seed inputs and `fuzz/fuzzing_dictionary` the fuzzer
dictionary; `fuzz/run_fuzz.sh` wraps the build-and-run commands
(`replay` | `libfuzzer` | `afl`).

## Supported Markdown Features

### Block Elements
- ATX headings (`# H1` through `###### H6`)
- Setext headings (underlined with `===` or `---`)
- Paragraphs
- Block quotes (`>`)
- Ordered lists (`1.`, `2)`, etc.)
- Unordered lists (`-`, `*`, `+`)
- Fenced code blocks (`` ``` `` or `~~~`)
- Indented code blocks
- Thematic breaks (`---`, `***`, `___`)
- Raw HTML blocks

### Inline Elements
- Emphasis (`*italic*` or `_italic_`)
- Strong emphasis (`**bold**` or `__bold__`)
- Code spans (`` `code` ``)
- Links (`[text](url)` and `[text][ref]`)
- Images (`![alt](url)`)
- Autolinks (`<https://example.com>`)
- Hard line breaks (trailing spaces or `\`)
- HTML entities (`&amp;`, `&#123;`, `&#x7B;`)
- Raw inline HTML
- Backslash escapes

### Link References

```markdown
[link text][ref]

[ref]: https://example.com "Optional Title"
```

### GFM Extensions (opt-in)

GitHub Flavored Markdown features, enabled individually via `Options` or the
CLI `-e` flag:

- **Tables** (`-e table`): pipe tables with header, separator, and alignment
- **Autolink** (`-e autolink`): bare URLs and email addresses become links
- **Strikethrough** (`-e strikethrough`): `~~text~~` renders as `<del>`
- **Task lists** (`-e tasklist`): `- [x]` / `- [ ]` render as checkboxes
- **Tag filter** (`-e tagfilter`): disallowed raw HTML tags are stripped

```markdown
| Col A | Col B |
|:------|------:|
| left  | right |

~~struck through~~ — done: - [x] shipped - [ ] planned
```

## Performance

Markus is optimized for speed through several techniques:

- **String Views**: Avoids unnecessary string copies using `std::string_view`
- **Lookup Tables**: O(1) character classification for punctuation, whitespace, and HTML escaping
- **Compact Node Storage**: AST nodes use 32-bit IDs instead of pointers

### Benchmarks

Benchmarks compare Markus against [cmark-gfm](https://github.com/github/cmark-gfm) and [md4c](https://github.com/mity/md4c) on GitHub Actions runners. cmark-gfm runs in plain CommonMark mode for the baseline suite, and with each GFM extension enabled for the extension benchmarks. md4c has no GFM extensions, so it runs in plain mode for the extension suite as well. A ratio below 1.00x means Markus is faster.

#### CommonMark (plain)

| Platform | Markus vs cmark-gfm | Markus vs md4c |
|----------|---------------------|----------------|
| Ubuntu (Linux) | 1.25x slower | 2.19x slower |
| Windows | 1.16x slower | 2.52x slower |
| macOS (Apple M1) | 1.06x faster | 2.10x slower |

**Average: ~1.12x vs cmark-gfm, ~2.27x slower than md4c.**

#### GFM Extensions

Each extension benchmark runs the corresponding parser with that extension enabled on extension-heavy input. Values are averaged across Ubuntu, Windows, and macOS.

| Extension | Markus vs cmark-gfm | Markus vs md4c |
|-----------|---------------------|----------------|
| Autolink | 1.11x slower | 2.49x slower |
| Strikethrough | 1.05x faster | 3.65x slower |
| Task list | 1.25x slower | 4.42x slower |
| Tag filter | 1.23x slower | 3.77x slower |

**Average: ~1.14x vs cmark-gfm, ~3.58x slower than md4c.**

The gap is most pronounced on nested block structures (block quotes, lists) and the GFM extension paths, where recursive descent parsing carries more overhead than cmark-gfm's optimized C implementation and md4c's lean table-driven core. Markus remains competitive with cmark-gfm overall and is faster than it on macOS.

## Unicode Support

- Full UTF-8 encoding and decoding
- Unicode-aware case folding for link label matching
- Unicode punctuation detection (P and S categories)
- Unicode whitespace handling (Zs category)

## Requirements

- C++20 compiler (GCC 10+, Clang 10+, MSVC 2019+)
- Standard library with `<memory_resource>` support

## License

MIT License - see [LICENSE.md](LICENSE.md)

## Acknowledgments

- [CommonMark](https://commonmark.org/) for the Markdown specification
- [cmark](https://github.com/commonmark/cmark) for the CommonMark reference implementation and spec test suite
- [cmark-gfm](https://github.com/github/cmark-gfm) for the GitHub-flavored reference implementation, benchmark, and GFM feature spec
