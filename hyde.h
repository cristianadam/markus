// Copyright 2024 Hyde Authors
// SPDX-License-Identifier: MIT
//
// Hyde: A single-header CommonMark parser for C++20
// No external dependencies beyond the C++20 standard library.

#ifndef HYDE_H_
#define HYDE_H_

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace hyde {

// =============================================================================
// Forward Declarations
// =============================================================================

struct Document;
struct Paragraph;
struct Heading;
struct ThematicBreak;
struct CodeBlock;
struct HtmlBlock;
struct BlockQuote;
struct List;
struct ListItem;
struct Text;
struct SoftBreak;
struct HardBreak;
struct Code;
struct Emphasis;
struct Strong;
struct Link;
struct Image;
struct HtmlInline;

// =============================================================================
// AST Node Types
// =============================================================================

// Node type enumeration for debugging and introspection
enum class NodeType {
  kDocument,
  kParagraph,
  kHeading,
  kThematicBreak,
  kCodeBlock,
  kHtmlBlock,
  kBlockQuote,
  kList,
  kListItem,
  kText,
  kSoftBreak,
  kHardBreak,
  kCode,
  kEmphasis,
  kStrong,
  kLink,
  kImage,
  kHtmlInline,
};

// Convert NodeType to string for debugging
inline std::string NodeTypeToString(NodeType type) {
  switch (type) {
    case NodeType::kDocument:
      return "Document";
    case NodeType::kParagraph:
      return "Paragraph";
    case NodeType::kHeading:
      return "Heading";
    case NodeType::kThematicBreak:
      return "ThematicBreak";
    case NodeType::kCodeBlock:
      return "CodeBlock";
    case NodeType::kHtmlBlock:
      return "HtmlBlock";
    case NodeType::kBlockQuote:
      return "BlockQuote";
    case NodeType::kList:
      return "List";
    case NodeType::kListItem:
      return "ListItem";
    case NodeType::kText:
      return "Text";
    case NodeType::kSoftBreak:
      return "SoftBreak";
    case NodeType::kHardBreak:
      return "HardBreak";
    case NodeType::kCode:
      return "Code";
    case NodeType::kEmphasis:
      return "Emphasis";
    case NodeType::kStrong:
      return "Strong";
    case NodeType::kLink:
      return "Link";
    case NodeType::kImage:
      return "Image";
    case NodeType::kHtmlInline:
      return "HtmlInline";
  }
  return "Unknown";
}

// Variant type for inline content
using InlineNode =
    std::variant<Text, SoftBreak, HardBreak, Code, Emphasis, Strong, Link,
                 Image, HtmlInline>;

// Variant type for block content
using BlockNode = std::variant<Paragraph, Heading, ThematicBreak, CodeBlock,
                               HtmlBlock, BlockQuote, List, ListItem>;

// =============================================================================
// Inline Node Definitions
// =============================================================================

struct Text {
  static constexpr NodeType kType = NodeType::kText;
  std::string content;

  explicit Text(std::string c) : content(std::move(c)) {}
  Text() = default;
};

struct SoftBreak {
  static constexpr NodeType kType = NodeType::kSoftBreak;
};

struct HardBreak {
  static constexpr NodeType kType = NodeType::kHardBreak;
};

struct Code {
  static constexpr NodeType kType = NodeType::kCode;
  std::string content;

  explicit Code(std::string c) : content(std::move(c)) {}
  Code() = default;
};

struct Emphasis {
  static constexpr NodeType kType = NodeType::kEmphasis;
  std::vector<InlineNode> children;
};

struct Strong {
  static constexpr NodeType kType = NodeType::kStrong;
  std::vector<InlineNode> children;
};

struct Link {
  static constexpr NodeType kType = NodeType::kLink;
  std::string destination;
  std::string title;
  std::vector<InlineNode> children;
};

struct Image {
  static constexpr NodeType kType = NodeType::kImage;
  std::string destination;
  std::string title;
  std::string alt_text;
};

struct HtmlInline {
  static constexpr NodeType kType = NodeType::kHtmlInline;
  std::string content;

  explicit HtmlInline(std::string c) : content(std::move(c)) {}
  HtmlInline() = default;
};

// =============================================================================
// Block Node Definitions
// =============================================================================

struct Paragraph {
  static constexpr NodeType kType = NodeType::kParagraph;
  std::vector<InlineNode> children;
  std::string raw_content;  // For debugging
};

struct Heading {
  static constexpr NodeType kType = NodeType::kHeading;
  int level = 1;  // 1-6
  std::vector<InlineNode> children;
  std::string raw_content;  // For debugging
};

struct ThematicBreak {
  static constexpr NodeType kType = NodeType::kThematicBreak;
};

struct CodeBlock {
  static constexpr NodeType kType = NodeType::kCodeBlock;
  std::string info_string;  // Language hint (e.g., "cpp", "python")
  std::string content;
  bool is_fenced = false;
};

struct HtmlBlock {
  static constexpr NodeType kType = NodeType::kHtmlBlock;
  std::string content;
  int block_type = 0;  // CommonMark HTML block type (1-7)
};

struct ListItem {
  static constexpr NodeType kType = NodeType::kListItem;
  std::vector<BlockNode> children;
  bool is_tight = true;
};

struct List {
  static constexpr NodeType kType = NodeType::kList;
  bool is_ordered = false;
  int start = 1;           // Starting number for ordered lists
  char delimiter = '.';    // '.' or ')' for ordered lists
  char bullet_char = '-';  // '-', '+', or '*' for unordered lists
  bool is_tight = true;
  std::vector<ListItem> items;
};

struct BlockQuote {
  static constexpr NodeType kType = NodeType::kBlockQuote;
  std::vector<BlockNode> children;
};

struct Document {
  static constexpr NodeType kType = NodeType::kDocument;
  std::vector<BlockNode> children;

  // Link reference definitions
  std::unordered_map<std::string, std::pair<std::string, std::string>>
      link_references;  // label -> (destination, title)
};

// =============================================================================
// Utility Functions
// =============================================================================

namespace detail {

// Check if a character is ASCII punctuation
inline bool IsAsciiPunctuation(char c) {
  return (c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) ||
         (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7E);
}

// Get the byte length of a UTF-8 character starting at the given byte
inline size_t Utf8CharLen(unsigned char c) {
  if ((c & 0x80) == 0) return 1;       // ASCII
  if ((c & 0xE0) == 0xC0) return 2;    // 110xxxxx
  if ((c & 0xF0) == 0xE0) return 3;    // 1110xxxx
  if ((c & 0xF8) == 0xF0) return 4;    // 11110xxx
  return 1;  // Invalid, treat as single byte
}

// Decode UTF-8 code point at position, return code point and bytes consumed
inline std::pair<uint32_t, size_t> DecodeUtf8At(std::string_view s, size_t pos) {
  if (pos >= s.size()) return {0, 0};
  unsigned char c = static_cast<unsigned char>(s[pos]);
  if ((c & 0x80) == 0) {
    return {c, 1};
  }
  if ((c & 0xE0) == 0xC0 && pos + 1 < s.size()) {
    uint32_t cp = (c & 0x1F) << 6;
    cp |= (static_cast<unsigned char>(s[pos + 1]) & 0x3F);
    return {cp, 2};
  }
  if ((c & 0xF0) == 0xE0 && pos + 2 < s.size()) {
    uint32_t cp = (c & 0x0F) << 12;
    cp |= (static_cast<unsigned char>(s[pos + 1]) & 0x3F) << 6;
    cp |= (static_cast<unsigned char>(s[pos + 2]) & 0x3F);
    return {cp, 3};
  }
  if ((c & 0xF8) == 0xF0 && pos + 3 < s.size()) {
    uint32_t cp = (c & 0x07) << 18;
    cp |= (static_cast<unsigned char>(s[pos + 1]) & 0x3F) << 12;
    cp |= (static_cast<unsigned char>(s[pos + 2]) & 0x3F) << 6;
    cp |= (static_cast<unsigned char>(s[pos + 3]) & 0x3F);
    return {cp, 4};
  }
  return {c, 1};
}

// Check if a Unicode code point is in P (punctuation) or S (symbol) categories
// This is used for CommonMark's definition of "Unicode punctuation character"
inline bool IsUnicodePunctuation(uint32_t cp) {
  // ASCII punctuation
  if (cp < 0x80) {
    return IsAsciiPunctuation(static_cast<char>(cp));
  }
  // Common Unicode punctuation and symbols (P and S categories)
  // This covers the most common cases; a full implementation would need
  // complete Unicode category tables

  // General Punctuation (U+2000-U+206F)
  if (cp >= 0x2000 && cp <= 0x206F) return true;
  // Supplemental Punctuation (U+2E00-U+2E7F)
  if (cp >= 0x2E00 && cp <= 0x2E7F) return true;
  // CJK Symbols and Punctuation (U+3000-U+303F)
  if (cp >= 0x3000 && cp <= 0x303F) return true;
  // Currency Symbols (U+20A0-U+20CF) - Sc category
  if (cp >= 0x20A0 && cp <= 0x20CF) return true;
  // Latin-1 punctuation and symbols
  if (cp >= 0x00A0 && cp <= 0x00BF) return true;  // Includes £ (00A3), ¥, etc.
  // Modifier symbols, letterlike symbols
  if (cp >= 0x02B0 && cp <= 0x02FF) return true;
  // Misc symbols (U+2600-U+26FF)
  if (cp >= 0x2600 && cp <= 0x26FF) return true;
  // Dingbats (U+2700-U+27BF)
  if (cp >= 0x2700 && cp <= 0x27BF) return true;
  // Math operators (U+2200-U+22FF)
  if (cp >= 0x2200 && cp <= 0x22FF) return true;
  // Arrows (U+2190-U+21FF)
  if (cp >= 0x2190 && cp <= 0x21FF) return true;
  // Box drawing (U+2500-U+257F)
  if (cp >= 0x2500 && cp <= 0x257F) return true;
  // Geometric shapes (U+25A0-U+25FF)
  if (cp >= 0x25A0 && cp <= 0x25FF) return true;
  // Misc Technical (U+2300-U+23FF)
  if (cp >= 0x2300 && cp <= 0x23FF) return true;
  // Halfwidth/Fullwidth punctuation (U+FF00-U+FFEF)
  if (cp >= 0xFF00 && cp <= 0xFFEF) return true;
  // Small form variants (U+FE50-U+FE6F)
  if (cp >= 0xFE50 && cp <= 0xFE6F) return true;
  // CJK compatibility (U+FE30-U+FE4F)
  if (cp >= 0xFE30 && cp <= 0xFE4F) return true;
  // Adlam Numbers (U+1E2FF is in this area - 𞋿)
  if (cp >= 0x1E2C0 && cp <= 0x1E2FF) return true;
  // Emoticons, emoji, etc. (various planes) - treat as symbols
  if (cp >= 0x1F300 && cp <= 0x1F9FF) return true;

  return false;
}

// Check if code point is Unicode whitespace (Zs category + ASCII whitespace)
inline bool IsUnicodeWhitespaceCodepoint(uint32_t cp) {
  // ASCII whitespace
  if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == '\f') {
    return true;
  }
  // Unicode Zs (space separator) category
  if (cp == 0x00A0) return true;  // NO-BREAK SPACE
  if (cp == 0x1680) return true;  // OGHAM SPACE MARK
  if (cp >= 0x2000 && cp <= 0x200A) return true;  // Various spaces
  if (cp == 0x202F) return true;  // NARROW NO-BREAK SPACE
  if (cp == 0x205F) return true;  // MEDIUM MATHEMATICAL SPACE
  if (cp == 0x3000) return true;  // IDEOGRAPHIC SPACE
  return false;
}

// Check if a character is Unicode whitespace (simplified)
// Includes ASCII whitespace and common Unicode whitespace chars
inline bool IsUnicodeWhitespace(char c) {
  unsigned char uc = static_cast<unsigned char>(c);
  return uc == ' ' || uc == '\t' || uc == '\n' || uc == '\r' || uc == '\f';
}

// Check for Unicode whitespace at position in a string (handles multi-byte UTF-8)
// Returns number of bytes to skip if whitespace, 0 otherwise
inline size_t IsUnicodeWhitespaceAt(std::string_view s, size_t pos) {
  if (pos >= s.size()) return 0;
  unsigned char c = static_cast<unsigned char>(s[pos]);
  if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
    return 1;
  }
  // UTF-8 NO-BREAK SPACE: 0xC2 0xA0
  if (c == 0xC2 && pos + 1 < s.size() &&
      static_cast<unsigned char>(s[pos + 1]) == 0xA0) {
    return 2;
  }
  return 0;
}

// Convert a Unicode code point to UTF-8
inline std::string CodePointToUtf8(uint32_t cp) {
  std::string result;
  if (cp == 0) {
    // Null character becomes replacement character
    result = "\xEF\xBF\xBD";
  } else if (cp < 0x80) {
    result += static_cast<char>(cp);
  } else if (cp < 0x800) {
    result += static_cast<char>(0xC0 | (cp >> 6));
    result += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    result += static_cast<char>(0xE0 | (cp >> 12));
    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    result += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x110000) {
    result += static_cast<char>(0xF0 | (cp >> 18));
    result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    result += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    // Invalid code point - use replacement character
    result = "\xEF\xBF\xBD";
  }
  return result;
}

