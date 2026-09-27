# SSVM C++17 Implementation Details

A deep dive into the object-oriented design and implementation of the Simple Stack Virtual Machine.

## Project Structure

```
SSVM (Simple Stack Virtual Machine)
├── Core VM Class
│   ├── SSVM.hpp (Class declaration)
│   └── SSVM.cpp (Implementation)
├── REPL Interface
│   ├── Repl.hpp (Class declaration)
│   └── Repl.cpp (Interactive shell)
└── Entry Point
    └── main.cpp (Startup & demos)
```

Total lines of code: ~700 lines (including comments)

## Core Classes

### 1. SimpleStackVM (The Virtual Machine)

**Header: SSVM.hpp**

```cpp
class SimpleStackVM {
public:
    explicit SimpleStackVM(size_t memorySize = 1024, 
                          size_t stackSize = 256);
    
    int assembleAndLoad(const std::string& source);
    void run(bool trace = true);
    void reset();
    
    // Read-only accessors
    int registerValue(int reg) const;
    int peekMemory(int address) const;
    int stackTop() const;
    bool isHalted() const;

private:
    // ... (private members and methods)
};
```

**Responsibilities:**
- ✅ Manage VM state (memory, stack, registers)
- ✅ Parse assembly language
- ✅ Execute bytecode
- ✅ Handle errors gracefully

**Key Design Decisions:**

1. **Encapsulation**: All state is private
   - `memory_`, `stack_`, `registers_` are not directly accessible
   - Access through controlled public interface
   - Prevents external code from corrupting VM state

2. **Exception-Safe Operations**: Every operation can throw
   - `push()` → `StackOverflowError`
   - `pop()` → `StackUnderflowError`
   - `assembleAndLoad()` → `AssemblyError`
   - Callers use try/catch for error handling

3. **STL Containers over Raw Arrays**
   - `std::vector<int> memory_` - Dynamic sizing, bounds checking
   - `std::vector<int> stack_` - RAII automatic cleanup
   - `std::array<int, 26> registers_` - Fixed size, stack allocated
   - Replaces C arrays which could leak or overflow

4. **Enum Class for Type Safety**
   ```cpp
   enum class Opcode {
       PSH = 0, ADD = 1, ... HLT = 9, UNKNOWN = 255
   };
   ```
   - Prevents accidental int→Opcode conversions
   - Compiler catches type mismatches
   - Original C enum couldn't do this

### 2. Two-Pass Assembler (Private Methods)

**Implementation: SSVM.cpp**

The assembler converts human-readable assembly to bytecode:

```
Pass 1: Scan for labels
┌─────────────────────────────────────┐
│ PSH 10                              │
│ PSH 5                               │
│ LOOP: MUL        ← Discovered!      │
│ JMP LOOP         ← References it    │
│ HLT                                 │
└─────────────────────────────────────┘

Label Table: {LOOP → address 4}

Pass 2: Generate bytecode using label table
┌─────────────────────────────────────┐
│ [0] = 0 (PSH)                       │
│ [1] = 10                            │
│ [2] = 0 (PSH)                       │
│ [3] = 5                             │
│ [4] = 3 (MUL)        ← Label "LOOP" │
│ [5] = 7 (JMP)                       │
│ [6] = 4              ← Resolved!    │
│ [7] = 9 (HLT)                       │
└─────────────────────────────────────┘
```

**Key Features:**

1. **firstPass() - Label Discovery**
   ```cpp
   void SimpleStackVM::firstPass(const std::vector<std::string>& lines,
                                 std::map<std::string, int>& labelTable) {
       int address = 0;
       for (const auto& raw : lines) {
           ParsedLine parsed = parseLine(raw);
           
           // If this line defines a label, record its address
           if (!parsed.label.empty()) {
               labelTable[parsed.label] = address;
           }
           
           // Count bytecode words (1 or 2 depending on opcode)
           if (parsed.hasInstruction) {
               Opcode op = mnemonicToOpcode(parsed.mnemonic);
               address += opcodeHasOperand(op) ? 2 : 1;
           }
       }
   }
   ```

