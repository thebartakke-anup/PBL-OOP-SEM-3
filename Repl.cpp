#include "Repl.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

SSVMRepl::SSVMRepl(size_t maxBufferChars)
    : vm_(std::make_unique<SimpleStackVM>()),
      buffer_(""),
      lineNumber_(1),
      maxBufferChars_(maxBufferChars) {}

void SSVMRepl::start() {
    std::cout << "========================================\n";
    std::cout << "Simple Stack VM (C++) -- Interactive REPL\n";
    std::cout << "========================================\n";
    std::cout << "Instructions : PSH n, ADD, SUB, MUL, PRN, SET reg, LD reg, HLT\n";
    std::cout << "Commands     : RUN, CLEAR, EXIT\n";
    std::cout << "Registers    : 0-25 (0=A, 1=B, ... 25=Z)\n";
    std::cout << "----------------------------------------\n\n";
    
    bool keepRunning = true;
    
    while (keepRunning) {
        std::cout << "[" << lineNumber_ << "] ";
        
        std::string input;
        if (!std::getline(std::cin, input)) {
            break;  // EOF
        }
        
        // Convert to uppercase for command checking
        std::string upperInput = input;
        std::transform(upperInput.begin(), upperInput.end(), 
                      upperInput.begin(), ::toupper);
        
        // Trim whitespace
        size_t start = upperInput.find_first_not_of(" \t");
        if (start != std::string::npos) {
            upperInput = upperInput.substr(start);
        }
        size_t end = upperInput.find_last_not_of(" \t");
        if (end != std::string::npos) {
            upperInput = upperInput.substr(0, end + 1);
        }
        
        if (upperInput.empty()) continue;
        
        // Check for commands
        if (upperInput == "RUN") {
            handleRun();
            lineNumber_ = 1;
        } else if (upperInput == "CLEAR") {
            handleClear();
        } else if (upperInput == "EXIT") {
            std::cout << "\nGoodbye!\n";
            keepRunning = false;
        } else if (upperInput == "HELP" || upperInput == "?") {
            printHelp();
        } else {
            // Add instruction to buffer
            if (buffer_.size() + input.size() > maxBufferChars_) {
                std::cout << "ERROR: Program buffer full\n";
            } else {
                if (!buffer_.empty()) {
                    buffer_ += "\n";
                }
                buffer_ += input;
                lineNumber_++;
            }
        }
    }
}

void SSVMRepl::handleRun() {
    if (buffer_.empty()) {
        std::cout << "Nothing to run.\n";
        return;
    }
    
    try {
        vm_->assembleAndLoad(buffer_);
        std::cout << "(" << std::to_string(vm_->peekMemory(0)) << " words assembled)\n";
        vm_->run(true);
        std::cout << "VM HALTED\n";
    } catch (const AssemblyError& e) {
        std::cout << "ASSEMBLY ERROR: " << e.what() << "\n";
    } catch (const StackOverflowError& e) {
        std::cout << "STACK OVERFLOW: " << e.what() << "\n";
    } catch (const StackUnderflowError& e) {
        std::cout << "STACK UNDERFLOW: " << e.what() << "\n";
    } catch (const InvalidOpcodeError& e) {
        std::cout << "INVALID OPCODE: " << e.what() << "\n";
    } catch (const VMException& e) {
        std::cout << "VM ERROR: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << "\n";
    }
}

void SSVMRepl::handleClear() {
    buffer_.clear();
    lineNumber_ = 1;
    vm_->reset();
    std::cout << "Buffer cleared. Registers reset.\n";
}

bool SSVMRepl::handleCommand(const std::string& upperLine) {
    return upperLine == "RUN" || upperLine == "CLEAR" || upperLine == "EXIT";
}

void SSVMRepl::printHelp() {
    std::cout << "\nSimple Stack Virtual Machine - Help\n";
    std::cout << "====================================\n";
    std::cout << "INSTRUCTIONS:\n";
    std::cout << "  PSH n      - Push integer n onto stack\n";
    std::cout << "  ADD        - Pop two values, push sum\n";
    std::cout << "  SUB        - Pop two values, push difference\n";
    std::cout << "  MUL        - Pop two values, push product\n";
    std::cout << "  PRN        - Print top of stack (does not pop)\n";
    std::cout << "  SET n      - Pop stack, store in register (0=A, 1=B, ...)\n";
    std::cout << "  LD n       - Push register value onto stack\n";
    std::cout << "  JMP addr   - Jump to address\n";
    std::cout << "  JZ addr    - Jump to address if top of stack equals 0\n";
    std::cout << "  HLT        - Halt execution\n";
    std::cout << "\nCOMMANDS:\n";
    std::cout << "  RUN        - Execute the buffered program\n";
    std::cout << "  CLEAR      - Clear buffer and reset VM\n";
    std::cout << "  EXIT       - Exit the REPL\n";
    std::cout << "  HELP or ?  - Show this help message\n";
    std::cout << "\nEXAMPLE:\n";
    std::cout << "  [1] psh 10\n";
    std::cout << "  [2] psh 5\n";
    std::cout << "  [3] mul\n";
    std::cout << "  [4] prn\n";
    std::cout << "  [5] hlt\n";
    std::cout << "  [6] run\n";
    std::cout << "\n";
}
