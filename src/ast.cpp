// =============================================================================
// ast.cpp — AST Printer (Debug Mode)
// =============================================================================

#include "../include/ast.hpp"
#include <iostream>

// Recursively pretty-print a node with indentation.
// Each level adds 2 spaces. Children are printed after their parent.
void printAST(const ASTNode* node, int indent) {
    if (!node) return;

    std::string pad(static_cast<std::size_t>(indent * 2), ' ');

    switch (node->kind) {
        case NodeKind::Program: {
            auto* n = static_cast<const ProgramNode*>(node);
            std::cout << pad << "[Program]\n";
            for (auto& s : n->stmts) printAST(s.get(), indent + 1);
            break;
        }
        case NodeKind::Block: {
            auto* n = static_cast<const BlockNode*>(node);
            std::cout << pad << "[Block]\n";
            for (auto& s : n->stmts) printAST(s.get(), indent + 1);
            break;
        }
        case NodeKind::NumberLit: {
            auto* n = static_cast<const NumberLitNode*>(node);
            std::cout << pad << "[Number: " << n->value << "]\n";
            break;
        }
        case NodeKind::BoolLit: {
            auto* n = static_cast<const BoolLitNode*>(node);
            std::cout << pad << "[Bool: " << (n->value ? "true" : "false") << "]\n";
            break;
        }
        case NodeKind::Identifier: {
            auto* n = static_cast<const IdentifierNode*>(node);
            std::cout << pad << "[Ident: " << n->name << "]\n";
            break;
        }
        case NodeKind::BinaryExpr: {
            auto* n = static_cast<const BinaryExprNode*>(node);
            std::cout << pad << "[BinaryExpr: " << n->op << "]\n";
            printAST(n->left.get(),  indent + 1);
            printAST(n->right.get(), indent + 1);
            break;
        }
        case NodeKind::VarDecl: {
            auto* n = static_cast<const VarDeclNode*>(node);
            if (n->fromInput) {
                std::cout << pad << "[VarDecl: " << n->name << " = <input>]\n";
            } else {
                std::cout << pad << "[VarDecl: " << n->name << "]\n";
                printAST(n->initializer.get(), indent + 1);
            }
            break;
        }
        case NodeKind::Assign: {
            auto* n = static_cast<const AssignNode*>(node);
            std::cout << pad << "[Assign: " << n->name << "]\n";
            printAST(n->value.get(), indent + 1);
            break;
        }
        case NodeKind::PrintStmt: {
            auto* n = static_cast<const PrintStmtNode*>(node);
            std::cout << pad << "[Print]\n";
            printAST(n->expr.get(), indent + 1);
            break;
        }
        case NodeKind::IfStmt: {
            auto* n = static_cast<const IfStmtNode*>(node);
            std::cout << pad << "[If]\n";
            std::cout << pad << "  [Condition]\n";
            printAST(n->cond.get(), indent + 2);
            std::cout << pad << "  [Then]\n";
            printAST(n->thenBlock.get(), indent + 2);
            if (n->elseBlock) {
                std::cout << pad << "  [Else]\n";
                printAST(n->elseBlock.get(), indent + 2);
            }
            break;
        }
        case NodeKind::WhileStmt: {
            auto* n = static_cast<const WhileStmtNode*>(node);
            std::cout << pad << "[While]\n";
            std::cout << pad << "  [Condition]\n";
            printAST(n->cond.get(), indent + 2);
            std::cout << pad << "  [Body]\n";
            printAST(n->body.get(), indent + 2);
            break;
        }
        default:
            std::cout << pad << "[Unknown Node]\n";
            break;
    }
}