2. **secondPass() - Bytecode Emission**
   ```cpp
   // Emit the opcode
   memory_.at(count++) = static_cast<int>(op);
   
   // If opcode has operand, resolve it
   if (opcodeHasOperand(op)) {
       int value;
       if (isInteger(parsed.operand)) {
           value = std::stoi(parsed.operand);  // Direct number
       } else {
           auto it = labelTable.find(parsed.operand);
           if (it == labelTable.end())
               throw AssemblyError(...);  // Label not found!
           value = it->second;  // Resolved address
       }
       memory_.at(count++) = value;
   }
   ```

3. **ParsedLine Helper Struct**
   ```cpp
   struct ParsedLine {
       std::string label;           // "LOOP:" → "LOOP"
       std::string mnemonic;        // "PSH", "JMP", etc
       std::string operand;         // "10", "LOOP", etc
       bool hasInstruction = false; // Is this a valid instruction?
   };
   ```
   - Returned by value (small object, moves efficiently)
   - Groups related data together
   - Makes parsing testable in isolation

### 3. Stack & Registers (Private Operations)

**Stack Operations:**

```cpp
void SimpleStackVM::push(int value) {
    if (sp_ + 1 >= static_cast<int>(stack_.size())) {
        throw StackOverflowError("Stack overflow");
    }
    stack_[++sp_] = value;  // Increment, then store
}

int SimpleStackVM::pop() {
    if (sp_ < 0) {
        throw StackUnderflowError("Pop on empty stack");
    }
    return stack_[sp_--];  // Return, then decrement
}

int SimpleStackVM::peek() const {
    if (sp_ < 0) {
        throw StackUnderflowError("Peek on empty stack");
    }
    return stack_[sp_];  // Just read, no modify
}
```

**Why Private?**
- Only `run()` can push/pop
- Prevents external code from cheating
- Maintains VM invariant: sp_ always points to logical top

**Register Access:**

```cpp
// In SET opcode:
registers_.at(reg) = pop();  // Bounds-checked access

// In LD opcode:
push(registers_.at(reg));    // at() throws if out of range

// Public read-only accessor:
int SimpleStackVM::registerValue(int reg) const {
    if (reg < 0 || reg >= 26)
        throw std::out_of_range("...");
    return registers_.at(reg);
}
```

### 4. Instruction Execution (The Main Loop)

**run() Method Overview:**

```cpp
void SimpleStackVM::run(bool trace = true) {
    halted_ = false;
    sp_ = -1;  // Reset stack
    
    while (pc_ < loadedLength_ && !halted_) {
        // 1. Fetch
        int opcode = memory_.at(pc_++);
        Opcode op = static_cast<Opcode>(opcode);
        
        // 2. Decode & Execute (via switch)
        switch (op) {
            case Opcode::PSH: {
                int value = memory_.at(pc_++);
                push(value);
                if (trace) std::cout << "OP: PUSH " << value << "\n";
                break;
            }
            // ... 9 more opcodes ...
        }
    }
}
```

**Fetch-Decode-Execute Cycle:**
1. Read opcode from memory at pc_
2. Match against enum class Opcode
3. Execute appropriate logic
4. Update pc_ (and sp_ as needed)
5. Print trace if requested

**Example: ADD Instruction**

```cpp
case Opcode::ADD: {
    int b = pop();        // Second value (top of stack)
    int a = pop();        // First value (below b)
    push(a + b);          // Push result
    if (trace) 
        std::cout << "OP: ADD (" << a << " + " << b << ")\n";
    break;
}
```

**Example: JZ Instruction (Conditional Jump)**

```cpp
case Opcode::JZ: {
    int addr = memory_.at(pc_++);  // Get target address
    int value = pop();              // Check condition
    
    if (value == 0) {
        pc_ = addr;                 // Jump!
        if (trace) std::cout << "OP: JZ (true, jump)\n";
    } else {
        if (trace) std::cout << "OP: JZ (false, continue)\n";
    }
    break;
}
```

### 5. Exception Hierarchy

