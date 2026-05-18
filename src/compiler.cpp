// =============================================================================
// compiler.cpp — CVM++ Compiler Implementation
// =============================================================================

#include "compiler.hpp"
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cstring>

// =============================================================================
// Public Interface
// =============================================================================

void Compiler::compile(const ASTNode* root, Chunk& chunk) {
    m_chunk = &chunk;
    m_chunk->code.clear(); // Fresh code for this snippet
    emitNode(root);
    emitOpcode(Opcode::HALT);
    m_chunk = nullptr;
}

// =============================================================================
// Node Dispatch
// =============================================================================

// Top-level dispatch: every AST node eventually routes through here.
void Compiler::emitNode(const ASTNode* node) {
    if (!node) return;
    switch (node->kind) {
        case NodeKind::Program: {
            auto* n = static_cast<const ProgramNode*>(node);
            for (auto& s : n->stmts) emitNode(s.get());
            break;
        }
        case NodeKind::Block: {
            auto* n = static_cast<const BlockNode*>(node);
            for (auto& s : n->stmts) emitNode(s.get());
            break;
        }
        // Statements
        case NodeKind::VarDecl:
        case NodeKind::Assign:
        case NodeKind::PrintStmt:
        case NodeKind::IfStmt:
        case NodeKind::WhileStmt:
            emitStmt(node);
            break;
        // Expressions
        default:
            emitExpr(node);
            break;
    }
}

// =============================================================================
// Statement Emitters
// =============================================================================

void Compiler::emitStmt(const ASTNode* node) {
    switch (node->kind) {

        // --- let x = expr; ---
        // Steps: define variable slot, evaluate expr, STORE into slot.
        case NodeKind::VarDecl: {
            auto* n = static_cast<const VarDeclNode*>(node);
            uint16_t slot = m_chunk->getOrDefineVar(n->name);
            if (n->fromInput) {
                // Emit INPUT opcode: reads int64 from stdin and pushes it.
                emitOpcode(Opcode::INPUT);
            } else {
                emitExpr(n->initializer.get());
            }
            emitOpcode(Opcode::STORE);
            emitUint16(slot);
            break;
        }

        // --- x = expr; ---
        case NodeKind::Assign: {
            auto* n = static_cast<const AssignNode*>(node);
            uint16_t slot = m_chunk->getVar(n->name);
            emitExpr(n->value.get());
            emitOpcode(Opcode::STORE);
            emitUint16(slot);
            break;
        }

        // --- print expr; ---
        // Evaluate the expression (leaves value on stack), then PRINT.
        case NodeKind::PrintStmt: {
            auto* n = static_cast<const PrintStmtNode*>(node);
            emitExpr(n->expr.get());
            emitOpcode(Opcode::PRINT);
            break;
        }

        // --- if (cond) { then } [else { else }] ---
        //
        // Bytecode layout:
        //   [cond]
        //   JUMP_IF_FALSE → L1
        //   [then block]
        //   JUMP          → L2      ← only emitted when else exists
        // L1:
        //   [else block]             ← only emitted when else exists
        // L2:
        //
        case NodeKind::IfStmt: {
            auto* n = static_cast<const IfStmtNode*>(node);

            // 1. Emit condition
            emitExpr(n->cond.get());

            // 2. JUMP_IF_FALSE to the else/end — address unknown yet
            std::size_t jumpToElse = emitJumpPlaceholder(Opcode::JUMP_IF_FALSE);

            // 3. Emit then-block
            emitNode(n->thenBlock.get());

            if (n->elseBlock) {
                // 4a. Jump over the else block (from end of then-block)
                std::size_t jumpToEnd = emitJumpPlaceholder(Opcode::JUMP);

                // 5a. Patch the JUMP_IF_FALSE to land here (start of else)
                patchJump(jumpToElse);

                // 6a. Emit else-block
                emitNode(n->elseBlock.get());

                // 7a. Patch the end-of-then JUMP to land here (after else)
                patchJump(jumpToEnd);
            } else {
                // 4b. No else: patch JUMP_IF_FALSE to land here (after then)
                patchJump(jumpToElse);
            }
            break;
        }

        // --- while (cond) { body } ---
        //
        // Bytecode layout:
        // LOOP_START:
        //   [cond]
        //   JUMP_IF_FALSE → LOOP_END
        //   [body]
        //   JUMP          → LOOP_START
        // LOOP_END:
        //
        case NodeKind::WhileStmt: {
            auto* n = static_cast<const WhileStmtNode*>(node);

            // Remember the address of the loop condition (for back-jump)
            std::size_t loopStart = m_chunk->code.size();

            // Emit condition
            emitExpr(n->cond.get());

            // JUMP_IF_FALSE out of loop
            std::size_t jumpToEnd = emitJumpPlaceholder(Opcode::JUMP_IF_FALSE);

            // Emit body
            emitNode(n->body.get());

            // Unconditional back-jump to loop start.
            // We compute the relative offset ourselves here.
            emitOpcode(Opcode::JUMP);
            // Current position is where the 4-byte payload goes.
            std::size_t jumpPayloadPos = m_chunk->code.size();
            emitInt32(0); // placeholder
            // Target = loopStart; offset from END of this instruction
            int32_t backOffset = static_cast<int32_t>(loopStart) -
                                 static_cast<int32_t>(jumpPayloadPos + 4);
            std::memcpy(m_chunk->code.data() + jumpPayloadPos, &backOffset, 4);

            // Patch the JUMP_IF_FALSE to the instruction after the back-jump
            patchJump(jumpToEnd);
            break;
        }

        default:
            throw std::runtime_error("Compiler: unknown statement node");
    }
}

