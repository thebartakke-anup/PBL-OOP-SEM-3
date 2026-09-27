#include "SSVM.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

SimpleStackVM::SimpleStackVM(size_t memorySize, size_t stackSize)
    : memory_(memorySize, 0),
      stack_(stackSize, 0),
      registers_{},
      sp_(-1),
      pc_(0),
      loadedLength_(0),
      halted_(false) {
    std::fill(registers_.begin(), registers_.end(), 0);
}

int SimpleStackVM::assembleAndLoad(const std::string& source) {
    std::vector<std::string> lines = splitLines(source);
    std::map<std::string, int> labelTable;
    
    reset();
    
    try {
        firstPass(lines, labelTable);
        loadedLength_ = secondPass(lines, labelTable);
    } catch (const AssemblyError& e) {
        throw;
    }
    
    pc_ = 0;
    halted_ = false;
    return loadedLength_;
}

void SimpleStackVM::firstPass(const std::vector<std::string>& lines,
                              std::map<std::string, int>& labelTable) {
    int address = 0;
    
    for (const auto& raw : lines) {
        ParsedLine parsed = parseLine(raw);
        
        if (!parsed.label.empty()) {
            labelTable[parsed.label] = address;
            if (!parsed.hasInstruction) continue;
        }
        
        if (!parsed.hasInstruction) continue;
        
        Opcode op = mnemonicToOpcode(parsed.mnemonic);
        if (op == Opcode::UNKNOWN) {
            throw AssemblyError("Unknown instruction '" + parsed.mnemonic + "'");
        }
        
        address += opcodeHasOperand(op) ? 2 : 1;
    }
}

int SimpleStackVM::secondPass(const std::vector<std::string>& lines,
                              const std::map<std::string, int>& labelTable) {
    int count = 0;
    
    for (const auto& raw : lines) {
        ParsedLine parsed = parseLine(raw);
        
        if (!parsed.hasInstruction) continue;
        
        Opcode op = mnemonicToOpcode(parsed.mnemonic);
        if (op == Opcode::UNKNOWN) {
            throw AssemblyError("Unknown instruction '" + parsed.mnemonic + "'");
        }
        
        memory_.at(count++) = static_cast<int>(op);
        
        if (opcodeHasOperand(op)) {
            int value = 0;
            
            if (isInteger(parsed.operand)) {
                value = std::stoi(parsed.operand);
            } else {
                std::string label = toUpper(parsed.operand);
                auto it = labelTable.find(label);
                if (it == labelTable.end()) {
                    throw AssemblyError("Unknown label '" + parsed.operand + "'");
                }
                value = it->second;
            }
            memory_.at(count++) = value;
        }
    }
    
    return count;
}

void SimpleStackVM::run(bool trace) {
    halted_ = false;
    sp_ = -1;
    
    while (pc_ < loadedLength_ && !halted_) {
        if (pc_ < 0 || pc_ >= static_cast<int>(memory_.size())) {
            throw InvalidOpcodeError("Program counter ran past loaded program");
        }
        
        int opcode = memory_.at(pc_++);
        Opcode op = static_cast<Opcode>(opcode);
        
        if (static_cast<int>(op) > static_cast<int>(Opcode::HLT) && 
            op != Opcode::UNKNOWN) {
            throw InvalidOpcodeError("Unknown opcode encountered at pc=" + 
                                   std::to_string(pc_ - 1));
        }
        
        switch (op) {
            case Opcode::PSH: {
                int value = memory_.at(pc_++);
                push(value);
                if (trace) std::cout << "OP: PUSH " << value << "\n";
                break;
            }
            
            case Opcode::ADD: {
                int b = pop();
                int a = pop();
                push(a + b);
                if (trace) std::cout << "OP: ADD (" << a << " + " << b << ")\n";
                break;
            }
            
            case Opcode::SUB: {
                int b = pop();
                int a = pop();
                push(a - b);
                if (trace) std::cout << "OP: SUB (" << a << " - " << b << ")\n";
                break;
            }
            
            case Opcode::MUL: {
                int b = pop();
                int a = pop();
                push(a * b);
                if (trace) std::cout << "OP: MUL (" << a << " * " << b << ")\n";
                break;
            }
            
            case Opcode::PRN: {
                int value = peek();
                if (trace) std::cout << "OP: PRINT\n";
                std::cout << "OUTPUT: " << value << "\n";
                break;
            }
            
            case Opcode::SET: {
                int reg = memory_.at(pc_++);
                if (reg < 0 || reg >= 26) {
                    throw InvalidOpcodeError("Invalid register index: " + 
                                           std::to_string(reg));
                }
                int value = pop();
                registers_.at(reg) = value;
                if (trace) {
                    char regName = static_cast<char>('A' + reg);
                    std::cout << "OP: SET Register " << regName << " = " << value << "\n";
                }
                break;
            }
            
            case Opcode::LD: {
                int reg = memory_.at(pc_++);
                if (reg < 0 || reg >= 26) {
                    throw InvalidOpcodeError("Invalid register index: " + 
                                           std::to_string(reg));
                }
                int value = registers_.at(reg);
                push(value);
                if (trace) {
                    char regName = static_cast<char>('A' + reg);
                    std::cout << "OP: LOAD Register " << regName << " (" << value << ")\n";
                }
                break;
            }
            
            case Opcode::JMP: {
                int addr = memory_.at(pc_++);
                pc_ = addr;
                if (trace) std::cout << "OP: JMP " << addr << "\n";
                break;
            }
            
            case Opcode::JZ: {
                int addr = memory_.at(pc_++);
                int value = pop();
                if (value == 0) {
                    pc_ = addr;
                    if (trace) std::cout << "OP: JZ (condition true, jump to " << addr << ")\n";
                } else {
                    if (trace) std::cout << "OP: JZ (condition false, continue)\n";
                }
                break;
            }
            
            case Opcode::HLT: {
                halted_ = true;
                if (trace) std::cout << "OP: HALT\n";
                break;
            }
            
            default:
                throw InvalidOpcodeError("Unknown opcode encountered at pc=" + 
                                       std::to_string(pc_ - 1));
        }
    }
    
    if (!halted_ && pc_ >= loadedLength_) {
        if (trace) std::cout << "VM HALTED\n";
    }
}

