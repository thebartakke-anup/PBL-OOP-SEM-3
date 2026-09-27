#ifndef SSVM_HPP
#define SSVM_HPP

#include <string>
#include <vector>
#include <array>
#include <map>
#include <memory>
#include <stdexcept>
#include <iostream>

// Exception Hierarchy
class VMException : public std::runtime_error {
public:
    explicit VMException(const std::string& msg) : std::runtime_error(msg) {}
};

class AssemblyError : public VMException {
public:
    explicit AssemblyError(const std::string& msg) : VMException(msg) {}
};

class StackOverflowError : public VMException {
public:
    explicit StackOverflowError(const std::string& msg) : VMException(msg) {}
};

class StackUnderflowError : public VMException {
public:
    explicit StackUnderflowError(const std::string& msg) : VMException(msg) {}
};

class InvalidOpcodeError : public VMException {
public:
    explicit InvalidOpcodeError(const std::string& msg) : VMException(msg) {}
};

// Opcode enum class for type safety
enum class Opcode {
    PSH = 0,    // Push
    ADD = 1,    // Add
    SUB = 2,    // Subtract
    MUL = 3,    // Multiply
    PRN = 4,    // Print
    SET = 5,    // Set register
    LD = 6,     // Load register
    JMP = 7,    // Jump
    JZ = 8,     // Jump if zero
    HLT = 9,    // Halt
    UNKNOWN = 255
};

class SimpleStackVM {
public:
    explicit SimpleStackVM(size_t memorySize = 1024, size_t stackSize = 256);
    
    // Public interface
    int assembleAndLoad(const std::string& source);
    void run(bool trace = true);
    void reset();
    
    // Read-only accessors
    int registerValue(int reg) const;
    int peekMemory(int address) const;
    int stackTop() const;
    bool isHalted() const;
    
private:
    // Memory and state
    std::vector<int> memory_;
    std::vector<int> stack_;
    std::array<int, 26> registers_;
    int sp_;  // Stack pointer
    int pc_;  // Program counter
    int loadedLength_;
    bool halted_;
    
    // Helper structures
    struct ParsedLine {
        std::string label;
        std::string mnemonic;
        std::string operand;
        bool hasInstruction = false;
    };
    
    // Assembler methods (private)
    void firstPass(const std::vector<std::string>& lines,
                   std::map<std::string, int>& labelTable);
    int secondPass(const std::vector<std::string>& lines,
                   const std::map<std::string, int>& labelTable);
    
    // Stack operations (private)
    void push(int value);
    int pop();
    int peek() const;
    
    // Helper methods
    ParsedLine parseLine(const std::string& line);
    Opcode mnemonicToOpcode(const std::string& mnemonic) const;
    bool opcodeHasOperand(Opcode op) const;
    bool isInteger(const std::string& s) const;
    std::vector<std::string> splitLines(const std::string& source);
    std::string trim(const std::string& str) const;
    std::string toUpper(const std::string& str) const;
};

#endif // SSVM_HPP
