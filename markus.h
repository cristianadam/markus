#ifndef MARKUS_H_
#define MARKUS_H_

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <memory_resource>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// =============================================================================
// Arena Allocator Setup
// =============================================================================

inline constexpr std::size_t kArenaSize = 128 * 1024 * 1024;  // 128 MiB

// The backing buffer (static storage). One per program because it's inline.
alignas(std::max_align_t) inline std::byte g_buffer[kArenaSize];

// The arena spills to heap via upstream new_delete_resource().
inline std::pmr::monotonic_buffer_resource g_arena{
    g_buffer, kArenaSize, std::pmr::new_delete_resource()};

// Set the arena as the default memory resource for all pmr containers
inline const bool kArenaInitialized = []() {
  std::pmr::set_default_resource(&g_arena);
  return true;
}();

namespace markus {

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
inline std::string_view NodeTypeToString(NodeType type) {
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

// =============================================================================
// Node ID Types - indices into node pools for compact storage
// =============================================================================

using InlineNodeId = uint32_t;
using BlockNodeId = uint32_t;
constexpr InlineNodeId kInvalidInlineNodeId = ~uint32_t(0);
constexpr BlockNodeId kInvalidBlockNodeId = ~uint32_t(0);

// =============================================================================
// Inline Node Definitions
// =============================================================================

struct Text {
  static constexpr NodeType kType = NodeType::kText;
  std::string_view content;

  explicit Text(std::string_view c) : content(c) {}
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
  std::pmr::string content;

  explicit Code(std::pmr::string c) : content(std::move(c)) {}
  Code() = default;
};

struct Emphasis {
  static constexpr NodeType kType = NodeType::kEmphasis;
  std::pmr::vector<InlineNodeId> children;
};

struct Strong {
  static constexpr NodeType kType = NodeType::kStrong;
  std::pmr::vector<InlineNodeId> children;
};

struct Link {
  static constexpr NodeType kType = NodeType::kLink;
  std::pmr::string destination;
  std::pmr::string title;
  std::pmr::vector<InlineNodeId> children;
};

struct Image {
  static constexpr NodeType kType = NodeType::kImage;
  std::pmr::string destination;
  std::pmr::string title;
  std::pmr::string alt_text;
};

struct HtmlInline {
  static constexpr NodeType kType = NodeType::kHtmlInline;
  std::string_view content;

  explicit HtmlInline(std::string_view c) : content(c) {}
  HtmlInline() = default;
};

// Variant type for inline content
using InlineNode = std::variant<Text, SoftBreak, HardBreak, Code, Emphasis,
                                Strong, Link, Image, HtmlInline>;

// =============================================================================
// Block Node Definitions
// =============================================================================

struct Paragraph {
  static constexpr NodeType kType = NodeType::kParagraph;
  std::pmr::vector<InlineNodeId> children;
  std::pmr::string raw_content;  // Temporary storage for inline parsing
};

struct Heading {
  static constexpr NodeType kType = NodeType::kHeading;
  int level = 1;  // 1-6
  std::pmr::vector<InlineNodeId> children;
  std::pmr::string raw_content;  // Temporary storage for inline parsing
};

struct ThematicBreak {
  static constexpr NodeType kType = NodeType::kThematicBreak;
};

struct CodeBlock {
  static constexpr NodeType kType = NodeType::kCodeBlock;
  std::pmr::string info_string;  // Language hint (e.g., "cpp", "python")
  std::pmr::string content;
  bool is_fenced = false;
};

struct HtmlBlock {
  static constexpr NodeType kType = NodeType::kHtmlBlock;
  std::pmr::string content;
  int block_type = 0;  // CommonMark HTML block type (1-7)
};

struct ListItem {
  static constexpr NodeType kType = NodeType::kListItem;
  std::pmr::vector<BlockNodeId> children;
  bool is_tight = true;
};

struct List {
  static constexpr NodeType kType = NodeType::kList;
  bool is_ordered = false;
  int start = 1;           // Starting number for ordered lists
  char delimiter = '.';    // '.' or ')' for ordered lists
  char bullet_char = '-';  // '-', '+', or '*' for unordered lists
  bool is_tight = true;
  std::pmr::vector<ListItem> items;
};

struct BlockQuote {
  static constexpr NodeType kType = NodeType::kBlockQuote;
  std::pmr::vector<BlockNodeId> children;
};

// Variant type for block content
using BlockNode = std::variant<Paragraph, Heading, ThematicBreak, CodeBlock,
                               HtmlBlock, BlockQuote, List, ListItem>;

// Type alias for link references map
using LinkRefMap =
    std::pmr::unordered_map<std::pmr::string,
                            std::pair<std::pmr::string, std::pmr::string>>;

struct Document {
  static constexpr NodeType kType = NodeType::kDocument;
  std::pmr::vector<BlockNode> children;

  // Link reference definitions (label -> (destination, title))
  LinkRefMap link_references;

  // Storage for decoded strings (entities, escapes) that inline nodes view into
  // Using deque to avoid reference invalidation on growth
  std::deque<std::pmr::string> string_storage;

  // Node pools for compact storage - nodes are referenced by ID (index)
  std::pmr::vector<InlineNode> inline_nodes;
  std::pmr::vector<BlockNode> block_nodes;

  // Add a block to the pool and return its ID
  BlockNodeId AddBlock(BlockNode&& node) {
    BlockNodeId id = static_cast<BlockNodeId>(block_nodes.size());
    block_nodes.push_back(std::move(node));
    return id;
  }
};

// =============================================================================
// Utility Functions
// =============================================================================

namespace detail {

using namespace std::literals;

// =============================================================================
// SIMD-Friendly Helper Functions
// =============================================================================

// Find first occurrence of any byte from a set in a chunk of data
// Returns position relative to start, or len if not found
// Designed for auto-vectorization by processing without early exits
inline size_t FindFirstSpecialChar(const char* data, size_t len,
                                   const uint8_t* special_table) {
  size_t i = 0;
  // Process 8 bytes at a time - compiler can vectorize this
  for (; i + 8 <= len; i += 8) {
    // Check 8 bytes - any special character found?
    uint8_t found = 0;
    found |= special_table[static_cast<unsigned char>(data[i])];
    found |= special_table[static_cast<unsigned char>(data[i + 1])];
    found |= special_table[static_cast<unsigned char>(data[i + 2])];
    found |= special_table[static_cast<unsigned char>(data[i + 3])];
    found |= special_table[static_cast<unsigned char>(data[i + 4])];
    found |= special_table[static_cast<unsigned char>(data[i + 5])];
    found |= special_table[static_cast<unsigned char>(data[i + 6])];
    found |= special_table[static_cast<unsigned char>(data[i + 7])];
    if (found) {
      // Found something in this chunk, find exact position
      for (size_t j = i; j < i + 8; ++j) {
        if (special_table[static_cast<unsigned char>(data[j])]) {
          return j;
        }
      }
    }
  }
  // Handle remaining bytes
  for (; i < len; ++i) {
    if (special_table[static_cast<unsigned char>(data[i])]) {
      return i;
    }
  }
  return len;
}

// Find first byte where safe_table[byte] == 0 (i.e., needs encoding)
// Used for URL encoding where table has 1 for safe, 0 for unsafe
inline size_t FindFirstUnsafeChar(const char* data, size_t len,
                                  const uint8_t* safe_table) {
  size_t i = 0;
  // Process 8 bytes at a time
  for (; i + 8 <= len; i += 8) {
    // Check 8 bytes - all safe?
    uint8_t all_safe = 1;
    all_safe &= safe_table[static_cast<unsigned char>(data[i])];
    all_safe &= safe_table[static_cast<unsigned char>(data[i + 1])];
    all_safe &= safe_table[static_cast<unsigned char>(data[i + 2])];
    all_safe &= safe_table[static_cast<unsigned char>(data[i + 3])];
    all_safe &= safe_table[static_cast<unsigned char>(data[i + 4])];
    all_safe &= safe_table[static_cast<unsigned char>(data[i + 5])];
    all_safe &= safe_table[static_cast<unsigned char>(data[i + 6])];
    all_safe &= safe_table[static_cast<unsigned char>(data[i + 7])];
    if (!all_safe) {
      // Found something unsafe, find exact position
      for (size_t j = i; j < i + 8; ++j) {
        if (!safe_table[static_cast<unsigned char>(data[j])]) {
          return j;
        }
      }
    }
  }
  // Handle remaining bytes
  for (; i < len; ++i) {
    if (!safe_table[static_cast<unsigned char>(data[i])]) {
      return i;
    }
  }
  return len;
}

// Check if a span contains only blank characters (space, tab, \r, \n)
inline bool IsSpanBlank(const char* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = static_cast<unsigned char>(data[i]);
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      return false;
    }
  }
  return true;
}

// Count occurrences and check for special bytes in a span
// Returns: pair<line_count, has_special>
// Looks for \n, \r (line endings) and \0 (null)
// SIMD-friendly: accumulates without branching in inner loop
inline std::pair<size_t, bool> ScanForLinesAndNulls(const char* data,
                                                    size_t len) {
  size_t line_count = 0;
  bool has_nulls = false;
  size_t i = 0;

  // Process 8 bytes at a time, accumulating counts
  for (; i + 8 <= len; i += 8) {
    // Check each byte for line endings and nulls
    for (size_t j = 0; j < 8; ++j) {
      char c = data[i + j];
      if (c == '\n') {
        ++line_count;
      } else if (c == '\r') {
        ++line_count;
        // Check for \r\n (but be careful at chunk boundary)
        if (j + 1 < 8 && data[i + j + 1] == '\n') {
          ++j;  // Skip the \n
        } else if (i + j + 1 < len && data[i + j + 1] == '\n') {
          // Will be handled in next iteration or remainder
        }
      } else if (c == '\0') {
        has_nulls = true;
      }
    }
  }

  // Handle remaining bytes
  for (; i < len; ++i) {
    char c = data[i];
    if (c == '\n') {
      ++line_count;
    } else if (c == '\r') {
      ++line_count;
      if (i + 1 < len && data[i + 1] == '\n') {
        ++i;  // Skip the \n in \r\n
      }
    } else if (c == '\0') {
      has_nulls = true;
    }
  }

  return {line_count, has_nulls};
}

// Find next line ending (\n or \r) starting from pos
// Returns position of line ending, or len if not found
inline size_t FindNextLineEnding(const char* data, size_t len, size_t start) {
  size_t i = start;
  // Process 8 bytes at a time
  for (; i + 8 <= len; i += 8) {
    // Check 8 bytes for line endings
    bool found = false;
    for (size_t j = 0; j < 8; ++j) {
      char c = data[i + j];
      if (c == '\n' || c == '\r') {
        found = true;
        break;
      }
    }
    if (found) {
      // Find exact position
      for (size_t j = i; j < i + 8 && j < len; ++j) {
        if (data[j] == '\n' || data[j] == '\r') {
          return j;
        }
      }
    }
  }
  // Handle remaining bytes
  for (; i < len; ++i) {
    if (data[i] == '\n' || data[i] == '\r') {
      return i;
    }
  }
  return len;
}

// Check if a character is ASCII punctuation
// Uses a 256-byte lookup table for O(1) performance
inline bool IsAsciiPunctuation(char c) {
  // Lookup table: 1 if ASCII punctuation, 0 otherwise
  // ASCII punctuation: 0x21-0x2F, 0x3A-0x40, 0x5B-0x60, 0x7B-0x7E
  static constexpr uint8_t kPunctuationTable[256] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x00-0x0F
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x10-0x1F
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x20-0x2F (! to /)
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,  // 0x30-0x3F (: to ?)
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x40-0x4F (@)
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,  // 0x50-0x5F ([ to _)
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x60-0x6F (`)
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,  // 0x70-0x7F ({ to ~)
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80-0x8F
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90-0x9F
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xA0-0xAF
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xB0-0xBF
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xC0-0xCF
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xD0-0xDF
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xE0-0xEF
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xF0-0xFF
  };
  return kPunctuationTable[static_cast<unsigned char>(c)] != 0;
}

// Get the byte length of a UTF-8 character starting at the given byte
inline size_t Utf8CharLen(unsigned char c) {
  if ((c & 0x80) == 0) return 1;     // ASCII
  if ((c & 0xE0) == 0xC0) return 2;  // 110xxxxx
  if ((c & 0xF0) == 0xE0) return 3;  // 1110xxxx
  if ((c & 0xF8) == 0xF0) return 4;  // 11110xxx
  return 1;                          // Invalid, treat as single byte
}

// Decode UTF-8 code point at position, return code point and bytes consumed
inline std::pair<uint32_t, size_t> DecodeUtf8At(std::string_view s,
                                                size_t pos) {
  if (pos >= s.size()) [[unlikely]]
    return {0, 0};
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
  if (cp == 0x00A0) return true;                  // NO-BREAK SPACE
  if (cp == 0x1680) return true;                  // OGHAM SPACE MARK
  if (cp >= 0x2000 && cp <= 0x200A) return true;  // Various spaces
  if (cp == 0x202F) return true;                  // NARROW NO-BREAK SPACE
  if (cp == 0x205F) return true;                  // MEDIUM MATHEMATICAL SPACE
  if (cp == 0x3000) return true;                  // IDEOGRAPHIC SPACE
  return false;
}

// Check if a character is Unicode whitespace (simplified)
// Uses lookup table for O(1) performance
inline bool IsUnicodeWhitespace(char c) {
  // Lookup table for ASCII whitespace: space(0x20), tab(0x09), newline(0x0A),
  // carriage return(0x0D), form feed(0x0C)
  static constexpr uint8_t kWhitespaceTable[256] = {
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      1,
      1,
      0,
      1,
      1,
      0,
      0,  // 0x00-0x0F:
          // tab,lf,ff,cr
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x10-0x1F
      1,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x20-0x2F: space
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x30-0x3F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x40-0x7F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x80-0xFF
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
  };
  return kWhitespaceTable[static_cast<unsigned char>(c)] != 0;
}

// Check for Unicode whitespace at position in a string (handles multi-byte
// UTF-8) Returns number of bytes to skip if whitespace, 0 otherwise
inline size_t IsUnicodeWhitespaceAt(std::string_view s, size_t pos) {
  if (pos >= s.size()) return 0;
  unsigned char c = static_cast<unsigned char>(s[pos]);
  // Fast path for ASCII whitespace using existing lookup
  if (IsUnicodeWhitespace(static_cast<char>(c))) {
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
// Uses fixed buffer to avoid multiple string reallocations
inline std::pmr::string CodePointToUtf8(uint32_t cp) {
  char buf[4];
  size_t len;

  if (cp == 0 || cp >= 0x110000) [[unlikely]] {
    // Null or invalid code point - use replacement character U+FFFD
    return "\xEF\xBF\xBD";
  } else if (cp < 0x80) {
    buf[0] = static_cast<char>(cp);
    len = 1;
  } else if (cp < 0x800) {
    buf[0] = static_cast<char>(0xC0 | (cp >> 6));
    buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
    len = 2;
  } else if (cp < 0x10000) {
    buf[0] = static_cast<char>(0xE0 | (cp >> 12));
    buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
    len = 3;
  } else {
    buf[0] = static_cast<char>(0xF0 | (cp >> 18));
    buf[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    buf[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    buf[3] = static_cast<char>(0x80 | (cp & 0x3F));
    len = 4;
  }
  return std::pmr::string(buf, len);
}

// Simple Unicode case-folding for a code point
// Returns the lowercase equivalent (or the same code point if no folding)
inline uint32_t UnicodeCaseFold(uint32_t cp) {
  // ASCII uppercase -> lowercase
  if (cp >= 'A' && cp <= 'Z') {
    return cp + 32;
  }
  // Latin-1 Supplement uppercase (U+00C0-U+00DE, except U+00D7 multiplication
  // sign)
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
  // German capital sharp S (ẞ U+1E9E) is handled specially in
  // NormalizeLinkLabel because it folds to "ss" (two characters) in full case
  // folding Latin Extended Additional
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
// Optimized with fast path for ASCII-only labels
inline std::pmr::string NormalizeLinkLabel(std::string_view label) {
  // Fast path: check if label is ASCII-only (common case)
  bool all_ascii = true;
  for (unsigned char c : label) {
    if (c >= 0x80) {
      all_ascii = false;
      break;
    }
  }

  std::pmr::string result;
  result.reserve(label.size());

  if (all_ascii) {
    // Fast ASCII-only path: simple lowercase + whitespace collapse
    bool in_whitespace = false;
    bool first = true;
    for (char c : label) {
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
        if (!first && !in_whitespace) {
          result += ' ';
          in_whitespace = true;
        }
      } else {
        // ASCII lowercase: 'A'-'Z' (0x41-0x5A) -> 'a'-'z' (0x61-0x7A)
        if (c >= 'A' && c <= 'Z') {
          result += static_cast<char>(c + 32);
        } else {
          result += c;
        }
        in_whitespace = false;
        first = false;
      }
    }
  } else {
    // Slow path: full Unicode handling
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

      // Apply case folding - special handling for characters that fold to
      // multiple chars
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
  }

  // Trim trailing space
  if (!result.empty() && result.back() == ' ') {
    result.pop_back();
  }

  return result;
}

// HTML entity escaping - optimized with lookup table and batch copying
inline std::pmr::string EscapeHtml(std::string_view text) {
  // Lookup table: 0 = no escape needed, non-zero = escape index
  // 1=&, 2=<, 3=>, 4="
  static constexpr uint8_t kEscapeTable[256] = {
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x00-0x0F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x10-0x1F
      0,
      0,
      4,
      0,
      0,
      0,
      1,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x20-0x2F: " at 0x22,
          // & at 0x26
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      2,
      0,
      3,
      0,  // 0x30-0x3F: < at 0x3C,
          // > at 0x3E
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x40-0x4F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x50-0x5F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x60-0x6F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x70-0x7F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x80-0xFF (high bytes)
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
  };
  static constexpr const char* kEscapeStrings[] = {nullptr, "&amp;", "&lt;",
                                                   "&gt;", "&quot;"};

  std::pmr::string result;
  result.reserve(text.size() + text.size() / 8);  // Slightly over-reserve

  size_t i = 0;
  while (i < text.size()) {
    // Use SIMD-friendly helper to find next character needing escape
    size_t next_special =
        FindFirstSpecialChar(text.data() + i, text.size() - i, kEscapeTable);
    // Batch copy non-escaped span
    if (next_special > 0) {
      result.append(text.data() + i, next_special);
    }
    i += next_special;
    // Handle escaped character
    if (i < text.size()) {
      result +=
          kEscapeStrings[kEscapeTable[static_cast<unsigned char>(text[i])]];
      ++i;
    }
  }
  return result;
}

// URL encoding for link destinations
// Uses lookup table for O(1) character classification and precomputed hex table
inline std::pmr::string EncodeUrl(std::string_view url) {
  // Lookup table: 1 = character is safe (no encoding needed), 0 = needs
  // encoding Safe chars: alphanumeric and -_.~/:?#@!$&'()*+,;=%
  static constexpr uint8_t kSafeTable[256] = {
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x00-0x0F
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x10-0x1F
      0,
      1,
      0,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,  // 0x20-0x2F:
          // !#$%&'()*+,-./
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      0,
      1,
      0,
      1,  // 0x30-0x3F: 0-9:;=?
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,  // 0x40-0x4F: @A-O
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      0,
      0,
      0,
      0,
      1,  // 0x50-0x5F: P-Z_
      0,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,  // 0x60-0x6F: a-o
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      1,
      0,
      0,
      0,
      1,
      0,  // 0x70-0x7F: p-z~
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x80-0xFF
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
  };
  // Hex encoding lookup (avoids snprintf)
  static constexpr char kHexChars[] = "0123456789ABCDEF";

  std::pmr::string result;
  result.reserve(url.size() + url.size() / 4);

  size_t i = 0;
  while (i < url.size()) {
    // Use SIMD-friendly helper to find first unsafe character
    size_t next_unsafe =
        FindFirstUnsafeChar(url.data() + i, url.size() - i, kSafeTable);
    // Batch copy safe span
    if (next_unsafe > 0) {
      result.append(url.data() + i, next_unsafe);
    }
    i += next_unsafe;
    // Encode unsafe character
    if (i < url.size()) {
      unsigned char c = static_cast<unsigned char>(url[i]);
      result += '%';
      result += kHexChars[c >> 4];
      result += kHexChars[c & 0x0F];
      ++i;
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

// Case-insensitive prefix check without temporary string allocations
inline bool StartsWithInsensitive(std::string_view str,
                                  std::string_view prefix) {
  if (str.size() < prefix.size()) return false;
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(str[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }
  return true;
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
// Optimized with fast path for common case of n spaces at start
inline std::pmr::string RemoveIndent(std::string_view line, int n) {
  if (n <= 0) {
    return std::pmr::string(line);
  }

  // Fast path: check if we have n spaces at the start (no tabs)
  size_t space_count = 0;
  while (space_count < line.size() && line[space_count] == ' ') {
    ++space_count;
  }
  if (space_count >= static_cast<size_t>(n)) {
    // Common case: just skip n spaces, return rest
    return std::pmr::string(line.substr(n));
  }

  // Slow path: handle tabs
  std::pmr::string result;
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
        int leftover = tab_width - remaining;
        result.append(leftover, ' ');
      }
    } else {
      break;
    }
  }

  result += line.substr(i);
  return result;
}

// Remove blockquote prefix (> and optional following space) with proper tab
// handling Returns the remaining content with proper indentation preserved
inline std::pmr::string RemoveBlockQuotePrefix(std::string_view line) {
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
    return std::pmr::string(line);
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
      std::pmr::string result;
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
  std::pmr::string result;
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
// Optimized with fast path and batch copying
inline std::pmr::string ExpandTabs(std::string_view line) {
  // Fast path: no tabs present
  size_t first_tab = line.find('\t');
  if (first_tab == std::string_view::npos) {
    return std::pmr::string(line);  // No tabs, zero-copy
  }

  std::pmr::string result;
  result.reserve(line.size() + 16);  // Extra space for tab expansion

  // Copy everything before first tab
  result.append(line.data(), first_tab);
  int column = static_cast<int>(first_tab);

  for (size_t i = first_tab; i < line.size(); ++i) {
    char c = line[i];
    if (c == '\t') {
      int next_col = (column / 4 + 1) * 4;
      int spaces = next_col - column;
      result.append(spaces, ' ');
      column = next_col;
    } else {
      result += c;
      ++column;
    }
  }
  return result;
}

// Check if line is blank (only whitespace)
// Uses lookup table for O(1) per-character checks
inline bool IsBlankLine(std::string_view line) {
  // Lookup table: 1 = blank character (space, tab, \r, \n), 0 = non-blank
  static constexpr uint8_t kBlankTable[256] = {
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      1,
      1,
      0,
      0,
      1,
      0,
      0,  // 0x00-0x0F: tab,lf,cr
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x10-0x1F
      1,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x20-0x2F: space
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,  // 0x30-0xFF (all
          // non-blank)
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
  };
  // Use SIMD-friendly helper for bulk processing
  if (line.size() >= 8) {
    return IsSpanBlank(line.data(), line.size());
  }
  // Scalar fallback for short lines
  for (char c : line) {
    if (!kBlankTable[static_cast<unsigned char>(c)]) {
      return false;
    }
  }
  return true;
}

// =============================================================================
// LineBuffer - Cache-friendly line storage
// =============================================================================
// Instead of vector<string> (many allocations, poor cache locality),
// stores line offsets into a contiguous buffer. Only copies data when
// null characters need replacement.

class LineBuffer {
 public:
  explicit LineBuffer(std::string_view input) {
    if (input.empty()) [[unlikely]] {
      return;
    }

    // First pass: check for nulls and count lines
    bool has_nulls = false;
    size_t line_count = 1;
    for (size_t i = 0; i < input.size(); ++i) {
      if (input[i] == '\0') has_nulls = true;
      if (input[i] == '\n')
        ++line_count;
      else if (input[i] == '\r') {
        ++line_count;
        if (i + 1 < input.size() && input[i + 1] == '\n') ++i;
      }
    }

    // Reserve space for line offsets (cache-friendly contiguous array)
    line_offsets_.reserve(line_count + 1);

    if (has_nulls) [[unlikely]] {
      // Slow path: copy and replace nulls
      buffer_.reserve(input.size() +
                      line_count);  // Extra for replacement chars
      size_t line_start = 0;
      for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '\n') {
          line_offsets_.emplace_back(line_start, buffer_.size() - line_start);
          line_start = buffer_.size();
        } else if (c == '\r') {
          line_offsets_.emplace_back(line_start, buffer_.size() - line_start);
          if (i + 1 < input.size() && input[i + 1] == '\n') ++i;
          line_start = buffer_.size();
        } else if (c == '\0') {
          buffer_ += "\xEF\xBF\xBD";  // UTF-8 replacement character
        } else {
          buffer_ += c;
        }
      }
      // Add final line
      if (buffer_.size() > line_start ||
          (input.back() == '\n' || input.back() == '\r')) {
        line_offsets_.emplace_back(line_start, buffer_.size() - line_start);
      }
      data_ptr_ = buffer_.data();
    } else {
      // Fast path: just store offsets into original input (zero-copy)
      data_ptr_ = input.data();
      size_t line_start = 0;
      for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '\n') {
          line_offsets_.emplace_back(line_start, i - line_start);
          line_start = i + 1;
        } else if (c == '\r') {
          line_offsets_.emplace_back(line_start, i - line_start);
          if (i + 1 < input.size() && input[i + 1] == '\n') ++i;
          line_start = i + 1;
        }
      }
      // Add final line
      if (input.size() > line_start ||
          (input.back() == '\n' || input.back() == '\r')) {
        line_offsets_.emplace_back(line_start, input.size() - line_start);
      }
    }
  }

  size_t size() const { return line_offsets_.size(); }
  bool empty() const { return line_offsets_.empty(); }

  std::string_view operator[](size_t idx) const {
    const auto& [offset, len] = line_offsets_[idx];
    return std::string_view(data_ptr_ + offset, len);
  }

 private:
  struct LineOffset {
    size_t offset;
    size_t length;
  };
  std::pmr::vector<LineOffset> line_offsets_;  // Contiguous array of offsets
  std::pmr::string buffer_;                    // Only used if nulls present
  const char* data_ptr_ = nullptr;  // Points to buffer_ or original input
};

// Legacy SplitLines for compatibility (used by some functions)
inline std::pmr::vector<std::pmr::string> SplitLines(std::string_view input) {
  LineBuffer buf(input);
  std::pmr::vector<std::pmr::string> lines;
  lines.reserve(buf.size());
  for (size_t i = 0; i < buf.size(); ++i) {
    lines.emplace_back(buf[i]);
  }
  return lines;
}

// Helper for transparent string_view lookup in unordered_map
struct StringHash {
  using is_transparent = void;
  size_t operator()(std::string_view sv) const noexcept {
    return std::hash<std::string_view>{}(sv);
  }
  size_t operator()(const std::pmr::string& s) const noexcept {
    return std::hash<std::string_view>{}(s);
  }
};

struct StringEqual {
  using is_transparent = void;
  bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
    return lhs == rhs;
  }
};

// HTML named entity map (common entities from CommonMark spec)
// Uses transparent lookup to avoid string_view->string conversion
inline std::string_view LookupHtmlEntity(std::string_view name) {
  // This is a subset - full CommonMark compliance requires all HTML5 entities
  static const std::pmr::unordered_map<std::pmr::string, std::string_view,
                                       StringHash, StringEqual>
      entities = {
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

  auto it = entities.find(name);
  if (it != entities.end()) {
    return it->second;
  }
  return "";
}

// Decode HTML entities (named, decimal, hex) and backslash escapes
// Optimized with batch copying for better cache locality
inline std::pmr::string DecodeEscapesAndEntities(std::string_view text) {
  // Fast path: check if any processing needed
  bool needs_processing = false;
  for (char c : text) {
    if (c == '\\' || c == '&') {
      needs_processing = true;
      break;
    }
  }
  if (!needs_processing) {
    return std::pmr::string(text);  // Zero-copy for common case
  }

  std::pmr::string result;
  result.reserve(text.size());

  size_t i = 0;
  while (i < text.size()) {
    // Find span of characters that don't need processing
    size_t span_start = i;
    while (i < text.size() && text[i] != '\\' && text[i] != '&') {
      ++i;
    }
    // Batch copy the span
    if (i > span_start) {
      result.append(text.data() + span_start, i - span_start);
    }
    if (i >= text.size()) break;

    // Backslash escape
    if (text[i] == '\\' && i + 1 < text.size() &&
        IsAsciiPunctuation(text[i + 1])) {
      result += text[i + 1];
      i += 2;
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
              code_point =
                  static_cast<uint32_t>(std::stoul(num_str, nullptr, 16));
            } else {
              code_point =
                  static_cast<uint32_t>(std::stoul(num_str, nullptr, 10));
            }
            result += CodePointToUtf8(code_point);
            i = num_end + 1;
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
          std::string_view entity =
              LookupHtmlEntity(text.substr(start, name_end - start));
          if (!entity.empty()) {
            result += entity;
            i = name_end + 1;
            continue;
          }
        }
      }
    }

    // No escape/entity matched, add literal character
    result += text[i];
    ++i;
  }

  return result;
}

// Decode backslash escapes only (no entities)
// Optimized with fast path and batch copying
inline std::pmr::string DecodeEscapes(std::string_view text) {
  // Fast path: check if any escapes present
  if (text.find('\\') == std::string_view::npos) {
    return std::pmr::string(text);  // No escapes, zero-copy
  }

  std::pmr::string result;
  result.reserve(text.size());

  size_t i = 0;
  while (i < text.size()) {
    // Find span without backslashes
    size_t span_start = i;
    while (i < text.size() && text[i] != '\\') {
      ++i;
    }
    // Batch copy the span
    if (i > span_start) {
      result.append(text.data() + span_start, i - span_start);
    }
    // Handle backslash
    if (i < text.size()) {
      if (i + 1 < text.size() && IsAsciiPunctuation(text[i + 1])) {
        result += text[i + 1];
        i += 2;
      } else {
        result += text[i];
        ++i;
      }
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
  explicit InlineParser(const LinkRefMap* link_refs = nullptr,
                        std::deque<std::pmr::string>* string_storage = nullptr,
                        std::pmr::vector<InlineNode>* inline_pool = nullptr)
      : link_references_(link_refs),
        string_storage_(string_storage),
        inline_pool_(inline_pool) {}

  std::pmr::vector<InlineNodeId> Parse(std::string_view text) {
    text_ = text;
    pos_ = 0;
    return ParseInlines();
  }

 private:
  std::string_view text_;
  size_t pos_ = 0;
  const LinkRefMap* link_references_ = nullptr;
  std::deque<std::pmr::string>* string_storage_ = nullptr;
  std::pmr::vector<InlineNode>* inline_pool_ = nullptr;

  // Store a string in persistent storage and return a view into it
  std::string_view StoreString(std::pmr::string s) {
    string_storage_->push_back(std::move(s));
    return string_storage_->back();
  }

  // Add a node to the pool and return its ID
  InlineNodeId AddToPool(InlineNode node) {
    InlineNodeId id = static_cast<InlineNodeId>(inline_pool_->size());
    inline_pool_->push_back(std::move(node));
    return id;
  }

  // Convert a vector of nodes to IDs by adding them all to the pool
  std::pmr::vector<InlineNodeId> NodesToIds(
      std::pmr::vector<InlineNode>& nodes) {
    std::pmr::vector<InlineNodeId> ids;
    ids.reserve(nodes.size());
    for (auto& node : nodes) {
      ids.push_back(AddToPool(std::move(node)));
    }
    return ids;
  }

  struct DelimiterNode {
    size_t pos;       // Position in result vector
    size_t text_pos;  // Position in original text (for extracting raw labels)
    size_t count;
    char delimiter;
    bool can_open;
    bool can_close;
    bool active;
    std::pmr::vector<InlineNode>* target;
  };

  // Check if brackets are balanced in text between start and end (exclusive)
  bool AreBracketsBalanced(size_t start, size_t end) {
    int depth = 0;
    for (size_t i = start; i < end && i < text_.size(); ++i) {
      if (text_[i] == '\\' && i + 1 < end) {
        ++i;  // Skip escaped character
        continue;
      }
      if (text_[i] == '[') ++depth;
      if (text_[i] == ']') --depth;
      if (depth < 0) [[unlikely]]
        return false;  // More closes than opens
    }
    return depth == 0;
  }

  std::pmr::vector<InlineNodeId> ParseInlines() {
    std::pmr::vector<InlineNode> result;
    // Reserve based on text length heuristic (~1 node per 20 chars)
    result.reserve(text_.size() / 20 + 4);

    std::pmr::vector<DelimiterNode> delimiter_stack;

    // Track spans instead of accumulating text
    size_t text_start = 0;
    bool in_span = false;

    auto flush_text = [&]() {
      if (in_span && text_start < pos_) {
        result.push_back(Text(text_.substr(text_start, pos_ - text_start)));
        in_span = false;
      }
    };

    auto flush_text_trimmed = [&]() {
      // Flush text but trim trailing spaces (for soft breaks)
      if (in_span && text_start < pos_) {
        size_t end = pos_;
        while (end > text_start && text_[end - 1] == ' ') {
          --end;
        }
        if (end > text_start) {
          result.push_back(Text(text_.substr(text_start, end - text_start)));
        }
        in_span = false;
      }
    };

    auto start_span = [&]() {
      if (!in_span) {
        text_start = pos_;
        in_span = true;
      }
    };

    while (pos_ < text_.size()) {
      char c = text_[pos_];

      // Check for backslash escape
      if (c == '\\' && pos_ + 1 < text_.size()) {
        char next = text_[pos_ + 1];
        if (detail::IsAsciiPunctuation(next)) {
          flush_text();
          // The escaped char is at pos_+1 in the input, use a view into it
          result.push_back(Text(text_.substr(pos_ + 1, 1)));
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
        // Flush text BEFORE trying to parse, since TryParseEntity advances pos_
        flush_text();
        auto entity_result = TryParseEntity();
        if (entity_result) {
          // Store decoded entity and create view into it
          result.push_back(Text(StoreString(std::move(*entity_result))));
          continue;
        }
        // Entity parse failed - start new span with the '&'
        start_span();
      }

      // Check for code span
      if (c == '`') {
        // Flush text BEFORE trying to parse, since TryParseCodeSpan advances
        // pos_
        flush_text();
        auto code_span = TryParseCodeSpan();
        if (code_span) {
          result.push_back(std::move(*code_span));
          continue;
        }
        // Code span didn't match - include backticks in current text span
        size_t backtick_count = 0;
        while (pos_ + backtick_count < text_.size() &&
               text_[pos_ + backtick_count] == '`') {
          ++backtick_count;
        }
        // Start new span that includes the backticks
        start_span();
        pos_ += backtick_count;
        continue;
      }

      // Check for autolink
      if (c == '<') {
        // Flush text BEFORE trying to parse, since Try* functions advance pos_
        flush_text();
        auto autolink = TryParseAutolink();
        if (autolink) {
          result.push_back(std::move(*autolink));
          continue;
        }

        // Check for HTML tag
        auto html = TryParseHtmlInline();
        if (html) {
          result.push_back(std::move(*html));
          continue;
        }
        // Neither autolink nor HTML - treat '<' as regular text
        start_span();
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
          can_open = left_flanking &&
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
        delimiter_stack.push_back({result.size(), pos_, run_length, c, can_open,
                                   can_close, true, &result});

        // Add the delimiter characters as text - view into input
        result.push_back(Text(text_.substr(pos_, run_length)));
        pos_ += run_length;
        continue;
      }

      // Check for link/image start
      if (c == '[' ||
          (c == '!' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '[')) {
        bool is_image = (c == '!');
        flush_text();

        size_t bracket_pos = result.size();
        size_t bracket_text_pos = pos_;  // Position in original text
        delimiter_stack.push_back({bracket_pos, bracket_text_pos, 1,
                                   is_image ? '!' : '[', true, false, true,
                                   &result});

        if (is_image) {
          result.push_back(Text(text_.substr(pos_, 2)));  // "!["
          pos_ += 2;
        } else {
          result.push_back(Text(text_.substr(pos_, 1)));  // "["
          pos_ += 1;
        }
        continue;
      }

      // Check for link close
      if (c == ']') {
        flush_text();

        // Look for matching opener - try multiple if brackets unbalanced
        auto opener = delimiter_stack.end();
        for (auto it = delimiter_stack.rbegin(); it != delimiter_stack.rend();
             ++it) {
          if ((it->delimiter == '[' || it->delimiter == '!') && it->active) {
            // Check if brackets are balanced between this opener and closer
            size_t content_start =
                it->text_pos + (it->delimiter == '!' ? 2 : 1);
            if (AreBracketsBalanced(content_start, pos_)) {
              opener = std::prev(it.base());
              break;
            }
            // Unbalanced - skip this opener, try earlier ones
          }
        }

        if (opener != delimiter_stack.end()) {
          // Try to parse link destination
          size_t saved_pos = pos_;
          ++pos_;  // Skip ']'

          auto link_result = TryParseLinkTail();
          if (link_result) {
            // Build the link/image
            bool is_image = (opener->delimiter == '!');

            // Collect inline content between opener and closer
            std::pmr::vector<InlineNode> link_content;
            for (size_t i = opener->pos + 1; i < result.size(); ++i) {
              link_content.push_back(std::move(result[i]));
            }

            // Extract delimiters that belong to link content and process
            // emphasis
            std::pmr::vector<DelimiterNode> link_delimiters;
            for (auto it = opener + 1; it != delimiter_stack.end(); ++it) {
              if (it->pos > opener->pos && it->pos < result.size()) {
                DelimiterNode d = *it;
                d.pos -= (opener->pos + 1);  // Adjust to link_content indices
                link_delimiters.push_back(d);
              }
            }
            ProcessEmphasis(link_content, link_delimiters);

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
              link.children = NodesToIds(link_content);
              result.push_back(std::move(link));
            }

            // Deactivate openers - when a link (not image) is formed,
            // all preceding [ openers are also deactivated (no nested links)
            if (!is_image) {
              for (auto& d : delimiter_stack) {
                if (d.delimiter == '[') {
                  d.active = false;
                }
              }
            }
            delimiter_stack.erase(opener, delimiter_stack.end());
            continue;
          }

          // Try shortcut/collapsed reference link
          // Check if TryParseLinkTail consumed a collapsed reference []
          bool is_collapsed_ref =
              (pos_ == saved_pos + 3 && saved_pos + 2 < text_.size() &&
               text_[saved_pos + 1] == '[' && text_[saved_pos + 2] == ']');

          // Check if there was a full reference [label] that failed lookup
          // In that case, don't try shortcut reference - per CommonMark spec,
          // if a full reference [foo][bar] fails because bar is undefined,
          // we don't fall back to shortcut [foo]
          bool had_full_ref_attempt =
              (saved_pos + 1 < text_.size() && text_[saved_pos + 1] == '[' &&
               !is_collapsed_ref);
          if (had_full_ref_attempt) {
            // Full reference was attempted but failed - don't try shortcut
            pos_ = saved_pos;
            opener->active = false;
            result.push_back(Text(text_.substr(pos_, 1)));  // "]"
            ++pos_;
            continue;
          }

          // Only reset pos_ if not a collapsed reference - in that case
          // TryParseLinkTail correctly consumed the [] but returned nullopt
          if (!is_collapsed_ref) {
            pos_ = saved_pos + 1;
          }

          // Look up the text inside brackets as a reference label
          // IMPORTANT: Use raw text from text_ to preserve escapes for matching
          // (Text nodes have already decoded escapes like \! to !)
          size_t label_start =
              opener->text_pos + (opener->delimiter == '!' ? 2 : 1);
          size_t label_end = saved_pos;  // Position of ']'
          std::string_view label_text =
              text_.substr(label_start, label_end - label_start);

          auto ref_result = LookupReference(label_text);
          if (ref_result) {
            bool is_image = (opener->delimiter == '!');

            std::pmr::vector<InlineNode> link_content;
            for (size_t i = opener->pos + 1; i < result.size(); ++i) {
              link_content.push_back(std::move(result[i]));
            }

            // Extract delimiters that belong to link content and process
            // emphasis
            std::pmr::vector<DelimiterNode> link_delimiters;
            for (auto it = opener + 1; it != delimiter_stack.end(); ++it) {
              if (it->pos > opener->pos && it->pos < result.size()) {
                DelimiterNode d = *it;
                d.pos -= (opener->pos + 1);  // Adjust to link_content indices
                link_delimiters.push_back(d);
              }
            }
            ProcessEmphasis(link_content, link_delimiters);

            result.resize(opener->pos);

            if (is_image) {
              Image img;
              img.destination = std::move(ref_result->first);
              img.title = std::move(ref_result->second);
              img.alt_text = GetAltText(link_content);
              result.push_back(std::move(img));
            } else {
              Link link;
              link.destination = std::move(ref_result->first);
              link.title = std::move(ref_result->second);
              link.children = NodesToIds(link_content);
              result.push_back(std::move(link));
            }

            // Deactivate openers - when a link (not image) is formed,
            // all preceding [ openers are also deactivated (no nested links)
            if (!is_image) {
              for (auto& d : delimiter_stack) {
                if (d.delimiter == '[') {
                  d.active = false;
                }
              }
            }
            delimiter_stack.erase(opener, delimiter_stack.end());
            // Note: pos_ was already advanced past ']' at line 1167
            continue;
          }

          pos_ = saved_pos;
          // Deactivate this opener since it couldn't form a link
          opener->active = false;
        }

        result.push_back(Text(text_.substr(pos_, 1)));  // "]"
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
        // Trim trailing spaces from text span
        flush_text_trimmed();
        result.push_back(SoftBreak{});
        ++pos_;
        continue;
      }

