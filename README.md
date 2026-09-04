# markus

Vibe coded single-header C++20 Markdown parser that converts Markdown 
to HTML. Implements the [CommonMark specification](https://spec.commonmark.org/) 
with zero external dependencies.

_Vibe coded with Claude Opus 4.5 and Opencode (qwen 3.6 35b)_

> [!CAUTION]
> DO NOT USE IN PRODUCTION. This is completely vibe coded and has not undergone
any reviews for memory safety.

## Features

- **Single Header**: Just include `markus.h` - no linking required
- **Zero Dependencies**: Uses only the C++20 standard library
- **CommonMark Compliant**: Passes the full CommonMark spec test suite (655 tests)
- **GitHub Flavored Markdown (GFM)**: Optional extensions for tables, autolinks, strikethrough, task lists, and tag filtering
- **High Performance**: Efficient parsing with lookup tables and inline optimizations
- **Full Unicode Support**: UTF-8 encoding/decoding, case folding, punctuation detection
- **AST Access**: Parse to an Abstract Syntax Tree for inspection or custom rendering
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
```

## Testing

The test suite uses the official CommonMark spec tests:

```bash
# Run all 655 CommonMark spec tests
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
