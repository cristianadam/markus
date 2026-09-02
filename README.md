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

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `markus::MarkdownToHtml(input)` | Convert Markdown string to HTML |
| `markus::Parse(input)` | Parse Markdown to AST (returns `Document`) |
| `markus::RenderHtml(doc)` | Render AST to HTML |
| `markus::DebugAst(doc)` | Get a debug string representation of the AST |

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
```

## Testing

The test suite uses the official CommonMark spec tests:

```bash
# Run all 655 CommonMark spec tests
./run_tests.sh
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

## Performance

Markus is optimized for speed through several techniques:

- **String Views**: Avoids unnecessary string copies using `std::string_view`
- **Lookup Tables**: O(1) character classification for punctuation, whitespace, and HTML escaping
- **Compact Node Storage**: AST nodes use 32-bit IDs instead of pointers

### Benchmarks

Benchmarks compare Markus against [cmark-gfm](https://github.com/github/cmark-gfm) (the GitHub CommonMark reference implementation, run in plain CommonMark mode) and [md4c](https://github.com/mity/md4c) on GitHub Actions runners. Results are averaged over multiple runs across Ubuntu, macOS, and Windows.

> The comparison target was switched from cmark to cmark-gfm. The figures below are from the prior cmark baseline and will be re-measured by CI.

| Platform | CPU | Markus vs cmark (baseline) | Markus vs Md4c |
|----------|-----|-----------------|----------------|
| Ubuntu (Linux) | 4x 3.5 GHz | 1.44x slower | 2.33x slower |
| macOS | 4x Apple M1 | faster | 2.11x slower |
| Windows | 4x 2.4 GHz | 1.03x slower | 2.51x slower |

**Average (cmark baseline): ~1.1x vs cmark, ~2.3x slower than md4c.**

Historically, Markus was within ~1.5x of cmark across platforms and faster than cmark on macOS, though still roughly 2-3x behind md4c. The gap was most noticeable on nested structures (block quotes, lists), where recursive descent parsing has more overhead than cmark's optimized C implementation.

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