// Simple Unicode case-folding for a code point
// Returns the lowercase equivalent (or the same code point if no folding)
inline uint32_t UnicodeCaseFold(uint32_t cp) {
  // ASCII uppercase -> lowercase
  if (cp >= 'A' && cp <= 'Z') {
    return cp + 32;
  }
  // Latin-1 Supplement uppercase (U+00C0-U+00DE, except U+00D7 multiplication sign)
  if (cp >= 0x00C0 && cp <= 0x00D6) {
    return cp + 32;  // À-Ö -> à-ö
  }
  if (cp >= 0x00D8 && cp <= 0x00DE) {
    return cp + 32;  // Ø-Þ -> ø-þ
  }
  // Latin Extended-A (pairs mostly 32 apart)
  if (cp >= 0x0100 && cp <= 0x012F) {
    if ((cp & 1) == 0) return cp + 1;  // Even code points are uppercase
  }
  if (cp >= 0x0132 && cp <= 0x0137) {
    if ((cp & 1) == 0) return cp + 1;
  }
  if (cp >= 0x0139 && cp <= 0x0148) {
    if ((cp & 1) == 1) return cp + 1;  // Odd code points are uppercase here
  }
  if (cp >= 0x014A && cp <= 0x0177) {
    if ((cp & 1) == 0) return cp + 1;
  }
  if (cp == 0x0178) return 0x00FF;  // Ÿ -> ÿ
  if (cp >= 0x0179 && cp <= 0x017E) {
    if ((cp & 1) == 1) return cp + 1;
  }
  // Greek uppercase (U+0391-U+03A9) -> lowercase (U+03B1-U+03C9)
  if (cp >= 0x0391 && cp <= 0x03A1) {
    return cp + 32;  // Α-Ρ -> α-ρ
  }
  if (cp >= 0x03A3 && cp <= 0x03A9) {
    return cp + 32;  // Σ-Ω -> σ-ω
  }
  // Cyrillic uppercase (U+0410-U+042F) -> lowercase (U+0430-U+044F)
  if (cp >= 0x0410 && cp <= 0x042F) {
    return cp + 32;
  }
  // More Cyrillic (U+0400-U+040F) -> (U+0450-U+045F)
  if (cp >= 0x0400 && cp <= 0x040F) {
    return cp + 80;
  }
  // German capital sharp S (ẞ U+1E9E) is handled specially in NormalizeLinkLabel
  // because it folds to "ss" (two characters) in full case folding
  // Latin Extended Additional
  if (cp >= 0x1E00 && cp <= 0x1E95) {
    if ((cp & 1) == 0) return cp + 1;
  }
  // Latin Extended-B (various)
  if (cp >= 0x01A0 && cp <= 0x01A5) {
    if ((cp & 1) == 0) return cp + 1;
  }
  if (cp >= 0x01DE && cp <= 0x01EF) {
    if ((cp & 1) == 0) return cp + 1;
  }
  // Greek Extended
  if (cp >= 0x1F08 && cp <= 0x1F0F) return cp - 8;
  if (cp >= 0x1F18 && cp <= 0x1F1D) return cp - 8;
  if (cp >= 0x1F28 && cp <= 0x1F2F) return cp - 8;
  if (cp >= 0x1F38 && cp <= 0x1F3F) return cp - 8;
  if (cp >= 0x1F48 && cp <= 0x1F4D) return cp - 8;
  if (cp >= 0x1F68 && cp <= 0x1F6F) return cp - 8;

  return cp;  // No case folding
}

// Normalize a link label (case-fold and collapse whitespace)
// Uses Unicode-aware case folding for CommonMark compliance
inline std::string NormalizeLinkLabel(std::string_view label) {
  std::string result;
  result.reserve(label.size());
  bool in_whitespace = false;
  bool first = true;
  size_t i = 0;

  while (i < label.size()) {
    // Check for whitespace (including multi-byte Unicode whitespace)
    size_t ws_len = IsUnicodeWhitespaceAt(label, i);
    if (ws_len > 0) {
      if (!first && !in_whitespace) {
        result += ' ';
        in_whitespace = true;
      }
      i += ws_len;
      continue;
    }

    // Decode UTF-8 code point
    auto [cp, len] = DecodeUtf8At(label, i);
    if (len == 0) {
      ++i;
      continue;
    }

    // Apply case folding - special handling for characters that fold to multiple chars
    if (cp == 0x1E9E) {
      // German capital sharp S (ẞ) folds to "ss" in full case folding
      result += "ss";
    } else {
      uint32_t folded = UnicodeCaseFold(cp);
      // Encode back to UTF-8
      result += CodePointToUtf8(folded);
    }

    in_whitespace = false;
    first = false;
    i += len;
  }

  // Trim trailing space
  if (!result.empty() && result.back() == ' ') {
    result.pop_back();
  }

  return result;
}

// HTML entity escaping
inline std::string EscapeHtml(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (char c : text) {
    switch (c) {
      case '&':
        result += "&amp;";
        break;
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '"':
        result += "&quot;";
        break;
      default:
        result += c;
        break;
    }
  }
  return result;
}

// URL encoding for link destinations
inline std::string EncodeUrl(std::string_view url) {
  std::string result;
  result.reserve(url.size());
  for (unsigned char c : url) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ||
        c == '/' || c == ':' || c == '?' || c == '#' || c == '@' || c == '!' ||
        c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' || c == '*' ||
        c == '+' || c == ',' || c == ';' || c == '=' || c == '%') {
      result += static_cast<char>(c);
    } else {
      char buf[4];
      std::snprintf(buf, sizeof(buf), "%%%02X", c);
      result += buf;
    }
  }
  return result;
}

// Trim leading and trailing whitespace
inline std::string_view Trim(std::string_view s) {
  size_t start = 0;
  while (start < s.size() && IsUnicodeWhitespace(s[start])) {
    ++start;
  }
  size_t end = s.size();
  while (end > start && IsUnicodeWhitespace(s[end - 1])) {
    --end;
  }
  return s.substr(start, end - start);
}

// Trim only leading whitespace
inline std::string_view TrimLeft(std::string_view s) {
  size_t start = 0;
  while (start < s.size() && IsUnicodeWhitespace(s[start])) {
    ++start;
  }
  return s.substr(start);
}

// Count leading spaces (tabs count as 4 spaces to next tab stop)
inline int CountIndent(std::string_view line, int* consumed_chars = nullptr) {
  int indent = 0;
  size_t i = 0;
  for (; i < line.size(); ++i) {
    if (line[i] == ' ') {
      ++indent;
    } else if (line[i] == '\t') {
      indent = (indent / 4 + 1) * 4;  // Advance to next tab stop
    } else {
      break;
    }
  }
  if (consumed_chars) {
    *consumed_chars = static_cast<int>(i);
  }
  return indent;
}

// Remove N spaces of indentation (handling tabs)
inline std::string RemoveIndent(std::string_view line, int n) {
  std::string result;
  int removed = 0;
  size_t i = 0;

  while (i < line.size() && removed < n) {
    if (line[i] == ' ') {
      ++removed;
      ++i;
    } else if (line[i] == '\t') {
      int tab_width = 4 - (removed % 4);
      if (removed + tab_width <= n) {
        removed += tab_width;
        ++i;
      } else {
        // Partial tab - add remaining spaces
        int remaining = n - removed;
        removed = n;
        ++i;
        // Add spaces for the portion of the tab we didn't use
        for (int j = 0; j < tab_width - remaining; ++j) {
          result += ' ';
        }
      }
    } else {
      break;
    }
  }

  result += line.substr(i);
  return result;
}

// Remove blockquote prefix (> and optional following space) with proper tab handling
// Returns the remaining content with proper indentation preserved
inline std::string RemoveBlockQuotePrefix(std::string_view line) {
  size_t i = 0;
  int column = 0;

  // Skip leading whitespace (up to 3 spaces)
  while (i < line.size() && column < 3) {
    if (line[i] == ' ') {
      ++column;
      ++i;
    } else if (line[i] == '\t') {
      int next_col = (column / 4 + 1) * 4;
      if (next_col <= 3) {
        column = next_col;
        ++i;
      } else {
        break;
      }
    } else {
      break;
    }
  }

  // Check for >
  if (i >= line.size() || line[i] != '>') {
    return std::string(line);
  }
  ++i;
  ++column;

  // Handle optional space after > (can be partial tab)
  int content_start_column = column;
  if (i < line.size()) {
    if (line[i] == ' ') {
      ++i;
      content_start_column = column + 1;
    } else if (line[i] == '\t') {
      // Tab after > - consume part of the tab as the "optional space"
      // The remaining virtual spaces stay as indentation
      int tab_end = (column / 4 + 1) * 4;
      int spaces_from_tab = tab_end - column;
      // Consume one space's worth, rest becomes leading indent
      ++i;
      content_start_column = tab_end;
      // The remaining (spaces_from_tab - 1) spaces become content indent
      std::string result;
      for (int j = 0; j < spaces_from_tab - 1; ++j) {
        result += ' ';
      }
      // Expand remaining tabs in the content
      int col = content_start_column;
      while (i < line.size()) {
        if (line[i] == '\t') {
          int next_col = (col / 4 + 1) * 4;
          for (int k = 0; k < next_col - col; ++k) {
            result += ' ';
          }
          col = next_col;
        } else {
          result += line[i];
          ++col;
        }
        ++i;
      }
      return result;
    }
  }

  // Expand any remaining tabs in content
  std::string result;
  int col = content_start_column;
  while (i < line.size()) {
    if (line[i] == '\t') {
      int next_col = (col / 4 + 1) * 4;
      for (int k = 0; k < next_col - col; ++k) {
        result += ' ';
      }
      col = next_col;
    } else {
      result += line[i];
      ++col;
    }
    ++i;
  }
  return result;
}

// Expand tabs to spaces (tab stops every 4 columns)
inline std::string ExpandTabs(std::string_view line) {
  std::string result;
  int column = 0;
  for (char c : line) {
    if (c == '\t') {
      int next_col = (column / 4 + 1) * 4;
      int spaces = next_col - column;
      for (int j = 0; j < spaces; ++j) {
        result += ' ';
      }
      column = next_col;
    } else {
      result += c;
      ++column;
    }
  }
  return result;
}

// Check if line is blank (only whitespace)
inline bool IsBlankLine(std::string_view line) {
  for (char c : line) {
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
      return false;
    }
  }
  return true;
}

// Split input into lines
inline std::vector<std::string> SplitLines(std::string_view input) {
  std::vector<std::string> lines;
  std::string current_line;

  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    if (c == '\n') {
      lines.push_back(std::move(current_line));
      current_line.clear();
    } else if (c == '\r') {
      lines.push_back(std::move(current_line));
      current_line.clear();
      if (i + 1 < input.size() && input[i + 1] == '\n') {
        ++i;  // Skip LF after CR
      }
    } else if (c == '\0') {
      // Replace null with replacement character
      current_line += "\xEF\xBF\xBD";  // UTF-8 for U+FFFD
    } else {
      current_line += c;
    }
  }

  // Add last line if non-empty or if input ended with newline
  if (!current_line.empty() ||
      (!input.empty() && (input.back() == '\n' || input.back() == '\r'))) {
    lines.push_back(std::move(current_line));
  }

  // Handle empty input
  if (lines.empty() && !input.empty()) {
    lines.push_back(std::string(input));
  }

  return lines;
}

// HTML named entity map (common entities from CommonMark spec)
inline std::string LookupHtmlEntity(std::string_view name) {
  // This is a subset - full CommonMark compliance requires all HTML5 entities
  static const std::unordered_map<std::string, std::string> entities = {
      {"nbsp", "\xC2\xA0"},
      {"amp", "&"},
      {"lt", "<"},
      {"gt", ">"},
      {"quot", "\""},
      {"apos", "'"},
      {"copy", "\xC2\xA9"},
      {"reg", "\xC2\xAE"},
      {"AElig", "\xC3\x86"},
      {"Dcaron", "\xC4\x8E"},
      {"frac34", "\xC2\xBE"},
      {"HilbertSpace", "\xE2\x84\x8B"},
      {"DifferentialD", "\xE2\x85\x86"},
      {"ClockwiseContourIntegral", "\xE2\x88\xB2"},
      {"ngE", "\xE2\x89\xA7\xCC\xB8"},
      {"ouml", "\xC3\xB6"},
      {"Ouml", "\xC3\x96"},
      {"auml", "\xC3\xA4"},
      {"Auml", "\xC3\x84"},
      {"uuml", "\xC3\xBC"},
      {"Uuml", "\xC3\x9C"},
      {"szlig", "\xC3\x9F"},
      {"euro", "\xE2\x82\xAC"},
      {"pound", "\xC2\xA3"},
      {"yen", "\xC2\xA5"},
      {"cent", "\xC2\xA2"},
      {"deg", "\xC2\xB0"},
      {"plusmn", "\xC2\xB1"},
      {"times", "\xC3\x97"},
      {"divide", "\xC3\xB7"},
      {"frac12", "\xC2\xBD"},
      {"frac14", "\xC2\xBC"},
      {"para", "\xC2\xB6"},
      {"sect", "\xC2\xA7"},
      {"dagger", "\xE2\x80\xA0"},
      {"Dagger", "\xE2\x80\xA1"},
      {"bull", "\xE2\x80\xA2"},
      {"hellip", "\xE2\x80\xA6"},
      {"ndash", "\xE2\x80\x93"},
      {"mdash", "\xE2\x80\x94"},
      {"lsquo", "\xE2\x80\x98"},
      {"rsquo", "\xE2\x80\x99"},
      {"ldquo", "\xE2\x80\x9C"},
      {"rdquo", "\xE2\x80\x9D"},
      {"laquo", "\xC2\xAB"},
      {"raquo", "\xC2\xBB"},
      {"trade", "\xE2\x84\xA2"},
      {"larr", "\xE2\x86\x90"},
      {"rarr", "\xE2\x86\x92"},
      {"uarr", "\xE2\x86\x91"},
      {"darr", "\xE2\x86\x93"},
      {"harr", "\xE2\x86\x94"},
      {"spades", "\xE2\x99\xA0"},
      {"clubs", "\xE2\x99\xA3"},
      {"hearts", "\xE2\x99\xA5"},
      {"diams", "\xE2\x99\xA6"},
  };

  std::string name_str(name);
  auto it = entities.find(name_str);
  if (it != entities.end()) {
    return it->second;
  }
  return "";
}

// Decode HTML entities (named, decimal, hex) and backslash escapes
inline std::string DecodeEscapesAndEntities(std::string_view text) {
  std::string result;
  result.reserve(text.size());

  for (size_t i = 0; i < text.size(); ++i) {
    // Backslash escape
    if (text[i] == '\\' && i + 1 < text.size() &&
        IsAsciiPunctuation(text[i + 1])) {
      result += text[++i];
      continue;
    }

    // HTML entity
    if (text[i] == '&' && i + 1 < text.size()) {
      size_t start = i + 1;

      // Numeric character reference
      if (text[start] == '#') {
        size_t num_start = start + 1;
        bool is_hex = false;

        if (num_start < text.size() &&
            (text[num_start] == 'x' || text[num_start] == 'X')) {
          is_hex = true;
          ++num_start;
        }

        size_t num_end = num_start;
        if (is_hex) {
          while (num_end < text.size() &&
                 std::isxdigit(static_cast<unsigned char>(text[num_end]))) {
            ++num_end;
          }
        } else {
          while (num_end < text.size() &&
                 std::isdigit(static_cast<unsigned char>(text[num_end]))) {
            ++num_end;
          }
        }

        if (num_end > num_start && num_end < text.size() &&
            text[num_end] == ';') {
          std::string num_str(text.substr(num_start, num_end - num_start));
          try {
            uint32_t code_point;
            if (is_hex) {
              code_point = static_cast<uint32_t>(std::stoul(num_str, nullptr, 16));
            } else {
              code_point = static_cast<uint32_t>(std::stoul(num_str, nullptr, 10));
            }
            result += CodePointToUtf8(code_point);
            i = num_end;
            continue;
          } catch (...) {
            // Invalid number, treat as literal
          }
        }
      } else {
        // Named entity
        size_t name_end = start;
        while (name_end < text.size() &&
               std::isalnum(static_cast<unsigned char>(text[name_end]))) {
          ++name_end;
        }

        if (name_end > start && name_end < text.size() &&
            text[name_end] == ';') {
          std::string entity = LookupHtmlEntity(text.substr(start, name_end - start));
          if (!entity.empty()) {
            result += entity;
            i = name_end;
            continue;
          }
        }
      }
    }

    result += text[i];
  }

  return result;
}