// =============================================================================
// Expression Emitters
// =============================================================================

void Compiler::emitExpr(const ASTNode* node) {
    switch (node->kind) {

        // Push an integer literal
        case NodeKind::NumberLit: {
            auto* n = static_cast<const NumberLitNode*>(node);
            emitOpcode(Opcode::PUSH_INT);
            emitInt64(n->value);
            break;
        }

        // Push a boolean literal
        case NodeKind::BoolLit: {
            auto* n = static_cast<const BoolLitNode*>(node);
            emitOpcode(Opcode::PUSH_BOOL);
            emitByte(n->value ? 1 : 0);
            break;
        }

        // Load a variable's value onto the stack
        case NodeKind::Identifier: {
            auto* n = static_cast<const IdentifierNode*>(node);
            uint16_t slot = m_chunk->getVar(n->name);
            emitOpcode(Opcode::LOAD);
            emitUint16(slot);
            break;
        }

        // Binary expression: emit both sides, then the operator
        // Stack before ADD:   [..., left, right]
        // Stack after  ADD:   [..., result]
        case NodeKind::BinaryExpr: {
            auto* n = static_cast<const BinaryExprNode*>(node);
            emitExpr(n->left.get());
            emitExpr(n->right.get());

            if      (n->op == "+")  emitOpcode(Opcode::ADD);
            else if (n->op == "-")  emitOpcode(Opcode::SUB);
            else if (n->op == "*")  emitOpcode(Opcode::MUL);
            else if (n->op == "/")  emitOpcode(Opcode::DIV);
            else if (n->op == "==") emitOpcode(Opcode::CMP_EQ);
            else if (n->op == "<")  emitOpcode(Opcode::CMP_LT);
            else throw std::runtime_error("Compiler: unknown operator '" + n->op + "'");
            break;
        }

        default:
            throw std::runtime_error("Compiler: unexpected expression node kind");
    }
}

// =============================================================================
// Low-Level Byte Emitters
// =============================================================================

void Compiler::emitByte(uint8_t b) {
    m_chunk->code.push_back(b);
}

void Compiler::emitOpcode(Opcode op) {
    emitByte(static_cast<uint8_t>(op));
}

// Little-endian 8-byte int64
void Compiler::emitInt64(int64_t v) {
    uint8_t buf[8];
    std::memcpy(buf, &v, 8);
    for (int i = 0; i < 8; ++i) emitByte(buf[i]);
}

