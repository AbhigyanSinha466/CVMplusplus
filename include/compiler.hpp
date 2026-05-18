#pragma once
// =============================================================================
// compiler.hpp — CVM++ Compiler (AST → Bytecode)
//
// The Compiler is the THIRD stage of the pipeline:
//   ProgramNode (AST)  -->  [COMPILER]  -->  Chunk (flat bytecode vector)
//
// It performs a single-pass recursive walk of the AST, emitting bytecode
// instructions into a flat std::vector<uint8_t> called a "Chunk".
//
// Key mechanisms:
//   1. Variable table: maps string names to uint16_t slot indices.
//      The VM stores variables in a flat array indexed by these slots.
//
//   2. Jump patching: for if/while, we emit a placeholder jump address
//      (0x00000000), then PATCH it after we know how far to jump.
//      This is the classic "backpatch" technique.
//
//   3. Payload encoding: multi-byte payloads (int64, uint16, int32) are
//      written in little-endian order directly into the uint8_t stream.
// =============================================================================

#include "../include/ast.hpp"
#include "../include/bytecode.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstring>    // memcpy
#include <stdexcept>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <iostream>

// -----------------------------------------------------------------------------
// Chunk — The compiled bytecode container.
// Also stores the variable name table for disassembly.
// -----------------------------------------------------------------------------
struct Chunk {
    std::vector<uint8_t>               code;       // Raw bytecode bytes
    std::vector<std::string>           varNames;   // varNames[slot] = name
    std::unordered_map<std::string,
                       uint16_t>       varIndex;   // name → slot

    // Retrieve (or allocate) the slot index for a variable name.
    uint16_t getOrDefineVar(const std::string& name) {
        auto it = varIndex.find(name);
        if (it != varIndex.end()) return it->second;
        uint16_t idx = static_cast<uint16_t>(varNames.size());
        varNames.push_back(name);
        varIndex[name] = idx;
        return idx;
    }

    uint16_t getVar(const std::string& name) const {
        auto it = varIndex.find(name);
        if (it == varIndex.end())
            throw std::runtime_error("Compiler: undefined variable '" + name + "'");
        return it->second;
    }
};

// -----------------------------------------------------------------------------
// Compiler — Walks the AST and fills a Chunk with bytecode.
// -----------------------------------------------------------------------------
class Compiler {
public:
    // Compile the full program AST into the provided Chunk.
    void compile(const ASTNode* root, Chunk& chunk);

    // Disassemble the chunk to human-readable instructions (debug mode).
    static void disassemble(const Chunk& chunk);

private:
    Chunk* m_chunk = nullptr; // Current chunk being written to

    // --- High-level recursive emitters ---
    void emitNode(const ASTNode* node);
    void emitStmt(const ASTNode* node);
    void emitExpr(const ASTNode* node);

    // --- Low-level byte emitters ---
    void emitByte(uint8_t b);
    void emitOpcode(Opcode op);
    void emitInt64(int64_t v);        // Encodes 8 bytes (little-endian)
    void emitUint16(uint16_t v);      // Encodes 2 bytes (little-endian)
    void emitInt32(int32_t v);        // Encodes 4 bytes (little-endian)

    // --- Jump patching helpers ---
    // Returns the offset of the placeholder in m_chunk->code.
    std::size_t emitJumpPlaceholder(Opcode jumpOp);
    // Fills in the placeholder with the actual jump target.
    void patchJump(std::size_t patchOffset);
};