// Decode backslash escapes only (no entities)
inline std::string DecodeEscapes(std::string_view text) {
  std::string result;
  result.reserve(text.size());

  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1 < text.size() &&
        IsAsciiPunctuation(text[i + 1])) {
      result += text[++i];
    } else {
      result += text[i];
    }
  }

  return result;
}

}  // namespace detail

// =============================================================================
// Inline Parser
// =============================================================================

class InlineParser {
 public:
  explicit InlineParser(
      const std::unordered_map<std::string, std::pair<std::string, std::string>>*
          link_refs = nullptr)
      : link_references_(link_refs) {}

  std::vector<InlineNode> Parse(std::string_view text) {
    text_ = text;
    pos_ = 0;
    return ParseInlines();
  }

 private:
  std::string_view text_;
  size_t pos_ = 0;
  const std::unordered_map<std::string, std::pair<std::string, std::string>>*
      link_references_ = nullptr;

  struct DelimiterNode {
    size_t pos;
    size_t count;
    char delimiter;
    bool can_open;
    bool can_close;
    bool active;
    std::vector<InlineNode>* target;
  };

  std::vector<InlineNode> ParseInlines() {
    std::vector<InlineNode> result;
    std::vector<DelimiterNode> delimiter_stack;
    std::string pending_text;

    auto flush_text = [&]() {
      if (!pending_text.empty()) {
        result.push_back(Text(std::move(pending_text)));
        pending_text.clear();
      }
    };

    while (pos_ < text_.size()) {
      char c = text_[pos_];

      // Check for backslash escape
      if (c == '\\' && pos_ + 1 < text_.size()) {
        char next = text_[pos_ + 1];
        if (detail::IsAsciiPunctuation(next)) {
          flush_text();
          result.push_back(Text(std::string(1, next)));
          pos_ += 2;
          continue;
        } else if (next == '\n') {
          flush_text();
          result.push_back(HardBreak{});
          pos_ += 2;
          continue;
        }
      }

      // Check for HTML entity or numeric character reference
      if (c == '&' && pos_ + 1 < text_.size()) {
        auto entity_result = TryParseEntity();
        if (entity_result) {
          pending_text += *entity_result;
          continue;
        }
      }

      // Check for code span
      if (c == '`') {
        auto code_span = TryParseCodeSpan();
        if (code_span) {
          flush_text();
          result.push_back(std::move(*code_span));
          continue;
        }
        // Code span didn't match - add the entire backtick run as text
        // and skip past it to avoid trying shorter runs
        size_t backtick_count = 0;
        while (pos_ + backtick_count < text_.size() &&
               text_[pos_ + backtick_count] == '`') {
          ++backtick_count;
        }
        pending_text += std::string(backtick_count, '`');
        pos_ += backtick_count;
        continue;
      }

      // Check for autolink
      if (c == '<') {
        auto autolink = TryParseAutolink();
        if (autolink) {
          flush_text();
          result.push_back(std::move(*autolink));
          continue;
        }

        // Check for HTML tag
        auto html = TryParseHtmlInline();
        if (html) {
          flush_text();
          result.push_back(std::move(*html));
          continue;
        }
      }

      // Check for emphasis markers
      if (c == '*' || c == '_') {
        size_t run_start = pos_;
        size_t run_length = 0;
        while (pos_ + run_length < text_.size() &&
               text_[pos_ + run_length] == c) {
          ++run_length;
        }

        bool left_flanking = IsLeftFlanking(run_start, run_length);
        bool right_flanking = IsRightFlanking(run_start, run_length);

        bool can_open = left_flanking;
        bool can_close = right_flanking;

        if (c == '_') {
          can_open =
              left_flanking &&
              (!right_flanking ||
               (run_start > 0 &&
                detail::IsAsciiPunctuation(text_[run_start - 1])));
          can_close =
              right_flanking &&
              (!left_flanking ||
               (run_start + run_length < text_.size() &&
                detail::IsAsciiPunctuation(text_[run_start + run_length])));
        }

        flush_text();

        // Add delimiter to stack
        delimiter_stack.push_back(
            {result.size(), run_length, c, can_open, can_close, true, &result});

        // Add the delimiter characters as text for now
        result.push_back(Text(std::string(run_length, c)));
        pos_ += run_length;
        continue;
      }

      // Check for link/image start
      if (c == '[' ||
          (c == '!' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '[')) {
        bool is_image = (c == '!');
        flush_text();

        size_t bracket_pos = result.size();
        delimiter_stack.push_back(
            {bracket_pos, 1, is_image ? '!' : '[', true, false, true, &result});

        if (is_image) {
          result.push_back(Text("!["));
          pos_ += 2;
        } else {
          result.push_back(Text("["));
          pos_ += 1;
        }
        continue;
      }

      // Check for link close
      if (c == ']') {
        flush_text();

        // Look for matching opener
        auto opener = FindLinkOpener(delimiter_stack);
        if (opener != delimiter_stack.end() && opener->active) {
          // Try to parse link destination
          size_t saved_pos = pos_;
          ++pos_;  // Skip ']'

          auto link_result = TryParseLinkTail();
          if (link_result) {
            // Build the link/image
            bool is_image = (opener->delimiter == '!');

            // Collect inline content between opener and closer
            std::vector<InlineNode> link_content;
            for (size_t i = opener->pos + 1; i < result.size(); ++i) {
              link_content.push_back(std::move(result[i]));
            }

            // Remove everything from opener position
            result.resize(opener->pos);

            if (is_image) {
              Image img;
              img.destination = std::move(link_result->first);
              img.title = std::move(link_result->second);
              img.alt_text = GetAltText(link_content);
              result.push_back(std::move(img));
            } else {
              Link link;
              link.destination = std::move(link_result->first);
              link.title = std::move(link_result->second);
              link.children = std::move(link_content);
              result.push_back(std::move(link));
            }

            // Deactivate openers
            for (auto it = opener; it != delimiter_stack.end(); ++it) {
              if (it->delimiter == '[' || it->delimiter == '!') {
                it->active = false;
              }
            }
            delimiter_stack.erase(opener, delimiter_stack.end());
            continue;
          }

          // Try shortcut reference link: [foo] with no following () or []
          // Look up the text inside brackets as a reference label
          std::string label_text;
          for (size_t i = opener->pos + 1; i < result.size(); ++i) {
            std::visit(
                [&label_text](auto&& arg) {
                  using T = std::decay_t<decltype(arg)>;
                  if constexpr (std::is_same_v<T, Text>) {
                    label_text += arg.content;
                  }
                },
                result[i]);
          }

          auto ref_result = LookupReference(label_text);
          if (ref_result) {
            bool is_image = (opener->delimiter == '!');

            std::vector<InlineNode> link_content;
            for (size_t i = opener->pos + 1; i < result.size(); ++i) {
              link_content.push_back(std::move(result[i]));
            }

            result.resize(opener->pos);

            if (is_image) {
              Image img;
              img.destination = std::move(ref_result->first);
              img.title = std::move(ref_result->second);
              img.alt_text = label_text;
              result.push_back(std::move(img));
            } else {
              Link link;
              link.destination = std::move(ref_result->first);
              link.title = std::move(ref_result->second);
              link.children = std::move(link_content);
              result.push_back(std::move(link));
            }

            for (auto it = opener; it != delimiter_stack.end(); ++it) {
              if (it->delimiter == '[' || it->delimiter == '!') {
                it->active = false;
              }
            }
            delimiter_stack.erase(opener, delimiter_stack.end());
            ++pos_;  // Skip ']'
            continue;
          }

          pos_ = saved_pos;
          // Deactivate this opener since it couldn't form a link
          opener->active = false;
        }

        result.push_back(Text("]"));
        ++pos_;
        continue;
      }

      // Check for hard break (two spaces at end of line)
      if (c == ' ') {
        size_t space_count = 0;
        while (pos_ + space_count < text_.size() &&
               text_[pos_ + space_count] == ' ') {
          ++space_count;
        }
        if (pos_ + space_count < text_.size() &&
            text_[pos_ + space_count] == '\n') {
          if (space_count >= 2) {
            flush_text();
            result.push_back(HardBreak{});
            pos_ += space_count + 1;
            continue;
          }
        }
      }

      // Check for soft break
      if (c == '\n') {
        // Trim trailing spaces from pending text
        while (!pending_text.empty() && pending_text.back() == ' ') {
          pending_text.pop_back();
        }
        flush_text();
        result.push_back(SoftBreak{});
        ++pos_;
        continue;
      }

      // Regular character
      pending_text += c;
      ++pos_;
    }

    flush_text();

    // Process emphasis
    ProcessEmphasis(result, delimiter_stack);

    return result;
  }

  // Find the start position of the UTF-8 character before the given position
  size_t FindPrevCharStart(size_t pos) const {
    if (pos == 0) return 0;
    size_t i = pos - 1;
    // Walk back through continuation bytes (10xxxxxx)
    while (i > 0 && (static_cast<unsigned char>(text_[i]) & 0xC0) == 0x80) {
      --i;
    }
    return i;
  }

  bool IsLeftFlanking(size_t pos, size_t length) {
    if (pos + length >= text_.size()) return false;

    // Get code point after the delimiter run
    auto [after_cp, after_len] = detail::DecodeUtf8At(text_, pos + length);

    // Not left-flanking if followed by Unicode whitespace
    if (detail::IsUnicodeWhitespaceCodepoint(after_cp)) return false;

    // Left-flanking if not followed by Unicode punctuation
    if (!detail::IsUnicodePunctuation(after_cp)) return true;

    // Followed by punctuation - check if preceded by whitespace or punctuation
    if (pos == 0) return true;  // Start of string is like whitespace

    // Get code point before the delimiter run
    size_t before_start = FindPrevCharStart(pos);
    auto [before_cp, before_len] = detail::DecodeUtf8At(text_, before_start);

    return detail::IsUnicodeWhitespaceCodepoint(before_cp) ||
           detail::IsUnicodePunctuation(before_cp);
  }

  bool IsRightFlanking(size_t pos, size_t length) {
    if (pos == 0) return false;

    // Get code point before the delimiter run
    size_t before_start = FindPrevCharStart(pos);
    auto [before_cp, before_len] = detail::DecodeUtf8At(text_, before_start);

    // Not right-flanking if preceded by Unicode whitespace
    if (detail::IsUnicodeWhitespaceCodepoint(before_cp)) return false;

    // Right-flanking if not preceded by Unicode punctuation
    if (!detail::IsUnicodePunctuation(before_cp)) return true;

    // Preceded by punctuation - check if followed by whitespace or punctuation
    if (pos + length >= text_.size()) return true;  // End of string is like whitespace

    // Get code point after the delimiter run
    auto [after_cp, after_len] = detail::DecodeUtf8At(text_, pos + length);

    return detail::IsUnicodeWhitespaceCodepoint(after_cp) ||
           detail::IsUnicodePunctuation(after_cp);
  }

