// =============================================================================
// parser.cpp — CVM++ Recursive Descent Parser Implementation
// =============================================================================

#include "parser.hpp"
#include <stdexcept>

// =============================================================================
// Constructor
// =============================================================================
Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens)), m_pos(0) {}

// =============================================================================
// Token-Level Helpers
// =============================================================================

const Token& Parser::peek() const {
    return m_tokens[m_pos];
}

const Token& Parser::previous() const {
    return m_tokens[m_pos - 1];
}

bool Parser::check(TokenType t) const {
    return !isAtEnd() && peek().type == t;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

// Consume and return the current token, advancing the cursor.
const Token& Parser::advance() {
    if (!isAtEnd()) ++m_pos;
    return previous();
}

// Consume the current token if it matches `t`; otherwise throw a ParseError.
const Token& Parser::expect(TokenType t, const char* msg) {
    if (check(t)) return advance();
    const Token& tok = peek();
    throw ParseError(std::string("Parse error at line ") +
                     std::to_string(tok.line) + " near '" + tok.lexeme +
                     "': " + msg);
}

// =============================================================================
// Public Entry Point
// =============================================================================

// parse() → builds the complete ProgramNode by consuming all statements
// until EOF_TOKEN.
NodePtr Parser::parse() {
    auto program = std::make_unique<ProgramNode>(1);
    while (!isAtEnd()) {
        program->stmts.push_back(parseStmt());
    }
    return program;
}

// =============================================================================
// Statement Parsers
// =============================================================================

// Dispatch to the correct statement parser based on the current token.
NodePtr Parser::parseStmt() {
    if (check(TokenType::LET))   return parseVarDecl();
    if (check(TokenType::IF))    return parseIfStmt();
    if (check(TokenType::WHILE)) return parseWhileStmt();
    if (check(TokenType::PRINT)) return parsePrintStmt();
    // Anything else: could be "x = ..." (assignment) or a bare expression
    return parseAssignOrExprStmt();
}

// --- Variable Declaration: let IDENT = (expr | input) ; ---
NodePtr Parser::parseVarDecl() {
    int line = peek().line;
    expect(TokenType::LET, "expected 'let'");
    const Token& name = expect(TokenType::IDENTIFIER, "expected variable name after 'let'");
    expect(TokenType::ASSIGN, "expected '=' after variable name");

    // Check if the RHS is the special 'input' keyword
    if (check(TokenType::INPUT)) {
        advance(); // consume 'input'
        expect(TokenType::SEMICOLON, "expected ';' after 'input'");
        return std::make_unique<VarDeclNode>(name.lexeme, nullptr, /*fromInput=*/true, line);
    }

    NodePtr init = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after variable initializer");
    return std::make_unique<VarDeclNode>(name.lexeme, std::move(init), /*fromInput=*/false, line);
}

// --- Assignment: IDENT = expr ;  (parsed after we see IDENT '=') ---
NodePtr Parser::parseAssignOrExprStmt() {
    int line = peek().line;

    // Look-ahead: if current is IDENTIFIER and next is ASSIGN → assignment
    if (check(TokenType::IDENTIFIER) && m_pos + 1 < m_tokens.size() &&
        m_tokens[m_pos + 1].type == TokenType::ASSIGN)
    {
        const Token& name = advance();  // consume IDENT
        advance();                      // consume '='
        NodePtr value = parseExpr();
        expect(TokenType::SEMICOLON, "expected ';' after assignment");
        return std::make_unique<AssignNode>(name.lexeme, std::move(value), line);
    }

    // Otherwise treat as a standalone expression statement (e.g., function calls
    // would go here — not currently in scope, but the slot is reserved).
    // For now, just parse and discard the result (prints nothing).
    NodePtr expr = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after expression");

    // Wrap in a PrintStmt variant? No — just throw a helpful error for now.
    throw ParseError("Parse error at line " + std::to_string(line) +
                     ": bare expression statement without 'print'.");
}

// --- Print Statement: print expr ; ---
NodePtr Parser::parsePrintStmt() {
    int line = peek().line;
    expect(TokenType::PRINT, "expected 'print'");
    NodePtr expr = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after print expression");
    return std::make_unique<PrintStmtNode>(std::move(expr), line);
}

// --- If Statement: if ( expr ) block [ else block ] ---
NodePtr Parser::parseIfStmt() {
    int line = peek().line;
    expect(TokenType::IF,     "expected 'if'");
    expect(TokenType::LPAREN, "expected '(' after 'if'");
    NodePtr cond = parseExpr();
    expect(TokenType::RPAREN, "expected ')' after if condition");
    NodePtr thenBlock = parseBlock();
    NodePtr elseBlock;

    if (check(TokenType::ELSE)) {
        advance(); // consume 'else'
        elseBlock = parseBlock();
    }

    return std::make_unique<IfStmtNode>(
        std::move(cond), std::move(thenBlock), std::move(elseBlock), line);
}

// --- While Statement: while ( expr ) block ---
NodePtr Parser::parseWhileStmt() {
    int line = peek().line;
    expect(TokenType::WHILE,  "expected 'while'");
    expect(TokenType::LPAREN, "expected '(' after 'while'");
    NodePtr cond = parseExpr();
    expect(TokenType::RPAREN, "expected ')' after while condition");
    NodePtr body = parseBlock();
    return std::make_unique<WhileStmtNode>(std::move(cond), std::move(body), line);
}

// --- Block: { stmt* } ---
NodePtr Parser::parseBlock() {
    int line = peek().line;
    expect(TokenType::LBRACE, "expected '{'");
    auto block = std::make_unique<BlockNode>(line);
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->stmts.push_back(parseStmt());
    }
    expect(TokenType::RBRACE, "expected '}' to close block");
    return block;
}

