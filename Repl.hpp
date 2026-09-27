#ifndef REPL_HPP
#define REPL_HPP

#include "SSVM.hpp"
#include <string>
#include <memory>

class SSVMRepl {
public:
    explicit SSVMRepl(size_t maxBufferChars = 8192);
    
    void start();
    
private:
    std::unique_ptr<SimpleStackVM> vm_;
    std::string buffer_;
    int lineNumber_;
    size_t maxBufferChars_;
    
    void handleRun();
    void handleClear();
    bool handleCommand(const std::string& upperLine);
    void printHelp();
};

#endif // REPL_HPP