  std::vector<DelimiterNode>::iterator FindLinkOpener(
      std::vector<DelimiterNode>& stack) {
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
      if ((it->delimiter == '[' || it->delimiter == '!') && it->active) {
        return std::prev(it.base());
      }
    }
    return stack.end();
  }

  std::optional<Code> TryParseCodeSpan() {
    size_t start = pos_;
    size_t backtick_count = 0;
    while (pos_ + backtick_count < text_.size() &&
           text_[pos_ + backtick_count] == '`') {
      ++backtick_count;
    }

    size_t content_start = pos_ + backtick_count;
    size_t search_pos = content_start;

    while (search_pos < text_.size()) {
      if (text_[search_pos] == '`') {
        size_t close_count = 0;
        while (search_pos + close_count < text_.size() &&
               text_[search_pos + close_count] == '`') {
          ++close_count;
        }
        if (close_count == backtick_count) {
          // Found matching closer
          std::string content(
              text_.substr(content_start, search_pos - content_start));

          // Normalize: replace newlines with spaces
          for (char& code_char : content) {
            if (code_char == '\n') code_char = ' ';
          }

          // Strip single leading and trailing space if both present
          if (content.size() >= 2 && content.front() == ' ' &&
              content.back() == ' ' &&
              content.find_first_not_of(' ') != std::string::npos) {
            content = content.substr(1, content.size() - 2);
          }

          pos_ = search_pos + close_count;
          return Code(std::move(content));
        }
        search_pos += close_count;
      } else {
        ++search_pos;
      }
    }

    pos_ = start;
    return std::nullopt;
  }

  std::optional<InlineNode> TryParseAutolink() {
    if (pos_ >= text_.size() || text_[pos_] != '<') return std::nullopt;

    size_t start = pos_ + 1;
    size_t end = start;

    while (end < text_.size() && text_[end] != '>' && text_[end] != '<' &&
           text_[end] != '\n') {
      ++end;
    }

    if (end >= text_.size() || text_[end] != '>') return std::nullopt;

    std::string_view content = text_.substr(start, end - start);

    // Check for URI autolink
    static const std::regex uri_regex(
        R"(^[a-zA-Z][a-zA-Z0-9+.-]{1,31}:[^\s<>]*$)");
    std::string content_str(content);
    if (std::regex_match(content_str, uri_regex)) {
      pos_ = end + 1;
      Link link;
      link.destination = detail::EncodeUrl(content);
      link.children.push_back(Text(content_str));
      return link;
    }

    // Check for email autolink
    static const std::regex email_regex(
        R"(^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9])"
        R"((?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)"
        R"((?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$)");
    if (std::regex_match(content_str, email_regex)) {
      pos_ = end + 1;
      Link link;
      link.destination = "mailto:" + content_str;
      link.children.push_back(Text(content_str));
      return link;
    }

    return std::nullopt;
  }

  std::optional<std::string> TryParseEntity() {
    if (pos_ >= text_.size() || text_[pos_] != '&') return std::nullopt;

    size_t start = pos_ + 1;

    // Numeric character reference
    if (start < text_.size() && text_[start] == '#') {
      size_t num_start = start + 1;
      bool is_hex = false;

      if (num_start < text_.size() &&
          (text_[num_start] == 'x' || text_[num_start] == 'X')) {
        is_hex = true;
        ++num_start;
      }

      size_t num_end = num_start;
      if (is_hex) {
        while (num_end < text_.size() &&
               std::isxdigit(static_cast<unsigned char>(text_[num_end]))) {
          ++num_end;
        }
      } else {
        while (num_end < text_.size() &&
               std::isdigit(static_cast<unsigned char>(text_[num_end]))) {
          ++num_end;
        }
      }

      if (num_end > num_start && num_end < text_.size() &&
          text_[num_end] == ';') {
        std::string num_str(text_.substr(num_start, num_end - num_start));
        try {
          unsigned long code_point_ul;
          if (is_hex) {
            code_point_ul = std::stoul(num_str, nullptr, 16);
          } else {
            code_point_ul = std::stoul(num_str, nullptr, 10);
          }
          // Only decode valid Unicode code points (0 to 0x10FFFF)
          if (code_point_ul <= 0x10FFFF) {
            pos_ = num_end + 1;
            return detail::CodePointToUtf8(
                static_cast<uint32_t>(code_point_ul));
          }
          // Invalid code point - leave as literal
        } catch (...) {
          // Invalid number, treat as literal
        }
      }
    } else if (start < text_.size()) {
      // Named entity
      size_t name_end = start;
      while (name_end < text_.size() &&
             std::isalnum(static_cast<unsigned char>(text_[name_end]))) {
        ++name_end;
      }

      if (name_end > start && name_end < text_.size() &&
          text_[name_end] == ';') {
        std::string entity =
            detail::LookupHtmlEntity(text_.substr(start, name_end - start));
        if (!entity.empty()) {
          pos_ = name_end + 1;
          return entity;
        }
      }
    }

    return std::nullopt;
  }

  std::optional<HtmlInline> TryParseHtmlInline() {
    if (pos_ >= text_.size() || text_[pos_] != '<') return std::nullopt;

    size_t start = pos_;
    size_t end = pos_ + 1;

    // Simple HTML tag detection
    if (end >= text_.size()) return std::nullopt;

    bool is_closing = (text_[end] == '/');
    if (is_closing) ++end;

    // Check for valid tag name start
    if (end >= text_.size() ||
        !std::isalpha(static_cast<unsigned char>(text_[end]))) {
      // Check for comment, CDATA, processing instruction, or declaration
      if (text_.substr(pos_).starts_with("<!--")) {
        end = text_.find("-->", pos_ + 4);
        if (end != std::string_view::npos) {
          pos_ = end + 3;
          return HtmlInline(std::string(text_.substr(start, pos_ - start)));
        }
      }
      if (text_.substr(pos_).starts_with("<![CDATA[")) {
        end = text_.find("]]>", pos_ + 9);
        if (end != std::string_view::npos) {
          pos_ = end + 3;
          return HtmlInline(std::string(text_.substr(start, pos_ - start)));
        }
      }
      if (text_.substr(pos_).starts_with("<?")) {
        end = text_.find("?>", pos_ + 2);
        if (end != std::string_view::npos) {
          pos_ = end + 2;
          return HtmlInline(std::string(text_.substr(start, pos_ - start)));
        }
      }
      if (text_.substr(pos_).starts_with("<!") && end + 1 < text_.size() &&
          std::isupper(static_cast<unsigned char>(text_[end + 1]))) {
        end = text_.find('>', pos_ + 2);
        if (end != std::string_view::npos) {
          pos_ = end + 1;
          return HtmlInline(std::string(text_.substr(start, pos_ - start)));
        }
      }
      return std::nullopt;
    }

    // Parse tag name
    while (end < text_.size() &&
           (std::isalnum(static_cast<unsigned char>(text_[end])) ||
            text_[end] == '-')) {
      ++end;
    }

    // Skip attributes and find closing - must follow HTML tag syntax
    // After tag name, expect whitespace, /, or > - anything else is invalid
    bool seen_whitespace = false;
    while (end < text_.size()) {
      char c = text_[end];
      if (c == '>') {
        pos_ = end + 1;
        return HtmlInline(std::string(text_.substr(start, pos_ - start)));
      } else if (c == '/') {
        // Self-closing: must be followed by >
        if (end + 1 < text_.size() && text_[end + 1] == '>') {
          pos_ = end + 2;
          return HtmlInline(std::string(text_.substr(start, pos_ - start)));
        }
        return std::nullopt;  // / not followed by >
      } else if (c == ' ' || c == '\t' || c == '\n') {
        seen_whitespace = true;
        ++end;
      } else if (seen_whitespace &&
                 (std::isalpha(static_cast<unsigned char>(c)) || c == '_' ||
                  c == ':')) {
        // Start of attribute name (must be preceded by whitespace)
        ++end;
        while (end < text_.size()) {
          char ac = text_[end];
          if (std::isalnum(static_cast<unsigned char>(ac)) || ac == '_' ||
              ac == ':' || ac == '.' || ac == '-') {
            ++end;
          } else {
            break;
          }
        }
        // Optional attribute value
        // Skip whitespace
        while (end < text_.size() &&
               (text_[end] == ' ' || text_[end] == '\t' || text_[end] == '\n')) {
          ++end;
        }
        if (end < text_.size() && text_[end] == '=') {
          ++end;
          // Skip whitespace
          while (
              end < text_.size() &&
              (text_[end] == ' ' || text_[end] == '\t' || text_[end] == '\n')) {
            ++end;
          }
          if (end >= text_.size()) return std::nullopt;
          if (text_[end] == '"' || text_[end] == '\'') {
            char quote = text_[end];
            ++end;
            while (end < text_.size() && text_[end] != quote) {
              ++end;
            }
            if (end >= text_.size()) return std::nullopt;
            ++end;
          } else {
            // Unquoted value - no spaces, quotes, =, <, >, or `
            while (end < text_.size()) {
              char vc = text_[end];
              if (vc == ' ' || vc == '\t' || vc == '\n' || vc == '"' ||
                  vc == '\'' || vc == '=' || vc == '<' || vc == '>' ||
                  vc == '`') {
                break;
              }
              ++end;
            }
          }
        }
      } else {
        // Invalid character in tag
        return std::nullopt;
      }
    }

    return std::nullopt;
  }

  std::optional<std::pair<std::string, std::string>> TryParseLinkTail() {
    if (pos_ >= text_.size()) return std::nullopt;

    // Inline link: (destination "title")
    if (text_[pos_] == '(') {
      ++pos_;
      SkipWhitespace();

      std::string destination;
      std::string title;

      // Parse destination
      if (pos_ < text_.size() && text_[pos_] == '<') {
        // Angle-bracketed destination
        ++pos_;
        size_t dest_start = pos_;
        while (pos_ < text_.size() && text_[pos_] != '>' &&
               text_[pos_] != '\n') {
          if (text_[pos_] == '\\' && pos_ + 1 < text_.size()) {
            ++pos_;
          }
          ++pos_;
        }
        if (pos_ >= text_.size() || text_[pos_] != '>') return std::nullopt;
        destination = detail::DecodeEscapesAndEntities(
            text_.substr(dest_start, pos_ - dest_start));
        ++pos_;
      } else if (pos_ < text_.size() && text_[pos_] != ')') {
        // Regular destination - only ASCII space/tab/newline are separators
        size_t dest_start = pos_;
        int paren_depth = 0;
        while (pos_ < text_.size() && text_[pos_] != ' ' &&
               text_[pos_] != '\t' && text_[pos_] != '\n' &&
               text_[pos_] != '\r') {
          if (text_[pos_] == '(') {
            ++paren_depth;
          } else if (text_[pos_] == ')') {
            if (paren_depth == 0) break;
            --paren_depth;
          } else if (text_[pos_] == '\\' && pos_ + 1 < text_.size()) {
            ++pos_;
          }
          ++pos_;
        }
        destination = detail::DecodeEscapesAndEntities(
            text_.substr(dest_start, pos_ - dest_start));
      }

      SkipWhitespace();

      // Parse optional title
      if (pos_ < text_.size() && (text_[pos_] == '"' || text_[pos_] == '\'' ||
                                  text_[pos_] == '(')) {
        char close_char = text_[pos_] == '(' ? ')' : text_[pos_];
        ++pos_;
        size_t title_start = pos_;
        while (pos_ < text_.size() && text_[pos_] != close_char) {
          if (text_[pos_] == '\\' && pos_ + 1 < text_.size()) {
            ++pos_;
          }
          ++pos_;
        }
        if (pos_ >= text_.size()) return std::nullopt;
        title = detail::DecodeEscapesAndEntities(
            text_.substr(title_start, pos_ - title_start));
        ++pos_;
        SkipWhitespace();
      }

      if (pos_ >= text_.size() || text_[pos_] != ')') return std::nullopt;
      ++pos_;

      return std::make_pair(detail::EncodeUrl(destination), title);
    }

    // Reference link: [label] or []
    if (text_[pos_] == '[') {
      ++pos_;
      size_t label_start = pos_;
      int bracket_depth = 1;
      while (pos_ < text_.size() && bracket_depth > 0) {
        if (text_[pos_] == '[') {
          ++bracket_depth;
        } else if (text_[pos_] == ']') {
          --bracket_depth;
        } else if (text_[pos_] == '\\' && pos_ + 1 < text_.size()) {
          ++pos_;
        }
        if (bracket_depth > 0) ++pos_;
      }

      if (pos_ >= text_.size()) return std::nullopt;

      std::string label(text_.substr(label_start, pos_ - label_start));
      ++pos_;

      if (label.empty()) {
        // Collapsed reference - label comes from link text
        return std::nullopt;  // Need to look up later
      }

      return LookupReference(label);
    }

    return std::nullopt;
  }

  std::optional<std::pair<std::string, std::string>> LookupReference(
      const std::string& label) {
    if (!link_references_) return std::nullopt;

    std::string normalized = detail::NormalizeLinkLabel(label);
    auto it = link_references_->find(normalized);
    if (it != link_references_->end()) {
      return it->second;
    }
    return std::nullopt;
  }

  void SkipWhitespace() {
    while (pos_ < text_.size() && (text_[pos_] == ' ' || text_[pos_] == '\t' ||
                                   text_[pos_] == '\n' || text_[pos_] == '\r')) {
      ++pos_;
    }
  }

  std::string GetAltText(const std::vector<InlineNode>& nodes) {
    std::string result;
    for (const auto& node : nodes) {
      std::visit(
          [&result, this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Text>) {
              result += arg.content;
            } else if constexpr (std::is_same_v<T, Code>) {
              result += arg.content;
            } else if constexpr (std::is_same_v<T, SoftBreak>) {
              result += ' ';
            } else if constexpr (std::is_same_v<T, Emphasis>) {
              // Recursively get alt text
              for (const auto& child : arg.children) {
                std::visit(
                    [&result](auto&& inner) {
                      using U = std::decay_t<decltype(inner)>;
                      if constexpr (std::is_same_v<U, Text>) {
                        result += inner.content;
                      }
                    },
                    child);
              }
            }
          },
          node);
    }
    return result;
  }

  void ProcessEmphasis(std::vector<InlineNode>& nodes,
                       std::vector<DelimiterNode>& delimiters) {
    if (delimiters.empty()) return;

    // Process emphasis using the algorithm from CommonMark spec
    size_t closer_idx = 0;
    while (closer_idx < delimiters.size()) {
      auto& closer = delimiters[closer_idx];

      if (!closer.can_close || !closer.active ||
          (closer.delimiter != '*' && closer.delimiter != '_')) {
        ++closer_idx;
        continue;
      }

      // Find opener
      bool found_opener = false;
      for (size_t opener_idx = closer_idx; opener_idx > 0; --opener_idx) {
        auto& opener = delimiters[opener_idx - 1];

        if (!opener.can_open || !opener.active ||
            opener.delimiter != closer.delimiter) {
          continue;
        }

        // Check if sum of counts is multiple of 3 (special rule)
        if ((opener.can_open && opener.can_close) ||
            (closer.can_open && closer.can_close)) {
          if ((opener.count + closer.count) % 3 == 0 &&
              opener.count % 3 != 0 && closer.count % 3 != 0) {
            continue;
          }
        }

        found_opener = true;

        // Determine emphasis type
        bool is_strong = opener.count >= 2 && closer.count >= 2;
        size_t delim_count = is_strong ? 2 : 1;

        // Build emphasis node
        std::vector<InlineNode> content;
        for (size_t i = opener.pos + 1; i < closer.pos; ++i) {
          content.push_back(std::move(nodes[i]));
        }

        // Create emphasis node
        InlineNode emph_node;
        if (is_strong) {
          Strong strong;
          strong.children = std::move(content);
          emph_node = std::move(strong);
        } else {
          Emphasis em;
          em.children = std::move(content);
          emph_node = std::move(em);
        }

        // Update opener text
        if (opener.count > delim_count) {
          std::get<Text>(nodes[opener.pos]).content =
              std::string(opener.count - delim_count, opener.delimiter);
          opener.count -= delim_count;
        } else {
          nodes[opener.pos] = Text("");
          opener.active = false;
        }

        // Update closer text
        if (closer.count > delim_count) {
          std::get<Text>(nodes[closer.pos]).content =
              std::string(closer.count - delim_count, closer.delimiter);
          closer.count -= delim_count;
        } else {
          nodes[closer.pos] = Text("");
          closer.active = false;
        }

        // Insert emphasis node, removing the content in between
        std::vector<InlineNode> new_nodes;
        for (size_t i = 0; i <= opener.pos; ++i) {
          new_nodes.push_back(std::move(nodes[i]));
        }
        new_nodes.push_back(std::move(emph_node));
        for (size_t i = closer.pos; i < nodes.size(); ++i) {
          new_nodes.push_back(std::move(nodes[i]));
        }

        // Adjust positions in delimiter stack
        size_t removed = closer.pos - opener.pos - 1;
        for (auto& d : delimiters) {
          if (d.pos > opener.pos && d.pos < closer.pos) {
            d.active = false;
          } else if (d.pos >= closer.pos) {
            d.pos -= removed;
            d.pos += 1;  // Account for inserted emphasis node
          }
        }

        nodes = std::move(new_nodes);
        break;
      }

      if (!found_opener) {
        ++closer_idx;
      }
    }

    // Remove empty text nodes
    std::vector<InlineNode> filtered;
    for (auto& node : nodes) {
      if (auto* text = std::get_if<Text>(&node)) {
        if (!text->content.empty()) {
          filtered.push_back(std::move(node));
        }
      } else {
        filtered.push_back(std::move(node));
      }
    }
    nodes = std::move(filtered);
  }
};

