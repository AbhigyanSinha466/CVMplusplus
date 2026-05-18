// =============================================================================
// main.cpp — CVM++ CLI: File Runner and REPL
//
// Ties the entire pipeline together:
//
//   Source Code
//       │
//       ▼
//   [Lexer]  → tokens
//       │
//       ▼
//   [Parser] → AST (ProgramNode)
//       │
//       ▼
//   [Compiler] → Chunk (bytecode)
//       │
//       ▼
//   [VM]     → execution / output
//
// Usage:
//   cvm                        # interactive REPL
//   cvm script.cvm             # run a script file
//   cvm -d script.cvm          # run with debug output (AST + disassembly)
//   cvm --help                 # show help
//
// Debug mode (-d):
//   Prints the token list, the AST tree, and the bytecode disassembly
//   before executing the script.
// =============================================================================

#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/compiler.hpp"
#include "../include/vm.hpp"
#include "../include/ast.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

// =============================================================================
// run() — Execute a complete source string through the full pipeline.
// =============================================================================
static void run(const std::string& source, bool debug, 
                Compiler& compiler, VM& vm, Chunk& chunk) {
    // ---- Stage 1: Lexing ----
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    if (debug) {
        std::cout << "=== Tokens ===\n";
        for (auto& tok : tokens) {
            std::cout << "  [" << tokenTypeName(tok.type) << "] "
                      << '"' << tok.lexeme << '"'
                      << " (line " << tok.line << ")\n";
        }
        std::cout << "\n";
    }

    // ---- Stage 2: Parsing ----
    Parser parser(std::move(tokens));
    NodePtr ast = parser.parse();

    if (debug) {
        std::cout << "=== AST ===\n";
        printAST(ast.get());
        std::cout << "\n";
    }

    // ---- Stage 3: Compiling ----
    compiler.compile(ast.get(), chunk);

    if (debug) {
        Compiler::disassemble(chunk);
    }

    // ---- Stage 4: Executing ----
    vm.execute(chunk);
}

// =============================================================================
// runFile() — Read a .cvm file and execute it.
// =============================================================================
static void runFile(const std::string& path, bool debug) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open file '" << path << "'\n";
        std::exit(1);
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    Compiler compiler;
    VM vm;
    Chunk chunk;

    try {
        run(source, debug, compiler, vm, chunk);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        std::exit(1);
    }
}

// =============================================================================
// runREPL() — Interactive Read-Eval-Print Loop.
// Variables and VM state persist between lines.
// =============================================================================
static void runREPL(bool debug) {
    std::cout << "CVM++ Interactive REPL\n";
    std::cout << "Type CVM++ statements ending with ';'. Enter 'exit' to quit.\n";
    std::cout << "Use '-d' flag at launch for debug mode.\n\n";

    Compiler compiler;
    VM vm;
    Chunk chunk;

    std::string line;
    while (true) {
        std::cout << "cvm> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break; // EOF
        }
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        try {
            run(line, debug, compiler, vm, chunk);
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
            // Don't exit on error in REPL — just print and continue
        }
    }
}

// =============================================================================
// printHelp() — Usage information
// =============================================================================
static void printHelp(const char* progName) {
    std::cout <<
        "CVM++ — A Lightweight Scripting Language with Stack-Based VM\n"
        "Coding Club, IIT Guwahati — Even Semester Projects 2026\n\n"
        "Usage:\n"
        "  " << progName << "                    # Interactive REPL\n"
        "  " << progName << " <script.cvm>       # Run a script file\n"
        "  " << progName << " -d <script.cvm>    # Run with AST + bytecode debug output\n"
        "  " << progName << " --help             # Show this help\n\n"
        "Language features:\n"
        "  Data types : Integer, Boolean\n"
        "  Operators  : + - * / == <\n"
        "  Variables  : let x = 10;\n"
        "  Assignment : x = x + 1;\n"
        "  Control    : if (cond) { ... } else { ... }\n"
        "               while (cond) { ... }\n"
        "  I/O        : print expr;   let x = input;\n"
        "  Comments   : // this is a comment\n\n"
        "Example:\n"
        "  let n = 5;\n"
        "  let result = 1;\n"
        "  while (0 < n) {\n"
        "      result = result * n;\n"
        "      n = n - 1;\n"
        "  }\n"
        "  print result;\n";
}

// =============================================================================
// main() — Entry point
// =============================================================================
int main(int argc, char* argv[]) {
    bool        debug    = false;
    std::string filePath;

    // --- Argument parsing ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            debug = true;
        } else if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (!arg.empty() && arg[0] != '-') {
            filePath = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            std::cerr << "Run '" << argv[0] << " --help' for usage.\n";
            return 1;
        }
    }

    if (!filePath.empty()) {
        runFile(filePath, debug);
    } else {
        runREPL(debug);
    }

    return 0;
}
