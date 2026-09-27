# SSVM Quick Start Guide

Get the Simple Stack Virtual Machine running in 2 minutes!

## Option 1: Run Pre-Compiled Binary (Fastest)

If you're on Linux/macOS, you can run the included compiled executable:

```bash
# Run interactive REPL
./ssvm

# Or run demo programs
./ssvm --demo
```

Done! ✓

## Option 2: Build from Source (30 seconds)

```bash
# Navigate to the directory
cd ssvm-cpp17

# Compile (requires g++ with C++17 support)
make

# Run
./ssvm --demo
```

Expected output:
```
=== Demo Mode: Running Example Programs ===

--- Example 1: Basic Arithmetic (10 * 5) + 20 = 70 ---
OP: PUSH 10
OP: PUSH 5
OP: MUL (10 * 5)
OP: PUSH 20
OP: ADD (50 + 20)
...
OUTPUT: 70
OP: HALT
```

## Option 3: Interactive REPL

```bash
./ssvm
```

```
========================================
Simple Stack VM (C++) -- Interactive REPL
========================================
Instructions : PSH n, ADD, SUB, MUL, PRN, SET reg, LD reg, HLT
Commands     : RUN, CLEAR, EXIT
Registers    : 0-25 (0=A, 1=B, ... 25=Z)
----------------------------------------

[1] psh 10
[2] psh 5
[3] mul
[4] prn
[5] hlt
[6] run
```

Output:
```
OP: PUSH 10
OP: PUSH 5
OP: MUL (10 * 5)
OP: PRINT
OUTPUT: 50
OP: HALT
VM HALTED
```

## First Program: Hello Calculator

Let's compute: (3 + 5) × 2

In the REPL:
```
[1] psh 3
[2] psh 5
[3] add
[4] psh 2
[5] mul
[6] prn
[7] hlt
[8] run
```

Expected: `OUTPUT: 16`

## Five Example Programs Included

All can be run with `./ssvm --demo`:

1. **Arithmetic**: (10 × 5) + 20 = **70**
2. **Registers**: 15 + 27 = **42**
3. **Loop**: Countdown 5 → 1
4. **Factorial**: 5! = **120**
5. **Sum**: 1+2+...+10 = **55**

## Key Commands

| Command | What it does |
|---------|--------------|
| `PSH n` | Push number n onto stack |
| `ADD` | Pop 2 numbers, push their sum |
| `SUB` | Pop 2 numbers, push difference |
| `MUL` | Pop 2 numbers, push product |
| `PRN` | Print top of stack |
| `SET n` | Pop stack, store in register n (0=A, 1=B, etc) |
| `LD n` | Push register n onto stack |
| `HLT` | Stop execution |
| `JMP addr` | Jump to address |
| `JZ addr` | Jump if top of stack is zero |

## Registers A-Z

Store values for later use:

```
[1] psh 42
[2] set 0           ; Store 42 in register A (index 0)
[3] ld 0            ; Load A back onto stack
[4] prn             ; Print
[5] hlt
[6] run
```

## Labels and Loops

```
[1] psh 3
[2] set 0           ; A = 3
[3] loop: ld 0
[4] prn             ; Print current
[5] ld 0
[6] psh 1
[7] sub             ; A - 1
[8] set 0
[9] ld 0
[10] jz done        ; If zero, jump to done
[11] jmp loop       ; Otherwise jump back
[12] done: hlt
[13] run
```

Output: `3 2 1`

## Troubleshooting

**"g++: command not found"**
- Install GCC: `sudo apt install build-essential` (Ubuntu/Debian)
- Or use macOS: `xcode-select --install`

**"make: command not found"**
- Install make: `sudo apt install make` (Ubuntu/Debian)

**Permission denied when running ./ssvm**
- Make executable: `chmod +x ssvm`

**Stack overflow error**
- You're pushing too many values without popping
- Every ADD/SUB/MUL needs 2 values on stack

**Unknown label error**
- Check label spelling matches exactly
- Labels end with `:` (e.g., `LOOP:` not `LOOP`)
- Labels are case-insensitive

## Next Steps

1. ✅ Run `./ssvm --demo` to see all examples
2. ✅ Try the REPL with `./ssvm` - experiment!
3. ✅ Read **README.md** for full documentation
4. ✅ Review the source code - it's well-commented C++17

## Need Help?

- **Usage**: See the REPL `HELP` command
- **Instructions**: Check the README Instruction Set table
- **Examples**: Run demo mode or review Example 1-5 in README
- **Errors**: Read Error Reference section in README

---

**You're ready! Open a terminal and run: `./ssvm --demo`**

Happy computing! 🚀