// =============================================================================
// Block Parser
// =============================================================================

class BlockParser {
 public:
  Document Parse(std::string_view input) {
    Document doc;
    lines_ = detail::SplitLines(input);
    line_idx_ = 0;

    // First pass: extract link reference definitions
    ExtractLinkReferences(doc);

    // Second pass: parse blocks
    line_idx_ = 0;
    ParseBlocks(doc.children);

    // Third pass: parse inlines
    InlineParser inline_parser(&doc.link_references);
    ParseInlines(doc.children, inline_parser);

    return doc;
  }

 private:
  std::vector<std::string> lines_;
  size_t line_idx_ = 0;

  bool AtEnd() const { return line_idx_ >= lines_.size(); }

  std::string_view CurrentLine() const {
    if (AtEnd()) return "";
    return lines_[line_idx_];
  }

  void Advance() {
    if (!AtEnd()) ++line_idx_;
  }

  // Helper to find closing bracket accounting for escapes
  size_t FindClosingBracket(std::string_view s, size_t start) {
    for (size_t i = start; i < s.size(); ++i) {
      if (s[i] == '\\' && i + 1 < s.size()) {
        ++i;  // Skip escaped character
      } else if (s[i] == ']') {
        return i;
      }
    }
    return std::string_view::npos;
  }

  // Helper to parse destination with balanced parentheses
  std::pair<std::string, size_t> ParseLinkDestination(std::string_view s) {
    if (s.empty()) return {"", 0};

    if (s[0] == '<') {
      // Angle-bracket destination
      for (size_t i = 1; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
          ++i;
        } else if (s[i] == '>') {
          return {std::string(s.substr(1, i - 1)), i + 1};
        } else if (s[i] == '<' || s[i] == '\n') {
          return {"", 0};
        }
      }
      return {"", 0};
    }

