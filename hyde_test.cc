// Copyright 2024 Hyde Authors
// SPDX-License-Identifier: MIT

#include "hyde.h"

#include <gtest/gtest.h>

#include <string>

namespace hyde {
namespace {

// =============================================================================
// Paragraph Tests
// =============================================================================

TEST(ParagraphTest, SimpleParagraph) {
  std::string html = MarkdownToHtml("Hello, World!");
  EXPECT_EQ("<p>Hello, World!</p>\n", html);
}

TEST(ParagraphTest, MultipleParagraphs) {
  std::string html = MarkdownToHtml("First paragraph.\n\nSecond paragraph.");
  EXPECT_EQ("<p>First paragraph.</p>\n<p>Second paragraph.</p>\n", html);
}

TEST(ParagraphTest, ParagraphWithSoftBreak) {
  std::string html = MarkdownToHtml("Line one\nLine two");
  EXPECT_EQ("<p>Line one\nLine two</p>\n", html);
}

// =============================================================================
// Heading Tests
// =============================================================================

TEST(HeadingTest, AtxHeadings) {
  EXPECT_EQ("<h1>Heading 1</h1>\n", MarkdownToHtml("# Heading 1"));
  EXPECT_EQ("<h2>Heading 2</h2>\n", MarkdownToHtml("## Heading 2"));
  EXPECT_EQ("<h3>Heading 3</h3>\n", MarkdownToHtml("### Heading 3"));
  EXPECT_EQ("<h4>Heading 4</h4>\n", MarkdownToHtml("#### Heading 4"));
  EXPECT_EQ("<h5>Heading 5</h5>\n", MarkdownToHtml("##### Heading 5"));
  EXPECT_EQ("<h6>Heading 6</h6>\n", MarkdownToHtml("###### Heading 6"));
}

TEST(HeadingTest, AtxHeadingWithTrailingHashes) {
  EXPECT_EQ("<h1>Heading</h1>\n", MarkdownToHtml("# Heading #"));
  EXPECT_EQ("<h2>Heading</h2>\n", MarkdownToHtml("## Heading ##"));
}

TEST(HeadingTest, SetextHeadingLevel1) {
  std::string html = MarkdownToHtml("Heading\n=======");
  EXPECT_EQ("<h1>Heading</h1>\n", html);
}

TEST(HeadingTest, SetextHeadingLevel2) {
  std::string html = MarkdownToHtml("Heading\n-------");
  EXPECT_EQ("<h2>Heading</h2>\n", html);
}

TEST(HeadingTest, EmptyAtxHeading) {
  EXPECT_EQ("<h1></h1>\n", MarkdownToHtml("#"));
  EXPECT_EQ("<h2></h2>\n", MarkdownToHtml("##"));
}

// =============================================================================
// Thematic Break Tests
// =============================================================================

TEST(ThematicBreakTest, Dashes) {
  EXPECT_EQ("<hr />\n", MarkdownToHtml("---"));
  EXPECT_EQ("<hr />\n", MarkdownToHtml("----"));
  EXPECT_EQ("<hr />\n", MarkdownToHtml("- - -"));
}

TEST(ThematicBreakTest, Asterisks) {
  EXPECT_EQ("<hr />\n", MarkdownToHtml("***"));
  EXPECT_EQ("<hr />\n", MarkdownToHtml("* * *"));
}

TEST(ThematicBreakTest, Underscores) {
  EXPECT_EQ("<hr />\n", MarkdownToHtml("___"));
  EXPECT_EQ("<hr />\n", MarkdownToHtml("_ _ _"));
}

// =============================================================================
// Code Block Tests
// =============================================================================

TEST(CodeBlockTest, IndentedCodeBlock) {
  std::string html = MarkdownToHtml("    code line 1\n    code line 2");
  EXPECT_EQ("<pre><code>code line 1\ncode line 2\n</code></pre>\n", html);
}

TEST(CodeBlockTest, FencedCodeBlock) {
  std::string html = MarkdownToHtml("```\ncode here\n```");
  EXPECT_EQ("<pre><code>code here\n</code></pre>\n", html);
}

TEST(CodeBlockTest, FencedCodeBlockWithLanguage) {
  std::string html = MarkdownToHtml("```cpp\nint main() {}\n```");
  EXPECT_EQ("<pre><code class=\"language-cpp\">int main() {}\n</code></pre>\n",
            html);
}

TEST(CodeBlockTest, FencedCodeBlockTildes) {
  std::string html = MarkdownToHtml("~~~\ncode\n~~~");
  EXPECT_EQ("<pre><code>code\n</code></pre>\n", html);
}

TEST(CodeBlockTest, FencedCodeBlockNoCloser) {
  std::string html = MarkdownToHtml("```\ncode without closing");
  EXPECT_EQ("<pre><code>code without closing\n</code></pre>\n", html);
}

// =============================================================================
// Block Quote Tests
// =============================================================================

TEST(BlockQuoteTest, SimpleBlockQuote) {
  std::string html = MarkdownToHtml("> Quote");
  EXPECT_EQ("<blockquote>\n<p>Quote</p>\n</blockquote>\n", html);
}

TEST(BlockQuoteTest, MultiLineBlockQuote) {
  std::string html = MarkdownToHtml("> Line 1\n> Line 2");
  EXPECT_EQ("<blockquote>\n<p>Line 1\nLine 2</p>\n</blockquote>\n", html);
}

TEST(BlockQuoteTest, NestedBlockQuote) {
  std::string html = MarkdownToHtml("> > Nested");
  EXPECT_EQ(
      "<blockquote>\n<blockquote>\n<p>Nested</p>\n</blockquote>\n</blockquote>\n",
      html);
}

TEST(BlockQuoteTest, BlockQuoteWithMultipleParagraphs) {
  std::string html = MarkdownToHtml("> Para 1\n>\n> Para 2");
  EXPECT_EQ("<blockquote>\n<p>Para 1</p>\n<p>Para 2</p>\n</blockquote>\n", html);
}

// =============================================================================
// List Tests
// =============================================================================

TEST(ListTest, UnorderedListDash) {
  std::string html = MarkdownToHtml("- Item 1\n- Item 2");
  EXPECT_EQ("<ul>\n<li>Item 1</li>\n<li>Item 2</li>\n</ul>\n", html);
}

TEST(ListTest, UnorderedListAsterisk) {
  std::string html = MarkdownToHtml("* Item 1\n* Item 2");
  EXPECT_EQ("<ul>\n<li>Item 1</li>\n<li>Item 2</li>\n</ul>\n", html);
}

TEST(ListTest, UnorderedListPlus) {
  std::string html = MarkdownToHtml("+ Item 1\n+ Item 2");
  EXPECT_EQ("<ul>\n<li>Item 1</li>\n<li>Item 2</li>\n</ul>\n", html);
}

TEST(ListTest, OrderedList) {
  std::string html = MarkdownToHtml("1. First\n2. Second\n3. Third");
  EXPECT_EQ(
      "<ol>\n<li>First</li>\n<li>Second</li>\n<li>Third</li>\n</ol>\n", html);
}

TEST(ListTest, OrderedListStartNumber) {
  std::string html = MarkdownToHtml("5. Fifth\n6. Sixth");
  EXPECT_EQ("<ol start=\"5\">\n<li>Fifth</li>\n<li>Sixth</li>\n</ol>\n", html);
}

TEST(ListTest, LooseList) {
  std::string html = MarkdownToHtml("- Item 1\n\n- Item 2");
  EXPECT_EQ(
      "<ul>\n<li>\n<p>Item 1</p>\n</li>\n<li>\n<p>Item 2</p>\n</li>\n</ul>\n",
      html);
}

TEST(ListTest, OrderedListParen) {
  std::string html = MarkdownToHtml("1) First\n2) Second");
  EXPECT_EQ("<ol>\n<li>First</li>\n<li>Second</li>\n</ol>\n", html);
}

// =============================================================================
// Inline Tests - Emphasis
// =============================================================================

TEST(EmphasisTest, AsteriskEmphasis) {
  EXPECT_EQ("<p><em>emphasis</em></p>\n", MarkdownToHtml("*emphasis*"));
}

TEST(EmphasisTest, UnderscoreEmphasis) {
  EXPECT_EQ("<p><em>emphasis</em></p>\n", MarkdownToHtml("_emphasis_"));
}

TEST(EmphasisTest, AsteriskStrong) {
  EXPECT_EQ("<p><strong>strong</strong></p>\n", MarkdownToHtml("**strong**"));
}

TEST(EmphasisTest, UnderscoreStrong) {
  EXPECT_EQ("<p><strong>strong</strong></p>\n", MarkdownToHtml("__strong__"));
}

TEST(EmphasisTest, StrongWithEmphasis) {
  // CommonMark spec: either order is valid, our parser produces em(strong())
  EXPECT_EQ("<p><em><strong>both</strong></em></p>\n",
            MarkdownToHtml("***both***"));
}

// =============================================================================
// Inline Tests - Code Span
// =============================================================================

TEST(CodeSpanTest, Simple) {
  EXPECT_EQ("<p><code>code</code></p>\n", MarkdownToHtml("`code`"));
}

TEST(CodeSpanTest, WithBackticks) {
  EXPECT_EQ("<p><code>code with ` backtick</code></p>\n",
            MarkdownToHtml("`` code with ` backtick ``"));
}

TEST(CodeSpanTest, PreservesSpaces) {
  EXPECT_EQ("<p><code>a  b</code></p>\n", MarkdownToHtml("`a  b`"));
}

// =============================================================================
// Inline Tests - Links
// =============================================================================

TEST(LinkTest, InlineLink) {
  std::string html = MarkdownToHtml("[link](http://example.com)");
  EXPECT_EQ("<p><a href=\"http://example.com\">link</a></p>\n", html);
}

TEST(LinkTest, InlineLinkWithTitle) {
  std::string html =
      MarkdownToHtml("[link](http://example.com \"Title\")");
  EXPECT_EQ(
      "<p><a href=\"http://example.com\" title=\"Title\">link</a></p>\n", html);
}

TEST(LinkTest, InlineLinkAngleBrackets) {
  std::string html = MarkdownToHtml("[link](<http://example.com>)");
  EXPECT_EQ("<p><a href=\"http://example.com\">link</a></p>\n", html);
}

TEST(LinkTest, SimpleLinkText) {
  std::string html = MarkdownToHtml("[simple text](url)");
  EXPECT_EQ("<p><a href=\"url\">simple text</a></p>\n", html);
}

// =============================================================================
// Inline Tests - Images
// =============================================================================

TEST(ImageTest, Simple) {
  std::string html = MarkdownToHtml("![alt text](image.png)");
  EXPECT_EQ("<p><img src=\"image.png\" alt=\"alt text\" /></p>\n", html);
}

TEST(ImageTest, WithTitle) {
  std::string html = MarkdownToHtml("![alt](image.png \"Title\")");
  EXPECT_EQ(
      "<p><img src=\"image.png\" alt=\"alt\" title=\"Title\" /></p>\n", html);
}

// =============================================================================
// Inline Tests - Autolinks
// =============================================================================

TEST(AutolinkTest, Uri) {
  // Autolink within text context
  std::string html = MarkdownToHtml("Check <http://example.com> for info");
  EXPECT_EQ(
      "<p>Check <a href=\"http://example.com\">http://example.com</a> for info</p>\n",
      html);
}

TEST(AutolinkTest, Email) {
  // Email autolink within text context
  std::string html = MarkdownToHtml("Contact <test@example.com> now");
  EXPECT_EQ(
      "<p>Contact <a href=\"mailto:test@example.com\">test@example.com</a> now</p>\n",
      html);
}

// =============================================================================
// Inline Tests - Hard Breaks
// =============================================================================

TEST(HardBreakTest, TwoSpaces) {
  std::string html = MarkdownToHtml("Line 1  \nLine 2");
  EXPECT_EQ("<p>Line 1<br />\nLine 2</p>\n", html);
}

TEST(HardBreakTest, Backslash) {
  std::string html = MarkdownToHtml("Line 1\\\nLine 2");
  EXPECT_EQ("<p>Line 1<br />\nLine 2</p>\n", html);
}

// =============================================================================
// Inline Tests - Escapes
// =============================================================================

TEST(EscapeTest, BackslashEscape) {
  EXPECT_EQ("<p>*not emphasis*</p>\n", MarkdownToHtml("\\*not emphasis\\*"));
}

TEST(EscapeTest, AllPunctuation) {
  std::string input = "\\!\\\"\\#\\$\\%\\&\\'\\(\\)\\*\\+\\,\\-\\.\\/";
  std::string html = MarkdownToHtml(input);
  EXPECT_EQ("<p>!&quot;#$%&amp;'()*+,-./</p>\n", html);
}

// =============================================================================
// HTML Escaping Tests
// =============================================================================

TEST(HtmlEscapingTest, ScriptTagInText) {
  // When < is in text, it's escaped. But bare <script> starts HTML block.
  // Test angle brackets in context that won't trigger HTML block.
  EXPECT_EQ("<p>a &lt; b &gt; c</p>\n", MarkdownToHtml("a < b > c"));
}

TEST(HtmlEscapingTest, Ampersand) {
  EXPECT_EQ("<p>Tom &amp; Jerry</p>\n", MarkdownToHtml("Tom & Jerry"));
}

TEST(HtmlEscapingTest, Quotes) {
  // Quotes are escaped since we escape for safety in all contexts
  std::string html = MarkdownToHtml("He said \"hello\"");
  EXPECT_EQ("<p>He said &quot;hello&quot;</p>\n", html);
}

// =============================================================================
// HTML Block Tests
// =============================================================================

TEST(HtmlBlockTest, Div) {
  std::string html = MarkdownToHtml("<div>\nHello\n</div>");
  EXPECT_EQ("<div>\nHello\n</div>\n", html);
}

TEST(HtmlBlockTest, Comment) {
  std::string html = MarkdownToHtml("<!-- comment -->\n\nParagraph");
  EXPECT_EQ("<!-- comment -->\n<p>Paragraph</p>\n", html);
}

TEST(HtmlBlockTest, Script) {
  std::string html = MarkdownToHtml("<script>\nalert(1);\n</script>");
  EXPECT_EQ("<script>\nalert(1);\n</script>\n", html);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(EdgeCaseTest, EmptyInput) {
  EXPECT_EQ("", MarkdownToHtml(""));
}

TEST(EdgeCaseTest, WhitespaceOnly) {
  EXPECT_EQ("", MarkdownToHtml("   \n\n   \n"));
}

TEST(EdgeCaseTest, MixedContent) {
  std::string markdown =
      "# Heading\n\n"
      "Paragraph with *emphasis* and **strong**.\n\n"
      "- List item 1\n"
      "- List item 2\n\n"
      "> Blockquote\n\n"
      "```\ncode\n```";

  std::string html = MarkdownToHtml(markdown);

  EXPECT_TRUE(html.find("<h1>Heading</h1>") != std::string::npos);
  EXPECT_TRUE(html.find("<em>emphasis</em>") != std::string::npos);
  EXPECT_TRUE(html.find("<strong>strong</strong>") != std::string::npos);
  EXPECT_TRUE(html.find("<ul>") != std::string::npos);
  EXPECT_TRUE(html.find("<blockquote>") != std::string::npos);
  EXPECT_TRUE(html.find("<pre><code>") != std::string::npos);
}

// =============================================================================
// AST Structure Tests
// =============================================================================

TEST(AstTest, DocumentStructure) {
  Document doc = Parse("# Hello\n\nWorld");
  ASSERT_EQ(2u, doc.children.size());
  EXPECT_TRUE(std::holds_alternative<Heading>(doc.children[0]));
  EXPECT_TRUE(std::holds_alternative<Paragraph>(doc.children[1]));
}

TEST(AstTest, HeadingLevel) {
  Document doc = Parse("### Level 3");
  ASSERT_EQ(1u, doc.children.size());
  auto& heading = std::get<Heading>(doc.children[0]);
  EXPECT_EQ(3, heading.level);
}

TEST(AstTest, ListStructure) {
  Document doc = Parse("- One\n- Two\n- Three");
  ASSERT_EQ(1u, doc.children.size());
  auto& list = std::get<List>(doc.children[0]);
  EXPECT_FALSE(list.is_ordered);
  EXPECT_EQ(3u, list.items.size());
}

TEST(AstTest, OrderedListStructure) {
  Document doc = Parse("1. First\n2. Second");
  ASSERT_EQ(1u, doc.children.size());
  auto& list = std::get<List>(doc.children[0]);
  EXPECT_TRUE(list.is_ordered);
  EXPECT_EQ(1, list.start);
  EXPECT_EQ(2u, list.items.size());
}

TEST(AstTest, CodeBlockStructure) {
  Document doc = Parse("```python\nprint('hello')\n```");
  ASSERT_EQ(1u, doc.children.size());
  auto& code_block = std::get<CodeBlock>(doc.children[0]);
  EXPECT_EQ("python", code_block.info_string);
  EXPECT_TRUE(code_block.is_fenced);
}

TEST(AstTest, EmphasisStructure) {
  Document doc = Parse("*hello*");
  ASSERT_EQ(1u, doc.children.size());
  auto& para = std::get<Paragraph>(doc.children[0]);
  ASSERT_EQ(1u, para.children.size());
  EXPECT_TRUE(std::holds_alternative<Emphasis>(para.children[0]));
}

TEST(AstTest, LinkStructure) {
  Document doc = Parse("[text](url)");
  ASSERT_EQ(1u, doc.children.size());
  auto& para = std::get<Paragraph>(doc.children[0]);
  ASSERT_EQ(1u, para.children.size());
  auto& link = std::get<Link>(para.children[0]);
  EXPECT_EQ("url", link.destination);
}

TEST(AstTest, BlockQuoteStructure) {
  Document doc = Parse("> Quote text");
  ASSERT_EQ(1u, doc.children.size());
  EXPECT_TRUE(std::holds_alternative<BlockQuote>(doc.children[0]));
  auto& bq = std::get<BlockQuote>(doc.children[0]);
  ASSERT_EQ(1u, bq.children.size());
  EXPECT_TRUE(std::holds_alternative<Paragraph>(bq.children[0]));
}

// =============================================================================
// Debug AST Tests
// =============================================================================

TEST(DebugAstTest, Output) {
  Document doc = Parse("# Test");
  std::string ast = DebugAst(doc);
  EXPECT_TRUE(ast.find("Document") != std::string::npos);
  EXPECT_TRUE(ast.find("Heading") != std::string::npos);
  EXPECT_TRUE(ast.find("level 1") != std::string::npos);
}

// =============================================================================
// Link Reference Tests
// =============================================================================

TEST(LinkReferenceTest, Definition) {
  std::string markdown = "[foo]: /url \"title\"\n\n[foo]";
  Document doc = Parse(markdown);
  EXPECT_TRUE(doc.link_references.find("foo") != doc.link_references.end());
}

TEST(LinkReferenceTest, CaseInsensitive) {
  std::string markdown = "[FOO]: /url\n\n[foo]";
  Document doc = Parse(markdown);
  EXPECT_TRUE(doc.link_references.find("foo") != doc.link_references.end());
}

// =============================================================================
// Node Type Tests
// =============================================================================

TEST(NodeTypeTest, ToString) {
  EXPECT_EQ("Document", NodeTypeToString(NodeType::kDocument));
  EXPECT_EQ("Paragraph", NodeTypeToString(NodeType::kParagraph));
  EXPECT_EQ("Heading", NodeTypeToString(NodeType::kHeading));
  EXPECT_EQ("ThematicBreak", NodeTypeToString(NodeType::kThematicBreak));
  EXPECT_EQ("CodeBlock", NodeTypeToString(NodeType::kCodeBlock));
  EXPECT_EQ("HtmlBlock", NodeTypeToString(NodeType::kHtmlBlock));
  EXPECT_EQ("BlockQuote", NodeTypeToString(NodeType::kBlockQuote));
  EXPECT_EQ("List", NodeTypeToString(NodeType::kList));
  EXPECT_EQ("ListItem", NodeTypeToString(NodeType::kListItem));
  EXPECT_EQ("Text", NodeTypeToString(NodeType::kText));
  EXPECT_EQ("SoftBreak", NodeTypeToString(NodeType::kSoftBreak));
  EXPECT_EQ("HardBreak", NodeTypeToString(NodeType::kHardBreak));
  EXPECT_EQ("Code", NodeTypeToString(NodeType::kCode));
  EXPECT_EQ("Emphasis", NodeTypeToString(NodeType::kEmphasis));
  EXPECT_EQ("Strong", NodeTypeToString(NodeType::kStrong));
  EXPECT_EQ("Link", NodeTypeToString(NodeType::kLink));
  EXPECT_EQ("Image", NodeTypeToString(NodeType::kImage));
  EXPECT_EQ("HtmlInline", NodeTypeToString(NodeType::kHtmlInline));
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST(UtilityTest, EscapeHtml) {
  EXPECT_EQ("&amp;", detail::EscapeHtml("&"));
  EXPECT_EQ("&lt;", detail::EscapeHtml("<"));
  EXPECT_EQ("&gt;", detail::EscapeHtml(">"));
  EXPECT_EQ("&quot;", detail::EscapeHtml("\""));
  EXPECT_EQ("&lt;div&gt;", detail::EscapeHtml("<div>"));
}

TEST(UtilityTest, IsBlankLine) {
  EXPECT_TRUE(detail::IsBlankLine(""));
  EXPECT_TRUE(detail::IsBlankLine("   "));
  EXPECT_TRUE(detail::IsBlankLine("\t\t"));
  EXPECT_TRUE(detail::IsBlankLine("  \t  "));
  EXPECT_FALSE(detail::IsBlankLine("a"));
  EXPECT_FALSE(detail::IsBlankLine(" a "));
}

TEST(UtilityTest, CountIndent) {
  EXPECT_EQ(0, detail::CountIndent("no indent"));
  EXPECT_EQ(2, detail::CountIndent("  two spaces"));
  EXPECT_EQ(4, detail::CountIndent("    four spaces"));
  EXPECT_EQ(4, detail::CountIndent("\tone tab"));
}

TEST(UtilityTest, NormalizeLinkLabel) {
  EXPECT_EQ("foo", detail::NormalizeLinkLabel("foo"));
  EXPECT_EQ("foo", detail::NormalizeLinkLabel("FOO"));
  EXPECT_EQ("foo bar", detail::NormalizeLinkLabel("foo  bar"));
  EXPECT_EQ("foo bar", detail::NormalizeLinkLabel("  foo   bar  "));
}

}  // namespace
}  // namespace hyde
