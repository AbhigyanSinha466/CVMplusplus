// =============================================================================
// bytecode.cpp — Opcode name table
// =============================================================================

#include "bytecode.hpp"

const char* opcodeName(Opcode op) {
    switch (op) {
        case Opcode::PUSH_INT:      return "PUSH_INT";
        case Opcode::PUSH_BOOL:     return "PUSH_BOOL";
        case Opcode::POP:           return "POP";
        case Opcode::LOAD:          return "LOAD";
        case Opcode::STORE:         return "STORE";
        case Opcode::ADD:           return "ADD";
        case Opcode::SUB:           return "SUB";
        case Opcode::MUL:           return "MUL";
        case Opcode::DIV:           return "DIV";
        case Opcode::CMP_EQ:        return "CMP_EQ";
        case Opcode::CMP_LT:        return "CMP_LT";
        case Opcode::JUMP:          return "JUMP";
        case Opcode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case Opcode::PRINT:         return "PRINT";
        case Opcode::INPUT:         return "INPUT";
        case Opcode::HALT:          return "HALT";
        default:                    return "UNKNOWN";
    }
}