    // Regular destination with balanced parentheses
    int paren_depth = 0;
    size_t end = 0;
    while (end < s.size()) {
      char c = s[end];
      if (c == '\\' && end + 1 < s.size()) {
        end += 2;
      } else if (c == '(') {
        ++paren_depth;
        ++end;
      } else if (c == ')') {
        if (paren_depth == 0) break;
        --paren_depth;
        ++end;
      } else if (detail::IsUnicodeWhitespace(c) || c < 0x20) {
        break;
      } else {
        ++end;
      }
    }
    if (paren_depth != 0) return {"", 0};
    return {std::string(s.substr(0, end)), end};
  }

  void ExtractLinkReferences(Document& doc) {
    // Track fenced code block state
    bool in_fenced_code = false;
    char fence_char = 0;
    size_t fence_length = 0;

    // Track if previous line had content (to reject link refs after content)
    bool prev_line_had_content = false;

    while (!AtEnd()) {
      std::string_view line = CurrentLine();
      int indent = detail::CountIndent(line);
      auto trimmed = detail::TrimLeft(line);

      // Handle fenced code blocks - check for fence opening/closing
      if (!in_fenced_code && indent < 4 && !trimmed.empty() &&
          (trimmed[0] == '`' || trimmed[0] == '~')) {
        char potential_fence = trimmed[0];
        size_t potential_len = 0;
        while (potential_len < trimmed.size() &&
               trimmed[potential_len] == potential_fence) {
          ++potential_len;
        }
        if (potential_len >= 3) {
          // Check that backtick fence doesn't have backticks in info string
          if (potential_fence == '~' ||
              trimmed.substr(potential_len).find('`') == std::string_view::npos) {
            in_fenced_code = true;
            fence_char = potential_fence;
            fence_length = potential_len;
            Advance();
            continue;
          }
        }
      }

      if (in_fenced_code) {
        // Check for closing fence
        if (indent < 4 && !trimmed.empty() && trimmed[0] == fence_char) {
          size_t close_len = 0;
          while (close_len < trimmed.size() && trimmed[close_len] == fence_char) {
            ++close_len;
          }
          if (close_len >= fence_length) {
            // Check rest is whitespace only
            bool is_close = true;
            for (size_t j = close_len; j < trimmed.size(); ++j) {
              if (trimmed[j] != ' ' && trimmed[j] != '\t') {
                is_close = false;
                break;
              }
            }
            if (is_close) {
              in_fenced_code = false;
            }
          }
        }
        Advance();
        continue;
      }

      // Skip indented lines (4+ spaces)
      if (indent >= 4) {
        prev_line_had_content = !detail::IsBlankLine(line);
        Advance();
        continue;
      }

      // Check for blank line - resets prev_line_had_content
      if (detail::IsBlankLine(line)) {
        prev_line_had_content = false;
        Advance();
        continue;
      }

      // Check for block-level elements that end a block (not continuable to paragraph)
      // These reset prev_line_had_content to false
      bool is_block_element = false;
      if (trimmed.starts_with('#')) {
        // ATX heading
        size_t hash_count = 0;
        while (hash_count < trimmed.size() && trimmed[hash_count] == '#') ++hash_count;
        if (hash_count <= 6 && (hash_count >= trimmed.size() ||
            trimmed[hash_count] == ' ' || trimmed[hash_count] == '\t')) {
          is_block_element = true;
        }
      }
      if (!is_block_element && trimmed.starts_with('>')) {
        is_block_element = true;  // Block quote
      }
      if (!is_block_element && trimmed.size() >= 3) {
        // Check for thematic break
        char first = trimmed[0];
        if (first == '*' || first == '-' || first == '_') {
          int count = 0;
          bool valid = true;
          for (char ch : trimmed) {
            if (ch == first) ++count;
            else if (ch != ' ' && ch != '\t') { valid = false; break; }
          }
          if (valid && count >= 3) is_block_element = true;
        }
      }

      if (is_block_element) {
        prev_line_had_content = false;
        Advance();
        continue;
      }

      // Link ref definitions cannot follow non-blank paragraph content
      if (prev_line_had_content) {
        prev_line_had_content = true;  // This line also has content
        Advance();
        continue;
      }

      if (!trimmed.starts_with('[')) {
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // Find closing bracket (handling escapes)
      size_t close_bracket = FindClosingBracket(trimmed, 1);
      if (close_bracket == std::string_view::npos ||
          close_bracket + 1 >= trimmed.size() ||
          trimmed[close_bracket + 1] != ':') {
        prev_line_had_content = true;
        Advance();
        continue;
      }

      std::string label_raw(trimmed.substr(1, close_bracket - 1));
      // Decode escapes in label for normalization and matching
      std::string label = detail::DecodeEscapesAndEntities(label_raw);
      std::string normalized = detail::NormalizeLinkLabel(label);
      if (normalized.empty()) {
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // Get rest of first line after ':'
      std::string_view rest = detail::TrimLeft(trimmed.substr(close_bracket + 2));
      size_t start_line = line_idx_;
      Advance();

      // Destination might be on next line (no indentation required)
      if (rest.empty() || detail::IsBlankLine(rest)) {
        if (AtEnd()) {
          line_idx_ = start_line;
          prev_line_had_content = true;
          Advance();
          continue;
        }
        std::string_view next_line = CurrentLine();
        if (detail::IsBlankLine(next_line)) {
          line_idx_ = start_line;
          prev_line_had_content = true;
          Advance();
          continue;
        }
        rest = detail::TrimLeft(next_line);
        Advance();
      }

      // Parse destination
      auto [dest_raw, dest_len] = ParseLinkDestination(rest);
      if (dest_raw.empty() && dest_len == 0 && !rest.empty() && rest[0] != '<') {
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }
      std::string destination = detail::DecodeEscapesAndEntities(dest_raw);
      auto rest_after_dest = rest.substr(dest_len);
      bool has_whitespace_before_title =
          rest_after_dest.size() != detail::TrimLeft(rest_after_dest).size();
      rest = detail::TrimLeft(rest_after_dest);

      // Parse optional title (may be on same line or next line)
      std::string title;
      bool title_valid = true;
      bool title_on_new_line = false;
      size_t pre_title_line = line_idx_;

      if (rest.empty() && !AtEnd()) {
        std::string_view next_line = CurrentLine();
        // Title can be on next line if next line starts with title char
        auto trimmed_next = detail::TrimLeft(next_line);
        if (!detail::IsBlankLine(next_line) &&
            (trimmed_next.size() > 0 &&
             (trimmed_next[0] == '"' || trimmed_next[0] == '\'' ||
              trimmed_next[0] == '('))) {
          title_on_new_line = true;
          rest = trimmed_next;
          Advance();
        }
      }

      if (!rest.empty() && (rest[0] == '"' || rest[0] == '\'' || rest[0] == '(')) {
        // Title must be separated from destination by whitespace (or be on new line)
        if (!title_on_new_line && !has_whitespace_before_title) {
          title_valid = false;
        } else {
          char open_char = rest[0];
          char close_char = (open_char == '(') ? ')' : open_char;
          std::string title_content;
          size_t i = 1;

        // Title can span multiple lines
        while (title_valid) {
          while (i < rest.size()) {
            if (rest[i] == '\\' && i + 1 < rest.size()) {
              title_content += rest[i];
              title_content += rest[i + 1];
              i += 2;
            } else if (rest[i] == close_char) {
              // Found closing - rest must be whitespace only
              auto after = detail::TrimLeft(rest.substr(i + 1));
              if (!after.empty()) {
                title_valid = false;
              } else {
                title = detail::DecodeEscapesAndEntities(title_content);
              }
              goto title_done;
            } else if (rest[i] == '\n') {
              title_content += rest[i];
              ++i;
            } else {
              title_content += rest[i];
              ++i;
            }
          }
          // Continue to next line
          if (AtEnd()) {
            title_valid = false;
            break;
          }
          std::string_view next_line = CurrentLine();
          if (detail::IsBlankLine(next_line)) {
            title_valid = false;
            break;
          }
          title_content += '\n';
          rest = next_line;
          i = 0;
          Advance();
        }
      title_done:;
        }  // close else block for whitespace check
      } else if (!rest.empty()) {
        // Non-whitespace after destination with no title
        title_valid = false;
      }

      // If destination parsing returned length 0 (not angle brackets) and no destination, it's invalid
      // But angle bracket destinations like <> returning empty string with length > 0 are valid
      if (dest_raw.empty() && dest_len == 0) {
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // If title parsing failed but was on a new line, roll back just the title line
      // and accept the link ref without a title
      if (!title_valid && title_on_new_line) {
        line_idx_ = pre_title_line;
        title.clear();
      } else if (!title_valid) {
        // Title on same line failed - invalid link ref
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // Only add if not already defined
      if (doc.link_references.find(normalized) == doc.link_references.end()) {
        doc.link_references[normalized] = {detail::EncodeUrl(destination), title};
      }
      // After successful link ref extraction, next line starts fresh
      prev_line_had_content = false;
    }
    // Reset for block parsing
    line_idx_ = 0;
  }

  void ParseBlocks(std::vector<BlockNode>& blocks) {
    while (!AtEnd()) {
      std::string_view line = CurrentLine();

      // Skip blank lines
      if (detail::IsBlankLine(line)) {
        Advance();
        continue;
      }

      // Check for various block types
      if (auto block = TryParseThematicBreak()) {
        blocks.push_back(std::move(*block));
        continue;
      }

      if (auto block = TryParseAtxHeading()) {
        blocks.push_back(std::move(*block));
        continue;
      }

      if (auto block = TryParseFencedCodeBlock()) {
        blocks.push_back(std::move(*block));
        continue;
      }

      if (auto block = TryParseHtmlBlock()) {
        blocks.push_back(std::move(*block));
        continue;
      }

      if (auto block = TryParseBlockQuote()) {
        blocks.push_back(std::move(*block));
        continue;
      }

      if (auto block = TryParseList()) {
        blocks.push_back(std::move(*block));
        continue;
      }

      if (auto block = TryParseIndentedCodeBlock()) {
        blocks.push_back(std::move(*block));
        continue;
      }

      // Check for link reference definition (skip it, already extracted)
      if (TrySkipLinkReferenceDefinition()) {
        continue;
      }

      // Default: paragraph (may include setext heading)
      auto para = ParseParagraph();
      blocks.push_back(std::move(para));
    }
  }

  bool TrySkipLinkReferenceDefinition() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) return false;

    auto trimmed = detail::TrimLeft(line);
    if (!trimmed.starts_with('[')) return false;

    size_t close_bracket = FindClosingBracket(trimmed, 1);
    if (close_bracket == std::string_view::npos ||
        close_bracket + 1 >= trimmed.size() ||
        trimmed[close_bracket + 1] != ':') {
      return false;
    }

    std::string label(trimmed.substr(1, close_bracket - 1));
    std::string normalized = detail::NormalizeLinkLabel(label);
    if (normalized.empty()) return false;

    std::string_view rest = detail::TrimLeft(trimmed.substr(close_bracket + 2));
    size_t start_line = line_idx_;
    Advance();

    // Destination might be on next line (no indentation required)
    if (rest.empty() || detail::IsBlankLine(rest)) {
      if (AtEnd()) {
        line_idx_ = start_line;
        return false;
      }
      std::string_view next_line = CurrentLine();
      if (detail::IsBlankLine(next_line)) {
        line_idx_ = start_line;
        return false;
      }
      rest = detail::TrimLeft(next_line);
      Advance();
    }

    // Parse destination
    auto [dest_raw, dest_len] = ParseLinkDestination(rest);
    if (dest_raw.empty() && dest_len == 0 && !rest.empty() && rest[0] != '<') {
      line_idx_ = start_line;
      return false;
    }
    auto rest_after_dest = rest.substr(dest_len);
    bool has_whitespace_before_title =
        rest_after_dest.size() != detail::TrimLeft(rest_after_dest).size();
    rest = detail::TrimLeft(rest_after_dest);

    // Parse optional title (may be on next line)
    bool title_valid = true;
    bool title_on_new_line = false;
    size_t pre_title_line = line_idx_;

    if (rest.empty() && !AtEnd()) {
      std::string_view next_line = CurrentLine();
      auto trimmed_next = detail::TrimLeft(next_line);
      if (!detail::IsBlankLine(next_line) &&
          (trimmed_next.size() > 0 &&
           (trimmed_next[0] == '"' || trimmed_next[0] == '\'' ||
            trimmed_next[0] == '('))) {
        title_on_new_line = true;
        rest = trimmed_next;
        Advance();
      }
    }

    if (!rest.empty() && (rest[0] == '"' || rest[0] == '\'' || rest[0] == '(')) {
      // Title must be separated from destination by whitespace (or be on new line)
      if (!title_on_new_line && !has_whitespace_before_title) {
        title_valid = false;
      } else {
        char open_char = rest[0];
        char close_char = (open_char == '(') ? ')' : open_char;
        size_t i = 1;

        while (title_valid) {
        while (i < rest.size()) {
          if (rest[i] == '\\' && i + 1 < rest.size()) {
            i += 2;
          } else if (rest[i] == close_char) {
            auto after = detail::TrimLeft(rest.substr(i + 1));
            if (!after.empty()) {
              title_valid = false;
            }
            goto skip_title_done;
          } else {
            ++i;
          }
        }
        if (AtEnd()) {
          title_valid = false;
          break;
        }
        std::string_view next_line = CurrentLine();
        if (detail::IsBlankLine(next_line)) {
          title_valid = false;
          break;
        }
        rest = next_line;
        i = 0;
        Advance();
      }
    skip_title_done:;
      }  // close else block for whitespace check
    } else if (!rest.empty()) {
      title_valid = false;
    }

    // If title failed but was on a new line, roll back just title and accept without title
    if (!title_valid && title_on_new_line) {
      line_idx_ = pre_title_line;
    } else if (!title_valid) {
      line_idx_ = start_line;
      return false;
    }

    return true;
  }

  std::optional<ThematicBreak> TryParseThematicBreak() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty()) return std::nullopt;

    char marker = trimmed[0];
    if (marker != '-' && marker != '*' && marker != '_') return std::nullopt;

    int count = 0;
    for (char c : trimmed) {
      if (c == marker) {
        ++count;
      } else if (c != ' ' && c != '\t') {
        return std::nullopt;
      }
    }

    if (count >= 3) {
      Advance();
      return ThematicBreak{};
    }

    return std::nullopt;
  }

  std::optional<Heading> TryParseAtxHeading() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty() || trimmed[0] != '#') return std::nullopt;

    int level = 0;
    size_t i = 0;
    while (i < trimmed.size() && trimmed[i] == '#') {
      ++level;
      ++i;
    }

    if (level > 6) return std::nullopt;
    if (i < trimmed.size() && trimmed[i] != ' ' && trimmed[i] != '\t') {
      return std::nullopt;
    }

    // Skip space after #'s
    while (i < trimmed.size() && (trimmed[i] == ' ' || trimmed[i] == '\t')) {
      ++i;
    }

    // Get content (strip trailing #'s)
    std::string content(trimmed.substr(i));

    // Remove trailing #'s (preceded by spaces)
    while (!content.empty()) {
      size_t last_non_space = content.find_last_not_of(" \t");
      if (last_non_space == std::string::npos) {
        content.clear();
        break;
      }
      if (content[last_non_space] == '#') {
        // Check if preceded by space or at beginning
        size_t hash_start = last_non_space;
        while (hash_start > 0 && content[hash_start - 1] == '#') {
          --hash_start;
        }
        if (hash_start == 0 || content[hash_start - 1] == ' ' ||
            content[hash_start - 1] == '\t') {
          content = content.substr(0, hash_start);
          // Trim trailing spaces
          while (!content.empty() &&
                 (content.back() == ' ' || content.back() == '\t')) {
            content.pop_back();
          }
        } else {
          break;
        }
      } else {
        break;
      }
    }

    Advance();

    Heading heading;
    heading.level = level;
    heading.raw_content = content;
    return heading;
  }

  std::optional<CodeBlock> TryParseFencedCodeBlock() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty()) return std::nullopt;

    char fence_char = trimmed[0];
    if (fence_char != '`' && fence_char != '~') return std::nullopt;

    size_t fence_length = 0;
    while (fence_length < trimmed.size() &&
           trimmed[fence_length] == fence_char) {
      ++fence_length;
    }

    if (fence_length < 3) return std::nullopt;

    // Check for backtick in info string (not allowed for backtick fences)
    std::string info_string(detail::Trim(trimmed.substr(fence_length)));
    if (fence_char == '`' && info_string.find('`') != std::string::npos) {
      return std::nullopt;
    }
    // Decode backslash escapes in info string
    info_string = detail::DecodeEscapesAndEntities(info_string);

    Advance();

    // Collect code content
    std::string content;
    bool found_closing = false;
    while (!AtEnd()) {
      std::string_view code_line = CurrentLine();

      // Check for closing fence - must be indented < 4 spaces
      int close_indent = detail::CountIndent(code_line);
      if (close_indent < 4) {
        auto code_trimmed = detail::TrimLeft(code_line);

        // Count fence characters at start
        size_t close_fence_len = 0;
        while (close_fence_len < code_trimmed.size() &&
               code_trimmed[close_fence_len] == fence_char) {
          ++close_fence_len;
        }

        // Closing fence must be at least as long as opening fence
        if (close_fence_len >= fence_length) {
          // Rest of line must be only whitespace
          bool is_closing = true;
          for (size_t j = close_fence_len; j < code_trimmed.size(); ++j) {
            if (code_trimmed[j] != ' ' && code_trimmed[j] != '\t') {
              is_closing = false;
              break;
            }
          }
          if (is_closing) {
            Advance();
            found_closing = true;
            break;
          }
        }
      }

      // Remove indentation from code line
      if (indent > 0) {
        content += detail::RemoveIndent(code_line, indent);
      } else {
        content += code_line;
      }
      content += '\n';
      Advance();
    }

    // If unclosed, remove the trailing newline that was added for the last line
    // (CommonMark spec: unclosed blocks don't include final newline)
    if (!found_closing && !content.empty() && content.back() == '\n') {
      content.pop_back();
    }

    CodeBlock block;
    block.info_string = info_string;
    block.content = content;
    block.is_fenced = true;
    return block;
  }

  std::optional<HtmlBlock> TryParseHtmlBlock() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty() || trimmed[0] != '<') return std::nullopt;

    int block_type = 0;
    std::string end_condition;

    // Type 1: <script>, <pre>, <style>, <textarea>
    static const std::vector<std::string> type1_tags = {"script", "pre",
                                                        "style", "textarea"};
    for (const auto& tag : type1_tags) {
      std::string open_tag = "<" + tag;
      if (trimmed.size() >= open_tag.size()) {
        std::string lower;
        for (size_t j = 0; j < open_tag.size(); ++j) {
          lower += static_cast<char>(
              std::tolower(static_cast<unsigned char>(trimmed[j])));
        }
        if (lower == open_tag) {
          // Tag at end of line, or followed by space/tab/>/newline
          if (trimmed.size() == open_tag.size()) {
            block_type = 1;
            end_condition = "</" + tag + ">";
            break;
          }
          char next = trimmed[open_tag.size()];
          if (next == ' ' || next == '>' || next == '\t' || next == '\n') {
            block_type = 1;
            end_condition = "</" + tag + ">";
            break;
          }
        }
      }
    }

    // Type 2: <!-- comment -->
    if (block_type == 0 && trimmed.starts_with("<!--")) {
      block_type = 2;
      end_condition = "-->";
    }

    // Type 3: <? processing instruction ?>
    if (block_type == 0 && trimmed.starts_with("<?")) {
      block_type = 3;
      end_condition = "?>";
    }

    // Type 4: <!DOCTYPE
    if (block_type == 0 && trimmed.size() >= 2 && trimmed[0] == '<' &&
        trimmed[1] == '!') {
      std::string prefix;
      for (size_t j = 0; j < std::min(size_t(9), trimmed.size()); ++j) {
        prefix += static_cast<char>(
            std::toupper(static_cast<unsigned char>(trimmed[j])));
      }
      if (prefix.starts_with("<!DOCTYPE")) {
        block_type = 4;
        end_condition = ">";
      }
    }

    // Type 5: <![CDATA[
    if (block_type == 0 && trimmed.starts_with("<![CDATA[")) {
      block_type = 5;
      end_condition = "]]>";
    }

    // Type 6: Block-level HTML tags
    static const std::vector<std::string> type6_tags = {
        "address",    "article",    "aside",      "base",      "basefont",
        "blockquote", "body",       "caption",    "center",    "col",
        "colgroup",   "dd",         "details",    "dialog",    "dir",
        "div",        "dl",         "dt",         "fieldset",  "figcaption",
        "figure",     "footer",     "form",       "frame",     "frameset",
        "h1",         "h2",         "h3",         "h4",        "h5",
        "h6",         "head",       "header",     "hr",        "html",
        "iframe",     "legend",     "li",         "link",      "main",
        "menu",       "menuitem",   "nav",        "noframes",  "ol",
        "optgroup",   "option",     "p",          "param",     "search",
        "section",    "summary",    "table",      "tbody",     "td",
        "tfoot",      "th",         "thead",      "title",     "tr",
        "track",      "ul"};

    if (block_type == 0) {
      bool is_closing = (trimmed.size() >= 2 && trimmed[1] == '/');
      size_t tag_start = is_closing ? 2 : 1;

      size_t tag_end = tag_start;
      while (tag_end < trimmed.size() &&
             (std::isalnum(static_cast<unsigned char>(trimmed[tag_end])) ||
              trimmed[tag_end] == '-')) {
        ++tag_end;
      }

      if (tag_end > tag_start) {
        std::string tag_name;
        for (size_t j = tag_start; j < tag_end; ++j) {
          tag_name += static_cast<char>(
              std::tolower(static_cast<unsigned char>(trimmed[j])));
        }

        for (const auto& t : type6_tags) {
          if (tag_name == t) {
            // Check for valid tag ending
            if (tag_end < trimmed.size()) {
              char next = trimmed[tag_end];
              if (next == ' ' || next == '>' || next == '\t' || next == '/' ||
                  next == '\n') {
                block_type = 6;
                break;
              }
            } else {
              block_type = 6;
              break;
            }
          }
        }
      }
    }

    // Type 7: Other HTML tags (not interrupted by blank line same as type 6)
    // Must be a complete tag followed only by whitespace
    if (block_type == 0) {
      // Check for opening or closing tag
      bool is_closing = (trimmed.size() >= 2 && trimmed[1] == '/');
      size_t tag_start = is_closing ? 2 : 1;

      if (tag_start < trimmed.size() &&
          std::isalpha(static_cast<unsigned char>(trimmed[tag_start]))) {
        size_t tag_end = tag_start + 1;
        while (tag_end < trimmed.size() &&
               (std::isalnum(static_cast<unsigned char>(trimmed[tag_end])) ||
                trimmed[tag_end] == '-')) {
          ++tag_end;
        }

        // After tag name, must have valid HTML tag continuation: whitespace, /, >
        // Anything else (like + or :) means this isn't a valid HTML tag
        bool valid_tag = false;
        if (tag_end >= trimmed.size()) {
          // Just tag name, no closing - not valid
        } else {
          char next = trimmed[tag_end];
          valid_tag = (next == ' ' || next == '\t' || next == '/' || next == '>');
        }
        if (valid_tag) {
          // Find the closing > of this tag
          size_t search_pos = tag_end;
          bool in_quote = false;
          char quote_char = 0;
          while (search_pos < trimmed.size()) {
            char c = trimmed[search_pos];
            if (in_quote) {
              if (c == quote_char) in_quote = false;
            } else {
              if (c == '"' || c == '\'') {
                in_quote = true;
                quote_char = c;
              } else if (c == '>') {
                break;
              }
            }
            ++search_pos;
          }

          if (search_pos < trimmed.size() && trimmed[search_pos] == '>') {
            // Check that rest of line is only whitespace
            bool only_whitespace = true;
            for (size_t j = search_pos + 1; j < trimmed.size(); ++j) {
              if (trimmed[j] != ' ' && trimmed[j] != '\t') {
                only_whitespace = false;
                break;
              }
            }
            if (only_whitespace) {
              block_type = 7;
            }
          }
        }
      }
    }

    if (block_type == 0) return std::nullopt;

    // Collect HTML block content
    std::string content;

    while (!AtEnd()) {
      std::string_view html_line = CurrentLine();
      content += html_line;
      content += '\n';

      // Check for end condition
      if (block_type <= 5) {
        std::string lower_line;
        for (char c : html_line) {
          lower_line +=
              static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (lower_line.find(end_condition) != std::string::npos) {
          Advance();
          break;
        }
      }

      Advance();

      // Types 6 and 7 end at blank line
      if ((block_type == 6 || block_type == 7) &&
          (AtEnd() || detail::IsBlankLine(CurrentLine()))) {
        break;
      }
    }

    HtmlBlock block;
    block.content = content;
    block.block_type = block_type;
    return block;
  }

  std::optional<BlockQuote> TryParseBlockQuote() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty() || trimmed[0] != '>') return std::nullopt;

    // Collect block quote lines
    std::vector<std::string> quote_lines;

    while (!AtEnd()) {
      std::string_view bq_line = CurrentLine();
      int bq_indent = detail::CountIndent(bq_line);

      auto bq_trimmed = detail::TrimLeft(bq_line);

      if (bq_trimmed.starts_with(">")) {
        // Remove > and optional space with proper tab handling
        std::string bq_content = detail::RemoveBlockQuotePrefix(bq_line);
        quote_lines.push_back(bq_content);
        Advance();
      } else if (!bq_trimmed.empty() && !quote_lines.empty()) {
        // Lazy continuation - only for paragraphs
        // Check if this could be a paragraph continuation
        bool is_continuation = true;

        // Indented code blocks (4+ spaces) cannot be lazy continuation
        if (bq_indent >= 4) {
          is_continuation = false;
        }

        // Check for block-level interrupts
        if (bq_trimmed.starts_with("#") || bq_trimmed.starts_with("```") ||
            bq_trimmed.starts_with("~~~") || bq_trimmed.starts_with("---") ||
            bq_trimmed.starts_with("***") || bq_trimmed.starts_with("___") ||
            bq_trimmed.starts_with("- ") || bq_trimmed.starts_with("* ") ||
            bq_trimmed.starts_with("+ ") ||
            (bq_trimmed.size() >= 2 &&
             std::isdigit(static_cast<unsigned char>(bq_trimmed[0])))) {
          is_continuation = false;
        }

        if (is_continuation) {
          // Mark lazy continuation with special prefix to prevent setext heading
          // 0x01 (SOH) is stripped by TrimLeft and prevents setext matching
          quote_lines.push_back(std::string(1, '\x01') + std::string(bq_trimmed));
          Advance();
        } else {
          break;
        }
      } else {
        break;
      }
    }

    if (quote_lines.empty()) return std::nullopt;

    // Parse the content of the block quote
    std::string quote_content;
    for (const auto& ql : quote_lines) {
      quote_content += ql;
      quote_content += '\n';
    }

    BlockParser nested_parser;
    Document nested_doc = nested_parser.Parse(quote_content);

    BlockQuote bq;
    bq.children = std::move(nested_doc.children);
    return bq;
  }

  std::optional<List> TryParseList() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty()) return std::nullopt;

    // Check for bullet list
    bool is_ordered = false;
    int start = 1;
    char delimiter = '.';
    char bullet_char = '-';
    int marker_width = 0;

    if (trimmed[0] == '-' || trimmed[0] == '+' || trimmed[0] == '*') {
      if (trimmed.size() < 2 || (trimmed[1] != ' ' && trimmed[1] != '\t')) {
        return std::nullopt;
      }
      bullet_char = trimmed[0];
      marker_width = 2;
    } else if (std::isdigit(static_cast<unsigned char>(trimmed[0]))) {
      // Ordered list
      is_ordered = true;
      size_t num_end = 0;
      while (num_end < trimmed.size() &&
             std::isdigit(static_cast<unsigned char>(trimmed[num_end]))) {
        ++num_end;
      }
      if (num_end == 0 || num_end > 9) return std::nullopt;
      if (num_end >= trimmed.size()) return std::nullopt;

      char delim = trimmed[num_end];
      if (delim != '.' && delim != ')') return std::nullopt;

      if (num_end + 1 >= trimmed.size() ||
          (trimmed[num_end + 1] != ' ' && trimmed[num_end + 1] != '\t')) {
        // Allow empty list item
        if (num_end + 1 < trimmed.size()) return std::nullopt;
      }

      start = std::stoi(std::string(trimmed.substr(0, num_end)));
      delimiter = delim;
      marker_width = static_cast<int>(num_end) + 2;
    } else {
      return std::nullopt;
    }

    // Parse list items
    List list;
    list.is_ordered = is_ordered;
    list.start = start;
    list.delimiter = delimiter;
    list.bullet_char = bullet_char;
    list.is_tight = true;

    bool had_blank_line = false;

    while (!AtEnd()) {
      std::string_view item_line = CurrentLine();
      int item_indent = detail::CountIndent(item_line);
      auto item_trimmed = detail::TrimLeft(item_line);

      // Check for blank line
      if (detail::IsBlankLine(item_line)) {
        had_blank_line = true;
        Advance();
        continue;
      }

      // Check for thematic break first - it takes precedence over list items
      // Only applies to bullet lists where the bullet could be a thematic break
      if (!is_ordered && item_indent < 4 && !item_trimmed.empty()) {
        char first = item_trimmed[0];
        if (first == '*' || first == '-' || first == '_') {
          int count = 0;
          bool valid_break = true;
          for (char ch : item_trimmed) {
            if (ch == first) {
              ++count;
            } else if (ch != ' ' && ch != '\t') {
              valid_break = false;
              break;
            }
          }
          if (valid_break && count >= 3) {
            // This is a thematic break, not a list item - end the list
            break;
          }
        }
      }

      // Check for new list item
      bool is_new_item = false;
      if (is_ordered) {
        if (item_indent < 4 && !item_trimmed.empty() &&
            std::isdigit(static_cast<unsigned char>(item_trimmed[0]))) {
          size_t num_end = 0;
          while (
              num_end < item_trimmed.size() &&
              std::isdigit(static_cast<unsigned char>(item_trimmed[num_end]))) {
            ++num_end;
          }
          if (num_end > 0 && num_end <= 9 && num_end < item_trimmed.size() &&
              item_trimmed[num_end] == delimiter) {
            if (num_end + 1 >= item_trimmed.size() ||
                item_trimmed[num_end + 1] == ' ' ||
                item_trimmed[num_end + 1] == '\t') {
              is_new_item = true;
            }
          }
        }
      } else {
        if (item_indent < 4 && !item_trimmed.empty() &&
            item_trimmed[0] == bullet_char && item_trimmed.size() >= 2 &&
            (item_trimmed[1] == ' ' || item_trimmed[1] == '\t')) {
          is_new_item = true;
        }
      }

      if (!is_new_item && !list.items.empty()) {
        // Not a new item - the list is complete.
        // Any valid continuation lines should have been collected by the inner
        // loop. If we're here, the line belongs to a different block.
        break;
      }

      if (is_new_item || list.items.empty()) {
        // Start new item
        if (had_blank_line && !list.items.empty()) {
          list.is_tight = false;
        }

        ListItem item;
        std::vector<std::string> item_lines;

        // Get first line content with proper tab handling
        // Expand tabs in the original line, then extract content after marker
        std::string expanded_line = detail::ExpandTabs(item_line);
        std::string_view expanded_trimmed = detail::TrimLeft(expanded_line);

        std::string first_content;
        if (is_ordered) {
          size_t num_end = 0;
          while (
              num_end < expanded_trimmed.size() &&
              std::isdigit(
                  static_cast<unsigned char>(expanded_trimmed[num_end]))) {
            ++num_end;
          }
          // Skip number, delimiter, and required space
          size_t skip = num_end + 1;  // number + delimiter
          if (skip < expanded_trimmed.size() &&
              expanded_trimmed[skip] == ' ') {
            ++skip;
          }
          first_content = std::string(expanded_trimmed.substr(skip));
        } else {
          // Skip bullet and required space
          size_t skip = 1;  // bullet
          if (skip < expanded_trimmed.size() &&
              expanded_trimmed[skip] == ' ') {
            ++skip;
          }
          first_content = std::string(expanded_trimmed.substr(skip));
        }
        item_lines.push_back(first_content);
        Advance();
        had_blank_line = false;

        // Calculate required indent based on content start position
        // For `-\tfoo`, the content starts at column 4, so required_indent is 4
        int expanded_item_indent = detail::CountIndent(expanded_line);
        int content_start = expanded_item_indent;
        if (is_ordered) {
          size_t num_end = 0;
          while (num_end < expanded_trimmed.size() &&
                 std::isdigit(
                     static_cast<unsigned char>(expanded_trimmed[num_end]))) {
            ++num_end;
          }
          content_start += static_cast<int>(num_end) + 1;  // +1 for delimiter
        } else {
          content_start += 1;  // +1 for bullet
        }
        // Add the space after marker
        content_start += 1;

        // Collect continuation lines
        int required_indent = content_start;
        while (!AtEnd()) {
          std::string_view cont_line = CurrentLine();

          if (detail::IsBlankLine(cont_line)) {
            item_lines.push_back("");
            had_blank_line = true;
            Advance();
            continue;
          }

          // Expand tabs for proper indent calculation
          std::string expanded_cont = detail::ExpandTabs(cont_line);
          int cont_indent = detail::CountIndent(expanded_cont);
          auto cont_trimmed = detail::TrimLeft(expanded_cont);

          // Check for new list item (same type) - only at original list indent
          // If indented to be content of current item, it should be parsed as
          // nested list by the BlockParser, not as a sibling item here
          bool is_another_item = false;
          if (cont_indent < required_indent && cont_indent < 4) {
            if (is_ordered && !cont_trimmed.empty() &&
                std::isdigit(static_cast<unsigned char>(cont_trimmed[0]))) {
              size_t num_end = 0;
              while (num_end < cont_trimmed.size() &&
                     std::isdigit(
                         static_cast<unsigned char>(cont_trimmed[num_end]))) {
                ++num_end;
              }
              if (num_end > 0 && num_end < cont_trimmed.size() &&
                  cont_trimmed[num_end] == delimiter) {
                is_another_item = true;
              }
            } else if (!is_ordered && !cont_trimmed.empty() &&
                       cont_trimmed[0] == bullet_char &&
                       cont_trimmed.size() >= 2 && cont_trimmed[1] == ' ') {
              is_another_item = true;
            }
          }

          if (is_another_item) {
            break;
          }

          // Check for other block-level interrupts
          if (cont_indent < required_indent) {
            // Check for block elements that can interrupt
            if (cont_trimmed.starts_with(">") ||
                cont_trimmed.starts_with("#") ||
                cont_trimmed.starts_with("```") ||
                cont_trimmed.starts_with("~~~")) {
              break;
            }
            // Check for thematic break
            if (cont_trimmed.size() >= 3) {
              char first = cont_trimmed[0];
              if (first == '-' || first == '*' || first == '_') {
                int count = 0;
                bool valid_break = true;
                for (char ch : cont_trimmed) {
                  if (ch == first) {
                    ++count;
                  } else if (ch != ' ') {
                    valid_break = false;
                    break;
                  }
                }
                if (valid_break && count >= 3) {
                  break;
                }
              }
            }
            // End list if not properly indented
            if (!had_blank_line) {
              // Lazy continuation
              item_lines.push_back(std::string(cont_trimmed));
              Advance();
            } else {
              break;
            }
          } else {
            // Properly indented continuation - remove required_indent spaces
            // If there was a blank line before this content, list becomes loose
            if (had_blank_line) {
              list.is_tight = false;
            }
            std::string dedented =
                detail::RemoveIndent(expanded_cont, required_indent);
            item_lines.push_back(dedented);
            Advance();
            had_blank_line = false;
          }
        }

        // Remove trailing blank lines
        while (!item_lines.empty() && item_lines.back().empty()) {
          item_lines.pop_back();
        }

        // Parse item content
        std::string item_content;
        for (const auto& il : item_lines) {
          item_content += il;
          item_content += '\n';
        }

        if (!item_content.empty()) {
          BlockParser item_parser;
          Document item_doc = item_parser.Parse(item_content);
          item.children = std::move(item_doc.children);
        }

        list.items.push_back(std::move(item));
      }
    }

    // Return nullopt if no items were collected
    if (list.items.empty()) {
      return std::nullopt;
    }

    // Check for loose list (items separated by blank lines)
    for (auto& item : list.items) {
      item.is_tight = list.is_tight;
    }

    return list;
  }

  std::optional<CodeBlock> TryParseIndentedCodeBlock() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent < 4) return std::nullopt;

    std::string content;

    while (!AtEnd()) {
      std::string_view code_line = CurrentLine();
      int code_indent = detail::CountIndent(code_line);

      if (detail::IsBlankLine(code_line)) {
        // Blank lines within code blocks preserve whitespace beyond 4 spaces
        if (code_indent >= 4) {
          content += detail::RemoveIndent(code_line, 4);
        }
        content += '\n';
        Advance();
        continue;
      }

      if (code_indent < 4) break;

      content += detail::RemoveIndent(code_line, 4);
      content += '\n';
      Advance();
    }

    // Remove trailing blank lines
    while (!content.empty() && content.back() == '\n') {
      size_t last_newline = content.find_last_not_of('\n');
      if (last_newline == std::string::npos) {
        content.clear();
      } else {
        content = content.substr(0, last_newline + 1);
        content += '\n';
        break;
      }
    }

    if (content.empty()) return std::nullopt;

    CodeBlock block;
    block.content = content;
    block.is_fenced = false;
    return block;
  }

  BlockNode ParseParagraph() {
    std::vector<std::string> para_lines;

    while (!AtEnd()) {
      std::string_view line = CurrentLine();

      if (detail::IsBlankLine(line)) break;

      int indent = detail::CountIndent(line);
      auto trimmed = detail::TrimLeft(line);

      // Check for setext heading underline
      // Skip lines marked with \x01 (lazy continuation in blockquotes)
      if (!para_lines.empty() && indent < 4 && !line.starts_with("\x01")) {
        if (!trimmed.empty() && (trimmed[0] == '=' || trimmed[0] == '-')) {
          char underline_char = trimmed[0];
          // Find where underline chars end
          size_t underline_end = 0;
          while (underline_end < trimmed.size() &&
                 trimmed[underline_end] == underline_char) {
            ++underline_end;
          }
          // Must have at least one underline char
          if (underline_end > 0) {
            // Rest must be only whitespace (no mixed chars like "= =")
            bool valid_underline = true;
            for (size_t j = underline_end; j < trimmed.size(); ++j) {
              if (trimmed[j] != ' ' && trimmed[j] != '\t') {
                valid_underline = false;
                break;
              }
            }
            if (valid_underline) {
              Advance();

              Heading heading;
              heading.level = (underline_char == '=') ? 1 : 2;

              std::string heading_content;
              for (size_t j = 0; j < para_lines.size(); ++j) {
                if (j > 0) heading_content += '\n';
                heading_content += para_lines[j];
              }
              heading.raw_content = heading_content;
              return heading;
            }
          }
        }
      }

      // Check for block-level interrupts
      if (indent < 4) {
        // ATX heading: must be #, ##, etc. followed by space or end of line
        if (trimmed.starts_with("#")) {
          size_t hash_count = 0;
          while (hash_count < trimmed.size() && trimmed[hash_count] == '#') {
            ++hash_count;
          }
          if (hash_count <= 6 &&
              (hash_count >= trimmed.size() ||
               trimmed[hash_count] == ' ' || trimmed[hash_count] == '\t')) {
            break;
          }
        }
        if (trimmed.starts_with(">")) {
          break;
        }
        // Fenced code block: must be at least 3 fence chars, and for backticks,
        // the info string cannot contain backticks
        if (trimmed.size() >= 3 &&
            (trimmed[0] == '`' || trimmed[0] == '~')) {
          char fence_char = trimmed[0];
          size_t fence_len = 0;
          while (fence_len < trimmed.size() && trimmed[fence_len] == fence_char) {
            ++fence_len;
          }
          if (fence_len >= 3) {
            // For backticks, check that info string doesn't contain backticks
            if (fence_char == '~') {
              break;
            }
            auto info = detail::Trim(trimmed.substr(fence_len));
            if (info.find('`') == std::string_view::npos) {
              break;
            }
          }
        }

        // Check for thematic break
        if (trimmed.size() >= 3) {
          char first = trimmed[0];
          if (first == '-' || first == '*' || first == '_') {
            int count = 0;
            bool valid_break = true;
            for (char ch : trimmed) {
              if (ch == first) {
                ++count;
              } else if (ch != ' ' && ch != '\t') {
                valid_break = false;
                break;
              }
            }
            if (valid_break && count >= 3) {
              break;
            }
          }
        }

        // Check for list item (but not if just -)
        if ((trimmed.starts_with("- ") || trimmed.starts_with("+ ") ||
             trimmed.starts_with("* ")) ||
            (trimmed.size() >= 2 &&
             std::isdigit(static_cast<unsigned char>(trimmed[0])))) {
          if (trimmed.starts_with("- ") || trimmed.starts_with("+ ") ||
              trimmed.starts_with("* ")) {
            break;
          }
          // Check for ordered list
          size_t num_end = 0;
          while (num_end < trimmed.size() &&
                 std::isdigit(static_cast<unsigned char>(trimmed[num_end]))) {
            ++num_end;
          }
          if (num_end > 0 && num_end < trimmed.size() &&
              (trimmed[num_end] == '.' || trimmed[num_end] == ')') &&
              (num_end + 1 >= trimmed.size() || trimmed[num_end + 1] == ' ' ||
               trimmed[num_end + 1] == '\t')) {
            // Only interrupt if starts with 1
            if (trimmed.substr(0, num_end) == "1") {
              break;
            }
          }
        }

        // Check for HTML block (types 1-6 can interrupt paragraphs)
        if (trimmed.starts_with("<")) {
          // Type 1: script, pre, style, textarea
          static const std::vector<std::string> type1_tags = {"script", "pre",
                                                              "style", "textarea"};
          for (const auto& tag : type1_tags) {
            std::string open_tag = "<" + tag;
            if (trimmed.size() >= open_tag.size()) {
              std::string lower;
              for (size_t j = 0; j < open_tag.size(); ++j) {
                lower += static_cast<char>(
                    std::tolower(static_cast<unsigned char>(trimmed[j])));
              }
              if (lower == open_tag &&
                  (trimmed.size() == open_tag.size() ||
                   trimmed[open_tag.size()] == ' ' ||
                   trimmed[open_tag.size()] == '>' ||
                   trimmed[open_tag.size()] == '\t')) {
                goto html_interrupt;
              }
            }
          }
          // Type 2-5
          if (trimmed.starts_with("<!--") || trimmed.starts_with("<?") ||
              trimmed.starts_with("<![CDATA[")) {
            goto html_interrupt;
          }
          if (trimmed.size() >= 2 && trimmed[1] == '!' &&
              trimmed.size() >= 9) {
            std::string upper;
            for (size_t j = 0; j < 9; ++j) {
              upper += static_cast<char>(
                  std::toupper(static_cast<unsigned char>(trimmed[j])));
            }
            if (upper.starts_with("<!DOCTYPE")) {
              goto html_interrupt;
            }
          }
          // Type 6: block-level tags
          static const std::vector<std::string> type6_tags = {
              "address", "article", "aside", "base", "basefont", "blockquote",
              "body", "caption", "center", "col", "colgroup", "dd", "details",
              "dialog", "dir", "div", "dl", "dt", "fieldset", "figcaption",
              "figure", "footer", "form", "frame", "frameset", "h1", "h2",
              "h3", "h4", "h5", "h6", "head", "header", "hr", "html", "iframe",
              "legend", "li", "link", "main", "menu", "menuitem", "nav",
              "noframes", "ol", "optgroup", "option", "p", "param", "search",
              "section", "summary", "table", "tbody", "td", "tfoot", "th",
              "thead", "title", "tr", "track", "ul"};
          bool is_closing = (trimmed.size() >= 2 && trimmed[1] == '/');
          size_t tag_start = is_closing ? 2 : 1;
          size_t tag_end = tag_start;
          while (tag_end < trimmed.size() &&
                 (std::isalnum(static_cast<unsigned char>(trimmed[tag_end])) ||
                  trimmed[tag_end] == '-')) {
            ++tag_end;
          }
          if (tag_end > tag_start) {
            std::string tag_name;
            for (size_t j = tag_start; j < tag_end; ++j) {
              tag_name += static_cast<char>(
                  std::tolower(static_cast<unsigned char>(trimmed[j])));
            }
            for (const auto& t : type6_tags) {
              if (tag_name == t) {
                if (tag_end >= trimmed.size() ||
                    trimmed[tag_end] == ' ' || trimmed[tag_end] == '>' ||
                    trimmed[tag_end] == '\t' || trimmed[tag_end] == '/') {
                  goto html_interrupt;
                }
              }
            }
          }
        }
        if (false) {
        html_interrupt:
          break;
        }
      }

      // Strip \x01 marker used for lazy continuation lines
      if (!trimmed.empty() && trimmed[0] == '\x01') {
        para_lines.push_back(std::string(trimmed.substr(1)));
      } else {
        para_lines.push_back(std::string(trimmed));
      }
      Advance();
    }

    Paragraph para;
    std::string para_content;
    for (size_t j = 0; j < para_lines.size(); ++j) {
      if (j > 0) para_content += '\n';
      para_content += para_lines[j];
    }
    para.raw_content = para_content;
    return para;
  }

  void ParseInlines(std::vector<BlockNode>& blocks, InlineParser& parser) {
    for (auto& block : blocks) {
      std::visit(
          [&parser, this](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Paragraph>) {
              node.children = parser.Parse(node.raw_content);
            } else if constexpr (std::is_same_v<T, Heading>) {
              node.children = parser.Parse(node.raw_content);
            } else if constexpr (std::is_same_v<T, BlockQuote>) {
              ParseInlines(node.children, parser);
            } else if constexpr (std::is_same_v<T, List>) {
              for (auto& item : node.items) {
                ParseInlines(item.children, parser);
              }
            }
          },
          block);
    }
  }
};

