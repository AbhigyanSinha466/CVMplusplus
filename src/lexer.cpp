// =============================================================================
// lexer.cpp — CVM++ Lexer Implementation
// =============================================================================

#include "../include/lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

// -----------------------------------------------------------------------------
// tokenTypeName — Returns a printable string for each TokenType.
// Used by the debug printer in main.cpp.
// -----------------------------------------------------------------------------
const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:       return "NUMBER";
        case TokenType::TRUE_LIT:     return "TRUE";
        case TokenType::FALSE_LIT:    return "FALSE";
        case TokenType::IDENTIFIER:   return "IDENT";
        case TokenType::LET:          return "LET";
        case TokenType::IF:           return "IF";
        case TokenType::ELSE:         return "ELSE";
        case TokenType::WHILE:        return "WHILE";
        case TokenType::PRINT:        return "PRINT";
        case TokenType::INPUT:        return "INPUT";
        case TokenType::PLUS:         return "PLUS";
        case TokenType::MINUS:        return "MINUS";
        case TokenType::STAR:         return "STAR";
        case TokenType::SLASH:        return "SLASH";
        case TokenType::PERCENT:      return "PERCENT";
        case TokenType::EQUAL_EQUAL:  return "EQUAL_EQUAL";
        case TokenType::BANG_EQUAL:   return "BANG_EQUAL";
        case TokenType::LESS:         return "LESS";
        case TokenType::LESS_EQUAL:   return "LESS_EQUAL";
        case TokenType::GREATER:      return "GREATER";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::ASSIGN:       return "ASSIGN";
        case TokenType::LPAREN:       return "LPAREN";
        case TokenType::RPAREN:       return "RPAREN";
        case TokenType::LBRACE:       return "LBRACE";
        case TokenType::RBRACE:       return "RBRACE";
        case TokenType::SEMICOLON:    return "SEMICOLON";
        case TokenType::EOF_TOKEN:    return "EOF";
        default:                      return "UNKNOWN";
    }
}

// -----------------------------------------------------------------------------
// Keyword lookup table: maps reserved words to their TokenType.
// Using an unordered_map gives O(1) average lookup.
// -----------------------------------------------------------------------------
static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"let",   TokenType::LET},
    {"if",    TokenType::IF},
    {"else",  TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"print", TokenType::PRINT},
    {"input", TokenType::INPUT},
    {"true",  TokenType::TRUE_LIT},
    {"false", TokenType::FALSE_LIT},
};

// =============================================================================
// Lexer — Constructor & Public Interface
// =============================================================================

Lexer::Lexer(std::string source)
    : m_source(std::move(source)), m_pos(0), m_line(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        char c = peek();

        // --- Numeric literals ---
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(readNumber());
            continue;
        }

        // --- Identifiers and keywords ---
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // --- Single and double character operators / punctuation ---
        advance(); // consume the character we're about to classify

        switch (c) {
            case '+': tokens.emplace_back(TokenType::PLUS,      "+", m_line); break;
            case '-': tokens.emplace_back(TokenType::MINUS,     "-", m_line); break;
            case '*': tokens.emplace_back(TokenType::STAR,      "*", m_line); break;
            case '/': tokens.emplace_back(TokenType::SLASH,     "/", m_line); break;
            case '%': tokens.emplace_back(TokenType::PERCENT,   "%", m_line); break;
            case '(': tokens.emplace_back(TokenType::LPAREN,    "(", m_line); break;
            case ')': tokens.emplace_back(TokenType::RPAREN,    ")", m_line); break;
            case '{': tokens.emplace_back(TokenType::LBRACE,    "{", m_line); break;
            case '}': tokens.emplace_back(TokenType::RBRACE,    "}", m_line); break;
            case ';': tokens.emplace_back(TokenType::SEMICOLON, ";", m_line); break;

            case '=':
                // Could be '=' (ASSIGN) or '==' (EQUAL_EQUAL)
                if (!isAtEnd() && peek() == '=') {
                    advance(); // consume second '='
                    tokens.emplace_back(TokenType::EQUAL_EQUAL, "==", m_line);
                } else {
                    tokens.emplace_back(TokenType::ASSIGN, "=", m_line);
                }
                break;

            case '!':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.emplace_back(TokenType::BANG_EQUAL, "!=", m_line);
                } else {
                    throw std::runtime_error(
                        "Lexer error at line " + std::to_string(m_line) +
                        ": unexpected character '!' (did you mean '!=')");
                }
                break;

            case '<':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.emplace_back(TokenType::LESS_EQUAL, "<=", m_line);
                } else {
                    tokens.emplace_back(TokenType::LESS, "<", m_line);
                }
                break;

            case '>':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.emplace_back(TokenType::GREATER_EQUAL, ">=", m_line);
                } else {
                    tokens.emplace_back(TokenType::GREATER, ">", m_line);
                }
                break;

            default:
                throw std::runtime_error(
                    "Lexer error at line " + std::to_string(m_line) +
                    ": unexpected character '" + c + "'");
        }
    }

    // Always terminate the token stream with an EOF sentinel
    tokens.emplace_back(TokenType::EOF_TOKEN, "", m_line);
    return tokens;
}

// =============================================================================
// Private Helpers
// =============================================================================

bool Lexer::isAtEnd() const {
    return m_pos >= m_source.size();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return m_source[m_pos];
}

char Lexer::peekNext() const {
    if (m_pos + 1 >= m_source.size()) return '\0';
    return m_source[m_pos + 1];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') ++m_line; // Track newlines for error reporting
    return c;
}

// Skip whitespace and single-line comments (// ... \n)
void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // Single-line comment: skip until newline
            while (!isAtEnd() && peek() != '\n') advance();
        } else {
            break; // Non-whitespace, non-comment: stop
        }
    }
}

// Read a sequence of digits and produce a NUMBER token.
Token Lexer::readNumber() {
    std::size_t start = m_pos;
    int line = m_line;
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }
    return Token(TokenType::NUMBER, m_source.substr(start, m_pos - start), line);
}

// Read an alphanumeric word. Check the keyword table; default to IDENTIFIER.
Token Lexer::readIdentifierOrKeyword() {
    std::size_t start = m_pos;
    int line = m_line;
    while (!isAtEnd() &&
           (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }
    std::string word = m_source.substr(start, m_pos - start);

    // Keyword lookup
    auto it = KEYWORDS.find(word);
    TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::IDENTIFIER;
    return Token(type, word, line);
}
