#include "SSVM.hpp"
#include "Repl.hpp"
#include <iostream>
#include <string>
#include <cstring>

void runDemo() {
    std::cout << "\n=== Demo Mode: Running Example Programs ===\n\n";
    
    // Example 1: Basic Arithmetic (10 * 5) + 20 = 70
    std::cout << "--- Example 1: Basic Arithmetic (10 * 5) + 20 = 70 ---\n";
    {
        SimpleStackVM vm;
        std::string program = R"(
            PSH 10
            PSH 5
            MUL
            PSH 20
            ADD
            SET 0
            LD 0
            PRN
            HLT
        )";
        
        try {
            vm.assembleAndLoad(program);
            vm.run(true);
            std::cout << "\n";
        } catch (const VMException& e) {
            std::cout << "ERROR: " << e.what() << "\n\n";
        }
    }
    
    // Example 2: Multiple Registers (15 + 27 = 42)
    std::cout << "--- Example 2: Multiple Registers (15 + 27 = 42) ---\n";
    {
        SimpleStackVM vm;
        std::string program = R"(
            PSH 15
            SET 0
            PSH 27
            SET 1
            LD 0
            LD 1
            ADD
            SET 2
            LD 2
            PRN
            HLT
        )";
        
        try {
            vm.assembleAndLoad(program);
            vm.run(true);
            std::cout << "\n";
        } catch (const VMException& e) {
            std::cout << "ERROR: " << e.what() << "\n\n";
        }
    }
    
    // Example 3: Countdown Loop
    std::cout << "--- Example 3: Countdown Loop (5 to 1) ---\n";
    {
        SimpleStackVM vm;
        std::string program = R"(
            PSH 5
            SET 0
            LOOP: LD 0
            PRN
            LD 0
            PSH 1
            SUB
            SET 0
            LD 0
            JZ DONE
            JMP LOOP
            DONE: HLT
        )";
        
        try {
            vm.assembleAndLoad(program);
            vm.run(true);
            std::cout << "\n";
        } catch (const VMException& e) {
            std::cout << "ERROR: " << e.what() << "\n\n";
        }
    }
    
    // Example 4: Factorial (5! = 120)
    std::cout << "--- Example 4: Factorial (5! = 120) ---\n";
    {
        SimpleStackVM vm;
        std::string program = R"(
            PSH 1
            SET 0
            PSH 5
            SET 1
            LOOP: LD 0
            LD 1
            MUL
            SET 0
            LD 1
            PSH 1
            SUB
            SET 1
            LD 1
            JZ DONE
            JMP LOOP
            DONE: LD 0
            PRN
            HLT
        )";
        
        try {
            vm.assembleAndLoad(program);
            vm.run(true);
            std::cout << "\n";
        } catch (const VMException& e) {
            std::cout << "ERROR: " << e.what() << "\n\n";
        }
    }
    
    // Example 5: Sum of 1 to N (Triangular Number 1+2+...+10 = 55)
    std::cout << "--- Example 5: Sum of 1 to 10 (Triangular Number = 55) ---\n";
    {
        SimpleStackVM vm;
        std::string program = R"(
            PSH 0
            SET 0
            PSH 10
            SET 1
            LOOP: LD 0
            LD 1
            ADD
            SET 0
            LD 1
            PSH 1
            SUB
            SET 1
            LD 1
            JZ DONE
            JMP LOOP
            DONE: LD 0
            PRN
            HLT
        )";
        
        try {
            vm.assembleAndLoad(program);
            vm.run(true);
            std::cout << "\n";
        } catch (const VMException& e) {
            std::cout << "ERROR: " << e.what() << "\n\n";
        }
    }
    
    std::cout << "=== Demo Complete ===\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc > 1 && std::strcmp(argv[1], "--demo") == 0) {
            runDemo();
        } else {
            SSVMRepl repl;
            repl.start();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