// =============================================================================
// HTML Renderer
// =============================================================================

class HtmlRenderer {
 public:
  std::string Render(const Document& doc) {
    std::string result;
    RenderBlocks(doc.children, result, false);
    return result;
  }

 private:
  void RenderBlocks(const std::vector<BlockNode>& blocks, std::string& out,
                    bool in_tight_list) {
    for (size_t i = 0; i < blocks.size(); ++i) {
      std::visit(
          [this, &out, in_tight_list](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Paragraph>) {
              RenderParagraph(node, out, in_tight_list);
            } else if constexpr (std::is_same_v<T, Heading>) {
              RenderHeading(node, out);
            } else if constexpr (std::is_same_v<T, ThematicBreak>) {
              out += "<hr />\n";
            } else if constexpr (std::is_same_v<T, CodeBlock>) {
              RenderCodeBlock(node, out);
            } else if constexpr (std::is_same_v<T, HtmlBlock>) {
              out += node.content;
            } else if constexpr (std::is_same_v<T, BlockQuote>) {
              RenderBlockQuote(node, out);
            } else if constexpr (std::is_same_v<T, List>) {
              RenderList(node, out);
            } else if constexpr (std::is_same_v<T, ListItem>) {
              // Handled by RenderList
            }
          },
          blocks[i]);
    }
  }

  void RenderParagraph(const Paragraph& para, std::string& out,
                       bool in_tight_list) {
    if (in_tight_list) {
      RenderInlines(para.children, out);
      out += '\n';
    } else {
      out += "<p>";
      RenderInlines(para.children, out);
      out += "</p>\n";
    }
  }

  void RenderHeading(const Heading& heading, std::string& out) {
    out += "<h" + std::to_string(heading.level) + ">";
    RenderInlines(heading.children, out);
    out += "</h" + std::to_string(heading.level) + ">\n";
  }

  void RenderCodeBlock(const CodeBlock& block, std::string& out) {
    out += "<pre><code";
    if (!block.info_string.empty()) {
      // Extract language (first word of info string)
      std::string lang;
      for (char c : block.info_string) {
        if (c == ' ' || c == '\t') break;
        lang += c;
      }
      if (!lang.empty()) {
        out += " class=\"language-" + detail::EscapeHtml(lang) + "\"";
      }
    }
    out += ">";
    out += detail::EscapeHtml(block.content);
    out += "</code></pre>\n";
  }

  void RenderBlockQuote(const BlockQuote& bq, std::string& out) {
    out += "<blockquote>\n";
    RenderBlocks(bq.children, out, false);
    out += "</blockquote>\n";
  }

  void RenderList(const List& list, std::string& out) {
    if (list.is_ordered) {
      if (list.start == 1) {
        out += "<ol>\n";
      } else {
        out += "<ol start=\"" + std::to_string(list.start) + "\">\n";
      }
    } else {
      out += "<ul>\n";
    }

    for (const auto& item : list.items) {
      out += "<li>";

      if (list.is_tight && item.children.size() == 1 &&
          std::holds_alternative<Paragraph>(item.children[0])) {
        // Tight list with single paragraph - render without <p> tags
        const auto& para = std::get<Paragraph>(item.children[0]);
        RenderInlines(para.children, out);
      } else if (list.is_tight) {
        // Tight list with multiple blocks
        out += '\n';
        RenderBlocks(item.children, out, true);
      } else {
        // Loose list
        out += '\n';
        RenderBlocks(item.children, out, false);
      }

      out += "</li>\n";
    }

    if (list.is_ordered) {
      out += "</ol>\n";
    } else {
      out += "</ul>\n";
    }
  }

  void RenderInlines(const std::vector<InlineNode>& nodes, std::string& out) {
    for (const auto& node : nodes) {
      std::visit(
          [this, &out](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, Text>) {
              out += detail::EscapeHtml(n.content);
            } else if constexpr (std::is_same_v<T, SoftBreak>) {
              out += '\n';
            } else if constexpr (std::is_same_v<T, HardBreak>) {
              out += "<br />\n";
            } else if constexpr (std::is_same_v<T, Code>) {
              out += "<code>";
              out += detail::EscapeHtml(n.content);
              out += "</code>";
            } else if constexpr (std::is_same_v<T, Emphasis>) {
              out += "<em>";
              RenderInlines(n.children, out);
              out += "</em>";
            } else if constexpr (std::is_same_v<T, Strong>) {
              out += "<strong>";
              RenderInlines(n.children, out);
              out += "</strong>";
            } else if constexpr (std::is_same_v<T, Link>) {
              out += "<a href=\"" + detail::EscapeHtml(n.destination) + "\"";
              if (!n.title.empty()) {
                out += " title=\"" + detail::EscapeHtml(n.title) + "\"";
              }
              out += ">";
              RenderInlines(n.children, out);
              out += "</a>";
            } else if constexpr (std::is_same_v<T, Image>) {
              out += "<img src=\"" + detail::EscapeHtml(n.destination) + "\"";
              out += " alt=\"" + detail::EscapeHtml(n.alt_text) + "\"";
              if (!n.title.empty()) {
                out += " title=\"" + detail::EscapeHtml(n.title) + "\"";
              }
              out += " />";
            } else if constexpr (std::is_same_v<T, HtmlInline>) {
              out += n.content;
            }
          },
          node);
    }
  }
};

