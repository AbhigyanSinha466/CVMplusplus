#pragma once
// =============================================================================
// ast.hpp — CVM++ Abstract Syntax Tree (AST) Node Definitions
//
// The Parser converts a flat list of Tokens into a TREE structure that
// captures the grammatical (hierarchical) relationships between expressions
// and statements.
//
// Example: "let x = 1 + 2 * 3;"
//
//   VarDeclNode("x")
//    └── BinaryExprNode(PLUS)
//         ├── NumberNode(1)
//         └── BinaryExprNode(STAR)
//              ├── NumberNode(2)
//              └── NumberNode(3)
//
// We use a classic Visitor-less design: each node carries a "kind" tag and
// stores children via unique_ptr to a base ASTNode. The Compiler walks the
// tree recursively (downcast via static_cast after checking kind).
// =============================================================================

#include <memory>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// NodeKind — discriminated-union tag for every AST node type.
// -----------------------------------------------------------------------------
enum class NodeKind {
    // Expressions
    NumberLit,      // integer literal
    BoolLit,        // boolean literal
    Identifier,     // variable reference
    BinaryExpr,     // left op right
    UnaryMinus,     // -expr  (future-proofing; not in scope but easy to add)

    // Statements
    VarDecl,        // let x = expr;
    Assign,         // x = expr;
    PrintStmt,      // print expr;
    InputStmt,      // let x = input;   (input is an rvalue here)
    IfStmt,         // if (cond) { then } [else { else }]
    WhileStmt,      // while (cond) { body }
    Block,          // { stmt* }
    Program,        // top-level list of statements
};

// -----------------------------------------------------------------------------
// ASTNode — polymorphic base for all nodes.
// We avoid virtual dispatch overhead by using NodeKind + static_cast.
// -----------------------------------------------------------------------------
struct ASTNode {
    NodeKind kind;
    int      line; // source line (for error messages)

    explicit ASTNode(NodeKind k, int ln = 0) : kind(k), line(ln) {}
    virtual ~ASTNode() = default;

    // Disable copy; nodes are owned exclusively via unique_ptr
    ASTNode(const ASTNode&)            = delete;
    ASTNode& operator=(const ASTNode&) = delete;
};

using NodePtr = std::unique_ptr<ASTNode>;

// =============================================================================
// Expression Nodes
// =============================================================================

// A literal integer: 42
struct NumberLitNode : ASTNode {
    int64_t value;
    NumberLitNode(int64_t v, int ln)
        : ASTNode(NodeKind::NumberLit, ln), value(v) {}
};

// A literal boolean: true / false
struct BoolLitNode : ASTNode {
    bool value;
    BoolLitNode(bool v, int ln)
        : ASTNode(NodeKind::BoolLit, ln), value(v) {}
};

// A variable reference: x
struct IdentifierNode : ASTNode {
    std::string name;
    IdentifierNode(std::string n, int ln)
        : ASTNode(NodeKind::Identifier, ln), name(std::move(n)) {}
};

// Binary expression: left OP right
// op is stored as the lexeme string ("+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=")
struct BinaryExprNode : ASTNode {
    std::string op;
    NodePtr     left;
    NodePtr     right;
    BinaryExprNode(std::string o, NodePtr l, NodePtr r, int ln)
        : ASTNode(NodeKind::BinaryExpr, ln),
          op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};

// =============================================================================
// Statement Nodes
// =============================================================================

// let x = expr;
struct VarDeclNode : ASTNode {
    std::string name;
    NodePtr     initializer; // Can be nullptr if "let x = input;"
    bool        fromInput;   // true when the RHS is the `input` keyword
    VarDeclNode(std::string n, NodePtr init, bool fi, int ln)
        : ASTNode(NodeKind::VarDecl, ln),
          name(std::move(n)), initializer(std::move(init)), fromInput(fi) {}
};

// x = expr;  (assignment to existing variable)
struct AssignNode : ASTNode {
    std::string name;
    NodePtr     value;
    AssignNode(std::string n, NodePtr v, int ln)
        : ASTNode(NodeKind::Assign, ln), name(std::move(n)), value(std::move(v)) {}
};

// print expr;
struct PrintStmtNode : ASTNode {
    NodePtr expr;
    PrintStmtNode(NodePtr e, int ln)
        : ASTNode(NodeKind::PrintStmt, ln), expr(std::move(e)) {}
};

// A group of statements: { stmt* }
struct BlockNode : ASTNode {
    std::vector<NodePtr> stmts;
    explicit BlockNode(int ln) : ASTNode(NodeKind::Block, ln) {}
};

// if (cond) thenBlock [else elseBlock]
struct IfStmtNode : ASTNode {
    NodePtr cond;
    NodePtr thenBlock;
    NodePtr elseBlock; // nullptr if no else
    IfStmtNode(NodePtr c, NodePtr t, NodePtr e, int ln)
        : ASTNode(NodeKind::IfStmt, ln),
          cond(std::move(c)), thenBlock(std::move(t)), elseBlock(std::move(e)) {}
};

// while (cond) body
struct WhileStmtNode : ASTNode {
    NodePtr cond;
    NodePtr body;
    WhileStmtNode(NodePtr c, NodePtr b, int ln)
        : ASTNode(NodeKind::WhileStmt, ln),
          cond(std::move(c)), body(std::move(b)) {}
};

// The entire program: a list of top-level statements
struct ProgramNode : ASTNode {
    std::vector<NodePtr> stmts;
    explicit ProgramNode(int ln) : ASTNode(NodeKind::Program, ln) {}
};

// =============================================================================
// AST Printer — pretty-prints the tree for debug mode (-d flag)
// =============================================================================
void printAST(const ASTNode* node, int indent = 0);