void SimpleStackVM::reset() {
    sp_ = -1;
    pc_ = 0;
    halted_ = true;
    std::fill(registers_.begin(), registers_.end(), 0);
    std::fill(memory_.begin(), memory_.end(), 0);
}

int SimpleStackVM::registerValue(int reg) const {
    if (reg < 0 || reg >= 26) {
        throw std::out_of_range("Register index out of range");
    }
    return registers_.at(reg);
}

int SimpleStackVM::peekMemory(int address) const {
    if (address < 0 || address >= static_cast<int>(memory_.size())) {
        throw std::out_of_range("Memory address out of range");
    }
    return memory_.at(address);
}

int SimpleStackVM::stackTop() const {
    if (sp_ < 0) {
        throw StackUnderflowError("Stack is empty");
    }
    return stack_.at(sp_);
}

bool SimpleStackVM::isHalted() const {
    return halted_;
}

void SimpleStackVM::push(int value) {
    if (sp_ + 1 >= static_cast<int>(stack_.size())) {
        throw StackOverflowError("Stack overflow");
    }
    stack_[++sp_] = value;
}

int SimpleStackVM::pop() {
    if (sp_ < 0) {
        throw StackUnderflowError("Pop on empty stack");
    }
    return stack_[sp_--];
}

int SimpleStackVM::peek() const {
    if (sp_ < 0) {
        throw StackUnderflowError("Peek on empty stack");
    }
    return stack_[sp_];
}

SimpleStackVM::ParsedLine SimpleStackVM::parseLine(const std::string& line) {
    ParsedLine result;
    std::string trimmed = trim(line);
    
    // Skip empty lines and comments
    if (trimmed.empty() || trimmed[0] == ';') {
        return result;
    }
    
    // Remove inline comments
    size_t commentPos = trimmed.find(';');
    if (commentPos != std::string::npos) {
        trimmed = trimmed.substr(0, commentPos);
        trimmed = trim(trimmed);
    }
    
    if (trimmed.empty()) return result;
    
    // Check for label (ends with ':')
    size_t colonPos = trimmed.find(':');
    if (colonPos != std::string::npos) {
        result.label = toUpper(trimmed.substr(0, colonPos));
        trimmed = trim(trimmed.substr(colonPos + 1));
    }
    
    if (trimmed.empty()) return result;
    
    // Parse mnemonic and operand
    std::istringstream iss(trimmed);
    std::string mnemonic, operand;
    
    if (iss >> mnemonic) {
        result.mnemonic = toUpper(mnemonic);
        result.hasInstruction = true;
        
        if (iss >> operand) {
            result.operand = operand;
        }
    }
    
    return result;
}

Opcode SimpleStackVM::mnemonicToOpcode(const std::string& mnemonic) const {
    static const std::map<std::string, Opcode> opcodeMap{
        {"PSH", Opcode::PSH},
        {"ADD", Opcode::ADD},
        {"SUB", Opcode::SUB},
        {"MUL", Opcode::MUL},
        {"PRN", Opcode::PRN},
        {"SET", Opcode::SET},
        {"LD", Opcode::LD},
        {"JMP", Opcode::JMP},
        {"JZ", Opcode::JZ},
        {"HLT", Opcode::HLT}
    };
    
    auto it = opcodeMap.find(toUpper(mnemonic));
    if (it != opcodeMap.end()) {
        return it->second;
    }
    return Opcode::UNKNOWN;
}

bool SimpleStackVM::opcodeHasOperand(Opcode op) const {
    return op == Opcode::PSH || op == Opcode::SET || op == Opcode::LD || 
           op == Opcode::JMP || op == Opcode::JZ;
}

bool SimpleStackVM::isInteger(const std::string& s) const {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (start >= s.length()) return false;
    return std::all_of(s.begin() + start, s.end(), ::isdigit);
}

std::vector<std::string> SimpleStackVM::splitLines(const std::string& source) {
    std::vector<std::string> lines;
    std::istringstream iss(source);
    std::string line;
    
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::string SimpleStackVM::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::string SimpleStackVM::toUpper(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}
