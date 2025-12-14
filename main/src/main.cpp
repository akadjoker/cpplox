#include "vm.h"
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>     
#include <cstdlib>      

// class REPL
// {
// private:
//     VM vm_;
//     bool running_;

// public:
//     REPL() : running_(true) {}

//     void run()
//     {
//         printWelcome();

//         std::string line;
//         std::string multiline;
//         bool in_multiline = false;

//         while (running_)
//         {
//             // Prompt
//             if (in_multiline)
//             {
//                 std::cout << "... ";
//             }
//             else
//             {
//                 std::cout << ">>> ";
//             }

//             // Lê linha
//             if (!std::getline(std::cin, line))
//             {
//                 break; // EOF (Ctrl+D)
//             }

//             // Trim whitespace
//             line = trim(line);

//             // Comandos especiais
//             if (!in_multiline && handleCommand(line))
//             {
//                 continue;
//             }

//             // Detecta multiline (function, if, while, etc)
//             if (needsMultiline(line))
//             {
//                 in_multiline = true;
//                 multiline = line + "\n";
//                 continue;
//             }

//             // Continua multiline
//             if (in_multiline)
//             {
//                 multiline += line + "\n";

//                 // Verifica se acabou (linha vazia ou fecha bracket)
//                 if (line.empty() || (line == "}" && countBraces(multiline) == 0))
//                 {
//                     execute(multiline);
//                     multiline.clear();
//                     in_multiline = false;
//                 }
//                 continue;
//             }

//             // Single line
//             execute(line);
//         }

//         std::cout << "\nBye! 👋\n";
//     }

// private:
//     void printWelcome()
//     {
//         std::cout << "╔════════════════════════════════════════╗\n";
//         std::cout << "║     Bytecode VM - Interactive REPL     ║\n";
//         std::cout << "║     Type .help for commands            ║\n";
//         std::cout << "╚════════════════════════════════════════╝\n";
//         std::cout << "\n";
//     }

//     bool handleCommand(const std::string &line)
//     {
//         if (line.empty())
//         {
//             return true;
//         }

//         if (line == ".help" || line == ".h")
//         {
//             printHelp();
//             return true;
//         }

//         if (line == ".exit" || line == ".quit" || line == ".q")
//         {
//             running_ = false;
//             return true;
//         }

//         if (line == ".clear" || line == ".c")
//         {
//             clearScreen();
//             return true;
//         }

//         if (line.rfind(".load ", 0) == 0)
//         {
//             std::string filename = line.substr(6);
//             loadFile(filename);
//             return true;
//         }

//         return false;
//     }

//     void printHelp()
//     {
//         std::cout << "Commands:\n";
//         std::cout << "  .help, .h       Show this help\n";
//         std::cout << "  .exit, .quit    Exit REPL\n";
//         std::cout << "  .clear, .c      Clear screen\n";
//         std::cout << "\n";
//     }

//     void clearScreen()
//     {
// #ifdef _WIN32
//         system("cls");
// #else
//         system("clear");
// #endif
//         printWelcome();
//     }

//     void loadFile(const std::string &filename)
//     {
//         std::ifstream file(filename);
//         if (!file)
//         {
//             std::cout << "Error: Could not open file '" << filename << "'\n";
//             return;
//         }

//         std::string code((std::istreambuf_iterator<char>(file)),
//                          std::istreambuf_iterator<char>());

//         execute(code);
//     }

//     void execute(const std::string &code)
//     {
//         if (code.empty())
//             return;

      

 
//                vm_.interpret(code);

        
 
//     }

//     bool needsMultiline(const std::string &line)
//     {
//         // Detecta início de bloco
//         return line.find("def ") == 0 ||
//                line.find("if ") == 0 ||
//                line.find("elif ") == 0 ||
//                line.find("else") == 0 ||
//                line.find("while ") == 0 ||
//                line.find("for ") == 0 ||
//                line.find("switch ") == 0 ||
//                (line.find("{") != std::string::npos &&
//                 line.find("}") == std::string::npos);
//     }

//     int countBraces(const std::string &code)
//     {
//         int count = 0;
//         for (char c : code)
//         {
//             if (c == '{')
//                 count++;
//             if (c == '}')
//                 count--;
//         }
//         return count;
//     }

//     std::string trim(const std::string &str)
//     {
//         size_t first = str.find_first_not_of(" \t\n\r");
//         if (first == std::string::npos)
//             return "";
//         size_t last = str.find_last_not_of(" \t\n\r");
//         return str.substr(first, last - first + 1);
//     }
// };

int main(int argc, char **argv)
{
    // if (argc > 1)
    // {
        // Modo script
        VM vm;
        std::ifstream file("main.cc");
        std::string code((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
        vm.interpret(code);
    // }
    // else
    // {
    //     // Modo REPL
    //     REPL repl;
    //     repl.run();
    // }

    return 0;
}