      // Regular character - extend current span
      start_span();
      ++pos_;
    }

    flush_text();

    // Process emphasis
    ProcessEmphasis(result, delimiter_stack);

    // Convert local nodes to pool IDs
    return NodesToIds(result);
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
    if (pos + length >= text_.size())
      return true;  // End of string is like whitespace

    // Get code point after the delimiter run
    auto [after_cp, after_len] = detail::DecodeUtf8At(text_, pos + length);

    return detail::IsUnicodeWhitespaceCodepoint(after_cp) ||
           detail::IsUnicodePunctuation(after_cp);
  }

  std::pmr::vector<DelimiterNode>::iterator FindLinkOpener(
      std::pmr::vector<DelimiterNode>& stack) {
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
          std::pmr::string content(
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
    if (pos_ >= text_.size() || text_[pos_] != '<') [[unlikely]]
      return std::nullopt;

    size_t start = pos_ + 1;
    size_t end = start;

    while (end < text_.size() && text_[end] != '>' && text_[end] != '<' &&
           text_[end] != '\n') {
      ++end;
    }

    if (end >= text_.size() || text_[end] != '>') [[unlikely]]
      return std::nullopt;

    std::string_view content = text_.substr(start, end - start);

    // Check for URI autolink
    static const std::regex uri_regex(
        R"(^[a-zA-Z][a-zA-Z0-9+.-]{1,31}:[^\s<>]*$)");
    std::string content_str(content);  // For regex matching only
    if (std::regex_match(content_str, uri_regex)) {
      pos_ = end + 1;
      Link link;
      link.destination = detail::EncodeUrl(content);
      link.children.push_back(AddToPool(Text(content)));
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
      link.destination = "mailto:" + std::pmr::string(content);
      link.children.push_back(AddToPool(Text(content)));
      return link;
    }

    return std::nullopt;
  }

  std::optional<std::pmr::string> TryParseEntity() {
    if (pos_ >= text_.size() || text_[pos_] != '&') [[unlikely]]
      return std::nullopt;

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
        std::string_view entity =
            detail::LookupHtmlEntity(text_.substr(start, name_end - start));
        if (!entity.empty()) {
          pos_ = name_end + 1;
          return std::pmr::string(entity);
        }
      }
    }

    return std::nullopt;
  }

  std::optional<HtmlInline> TryParseHtmlInline() {
    if (pos_ >= text_.size() || text_[pos_] != '<') [[unlikely]]
      return std::nullopt;

    size_t start = pos_;
    size_t end = pos_ + 1;

    // Simple HTML tag detection
    if (end >= text_.size()) [[unlikely]]
      return std::nullopt;

    bool is_closing = (text_[end] == '/');
    if (is_closing) ++end;

    // Check for valid tag name start
    if (end >= text_.size() ||
        !std::isalpha(static_cast<unsigned char>(text_[end]))) {
      // Check for comment, CDATA, processing instruction, or declaration
      if (text_.substr(pos_).starts_with("<!--")) {
        // Per CommonMark spec: <!--> and <!---> are valid (immediately closed)
        // Otherwise, find --> to close, but text must not start with > or ->
        if (pos_ + 4 < text_.size()) {
          char next = text_[pos_ + 4];
          if (next == '>') {
            // <!--> is a valid immediately-closed comment
            pos_ = pos_ + 5;
            return HtmlInline(text_.substr(start, pos_ - start));
          }
          if (next == '-' && pos_ + 5 < text_.size() &&
              text_[pos_ + 5] == '>') {
            // <!---> is a valid immediately-closed comment
            pos_ = pos_ + 6;
            return HtmlInline(text_.substr(start, pos_ - start));
          }
          // Normal comment: find -->
          end = text_.find("-->", pos_ + 4);
          if (end != std::string_view::npos) {
            pos_ = end + 3;
            return HtmlInline(text_.substr(start, pos_ - start));
          }
        } else if (pos_ + 4 == text_.size()) {
          // Just "<!--" at end - not a valid comment, leave as text
        }
      }
      if (text_.substr(pos_).starts_with("<![CDATA[")) {
        end = text_.find("]]>", pos_ + 9);
        if (end != std::string_view::npos) {
          pos_ = end + 3;
          return HtmlInline(text_.substr(start, pos_ - start));
        }
      }
      if (text_.substr(pos_).starts_with("<?")) {
        end = text_.find("?>", pos_ + 2);
        if (end != std::string_view::npos) {
          pos_ = end + 2;
          return HtmlInline(text_.substr(start, pos_ - start));
        }
      }
      if (text_.substr(pos_).starts_with("<!") && end + 1 < text_.size() &&
          std::isupper(static_cast<unsigned char>(text_[end + 1]))) {
        end = text_.find('>', pos_ + 2);
        if (end != std::string_view::npos) {
          pos_ = end + 1;
          return HtmlInline(text_.substr(start, pos_ - start));
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
    // For closing tags, only whitespace and > are allowed (no attributes)
    bool seen_whitespace = false;
    while (end < text_.size()) {
      char c = text_[end];
      if (c == '>') {
        pos_ = end + 1;
        return HtmlInline(text_.substr(start, pos_ - start));
      } else if (is_closing) {
        // Closing tags: only whitespace allowed before >
        if (c == ' ' || c == '\t' || c == '\n') {
          ++end;
        } else {
          return std::nullopt;  // Invalid char in closing tag
        }
      } else if (c == '/') {
        // Self-closing: must be followed by >
        if (end + 1 < text_.size() && text_[end + 1] == '>') {
          pos_ = end + 2;
          return HtmlInline(text_.substr(start, pos_ - start));
        }
        return std::nullopt;  // / not followed by >
      } else if (c == ' ' || c == '\t' || c == '\n') {
        seen_whitespace = true;
        ++end;
      } else if (seen_whitespace &&
                 (std::isalpha(static_cast<unsigned char>(c)) || c == '_' ||
                  c == ':')) {
        // Start of attribute name (must be preceded by whitespace)
        seen_whitespace = false;  // Reset - need whitespace before next attr
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
        while (end < text_.size() && (text_[end] == ' ' || text_[end] == '\t' ||
                                      text_[end] == '\n')) {
          seen_whitespace = true;
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
          if (end >= text_.size()) [[unlikely]]
            return std::nullopt;
          if (text_[end] == '"' || text_[end] == '\'') {
            char quote = text_[end];
            ++end;
            while (end < text_.size() && text_[end] != quote) {
              ++end;
            }
            if (end >= text_.size()) [[unlikely]]
              return std::nullopt;
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

  std::optional<std::pair<std::pmr::string, std::pmr::string>>
  TryParseLinkTail() {
    if (pos_ >= text_.size()) [[unlikely]]
      return std::nullopt;

    // Inline link: (destination "title")
    if (text_[pos_] == '(') {
      ++pos_;
      SkipWhitespace();

      std::pmr::string destination;
      std::pmr::string title;

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
        if (pos_ >= text_.size() || text_[pos_] != '>') [[unlikely]]
          return std::nullopt;
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
      if (pos_ < text_.size() &&
          (text_[pos_] == '"' || text_[pos_] == '\'' || text_[pos_] == '(')) {
        char close_char = text_[pos_] == '(' ? ')' : text_[pos_];
        ++pos_;
        size_t title_start = pos_;
        while (pos_ < text_.size() && text_[pos_] != close_char) {
          if (text_[pos_] == '\\' && pos_ + 1 < text_.size()) {
            ++pos_;
          }
          ++pos_;
        }
        if (pos_ >= text_.size()) [[unlikely]]
          return std::nullopt;
        title = detail::DecodeEscapesAndEntities(
            text_.substr(title_start, pos_ - title_start));
        ++pos_;
        SkipWhitespace();
      }

      if (pos_ >= text_.size() || text_[pos_] != ')') [[unlikely]]
        return std::nullopt;
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

      if (pos_ >= text_.size()) [[unlikely]]
        return std::nullopt;

      std::pmr::string label(text_.substr(label_start, pos_ - label_start));
      ++pos_;

      if (label.empty()) [[unlikely]] {
        // Collapsed reference - label comes from link text
        return std::nullopt;  // Need to look up later
      }

      return LookupReference(label);
    }

    return std::nullopt;
  }

  std::optional<std::pair<std::pmr::string, std::pmr::string>> LookupReference(
      std::string_view label) {
    if (!link_references_) [[unlikely]]
      return std::nullopt;

    std::pmr::string normalized = detail::NormalizeLinkLabel(label);
    auto it = link_references_->find(normalized);
    if (it != link_references_->end()) {
      return it->second;
    }
    return std::nullopt;
  }

  void SkipWhitespace() {
    while (pos_ < text_.size() &&
           (text_[pos_] == ' ' || text_[pos_] == '\t' || text_[pos_] == '\n' ||
            text_[pos_] == '\r')) {
      ++pos_;
    }
  }

  // Get alt text from a vector of InlineNodeIds (nodes already in pool)
  std::pmr::string GetAltTextFromIds(
      const std::pmr::vector<InlineNodeId>& node_ids) {
    std::pmr::string result;
    for (InlineNodeId id : node_ids) {
      const auto& node = (*inline_pool_)[id];
      std::visit(
          [&result, this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Text>) {
              result += arg.content;
            } else if constexpr (std::is_same_v<T, Code>) {
              result += arg.content;
            } else if constexpr (std::is_same_v<T, SoftBreak>) {
              result += ' ';
            } else if constexpr (std::is_same_v<T, HardBreak>) {
              result += ' ';
            } else if constexpr (std::is_same_v<T, Emphasis>) {
              result += GetAltTextFromIds(arg.children);
            } else if constexpr (std::is_same_v<T, Strong>) {
              result += GetAltTextFromIds(arg.children);
            } else if constexpr (std::is_same_v<T, Link>) {
              result += GetAltTextFromIds(arg.children);
            } else if constexpr (std::is_same_v<T, Image>) {
              result += arg.alt_text;
            }
          },
          node);
    }
    return result;
  }

  // Get alt text from a vector of local InlineNodes (not yet in pool)
  std::pmr::string GetAltText(const std::pmr::vector<InlineNode>& nodes) {
    std::pmr::string result;
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
            } else if constexpr (std::is_same_v<T, HardBreak>) {
              result += ' ';
            } else if constexpr (std::is_same_v<T, Emphasis>) {
              result += GetAltTextFromIds(arg.children);
            } else if constexpr (std::is_same_v<T, Strong>) {
              result += GetAltTextFromIds(arg.children);
            } else if constexpr (std::is_same_v<T, Link>) {
              result += GetAltTextFromIds(arg.children);
            } else if constexpr (std::is_same_v<T, Image>) {
              result += arg.alt_text;
            }
          },
          node);
    }
    return result;
  }

  void ProcessEmphasis(std::pmr::vector<InlineNode>& nodes,
                       std::pmr::vector<DelimiterNode>& delimiters) {
    if (delimiters.empty()) [[unlikely]]
      return;

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
          if ((opener.count + closer.count) % 3 == 0 && opener.count % 3 != 0 &&
              closer.count % 3 != 0) {
            continue;
          }
        }

        found_opener = true;

        // Determine emphasis type
        bool is_strong = opener.count >= 2 && closer.count >= 2;
        size_t delim_count = is_strong ? 2 : 1;

        // Build emphasis node - reserve capacity to avoid reallocations
        std::pmr::vector<InlineNode> content;
        content.reserve(closer.pos - opener.pos - 1);
        for (size_t i = opener.pos + 1; i < closer.pos; ++i) {
          content.push_back(std::move(nodes[i]));
        }

        // Create emphasis node - convert children to pool IDs
        InlineNode emph_node;
        if (is_strong) {
          Strong strong;
          strong.children = NodesToIds(content);
          emph_node = std::move(strong);
        } else {
          Emphasis em;
          em.children = NodesToIds(content);
          emph_node = std::move(em);
        }

        // Update opener text - delimiters consumed from the END of opener run
        // Remaining delimiters are at the START of the original view
        if (opener.count > delim_count) {
          auto& opener_content = std::get<Text>(nodes[opener.pos]).content;
          opener_content = opener_content.substr(0, opener.count - delim_count);
          opener.count -= delim_count;
        } else {
          nodes[opener.pos] = Text("");
          opener.active = false;
        }

        // Update closer text - delimiters consumed from the BEGINNING of closer
        // run Remaining delimiters are at the END of the original view
        if (closer.count > delim_count) {
          auto& closer_content = std::get<Text>(nodes[closer.pos]).content;
          closer_content = closer_content.substr(delim_count);
          closer.count -= delim_count;
        } else {
          nodes[closer.pos] = Text("");
          closer.active = false;
        }

        // Insert emphasis node, removing the content in between
        // Reserve capacity: opener nodes + 1 emph node + nodes after closer
        std::pmr::vector<InlineNode> new_nodes;
        new_nodes.reserve(opener.pos + 1 + 1 + (nodes.size() - closer.pos));
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

    // Remove empty text nodes in-place (avoids extra vector allocation)
    auto new_end =
        std::remove_if(nodes.begin(), nodes.end(), [](const InlineNode& node) {
          if (auto* text = std::get_if<Text>(&node)) {
            return text->content.empty();
          }
          return false;
        });
    nodes.erase(new_end, nodes.end());
  }
};

// =============================================================================
// Block Parser
// =============================================================================

class BlockParser {
 public:
  Document Parse(std::string_view input) {
    Document doc;
    doc_ = &doc;
    // Use LineBuffer for cache-friendly line storage (contiguous offsets)
    lines_ = std::make_unique<detail::LineBuffer>(input);
    line_idx_ = 0;
    parent_link_refs_ = &doc.link_references;

    // First pass: extract link reference definitions
    ExtractLinkReferences(doc);

    // Second pass: parse blocks
    line_idx_ = 0;
    ParseBlocks(doc.children);

    // Third pass: parse inlines
    InlineParser inline_parser(&doc.link_references, &doc.string_storage,
                               &doc.inline_nodes);
    ParseInlines(doc.children, inline_parser);

    return doc;
  }

 private:
  // LineBuffer provides cache-friendly storage: contiguous offset array
  // pointing into single buffer, vs vector<string> with many allocations
  std::unique_ptr<detail::LineBuffer> lines_;
  size_t line_idx_ = 0;
  LinkRefMap* parent_link_refs_ = nullptr;
  Document* doc_ = nullptr;

  // Transfer blocks from a nested document to the parent's pool and return IDs
  // Also transfers inline nodes, string storage, and remaps their IDs
  std::pmr::vector<BlockNodeId> TransferFromNestedDoc(Document& nested_doc) {
    // Transfer string_storage by COPYING to preserve string_view validity.
    // SSO strings have data inside the object, so moving would invalidate
    // views. We need to record how many strings we're copying to calculate new
    // indices.
    size_t string_storage_base = doc_->string_storage.size();
    for (const auto& s : nested_doc.string_storage) {
      doc_->string_storage.push_back(s);  // COPY, not move
    }

    // Helper to update a string_view to point to the new string location
    auto update_string_view = [&](std::string_view& sv) {
      const char* sv_data = sv.data();
      // Find the original string this view points into
      for (size_t i = 0; i < nested_doc.string_storage.size(); ++i) {
        const auto& old_str = nested_doc.string_storage[i];
        if (sv_data >= old_str.data() &&
            sv_data < old_str.data() + old_str.size()) {
          // Found it - compute offset and update to new location
          size_t offset = sv_data - old_str.data();
          const auto& new_str = doc_->string_storage[string_storage_base + i];
          sv = std::string_view(new_str.data() + offset, sv.size());
          return;
        }
      }
    };

    // First, transfer all inline nodes from nested to parent and build ID map
    std::pmr::vector<InlineNodeId> inline_id_map;
    inline_id_map.reserve(nested_doc.inline_nodes.size());
    for (auto& inline_node : nested_doc.inline_nodes) {
      // Update string_views in Text/HtmlInline nodes before moving
      // (Code has pmr::string, not string_view, so it doesn't need updating)
      std::visit(
          [&](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Text>) {
              update_string_view(node.content);
            } else if constexpr (std::is_same_v<T, HtmlInline>) {
              update_string_view(node.content);
            }
          },
          inline_node);
      InlineNodeId new_id =
          static_cast<InlineNodeId>(doc_->inline_nodes.size());
      doc_->inline_nodes.push_back(std::move(inline_node));
      inline_id_map.push_back(new_id);
    }

    // Helper to remap inline IDs in a vector
    auto remap_inline_ids = [&](std::pmr::vector<InlineNodeId>& ids) {
      for (auto& id : ids) {
        id = inline_id_map[id];
      }
    };

    // Remap inline IDs in nested inline nodes (for Emphasis/Strong/Link
    // children) These were just moved to doc_->inline_nodes
    size_t start_idx = doc_->inline_nodes.size() - inline_id_map.size();
    for (size_t i = start_idx; i < doc_->inline_nodes.size(); ++i) {
      std::visit(
          [&](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Emphasis> ||
                          std::is_same_v<T, Strong> ||
                          std::is_same_v<T, Link>) {
              remap_inline_ids(node.children);
            }
          },
          doc_->inline_nodes[i]);
    }

    // First, transfer nested block_nodes (for nested BlockQuotes/ListItems)
    // Build block_id_map so we can remap IDs in Lists/BlockQuotes
    std::pmr::vector<BlockNodeId> block_id_map;
    block_id_map.reserve(nested_doc.block_nodes.size());
    for (auto& nested_block : nested_doc.block_nodes) {
      // Remap inline IDs in nested blocks
      std::visit(
          [&](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Paragraph> ||
                          std::is_same_v<T, Heading>) {
              remap_inline_ids(node.children);
            }
          },
          nested_block);
      BlockNodeId new_id = doc_->AddBlock(std::move(nested_block));
      block_id_map.push_back(new_id);
    }

    // Helper to remap block IDs using block_id_map
    auto remap_block_ids = [&](auto& block) {
      std::visit(
          [&](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, BlockQuote>) {
              for (auto& id : node.children) {
                if (id < block_id_map.size()) {
                  id = block_id_map[id];
                }
              }
            } else if constexpr (std::is_same_v<T, List>) {
              for (auto& item : node.items) {
                for (auto& id : item.children) {
                  if (id < block_id_map.size()) {
                    id = block_id_map[id];
                  }
                }
              }
            } else if constexpr (std::is_same_v<T, ListItem>) {
              for (auto& id : node.children) {
                if (id < block_id_map.size()) {
                  id = block_id_map[id];
                }
              }
            }
          },
          block);
    };

    // Remap block IDs in the transferred block_nodes
    size_t block_start_idx = doc_->block_nodes.size() - block_id_map.size();
    for (size_t i = block_start_idx; i < doc_->block_nodes.size(); ++i) {
      remap_block_ids(doc_->block_nodes[i]);
    }

    // Now transfer blocks from nested_doc.children and remap their IDs
    std::pmr::vector<BlockNodeId> block_ids;
    block_ids.reserve(nested_doc.children.size());

    for (auto& block : nested_doc.children) {
      // Remap inline IDs in the block
      std::visit(
          [&](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Paragraph> ||
                          std::is_same_v<T, Heading>) {
              remap_inline_ids(node.children);
            }
          },
          block);
      // Remap block IDs in Lists/BlockQuotes
      remap_block_ids(block);
      block_ids.push_back(doc_->AddBlock(std::move(block)));
    }

    return block_ids;
  }

  bool AtEnd() const { return !lines_ || line_idx_ >= lines_->size(); }

  std::string_view CurrentLine() const {
    if (AtEnd()) return "";
    return (*lines_)[line_idx_];
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

  // Check if brackets are balanced in a label string
  bool IsLabelBracketsBalanced(std::string_view label) {
    int depth = 0;
    for (size_t i = 0; i < label.size(); ++i) {
      if (label[i] == '\\' && i + 1 < label.size()) {
        ++i;  // Skip escaped character
        continue;
      }
      if (label[i] == '[') ++depth;
      if (label[i] == ']') --depth;
      if (depth < 0) [[unlikely]]
        return false;
    }
    return depth == 0;
  }

  // Helper to parse destination with balanced parentheses
  std::pair<std::pmr::string, size_t> ParseLinkDestination(std::string_view s) {
    if (s.empty()) [[unlikely]]
      return {"", 0};

    if (s[0] == '<') {
      // Angle-bracket destination
      for (size_t i = 1; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
          ++i;
        } else if (s[i] == '>') {
          return {std::pmr::string(s.substr(1, i - 1)), i + 1};
        } else if (s[i] == '<' || s[i] == '\n') [[unlikely]] {
          return {"", 0};
        }
      }
      return {"", 0};  // Unclosed angle bracket - unlikely
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
      } else if (detail::IsUnicodeWhitespace(c) || static_cast<unsigned char>(c) < 0x20) {
        break;
      } else {
        ++end;
      }
    }
    if (paren_depth != 0) [[unlikely]]
      return {"", 0};
    return {std::pmr::string(s.substr(0, end)), end};
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
              trimmed.substr(potential_len).find('`') ==
                  std::string_view::npos) {
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
          while (close_len < trimmed.size() &&
                 trimmed[close_len] == fence_char) {
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

      // Check for block-level elements that end a block (not continuable to
      // paragraph) These reset prev_line_had_content to false
      bool is_block_element = false;
      if (trimmed.starts_with('#')) {
        // ATX heading
        size_t hash_count = 0;
        while (hash_count < trimmed.size() && trimmed[hash_count] == '#')
          ++hash_count;
        if (hash_count <= 6 &&
            (hash_count >= trimmed.size() || trimmed[hash_count] == ' ' ||
             trimmed[hash_count] == '\t')) {
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
            if (ch == first)
              ++count;
            else if (ch != ' ' && ch != '\t') {
              valid = false;
              break;
            }
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
        Advance();
        continue;
      }

      if (!trimmed.starts_with('[')) {
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // Find closing bracket (handling escapes, may span multiple lines)
      size_t start_line = line_idx_;
      std::pmr::string label_raw;
      std::string_view rest;
      bool found_close = false;

      // First check if closing bracket is on the same line
      size_t close_bracket = FindClosingBracket(trimmed, 1);
      if (close_bracket != std::string_view::npos &&
          close_bracket + 1 < trimmed.size() &&
          trimmed[close_bracket + 1] == ':') {
        label_raw = std::pmr::string(trimmed.substr(1, close_bracket - 1));
        rest = detail::TrimLeft(trimmed.substr(close_bracket + 2));
        found_close = true;
        Advance();
      } else {
        // Try multi-line label (label can span multiple lines)
        label_raw = std::pmr::string(trimmed.substr(1));  // Content after '['
        Advance();

        // Collect lines until we find ']:' (limit to reasonable number of
        // lines)
        int lines_collected = 0;
        while (!AtEnd() && lines_collected < 50) {
          std::string_view next_line = CurrentLine();
          auto next_trimmed = detail::TrimLeft(next_line);

          // Look for ]: at the start of a line (allowing leading whitespace)
          size_t colon_pos = next_trimmed.find("]:");
          if (colon_pos != std::string_view::npos) {
            // Found it - check if there's no unescaped ] before this position
            bool valid = true;
            for (size_t j = 0; j < colon_pos; ++j) {
              if (next_trimmed[j] == '\\' && j + 1 < colon_pos) {
                ++j;  // Skip escaped char
              } else if (next_trimmed[j] == ']') {
                valid = false;  // Unmatched ] before ]:
                break;
              }
            }
            if (valid) {
              label_raw += '\n';
              label_raw += std::pmr::string(next_trimmed.substr(0, colon_pos));
              rest = detail::TrimLeft(next_trimmed.substr(colon_pos + 2));
              found_close = true;
              Advance();
              break;
            }
          }

          // Check for blank line - that ends the attempt
          if (detail::IsBlankLine(next_line)) {
            break;
          }

          // Continue collecting label content
          label_raw += '\n';
          label_raw += std::pmr::string(next_trimmed);
          Advance();
          ++lines_collected;
        }
      }

      if (!found_close) {
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // Check that label has balanced brackets
      if (!IsLabelBracketsBalanced(label_raw)) {
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // Normalize label for matching (WITHOUT decoding escapes per CommonMark
      // spec)
      std::pmr::string normalized = detail::NormalizeLinkLabel(label_raw);
      if (normalized.empty()) [[unlikely]] {
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }

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
      if (dest_raw.empty() && dest_len == 0 && !rest.empty() &&
          rest[0] != '<') {
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }
      std::pmr::string destination = detail::DecodeEscapesAndEntities(dest_raw);
      auto rest_after_dest = rest.substr(dest_len);
      bool has_whitespace_before_title =
          rest_after_dest.size() != detail::TrimLeft(rest_after_dest).size();
      rest = detail::TrimLeft(rest_after_dest);

      // Parse optional title (may be on same line or next line)
      std::pmr::string title;
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

      if (!rest.empty() &&
          (rest[0] == '"' || rest[0] == '\'' || rest[0] == '(')) {
        // Title must be separated from destination by whitespace (or be on new
        // line)
        if (!title_on_new_line && !has_whitespace_before_title) {
          title_valid = false;
        } else {
          char open_char = rest[0];
          char close_char = (open_char == '(') ? ')' : open_char;
          std::pmr::string title_content;
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

      // If destination parsing returned length 0 (not angle brackets) and no
      // destination, it's invalid But angle bracket destinations like <>
      // returning empty string with length > 0 are valid
      if (dest_raw.empty() && dest_len == 0) {
        line_idx_ = start_line;
        prev_line_had_content = true;
        Advance();
        continue;
      }

      // If title parsing failed but was on a new line, roll back just the title
      // line and accept the link ref without a title
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
        doc.link_references[normalized] = {detail::EncodeUrl(destination),
                                           title};
      }
      // After successful link ref extraction, next line starts fresh
      prev_line_had_content = false;
    }
    // Reset for block parsing
    line_idx_ = 0;
  }

  void ParseBlocks(std::pmr::vector<BlockNode>& blocks) {
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

    // Find closing bracket (handling escapes, may span multiple lines)
    size_t start_line = line_idx_;
    std::pmr::string label_raw;
    std::string_view rest;
    bool found_close = false;

    // First check if closing bracket is on the same line
    size_t close_bracket = FindClosingBracket(trimmed, 1);
    if (close_bracket != std::string_view::npos &&
        close_bracket + 1 < trimmed.size() &&
        trimmed[close_bracket + 1] == ':') {
      label_raw = std::pmr::string(trimmed.substr(1, close_bracket - 1));
      rest = detail::TrimLeft(trimmed.substr(close_bracket + 2));
      found_close = true;
      Advance();
    } else {
      // Try multi-line label
      label_raw = std::pmr::string(trimmed.substr(1));
      Advance();

      int lines_collected = 0;
      while (!AtEnd() && lines_collected < 50) {
        std::string_view next_line = CurrentLine();
        auto next_trimmed = detail::TrimLeft(next_line);

        size_t colon_pos = next_trimmed.find("]:");
        if (colon_pos != std::string_view::npos) {
          bool valid = true;
          for (size_t j = 0; j < colon_pos; ++j) {
            if (next_trimmed[j] == '\\' && j + 1 < colon_pos) {
              ++j;
            } else if (next_trimmed[j] == ']') {
              valid = false;
              break;
            }
          }
          if (valid) {
            label_raw += '\n';
            label_raw += std::pmr::string(next_trimmed.substr(0, colon_pos));
            rest = detail::TrimLeft(next_trimmed.substr(colon_pos + 2));
            found_close = true;
            Advance();
            break;
          }
        }

        if (detail::IsBlankLine(next_line)) {
          break;
        }

        label_raw += '\n';
        label_raw += std::pmr::string(next_trimmed);
        Advance();
        ++lines_collected;
      }
    }

    if (!found_close) {
      line_idx_ = start_line;
      return false;
    }

    // Check that label has balanced brackets
    if (!IsLabelBracketsBalanced(label_raw)) {
      line_idx_ = start_line;
      return false;
    }

    std::pmr::string normalized = detail::NormalizeLinkLabel(label_raw);
    if (normalized.empty()) [[unlikely]] {
      line_idx_ = start_line;
      return false;
    }

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

    if (!rest.empty() &&
        (rest[0] == '"' || rest[0] == '\'' || rest[0] == '(')) {
      // Title must be separated from destination by whitespace (or be on new
      // line)
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

    // If title failed but was on a new line, roll back just title and accept
    // without title
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
    if (indent >= 4) [[unlikely]]
      return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty()) [[unlikely]]
      return std::nullopt;

    char marker = trimmed[0];
    if (marker != '-' && marker != '*' && marker != '_') [[unlikely]]
      return std::nullopt;

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
    if (indent >= 4) [[unlikely]]
      return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty() || trimmed[0] != '#') [[unlikely]]
      return std::nullopt;

    int level = 0;
    size_t i = 0;
    while (i < trimmed.size() && trimmed[i] == '#') {
      ++level;
      ++i;
    }

    if (level > 6) [[unlikely]]
      return std::nullopt;
    if (i < trimmed.size() && trimmed[i] != ' ' && trimmed[i] != '\t') {
      return std::nullopt;
    }

    // Skip space after #'s
    while (i < trimmed.size() && (trimmed[i] == ' ' || trimmed[i] == '\t')) {
      ++i;
    }

    // Get content (strip trailing #'s)
    std::pmr::string content(trimmed.substr(i));

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

    return Heading{.level = level, .raw_content = std::move(content)};
  }

  std::optional<CodeBlock> TryParseFencedCodeBlock() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) [[unlikely]]
      return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty()) [[unlikely]]
      return std::nullopt;

    char fence_char = trimmed[0];
    if (fence_char != '`' && fence_char != '~') [[unlikely]]
      return std::nullopt;

    size_t fence_length = 0;
    while (fence_length < trimmed.size() &&
           trimmed[fence_length] == fence_char) {
      ++fence_length;
    }

    if (fence_length < 3) [[unlikely]]
      return std::nullopt;

    // Check for backtick in info string (not allowed for backtick fences)
    std::pmr::string info_string(detail::Trim(trimmed.substr(fence_length)));
    if (fence_char == '`' && info_string.find('`') != std::string::npos)
        [[unlikely]] {
      return std::nullopt;
    }
    // Decode backslash escapes in info string
    info_string = detail::DecodeEscapesAndEntities(info_string);

    Advance();

    // Collect code content
    std::pmr::string content;
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

    return CodeBlock{.info_string = std::move(info_string),
                     .content = std::move(content), .is_fenced = true};
  }

  std::optional<HtmlBlock> TryParseHtmlBlock() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) [[unlikely]]
      return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty() || trimmed[0] != '<') [[unlikely]]
      return std::nullopt;

    int block_type = 0;
    std::pmr::string end_condition;

    // Type 1: <script>, <pre>, <style>, <textarea>
    static constexpr std::array type1_tags = {
        std::string_view("<script"), std::string_view("<pre"),
        std::string_view("<style"), std::string_view("<textarea")};
    for (auto tag : type1_tags) {
      if (detail::StartsWithInsensitive(trimmed, tag)) {
        // Tag at end of line, or followed by space/tab/>/newline
        if (trimmed.size() == tag.size()) {
          block_type = 1;
          end_condition = "</" + std::pmr::string(tag.substr(1)) + ">";
          break;
        }
        char next = trimmed[tag.size()];
        if (next == ' ' || next == '>' || next == '\t' || next == '\n') {
          block_type = 1;
          end_condition = "</" + std::pmr::string(tag.substr(1)) + ">";
          break;
        }
      }
    }

    // Type 2: <!-- comment -->
    // The text after <!-- cannot start with > or -> (per CommonMark spec)
    if (block_type == 0 && trimmed.starts_with("<!--")) {
      // Check that what follows is not > or ->
      if (trimmed.size() > 4) {
        char next = trimmed[4];
        if (next != '>' &&
            !(next == '-' && trimmed.size() > 5 && trimmed[5] == '>')) {
          block_type = 2;
          end_condition = "-->";
        }
      } else if (trimmed.size() == 4) {
        // Just "<!--" with nothing after - valid start
        block_type = 2;
        end_condition = "-->";
      }
    }

    // Type 3: <? processing instruction ?>
    if (block_type == 0 && trimmed.starts_with("<?")) {
      block_type = 3;
      end_condition = "?>";
    }

    // Type 4: <!DOCTYPE
    if (block_type == 0 && detail::StartsWithInsensitive(trimmed, "<!doctype")) {
      block_type = 4;
      end_condition = ">";
    }

    // Type 5: <![CDATA[
    if (block_type == 0 && trimmed.starts_with("<![CDATA[")) {
      block_type = 5;
      end_condition = "]]>";
    }

    // Type 6: Block-level HTML tags
    static constexpr std::array type6_tags = {
        std::string_view("address"),    std::string_view("article"),  std::string_view("aside"),   std::string_view("base"),     std::string_view("basefont"),
        std::string_view("blockquote"), std::string_view("body"),     std::string_view("caption"), std::string_view("center"),   std::string_view("col"),
        std::string_view("colgroup"),   std::string_view("dd"),       std::string_view("details"), std::string_view("dialog"),   std::string_view("dir"),
        std::string_view("div"),        std::string_view("dl"),       std::string_view("dt"),      std::string_view("fieldset"), std::string_view("figcaption"),
        std::string_view("figure"),     std::string_view("footer"),   std::string_view("form"),    std::string_view("frame"),    std::string_view("frameset"),
        std::string_view("h1"),         std::string_view("h2"),       std::string_view("h3"),      std::string_view("h4"),       std::string_view("h5"),
        std::string_view("h6"),         std::string_view("head"),     std::string_view("header"),  std::string_view("hr"),       std::string_view("html"),
        std::string_view("iframe"),     std::string_view("legend"),   std::string_view("li"),      std::string_view("link"),     std::string_view("main"),
        std::string_view("menu"),       std::string_view("menuitem"), std::string_view("nav"),     std::string_view("noframes"), std::string_view("ol"),
        std::string_view("optgroup"),   std::string_view("option"),   std::string_view("p"),       std::string_view("param"),    std::string_view("search"),
        std::string_view("section"),    std::string_view("summary"),  std::string_view("table"),   std::string_view("tbody"),    std::string_view("td"),
        std::string_view("tfoot"),      std::string_view("th"),       std::string_view("thead"),   std::string_view("title"),    std::string_view("tr"),
        std::string_view("track"),      std::string_view("ul")};

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
        std::string_view tag_name_sv = trimmed.substr(tag_start, tag_end - tag_start);

        for (auto t : type6_tags) {
          if (detail::StartsWithInsensitive(tag_name_sv, t) &&
              tag_name_sv.size() == t.size()) {
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

        // After tag name, must have valid HTML tag continuation: whitespace, /,
        // > Anything else (like + or :) means this isn't a valid HTML tag
        bool valid_tag = false;
        if (tag_end >= trimmed.size()) {
          // Just tag name, no closing - not valid
        } else {
          char next = trimmed[tag_end];
          valid_tag =
              (next == ' ' || next == '\t' || next == '/' || next == '>');
        }
        if (valid_tag) {
          // For closing tags, only whitespace is allowed after tag name
          if (is_closing) {
            size_t search_pos = tag_end;
            // Skip whitespace
            while (
                search_pos < trimmed.size() &&
                (trimmed[search_pos] == ' ' || trimmed[search_pos] == '\t')) {
              ++search_pos;
            }
            // Must end with >
            if (search_pos < trimmed.size() && trimmed[search_pos] == '>') {
              // Check rest is whitespace
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
          } else {
            // Validate attributes properly for type 7 HTML block
            size_t search_pos = tag_end;
            bool valid_attributes = true;
            bool need_whitespace =
                false;  // Track if we need whitespace before next attribute

            while (search_pos < trimmed.size() && valid_attributes) {
              char c = trimmed[search_pos];

              if (c == '>') {
                break;  // Found end of tag
              } else if (c == '/') {
                // Self-closing, must be followed by >
                if (search_pos + 1 < trimmed.size() &&
                    trimmed[search_pos + 1] == '>') {
                  search_pos++;  // Will break on next iteration
                } else {
                  valid_attributes = false;
                }
              } else if (c == ' ' || c == '\t' || c == '\n') {
                ++search_pos;             // Skip whitespace
                need_whitespace = false;  // Whitespace seen
              } else if (!need_whitespace &&
                         (std::isalpha(static_cast<unsigned char>(c)) ||
                          c == '_' || c == ':')) {
                // Start of attribute name (must have whitespace before if not
                // first)
                ++search_pos;
                while (search_pos < trimmed.size()) {
                  char ac = trimmed[search_pos];
                  if (std::isalnum(static_cast<unsigned char>(ac)) ||
                      ac == '_' || ac == ':' || ac == '.' || ac == '-') {
                    ++search_pos;
                  } else {
                    break;
                  }
                }
                // Skip whitespace after attribute name
                while (search_pos < trimmed.size() &&
                       (trimmed[search_pos] == ' ' ||
                        trimmed[search_pos] == '\t' ||
                        trimmed[search_pos] == '\n')) {
                  ++search_pos;
                  need_whitespace = false;
                }
                // Optional attribute value
                if (search_pos < trimmed.size() && trimmed[search_pos] == '=') {
                  ++search_pos;
                  // Skip whitespace after =
                  while (search_pos < trimmed.size() &&
                         (trimmed[search_pos] == ' ' ||
                          trimmed[search_pos] == '\t' ||
                          trimmed[search_pos] == '\n')) {
                    ++search_pos;
                  }
                  if (search_pos >= trimmed.size()) {
                    valid_attributes = false;
                  } else if (trimmed[search_pos] == '"' ||
                             trimmed[search_pos] == '\'') {
                    char quote = trimmed[search_pos];
                    ++search_pos;
                    while (search_pos < trimmed.size() &&
                           trimmed[search_pos] != quote) {
                      ++search_pos;
                    }
                    if (search_pos >= trimmed.size()) {
                      valid_attributes = false;
                    } else {
                      ++search_pos;  // Skip closing quote
                      need_whitespace =
                          true;  // Need whitespace before next attr
                    }
                  } else {
                    // Unquoted value
                    while (search_pos < trimmed.size()) {
                      char vc = trimmed[search_pos];
                      if (vc == ' ' || vc == '\t' || vc == '\n' || vc == '"' ||
                          vc == '\'' || vc == '=' || vc == '<' || vc == '>' ||
                          vc == '`') {
                        break;
                      }
                      ++search_pos;
                    }
                    need_whitespace = true;  // Need whitespace before next attr
                  }
                }
              } else {
                // Invalid character - not valid HTML tag
                valid_attributes = false;
              }
            }

            if (valid_attributes && search_pos < trimmed.size() &&
                trimmed[search_pos] == '>') {
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
          }  // End of else for opening tags
        }
      }
    }

    if (block_type == 0) [[unlikely]]
      return std::nullopt;

    // Collect HTML block content
    std::pmr::string content;

    while (!AtEnd()) {
      std::string_view html_line = CurrentLine();
      content += html_line;
      content += '\n';

      // Check for end condition
      if (block_type <= 5) {
        std::pmr::string lower_line;
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

    return HtmlBlock{.content = std::move(content), .block_type = block_type};
  }

  std::optional<BlockQuote> TryParseBlockQuote() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) [[unlikely]]
      return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty() || trimmed[0] != '>') [[unlikely]]
      return std::nullopt;

    // Collect block quote lines
    std::pmr::vector<std::pmr::string> quote_lines;

    // Track state for lazy continuation rules
    bool last_line_was_blank = false;
    bool in_fenced_code = false;
    char fence_char = 0;
    size_t fence_length = 0;
    bool in_indented_code = false;
    bool in_paragraph = false;

    while (!AtEnd()) {
      std::string_view bq_line = CurrentLine();
      auto bq_trimmed = detail::TrimLeft(bq_line);

      if (bq_trimmed.starts_with(">")) {
        // Remove > and optional space with proper tab handling
        std::pmr::string bq_content = detail::RemoveBlockQuotePrefix(bq_line);
        quote_lines.push_back(bq_content);

        // Track if this line is blank inside the blockquote
        auto inner_trimmed = detail::TrimLeft(bq_content);
        last_line_was_blank = inner_trimmed.empty();
        int inner_indent = detail::CountIndent(bq_content);

        // Track block types for lazy continuation rules
        if (!in_fenced_code) {
          if (inner_indent >= 4 && !inner_trimmed.empty()) {
            // Could be indented code block (if not continuing paragraph)
            if (!in_paragraph) {
              in_indented_code = true;
              in_paragraph = false;
            }
          } else if (!inner_trimmed.empty()) {
            // Non-indented, non-blank line starts/continues paragraph
            in_indented_code = false;
            in_paragraph = true;
          }
        }

        if (last_line_was_blank) {
          in_paragraph = false;
          in_indented_code = false;
        }

        // Track fenced code blocks inside the blockquote
        if (!in_fenced_code && !inner_trimmed.empty() &&
            (inner_trimmed[0] == '`' || inner_trimmed[0] == '~')) {
          char potential_fence = inner_trimmed[0];
          size_t potential_len = 0;
          while (potential_len < inner_trimmed.size() &&
                 inner_trimmed[potential_len] == potential_fence) {
            ++potential_len;
          }
          if (potential_len >= 3) {
            if (potential_fence == '~' ||
                inner_trimmed.substr(potential_len).find('`') ==
                    std::string_view::npos) {
              in_fenced_code = true;
              fence_char = potential_fence;
              fence_length = potential_len;
            }
          }
        } else if (in_fenced_code && !inner_trimmed.empty() &&
                   inner_trimmed[0] == fence_char) {
          size_t close_len = 0;
          while (close_len < inner_trimmed.size() &&
                 inner_trimmed[close_len] == fence_char) {
            ++close_len;
          }
          if (close_len >= fence_length) {
            bool is_close = true;
            for (size_t j = close_len; j < inner_trimmed.size(); ++j) {
              if (inner_trimmed[j] != ' ' && inner_trimmed[j] != '\t') {
                is_close = false;
                break;
              }
            }
            if (is_close) in_fenced_code = false;
          }
        }

        Advance();
      } else if (!bq_trimmed.empty() && !quote_lines.empty()) {
        // Lazy continuation - only for paragraphs, not after blank lines,
        // not inside fenced code blocks
        bool is_continuation = true;
        int bq_indent = detail::CountIndent(bq_line);

        // No lazy continuation after blank line in blockquote
        if (last_line_was_blank) {
          is_continuation = false;
        }

        // No lazy continuation inside fenced code block or indented code block
        if (in_fenced_code || in_indented_code) {
          is_continuation = false;
        }

        // Lazy continuation only continues paragraphs
        if (!in_paragraph) {
          is_continuation = false;
        }

        // Check for block-level interrupts - but only if not indented 4+ spaces
        // (4+ space indented lines can't start block-level elements, so they're
        // always lazy continuation if other conditions are met)
        if (is_continuation && bq_indent < 4) {
          if (bq_trimmed.starts_with("#") || bq_trimmed.starts_with("```") ||
              bq_trimmed.starts_with("~~~") || bq_trimmed.starts_with("---") ||
              bq_trimmed.starts_with("***") || bq_trimmed.starts_with("___") ||
              bq_trimmed.starts_with("- ") || bq_trimmed.starts_with("* ") ||
              bq_trimmed.starts_with("+ ") || bq_trimmed.starts_with(">") ||
              (bq_trimmed.size() >= 2 &&
               std::isdigit(static_cast<unsigned char>(bq_trimmed[0])))) {
            is_continuation = false;
          }
        }

        if (is_continuation) {
          // Mark lazy continuation with special prefix to prevent setext
          // heading (but don't add if already marked)
          if (!bq_trimmed.empty() && bq_trimmed[0] == '\x01') {
            quote_lines.push_back(std::pmr::string(bq_trimmed));
          } else {
            quote_lines.push_back(std::pmr::string(1, '\x01') +
                                  std::pmr::string(bq_trimmed));
          }
          last_line_was_blank = false;
          // Lazy continuation continues the paragraph
          in_paragraph = true;
          in_indented_code = false;
          Advance();
        } else {
          break;
        }
      } else {
        break;
      }
    }

    if (quote_lines.empty()) [[unlikely]]
      return std::nullopt;

    // Parse the content of the block quote
    std::pmr::string quote_content;
    for (const auto& ql : quote_lines) {
      quote_content += ql;
      quote_content += '\n';
    }

    BlockParser nested_parser;
    Document nested_doc = nested_parser.Parse(quote_content);

    // Promote link references from nested document to parent
    if (parent_link_refs_) {
      for (const auto& [label, dest_title] : nested_doc.link_references) {
        if (parent_link_refs_->find(label) == parent_link_refs_->end()) {
          parent_link_refs_->insert({label, dest_title});
        }
      }
    }

    // Transfer nested doc to parent's pools and get IDs
    BlockQuote bq;
    bq.children = TransferFromNestedDoc(nested_doc);
    return bq;
  }

  std::optional<List> TryParseList() {
    std::string_view line = CurrentLine();
    int indent = detail::CountIndent(line);
    if (indent >= 4) [[unlikely]]
      return std::nullopt;

    auto trimmed = detail::TrimLeft(line);
    if (trimmed.empty()) [[unlikely]]
      return std::nullopt;

    // Check for bullet list
    bool is_ordered = false;
    int start = 1;
    char delimiter = '.';
    char bullet_char = '-';

    if (trimmed[0] == '-' || trimmed[0] == '+' || trimmed[0] == '*') {
      bullet_char = trimmed[0];
      // Marker can be followed by space, or be at end of line (empty item)
      if (trimmed.size() == 1) {
        // Just the bullet - valid empty item
      } else if (trimmed[1] == ' ' || trimmed[1] == '\t') {
        // Bullet + space - valid
      } else {
        return std::nullopt;  // Not a list marker
      }
    } else if (std::isdigit(static_cast<unsigned char>(trimmed[0]))) {
      // Ordered list
      is_ordered = true;
      size_t num_end = 0;
      while (num_end < trimmed.size() &&
             std::isdigit(static_cast<unsigned char>(trimmed[num_end]))) {
        ++num_end;
      }
      if (num_end == 0 || num_end > 9) [[unlikely]]
        return std::nullopt;
      if (num_end >= trimmed.size()) [[unlikely]]
        return std::nullopt;

      char delim = trimmed[num_end];
      if (delim != '.' && delim != ')') [[unlikely]]
        return std::nullopt;

      start = std::stoi(std::string(trimmed.substr(0, num_end)));
      delimiter = delim;

      // Marker can be followed by space, or be at end of line (empty item)
      if (num_end + 1 >= trimmed.size()) {
        // Just number + delimiter - valid empty item
      } else if (trimmed[num_end + 1] == ' ' || trimmed[num_end + 1] == '\t') {
        // Number + delimiter + space - valid
      } else {
        return std::nullopt;  // Not a list marker
      }
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
            item_trimmed[0] == bullet_char) {
          // Match if: just the bullet, or bullet followed by space/tab
          if (item_trimmed.size() == 1 || item_trimmed[1] == ' ' ||
              item_trimmed[1] == '\t') {
            is_new_item = true;
          }
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
        std::pmr::vector<std::pmr::string> item_lines;

        // Get first line content with proper tab handling
        // Expand tabs in the original line, then extract content after marker
        std::pmr::string expanded_line = detail::ExpandTabs(item_line);
        std::string_view expanded_trimmed = detail::TrimLeft(expanded_line);

        // Calculate required indent based on content start position
        // For `-    foo`, the content starts at column 5, so required_indent is
        // 5 For `-\tfoo`, the content starts at column 4, so required_indent is
        // 4
        int expanded_item_indent = detail::CountIndent(expanded_line);
        int content_start = expanded_item_indent;

        // Find where the marker ends (bullet or number+delimiter)
        size_t marker_end = 0;
        if (is_ordered) {
          while (marker_end < expanded_trimmed.size() &&
                 std::isdigit(static_cast<unsigned char>(
                     expanded_trimmed[marker_end]))) {
            ++marker_end;
          }
          marker_end++;  // +1 for delimiter
        } else {
          marker_end = 1;  // bullet
        }

        // Find where content actually starts (skip all spaces after marker)
        size_t content_pos = marker_end;
        while (content_pos < expanded_trimmed.size() &&
               (expanded_trimmed[content_pos] == ' ' ||
                expanded_trimmed[content_pos] == '\t')) {
          ++content_pos;
        }

        // Calculate skip amount for first_content extraction
        size_t skip;

        // Apply the "5-space rule": if first non-blank content is 5+ positions
        // after the marker, it becomes an indented code block. Cap skip
        // at marker + 1, so the extra spaces become code block indent.
        if (content_pos == expanded_trimmed.size()) {
          // Empty item - content start is marker + 1 space
          content_start += static_cast<int>(marker_end) + 1;
          skip = content_pos;  // Skip everything (empty)
        } else if (static_cast<int>(content_pos) >
                   static_cast<int>(marker_end) + 4) {
          // 5-space rule: content starts at marker + 1, making excess into code
          content_start += static_cast<int>(marker_end) + 1;
          skip = marker_end + 1;  // Only skip marker + 1 space
        } else {
          content_start += static_cast<int>(content_pos);
          skip = content_pos;  // Skip to actual content
        }

        std::pmr::string first_content =
            std::pmr::string(expanded_trimmed.substr(skip));
        item_lines.push_back(first_content);
        Advance();
        had_blank_line = false;

        // Collect continuation lines
        int required_indent = content_start;
        bool first_line_empty =
            first_content.empty() || detail::IsBlankLine(first_content);

        // Track fenced code blocks to ignore blank lines within them
        bool in_item_fenced_code = false;
        char item_fence_char = 0;
        size_t item_fence_length = 0;

        // Track if we've seen a nested list marker (indent 1-3)
        // This helps distinguish nested content from direct code blocks
        bool has_nested_list = false;

        // Check if first line starts a fenced code block
        auto first_trimmed = detail::TrimLeft(first_content);
        if (!first_trimmed.empty() &&
            (first_trimmed[0] == '`' || first_trimmed[0] == '~')) {
          size_t fence_len = 0;
          while (fence_len < first_trimmed.size() &&
                 first_trimmed[fence_len] == first_trimmed[0]) {
            ++fence_len;
          }
          if (fence_len >= 3) {
            in_item_fenced_code = true;
            item_fence_char = first_trimmed[0];
            item_fence_length = fence_len;
          }
        }

        while (!AtEnd()) {
          std::string_view cont_line = CurrentLine();

          if (detail::IsBlankLine(cont_line)) {
            // If the first line was empty (just the marker), a blank line
            // ends this item - we can't continue after a blank for empty items
            if (first_line_empty) {
              Advance();
              had_blank_line = true;
              break;  // End item - don't collect more lines
            }
            item_lines.push_back("");
            // Only count blank lines outside fenced code blocks
            if (!in_item_fenced_code) {
              had_blank_line = true;
            }
            Advance();
            continue;
          }

          // Expand tabs for proper indent calculation
          std::pmr::string expanded_cont = detail::ExpandTabs(cont_line);
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
                       cont_trimmed[0] == bullet_char) {
              // Match if: just the bullet, or bullet followed by space/tab
              if (cont_trimmed.size() == 1 || cont_trimmed[1] == ' ' ||
                  cont_trimmed[1] == '\t') {
                is_another_item = true;
              }
            }
          }

          if (is_another_item) {
            break;
          }

          // Check for other block-level interrupts
          if (cont_indent < required_indent) {
            // Check for different list type (different bullet or delimiter)
            // A different list type starts a new list
            // List markers at indent < 4 end the current item (sibling level)
            // But at indent >= 4, they can be lazy continuation (not siblings)
            if (cont_indent < 4) {
              if (!cont_trimmed.empty() &&
                  (cont_trimmed[0] == '-' || cont_trimmed[0] == '+' ||
                   cont_trimmed[0] == '*')) {
                // Bullet list marker
                if (cont_trimmed.size() == 1 || cont_trimmed[1] == ' ' ||
                    cont_trimmed[1] == '\t') {
                  break;  // List marker ends item
                }
              }
              if (!cont_trimmed.empty() &&
                  std::isdigit(static_cast<unsigned char>(cont_trimmed[0]))) {
                size_t ne = 0;
                while (ne < cont_trimmed.size() &&
                       std::isdigit(
                           static_cast<unsigned char>(cont_trimmed[ne]))) {
                  ++ne;
                }
                if (ne > 0 && ne < cont_trimmed.size() &&
                    (cont_trimmed[ne] == '.' || cont_trimmed[ne] == ')')) {
                  break;  // List marker ends item
                }
              }
            }

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
              // Lazy continuation - mark with \x01 to prevent block parsing
              // (but don't add if already marked)
              if (!cont_trimmed.empty() && cont_trimmed[0] == '\x01') {
                item_lines.push_back(std::pmr::string(cont_trimmed));
              } else {
                item_lines.push_back(std::pmr::string(1, '\x01') +
                                     std::pmr::string(cont_trimmed));
              }
              Advance();
            } else {
              break;
            }
          } else {
            // Properly indented continuation - remove required_indent spaces
            std::pmr::string dedented =
                detail::RemoveIndent(expanded_cont, required_indent);

            // Track fenced code blocks
            auto dedented_trimmed = detail::TrimLeft(dedented);
            int dedented_indent = detail::CountIndent(dedented);
            if (!in_item_fenced_code && dedented_indent < 4 &&
                !dedented_trimmed.empty() &&
                (dedented_trimmed[0] == '`' || dedented_trimmed[0] == '~')) {
              size_t fence_len = 0;
              while (fence_len < dedented_trimmed.size() &&
                     dedented_trimmed[fence_len] == dedented_trimmed[0]) {
                ++fence_len;
              }
              if (fence_len >= 3) {
                in_item_fenced_code = true;
                item_fence_char = dedented_trimmed[0];
                item_fence_length = fence_len;
              }
            } else if (in_item_fenced_code && !dedented_trimmed.empty() &&
                       dedented_trimmed[0] == item_fence_char) {
              size_t close_len = 0;
              while (close_len < dedented_trimmed.size() &&
                     dedented_trimmed[close_len] == item_fence_char) {
                ++close_len;
              }
              if (close_len >= item_fence_length) {
                bool is_close = true;
                for (size_t j = close_len; j < dedented_trimmed.size(); ++j) {
                  if (dedented_trimmed[j] != ' ' &&
                      dedented_trimmed[j] != '\t') {
                    is_close = false;
                    break;
                  }
                }
                if (is_close) in_item_fenced_code = false;
              }
            }

            // Check for nested list markers (indent 0-3 with list marker)
            if (!has_nested_list && dedented_indent >= 0 &&
                dedented_indent < 4 && !dedented_trimmed.empty()) {
              char fc = dedented_trimmed[0];
              if (fc == '-' || fc == '+' || fc == '*') {
                if (dedented_trimmed.size() == 1 ||
                    dedented_trimmed[1] == ' ' || dedented_trimmed[1] == '\t') {
                  has_nested_list = true;
                }
              } else if (std::isdigit(static_cast<unsigned char>(fc))) {
                size_t ne = 0;
                while (ne < dedented_trimmed.size() &&
                       std::isdigit(
                           static_cast<unsigned char>(dedented_trimmed[ne]))) {
                  ++ne;
                }
                if (ne > 0 && ne < dedented_trimmed.size() &&
                    (dedented_trimmed[ne] == '.' ||
                     dedented_trimmed[ne] == ')')) {
                  has_nested_list = true;
                }
              }
            }

            // If there was a blank line before this content, list becomes loose
            // ONLY if the content is at the item's direct level:
            // - indent 0: direct paragraph
            // - indent >= 4 AND no nested list: direct indented code block
            if (had_blank_line) {
              if (dedented_indent == 0 ||
                  (dedented_indent >= 4 && !has_nested_list)) {
                list.is_tight = false;
              }
            }
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
        std::pmr::string item_content;
        for (const auto& il : item_lines) {
          item_content += il;
          item_content += '\n';
        }

        if (!item_content.empty()) {
          BlockParser item_parser;
          Document item_doc = item_parser.Parse(item_content);
          // Transfer nested doc to parent's pools and get IDs
          item.children = TransferFromNestedDoc(item_doc);

          // Promote link references from nested document to parent
          if (parent_link_refs_) {
            for (const auto& [label, dest_title] : item_doc.link_references) {
              if (parent_link_refs_->find(label) == parent_link_refs_->end()) {
                parent_link_refs_->insert({label, dest_title});
              }
            }
          }
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
    if (indent < 4) [[unlikely]]
      return std::nullopt;

    std::pmr::string content;

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

    if (content.empty()) [[unlikely]]
      return std::nullopt;

    return CodeBlock{.content = std::move(content), .is_fenced = false};
  }

  BlockNode ParseParagraph() {
    std::pmr::vector<std::pmr::string> para_lines;

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

              std::pmr::string heading_content;
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
      // Skip if line is marked with \x01 (lazy continuation)
      if (indent < 4 && !line.starts_with("\x01")) {
        // ATX heading: must be #, ##, etc. followed by space or end of line
        if (trimmed.starts_with("#")) {
          size_t hash_count = 0;
          while (hash_count < trimmed.size() && trimmed[hash_count] == '#') {
            ++hash_count;
          }
          if (hash_count <= 6 &&
              (hash_count >= trimmed.size() || trimmed[hash_count] == ' ' ||
               trimmed[hash_count] == '\t')) {
            break;
          }
        }
        if (trimmed.starts_with(">")) {
          break;
        }
        // Fenced code block: must be at least 3 fence chars, and for backticks,
        // the info string cannot contain backticks
        if (trimmed.size() >= 3 && (trimmed[0] == '`' || trimmed[0] == '~')) {
          char fence_char = trimmed[0];
          size_t fence_len = 0;
          while (fence_len < trimmed.size() &&
                 trimmed[fence_len] == fence_char) {
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
              num_end + 1 < trimmed.size() &&  // Must have content after marker
              (trimmed[num_end + 1] == ' ' || trimmed[num_end + 1] == '\t')) {
            // Only interrupt if starts with 1
            if (trimmed.substr(0, num_end) == "1") {
              break;
            }
          }
        }

        // Check for HTML block (types 1-6 can interrupt paragraphs)
        if (trimmed.starts_with("<")) {
          // Type 1: script, pre, style, textarea
          static constexpr std::array type1_tags = {
              std::string_view("<script"), std::string_view("<pre"), std::string_view("<style"), std::string_view("<textarea")};
          for (auto tag : type1_tags) {
            if (detail::StartsWithInsensitive(trimmed, tag) &&
                (trimmed.size() == tag.size() || trimmed[tag.size()] == ' ' ||
                 trimmed[tag.size()] == '>' || trimmed[tag.size()] == '\t')) {
              goto html_interrupt;
            }
          }
          // Type 2-5
          if (trimmed.starts_with("<!--") || trimmed.starts_with("<?") ||
              trimmed.starts_with("<![CDATA[")) {
            goto html_interrupt;
          }
          if (detail::StartsWithInsensitive(trimmed, "<!doctype")) {
            goto html_interrupt;
          }
          // Type 6: block-level tags
          static constexpr std::array type6_tags = {
              std::string_view("address"),    std::string_view("article"),  std::string_view("aside"),   std::string_view("base"),     std::string_view("basefont"),
              std::string_view("blockquote"), std::string_view("body"),     std::string_view("caption"), std::string_view("center"),   std::string_view("col"),
              std::string_view("colgroup"),   std::string_view("dd"),       std::string_view("details"), std::string_view("dialog"),   std::string_view("dir"),
              std::string_view("div"),        std::string_view("dl"),       std::string_view("dt"),      std::string_view("fieldset"), std::string_view("figcaption"),
              std::string_view("figure"),     std::string_view("footer"),   std::string_view("form"),    std::string_view("frame"),    std::string_view("frameset"),
              std::string_view("h1"),         std::string_view("h2"),       std::string_view("h3"),      std::string_view("h4"),       std::string_view("h5"),
              std::string_view("h6"),         std::string_view("head"),     std::string_view("header"),  std::string_view("hr"),       std::string_view("html"),
              std::string_view("iframe"),     std::string_view("legend"),   std::string_view("li"),      std::string_view("link"),     std::string_view("main"),
              std::string_view("menu"),       std::string_view("menuitem"), std::string_view("nav"),     std::string_view("noframes"), std::string_view("ol"),
              std::string_view("optgroup"),   std::string_view("option"),   std::string_view("p"),       std::string_view("param"),    std::string_view("search"),
              std::string_view("section"),    std::string_view("summary"),  std::string_view("table"),   std::string_view("tbody"),    std::string_view("td"),
              std::string_view("tfoot"),      std::string_view("th"),       std::string_view("thead"),   std::string_view("title"),    std::string_view("tr"),
              std::string_view("track"),      std::string_view("ul")};
          bool is_closing = (trimmed.size() >= 2 && trimmed[1] == '/');
          size_t tag_start = is_closing ? 2 : 1;
          size_t tag_end = tag_start;
          while (tag_end < trimmed.size() &&
                 (std::isalnum(static_cast<unsigned char>(trimmed[tag_end])) ||
                  trimmed[tag_end] == '-')) {
            ++tag_end;
          }
          if (tag_end > tag_start) {
            std::string_view tag_name_sv = trimmed.substr(tag_start, tag_end - tag_start);
            for (auto t : type6_tags) {
              if (detail::StartsWithInsensitive(tag_name_sv, t) &&
                  tag_name_sv.size() == t.size()) {
                if (tag_end >= trimmed.size() || trimmed[tag_end] == ' ' ||
                    trimmed[tag_end] == '>' || trimmed[tag_end] == '\t' ||
                    trimmed[tag_end] == '/') {
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
        para_lines.push_back(std::pmr::string(trimmed.substr(1)));
      } else {
        para_lines.push_back(std::pmr::string(trimmed));
      }
      Advance();
    }

    Paragraph para;
    std::pmr::string para_content;
    for (size_t j = 0; j < para_lines.size(); ++j) {
      if (j > 0) para_content += '\n';
      para_content += para_lines[j];
    }
    para.raw_content = para_content;
    return para;
  }

  void ParseInlines(std::pmr::vector<BlockNode>& blocks, InlineParser& parser) {
    for (auto& block : blocks) {
      std::visit(
          [&parser, this](auto&& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Paragraph>) {
              // Store raw_content in string_storage so string_views survive
              // moves (SSO strings have data inside the object, which becomes
              // invalid after move)
              doc_->string_storage.push_back(std::move(node.raw_content));
              std::string_view stable_content = doc_->string_storage.back();
              node.children = parser.Parse(stable_content);
            } else if constexpr (std::is_same_v<T, Heading>) {
              // Store raw_content in string_storage so string_views survive
              // moves
              doc_->string_storage.push_back(std::move(node.raw_content));
              std::string_view stable_content = doc_->string_storage.back();
              node.children = parser.Parse(stable_content);
            }
            // BlockQuote and List children are already parsed by their nested
            // BlockParser - do NOT recursively parse them again
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
  std::pmr::string Render(const Document& doc) {
    doc_ = &doc;
    std::pmr::string result;
    // Pre-allocate based on block count heuristic (~100 chars per block)
    result.reserve(doc.children.size() * 100 + 256);
    RenderBlocks(doc.children, result, false);
    return result;
  }

 private:
  const Document* doc_ = nullptr;
  void RenderBlocks(const std::pmr::vector<BlockNode>& blocks,
                    std::pmr::string& out, bool in_tight_list) {
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

  void RenderParagraph(const Paragraph& para, std::pmr::string& out,
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

  void RenderHeading(const Heading& heading, std::pmr::string& out) {
    // Precomputed heading tags for levels 1-6 (avoids std::to_string)
    static constexpr const char* kOpenTags[] = {"",     "<h1>", "<h2>", "<h3>",
                                                "<h4>", "<h5>", "<h6>"};
    static constexpr const char* kCloseTags[] = {
        "", "</h1>\n", "</h2>\n", "</h3>\n", "</h4>\n", "</h5>\n", "</h6>\n"};
    int level = heading.level;
    if (level >= 1 && level <= 6) {
      out += kOpenTags[level];
      RenderInlines(heading.children, out);
      out += kCloseTags[level];
    }
  }

  void RenderCodeBlock(const CodeBlock& block, std::pmr::string& out) {
    out += "<pre><code";
    if (!block.info_string.empty()) {
      // Extract language (first word of info string) using find
      size_t end = block.info_string.find_first_of(" \t");
      std::string_view lang =
          (end == std::string::npos)
              ? std::string_view(block.info_string)
              : std::string_view(block.info_string).substr(0, end);
      if (!lang.empty()) {
        out += " class=\"language-";
        out += detail::EscapeHtml(lang);
        out += "\"";
      }
    }
    out += ">";
    out += detail::EscapeHtml(block.content);
    out += "</code></pre>\n";
  }

  void RenderBlockQuote(const BlockQuote& bq, std::pmr::string& out) {
    out += "<blockquote>\n";
    RenderBlockIds(bq.children, out, false);
    out += "</blockquote>\n";
  }

  // Render blocks from a vector of BlockNodeIds
  void RenderBlockIds(const std::pmr::vector<BlockNodeId>& block_ids,
                      std::pmr::string& out, bool in_tight_list) {
    for (BlockNodeId id : block_ids) {
      const auto& block = doc_->block_nodes[id];
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
          block);
    }
  }

  void RenderList(const List& list, std::pmr::string& out) {
    if (list.is_ordered) {
      if (list.start == 1) {
        out += "<ol>\n";
      } else {
        out += "<ol start=\"" + std::pmr::string(std::to_string(list.start)) +
               "\">\n";
      }
    } else {
      out += "<ul>\n";
    }

    for (const auto& item : list.items) {
      out += "<li>";

      if (list.is_tight && item.children.size() == 1) {
        // Look up the first block by ID
        const auto& first_block = doc_->block_nodes[item.children[0]];
        if (std::holds_alternative<Paragraph>(first_block)) {
          // Tight list with single paragraph - render without <p> tags
          const auto& para = std::get<Paragraph>(first_block);
          RenderInlines(para.children, out);
        } else {
          out += '\n';
          RenderBlockIds(item.children, out, true);
        }
      } else if (list.is_tight) {
        // Tight list with multiple blocks
        out += '\n';
        RenderBlockIds(item.children, out, true);
      } else {
        // Loose list
        out += '\n';
        RenderBlockIds(item.children, out, false);
      }

      out += "</li>\n";
    }

    if (list.is_ordered) {
      out += "</ol>\n";
    } else {
      out += "</ul>\n";
    }
  }

  void RenderInlines(const std::pmr::vector<InlineNodeId>& node_ids,
                     std::pmr::string& out) {
    for (InlineNodeId id : node_ids) {
      const auto& node = doc_->inline_nodes[id];
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
  // std::pmr::set_default_resource(&g_arena);
  BlockParser parser;
  return parser.Parse(input);
}

// Render a document AST to HTML
inline std::pmr::string RenderHtml(const Document& doc) {
  // std::pmr::set_default_resource(&g_arena);
  HtmlRenderer renderer;
  return renderer.Render(doc);
}

// Convenience function: parse Markdown and render to HTML
inline std::pmr::string MarkdownToHtml(std::string_view input) {
  std::pmr::set_default_resource(&g_arena);
  return RenderHtml(Parse(input));
}

// Debug: print AST structure
inline std::pmr::string DebugAst(const Document& doc, int indent = 0) {
  std::pmr::string result;
  std::pmr::string prefix(indent * 2, ' ');

  result += prefix + "Document\n";

  std::function<void(const std::pmr::vector<BlockNode>&, int)> print_blocks;
  std::function<void(const std::pmr::vector<BlockNodeId>&, int)>
      print_block_ids;
  std::function<void(const std::pmr::vector<InlineNodeId>&, int)> print_inlines;

  print_inlines = [&](const std::pmr::vector<InlineNodeId>& node_ids, int ind) {
    std::pmr::string p(ind * 2, ' ');
    for (InlineNodeId id : node_ids) {
      const auto& node = doc.inline_nodes[id];
      std::visit(
          [&](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, Text>) {
              result += p;
              result += "Text: \"";
              result += n.content;
              result += "\"\n";
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
              result += p;
              result += "HtmlInline: ";
              result += n.content;
              result += "\n";
            }
          },
          node);
    }
  };

  print_block_ids = [&](const std::pmr::vector<BlockNodeId>& block_ids,
                        int ind) {
    std::pmr::string p(ind * 2, ' ');
    for (BlockNodeId id : block_ids) {
      const auto& block = doc.block_nodes[id];
      std::visit(
          [&](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, Paragraph>) {
              result += p + "Paragraph\n";
              print_inlines(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, Heading>) {
              result += p + "Heading (level " +
                        std::pmr::string(std::to_string(n.level)) + ")\n";
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
              result += p + "HtmlBlock (type " +
                        std::pmr::string(std::to_string(n.block_type)) + ")\n";
            } else if constexpr (std::is_same_v<T, BlockQuote>) {
              result += p + "BlockQuote\n";
              print_block_ids(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, List>) {
              result += p + "List (" +
                        (n.is_ordered ? "ordered" : "unordered") +
                        (n.is_tight ? ", tight" : ", loose") + ")\n";
              for (const auto& item : n.items) {
                result += p + "  ListItem\n";
                print_block_ids(item.children, ind + 2);
              }
            } else if constexpr (std::is_same_v<T, ListItem>) {
              result += p + "ListItem\n";
              print_block_ids(n.children, ind + 1);
            }
          },
          block);
    }
  };

  print_blocks = [&](const std::pmr::vector<BlockNode>& blocks, int ind) {
    std::pmr::string p(ind * 2, ' ');
    for (const auto& block : blocks) {
      std::visit(
          [&](auto&& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, Paragraph>) {
              result += p + "Paragraph\n";
              print_inlines(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, Heading>) {
              result += p + "Heading (level " +
                        std::pmr::string(std::to_string(n.level)) + ")\n";
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
              result += p + "HtmlBlock (type " +
                        std::pmr::string(std::to_string(n.block_type)) + ")\n";
            } else if constexpr (std::is_same_v<T, BlockQuote>) {
              result += p + "BlockQuote\n";
              print_block_ids(n.children, ind + 1);
            } else if constexpr (std::is_same_v<T, List>) {
              result += p + "List (" +
                        (n.is_ordered ? "ordered" : "unordered") +
                        (n.is_tight ? ", tight" : ", loose") + ")\n";
              for (const auto& item : n.items) {
                result += p + "  ListItem\n";
                print_block_ids(item.children, ind + 2);
              }
            } else if constexpr (std::is_same_v<T, ListItem>) {
              result += p + "ListItem\n";
              print_block_ids(n.children, ind + 1);
            }
          },
          block);
    }
  };

  print_blocks(doc.children, indent + 1);
  return result;
}

}  // namespace markus

#endif  // MARKUS_H_
