#pragma once
// =============================================================================
// parser.hpp — CVM++ Recursive Descent Parser
//
// The Parser is the SECOND stage of the pipeline:
//   std::vector<Token>  -->  [PARSER]  -->  std::unique_ptr<ProgramNode> (AST)
//
// It implements a classic Recursive Descent Parser (RDP).
// Each grammar rule maps 1-to-1 to a private method.
//
// Grammar (informal BNF):
//   program     := stmt*  EOF
//   stmt        := varDecl | assignStmt | printStmt | ifStmt | whileStmt
//   varDecl     := 'let' IDENT '=' (expr | 'input') ';'
//   assignStmt  := IDENT '=' expr ';'
//   printStmt   := 'print' expr ';'
//   ifStmt      := 'if' '(' expr ')' block ( 'else' block )?
//   whileStmt   := 'while' '(' expr ')' block
//   block       := '{' stmt* '}'
//   expr        := comparison
//   comparison  := addition ( ('==' | '<') addition )*
//   addition    := term ( ('+' | '-') term )*
//   term        := factor ( ('*' | '/') factor )*
//   factor      := NUMBER | 'true' | 'false' | IDENT | '(' expr ')'
// =============================================================================

#include "lexer.hpp"
#include "ast.hpp"
#include <vector>
#include <stdexcept>

// -----------------------------------------------------------------------------
// ParseError — Thrown when the source code violates the grammar.
// -----------------------------------------------------------------------------
struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// -----------------------------------------------------------------------------
// Parser — Consumes a token list and builds the AST.
// -----------------------------------------------------------------------------
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parse the full program and return the root ProgramNode.
    NodePtr parse();

private:
    std::vector<Token> m_tokens;
    std::size_t        m_pos; // Current token index

    // --- Token-level helpers ---
    const Token& peek() const;                     // Current token (no advance)
    const Token& previous() const;                 // Last consumed token
    bool         check(TokenType t) const;
    bool         isAtEnd() const;
    const Token& advance();                        // Consume and return current
    const Token& expect(TokenType t, const char* msg); // Consume or throw

    // --- Grammar rule methods (top-down, recursive) ---
    NodePtr parseStmt();
    NodePtr parseVarDecl();
    NodePtr parseAssignOrExprStmt();
    NodePtr parsePrintStmt();
    NodePtr parseIfStmt();
    NodePtr parseWhileStmt();
    NodePtr parseBlock();

    // Expression hierarchy (lowest to highest precedence):
    NodePtr parseExpr();
    NodePtr parseComparison();
    NodePtr parseAddition();
    NodePtr parseTerm();
    NodePtr parseFactor();
};
