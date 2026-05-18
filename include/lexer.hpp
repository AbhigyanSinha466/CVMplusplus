#pragma once
// =============================================================================
// lexer.hpp — CVM++ Lexer (Tokenizer)
//
// The Lexer is the FIRST stage of the pipeline:
//   Raw Source Code (std::string)  -->  [LEXER]  -->  std::vector<Token>
//
// It scans the source character by character and groups characters into
// meaningful "tokens" (the smallest meaningful units of the language).
// Example:  "let x = 10 + 2"
//           --> [LET] [IDENT:"x"] [ASSIGN] [NUMBER:10] [PLUS] [NUMBER:2]
// =============================================================================

#include <string>
#include <vector>
#include <stdexcept>

// -----------------------------------------------------------------------------
// TokenType — Every kind of token the language recognizes.
// Grouped logically: literals, keywords, operators, delimiters, special.
// -----------------------------------------------------------------------------
enum class TokenType {
    // --- Literals ---
    NUMBER,         // e.g. 42
    TRUE_LIT,       // true
    FALSE_LIT,      // false

    // --- Identifiers ---
    IDENTIFIER,     // e.g. x, myVar

    // --- Keywords ---
    LET,            // let
    IF,             // if
    ELSE,           // else
    WHILE,          // while
    PRINT,          // print
    INPUT,          // input

    // --- Arithmetic Operators ---
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /

    // --- Comparison Operators ---
    EQUAL_EQUAL,    // ==
    LESS,           // <

    // --- Assignment ---
    ASSIGN,         // =

    // --- Delimiters ---
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    SEMICOLON,      // ;

    // --- Special ---
    EOF_TOKEN       // marks end of input
};

// Human-readable names for each TokenType (used in debug output)
const char* tokenTypeName(TokenType t);

// -----------------------------------------------------------------------------
// Token — A single lexical unit produced by the Lexer.
// Carries its type, its raw text (lexeme), and its line number for errors.
// -----------------------------------------------------------------------------
struct Token {
    TokenType   type;
    std::string lexeme;  // The actual text slice from source
    int         line;    // 1-based line number for error messages

    Token(TokenType t, std::string lex, int ln)
        : type(t), lexeme(std::move(lex)), line(ln) {}
};

// -----------------------------------------------------------------------------
// Lexer — Converts a source string into a flat list of Tokens.
//
// Usage:
//   Lexer lexer(source_code);
//   std::vector<Token> tokens = lexer.tokenize();
// -----------------------------------------------------------------------------
class Lexer {
public:
    explicit Lexer(std::string source);

    // Scan the entire source and return all tokens, ending with EOF_TOKEN.
    std::vector<Token> tokenize();

private:
    std::string m_source;   // The full source text
    std::size_t m_pos;      // Current read position (index into m_source)
    int         m_line;     // Current line number (1-based)

    // --- Character-level helpers ---
    bool   isAtEnd() const;
    char   peek() const;           // Look at current char without consuming
    char   peekNext() const;       // Look one ahead without consuming
    char   advance();              // Consume and return current char

    // --- Token builders ---
    void   skipWhitespaceAndComments();
    Token  readNumber();
    Token  readIdentifierOrKeyword();
};