// =============================================================================
// Expression Parsers — ordered from LOWEST to HIGHEST precedence.
//
// Precedence table (low → high):
//   comparison  (==, <)
//   addition    (+, -)
//   term        (*, /)
//   factor      (literals, identifiers, grouped)
// =============================================================================

// Lowest entry point for expressions
NodePtr Parser::parseExpr() {
    return parseComparison();
}

// comparison := addition ( ('==' | '<') addition )*
NodePtr Parser::parseComparison() {
    NodePtr left = parseAddition();
    while (check(TokenType::EQUAL_EQUAL) || check(TokenType::LESS)) {
        const Token& op = advance();
        NodePtr right = parseAddition();
        left = std::make_unique<BinaryExprNode>(op.lexeme, std::move(left), std::move(right), op.line);
    }
    return left;
}

// addition := term ( ('+' | '-') term )*
NodePtr Parser::parseAddition() {
    NodePtr left = parseTerm();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        const Token& op = advance();
        NodePtr right = parseTerm();
        left = std::make_unique<BinaryExprNode>(op.lexeme, std::move(left), std::move(right), op.line);
    }
    return left;
}

// term := factor ( ('*' | '/') factor )*
NodePtr Parser::parseTerm() {
    NodePtr left = parseFactor();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        const Token& op = advance();
        NodePtr right = parseFactor();
        left = std::make_unique<BinaryExprNode>(op.lexeme, std::move(left), std::move(right), op.line);
    }
    return left;
}

// factor := NUMBER | 'true' | 'false' | IDENT | '(' expr ')'
NodePtr Parser::parseFactor() {
    const Token& tok = peek();

    if (check(TokenType::NUMBER)) {
        advance();
        int64_t val = std::stoll(tok.lexeme);
        return std::make_unique<NumberLitNode>(val, tok.line);
    }

    if (check(TokenType::TRUE_LIT)) {
        advance();
        return std::make_unique<BoolLitNode>(true, tok.line);
    }

    if (check(TokenType::FALSE_LIT)) {
        advance();
        return std::make_unique<BoolLitNode>(false, tok.line);
    }

    if (check(TokenType::IDENTIFIER)) {
        advance();
        return std::make_unique<IdentifierNode>(tok.lexeme, tok.line);
    }

    if (check(TokenType::LPAREN)) {
        advance(); // consume '('
        NodePtr expr = parseExpr();
        expect(TokenType::RPAREN, "expected ')' after grouped expression");
        return expr;
    }

    throw ParseError("Parse error at line " + std::to_string(tok.line) +
                     " near '" + tok.lexeme + "': expected an expression.");
}