```cpp
std::runtime_error
    │
    └── VMException (all SSVM errors derive here)
        │
        ├── AssemblyError
        │   └── Thrown during assembleAndLoad()
        │       - Unknown instruction
        │       - Unknown label
        │
        ├── StackOverflowError
        │   └── Thrown by push()
        │       - Too many values pushed
        │
        ├── StackUnderflowError
        │   └── Thrown by pop()/peek()
        │       - Popping from empty stack
        │
        └── InvalidOpcodeError
            └── Thrown during run()
                - Unknown opcode byte
                - PC out of bounds
```

**Why Hierarchy?**

```cpp
try {
    vm.assembleAndLoad(source);
    vm.run(false);
} catch (const AssemblyError& e) {
    // Caught specifically - syntax error
    handleAssemblyError(e);
} catch (const VMException& e) {
    // Caught at parent level - any runtime error
    handleRuntimeError(e);
}
```

Callers can handle errors at different granularity levels.

### 6. SSVMRepl - Interactive Interface

**Design: Composition over Inheritance**

```cpp
class SSVMRepl {
private:
    std::unique_ptr<SimpleStackVM> vm_;  // HAS-A relationship
    std::string buffer_;
    int lineNumber_;
};
```

**Why std::unique_ptr?**
- Automatic cleanup when REPL destroyed
- Single ownership (clear responsibility)
- Exception-safe (RAII principle)
- No manual delete needed

**REPL Loop:**

```cpp
void SSVMRepl::start() {
    // 1. Prompt user
    std::cout << "[" << lineNumber_ << "] ";
    
    // 2. Read input
    std::string input;
    std::getline(std::cin, input);
    
    // 3. Check if command
    if (input == "RUN") {
        handleRun();       // Compile & execute
    } else if (input == "CLEAR") {
        handleClear();     // Reset everything
    } else if (input == "EXIT") {
        return;            // Exit loop
    } else {
        buffer_ += input;  // Buffer instruction
        lineNumber_++;
    }
    // 4. Repeat
}
```

**Error Recovery:**

```cpp
void SSVMRepl::handleRun() {
    try {
        vm_->assembleAndLoad(buffer_);
        vm_->run(true);
    } catch (const AssemblyError& e) {
        std::cout << "ASSEMBLY ERROR: " << e.what() << "\n";
        // Buffer retained - user can debug
    } catch (const VMException& e) {
        std::cout << "VM ERROR: " << e.what() << "\n";
        // Program dies gracefully, REPL continues
    }
}
```

The REPL never crashes - exceptions are caught and reported.

## Helper Functions

### String Processing

```cpp
std::string SimpleStackVM::toUpper(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), 
                   result.begin(), ::toupper);
    return result;
}
```

**Modern C++ approach:**
- Uses `std::transform` algorithm instead of for loop
- `::toupper` is a function pointer to standard library
- More idiomatic, clearer intent

### Parsing

```cpp
SimpleStackVM::ParsedLine SimpleStackVM::parseLine(
        const std::string& line) {
    
    ParsedLine result;
    std::string trimmed = trim(line);  // Remove whitespace
    
    // Skip empty lines and comments
    if (trimmed.empty() || trimmed[0] == ';') return result;
    
    // Remove inline comments: "PSH 10 ; push ten"
    size_t commentPos = trimmed.find(';');
    if (commentPos != std::string::npos) {
        trimmed = trimmed.substr(0, commentPos);
    }
    
    // Check for label (ends with ':')
    size_t colonPos = trimmed.find(':');
    if (colonPos != std::string::npos) {
        result.label = toUpper(trimmed.substr(0, colonPos));
        trimmed = trimmed.substr(colonPos + 1);
    }
    
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
```

**Key Techniques:**
- `std::string::find()` for substring search
- `std::string::substr()` for extraction
- `std::istringstream` for tokenization
- Case-insensitive by converting to uppercase

## Memory Layout

### During Assembly

```
Source Code:
    PSH 10
    PSH 5
    MUL
    PRN
    HLT

After assembleAndLoad():
memory_: [0, 10, 0, 5, 3, 4, 9]
         │  │  │  │  │  │  └─ HLT
         │  │  │  │  │  └───── PRN
         │  │  │  │  └──────── MUL
         │  │  │  └─────────── Operand 5
         │  │  └────────────── PSH
         │  └───────────────── Operand 10
         └──────────────────── PSH
```