// Little-endian 2-byte uint16
void Compiler::emitUint16(uint16_t v) {
    emitByte(static_cast<uint8_t>(v & 0xFF));
    emitByte(static_cast<uint8_t>((v >> 8) & 0xFF));
}

// Little-endian 4-byte int32
void Compiler::emitInt32(int32_t v) {
    uint8_t buf[4];
    std::memcpy(buf, &v, 4);
    for (int i = 0; i < 4; ++i) emitByte(buf[i]);
}

// =============================================================================
// Jump Patching
// =============================================================================

// Emit a jump instruction with a 4-byte zeroed placeholder.
// Returns the byte offset of the placeholder (so we can patch it later).
std::size_t Compiler::emitJumpPlaceholder(Opcode jumpOp) {
    emitOpcode(jumpOp);
    std::size_t patchOffset = m_chunk->code.size();
    emitInt32(0); // Placeholder — will be filled in by patchJump()
    return patchOffset;
}

// Fill in the jump target for a previously emitted placeholder.
// The target is expressed as a RELATIVE offset from the end of the jump
// instruction (i.e., from patchOffset + 4) to the current code position.
//
// Example (JUMP_IF_FALSE):
//   Offset 10: JUMP_IF_FALSE [patchOffset=11] [xx xx xx xx]
//   ...emitted then-block...
//   Offset 30: ← current code size (jump lands here)
//
//   We write: 30 - (11 + 4) = 15  into bytes 11..14
void Compiler::patchJump(std::size_t patchOffset) {
    int32_t jumpDist = static_cast<int32_t>(m_chunk->code.size()) -
                       static_cast<int32_t>(patchOffset + 4);
    std::memcpy(m_chunk->code.data() + patchOffset, &jumpDist, 4);
}

// =============================================================================
// Disassembler — Decodes and prints bytecode instructions
// =============================================================================

void Compiler::disassemble(const Chunk& chunk) {
    std::cout << "=== Bytecode Disassembly ===\n";
    std::cout << "Variables:\n";
    for (std::size_t i = 0; i < chunk.varNames.size(); ++i) {
        std::cout << "  [" << i << "] " << chunk.varNames[i] << "\n";
    }
    std::cout << "\nInstructions:\n";

    const auto& code = chunk.code;
    std::size_t ip = 0;

    while (ip < code.size()) {
        // Print instruction offset
        std::cout << std::setw(6) << ip << "  ";

        Opcode op = static_cast<Opcode>(code[ip]);
        std::cout << opcodeName(op);
        ++ip;

        switch (op) {
            case Opcode::PUSH_INT: {
                int64_t v;
                std::memcpy(&v, code.data() + ip, 8);
                std::cout << "  " << v;
                ip += 8;
                break;
            }
            case Opcode::PUSH_BOOL: {
                std::cout << "  " << (code[ip] ? "true" : "false");
                ip += 1;
                break;
            }
            case Opcode::LOAD:
            case Opcode::STORE: {
                uint16_t slot;
                std::memcpy(&slot, code.data() + ip, 2);
                std::string name = (slot < chunk.varNames.size())
                                   ? chunk.varNames[slot] : "?";
                std::cout << "  [" << slot << "] (" << name << ")";
                ip += 2;
                break;
            }
            case Opcode::JUMP:
            case Opcode::JUMP_IF_FALSE: {
                int32_t offset;
                std::memcpy(&offset, code.data() + ip, 4);
                // Absolute target = current ip (after payload) + offset
                std::size_t target = static_cast<std::size_t>(
                    static_cast<int64_t>(ip + 4) + offset);
                std::cout << "  offset=" << offset << " -> " << target;
                ip += 4;
                break;
            }
            default:
                // Single-byte opcodes with no payload
                break;
        }
        std::cout << "\n";
        if (op == Opcode::HALT) break;
    }
    std::cout << "============================\n\n";
}