// =============================================================================
// Public API
// =============================================================================

// Parse Markdown input and return an AST
inline Document Parse(std::string_view input) {
  BlockParser parser;
  return parser.Parse(input);
}

// Render a document AST to HTML
inline std::string RenderHtml(const Document& doc) {
  HtmlRenderer renderer;
  return renderer.Render(doc);
}

// Convenience function: parse Markdown and render to HTML
inline std::string MarkdownToHtml(std::string_view input) {
  return RenderHtml(Parse(input));
}

// Debug: print AST structure
inline std::string DebugAst(const Document& doc, int indent = 0) {
  std::string result;
  std::string prefix(indent * 2, ' ');

  result += prefix + "Document\n";

  std::function<void(const std::vector<BlockNode>&, int)> print_blocks;
  std::function<void(const std::vector<InlineNode>&, int)> print_inlines;

  print_inlines = [&](const std::vector<InlineNode>& nodes, int ind) {
    std::string p(ind * 2, ' ');
    for (const auto& node : nodes) {
      std::visit(
          [&](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, Text>) {
              result += p + "Text: \"" + n.content + "\"\n";
            } else if constexpr (std::is_same_v<T, SoftBreak>) {
              result += p + "SoftBreak\n";
            } else if constexpr (std::is_same_v<T, HardBreak>) {
              result += p + "HardBreak\n";
            } else if constexpr (std::is_same_v<T, Code>) {
              result += p + "Code: \"" + n.content + "\"\n";
            } else if constexpr (std::is_same_v<T, Emphasis>) {
              result += p + "Emphasis\n";
              print_inlines(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, Strong>) {
              result += p + "Strong\n";
              print_inlines(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, Link>) {
              result += p + "Link: " + n.destination + "\n";
              print_inlines(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, Image>) {
              result += p + "Image: " + n.destination + "\n";
            } else if constexpr (std::is_same_v<T, HtmlInline>) {
              result += p + "HtmlInline: " + n.content + "\n";
            }
          },
          node);
    }
  };

  print_blocks = [&](const std::vector<BlockNode>& blocks, int ind) {
    std::string p(ind * 2, ' ');
    for (const auto& block : blocks) {
      std::visit(
          [&](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, Paragraph>) {
              result += p + "Paragraph\n";
              print_inlines(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, Heading>) {
              result += p + "Heading (level " + std::to_string(n.level) + ")\n";
              print_inlines(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, ThematicBreak>) {
              result += p + "ThematicBreak\n";
            } else if constexpr (std::is_same_v<T, CodeBlock>) {
              result += p + "CodeBlock";
              if (!n.info_string.empty()) {
                result += " (" + n.info_string + ")";
              }
              result += "\n";
            } else if constexpr (std::is_same_v<T, HtmlBlock>) {
              result += p + "HtmlBlock (type " + std::to_string(n.block_type) +
                        ")\n";
            } else if constexpr (std::is_same_v<T, BlockQuote>) {
              result += p + "BlockQuote\n";
              print_blocks(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, List>) {
              result += p + "List (" +
                        (n.is_ordered ? "ordered" : "unordered") +
                        (n.is_tight ? ", tight" : ", loose") + ")\n";
              for (const auto& item : n.items) {
                result += p + "  ListItem\n";
                print_blocks(item.children, ind + 2);
              }
            } else if constexpr (std::is_same_v<T, ListItem>) {
              result += p + "ListItem\n";
              print_blocks(n.children, ind + 1);
            }
          },
          block);
    }
  };

  print_blocks(doc.children, indent + 1);
  return result;
}

}  // namespace hyde

#endif  // HYDE_H_