### During Execution

```
Initial: pc=0, sp=-1
Stack: []

After PSH 10: pc=2, sp=0
Stack: [10]

After PSH 5: pc=4, sp=1
Stack: [10, 5]

After MUL: pc=5, sp=0
Stack: [50]

After PRN: pc=6, sp=0
Stack: [50]  (Print outputs: 50)

After HLT: pc=7, halted=true
```

## Type Safety Improvements Over C

| C Version | C++17 Version | Why Better |
|-----------|---------------|-----------|
| `#define PSH 0` | `enum class Opcode { PSH = 0 }` | Type-checked, scoped |
| `int stack[256]` | `std::vector<int> stack_` | Bounds-checked, dynamic |
| `malloc/free` | RAII (std::vector) | No memory leaks |
| `int regs[26]` | `std::array<int, 26>` | Stack-allocated, bounds-checked |
| Global state | Class members (private) | Encapsulated, testable |
| Silent errors | Exception hierarchy | Explicit error handling |
| Manual parsing | String methods + parsing | Robust tokenization |

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `push()` | O(1) | Amortized (vector) |
| `pop()` | O(1) | Just decrement pointer |
| `assembleAndLoad()` | O(n) | Two passes over source |
| `run()` | O(m) | m = instructions executed |
| Label lookup | O(log n) | std::map overhead |
| `at()` bounds check | O(1) | Array access + assert |

## Design Patterns Used

### 1. RAII (Resource Acquisition Is Initialization)
```cpp
SimpleStackVM vm;  // Constructor allocates memory
// ...use vm...
// Destructor automatically frees vectors
```

### 2. Composition (Has-A)
```cpp
class SSVMRepl {
    std::unique_ptr<SimpleStackVM> vm_;  // Owns a VM
};
```

### 3. Exception Safety
```cpp
try {
    dangerous_operation();
} catch (const SpecificException& e) {
    handle_specific(e);
} catch (const BaseException& e) {
    handle_generic(e);
}
```

### 4. Value Types (Returned by Value)
```cpp
ParsedLine result;     // Allocated on stack
return result;         // Moved, not copied (C++17)
```

### 5. Enum Class (Strongly-Typed Enum)
```cpp
enum class Opcode { ... };  // Can't implicitly convert to int
Opcode op = static_cast<Opcode>(42);  // Explicit cast required
```

## Extending the VM

### Adding a DIV Instruction

**1. Update enum:**
```cpp
enum class Opcode {
    PSH = 0, ADD = 1, SUB = 2, MUL = 3, DIV = 4,  // Add here
    PRN = 5, SET = 6, LD = 7, JMP = 8, JZ = 9, HLT = 10
};
```

**2. Add exception:**
```cpp
class DivisionByZeroError : public VMException {
public:
    explicit DivisionByZeroError(const std::string& msg) 
        : VMException(msg) {}
};
```

**3. Add to mnemonic map:**
```cpp
static const std::map<std::string, Opcode> opcodeMap{
    // ...
    {"DIV", Opcode::DIV},
};
```

**4. Update opcodeHasOperand:**
```cpp
bool SimpleStackVM::opcodeHasOperand(Opcode op) const {
    return op == Opcode::PSH || /* ... */ ;  // DIV has no operand
}
```

**5. Add case in run():**
```cpp
case Opcode::DIV: {
    int b = pop();
    int a = pop();
    if (b == 0) throw DivisionByZeroError("Division by zero");
    push(a / b);
    if (trace) std::cout << "OP: DIV (" << a << " / " << b << ")\n";
    break;
}
```

**Type Safety in Action:**
- Compiler ensures all 5 steps are coordinated
- Forgetting any step causes a compile error
- The original C version could miss steps silently

## Conclusion

The C++17 implementation demonstrates that modern C++ provides:

✅ **Safety**: Type checking, bounds checking, no memory leaks  
✅ **Clarity**: Classes and exceptions express intent clearly  
✅ **Robustness**: Standard library containers handle edge cases  
✅ **Extensibility**: Adding features is coordinated and type-safe  
✅ **Teachability**: Shows core OOP principles in action  

All while producing the same computational results as the original C version! 🎯
