#include "compiler.h"
#include "vm.h"
#include "debug.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>


// Execute expression and print result
void runExpression(const std::string &source)
{
   

    VM vm;
    InterpretResult result = vm.interpretExpression(source);

    if (result == InterpretResult::OK)
    {
        Value v = vm.Pop();
        printValue(v);
        std::cout << "\n";
    }
    else
    {
        std::cerr << "Runtime error\n";
    }
 
}

// Execute program (statements)
void runProgram(const std::string &source)
{
    
    VM vm;
    InterpretResult result = vm.interpret(source);

    if (result == InterpretResult::OK)
    {
        Value v = vm.Pop();
        printValue(v);
        std::cout << "\n";
    }
    else
    {
        std::cerr << "Runtime error\n";
    }
 
}

// Read file contents
std::string readFile(const char *path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "Could not open file: " << path << "\n";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main()
{
    std::string code = R"(
    
        def teste( )
        {
            print("Teste da function");
            
        }

        def sub(a,b)
        {
            return a - b;
        }

        def add(a,b)
        {
            return a + b;
        }
        var result = sub(10,5);
        print(result);
        var result2 = add(10,5);
        print(result2);
      
       

        var a = 1;
        var b = 2;
        var c = a + b;
        print(c);

        teste();

        
 
    
    
    )";
    runProgram(code);

    return 0;

    // if (argc < 2)
    // {
    //     printUsage(argv[0]);
    //     return 1;
    // }

    // std::string option = argv[1];

    // if (option == "-e" && argc == 3)
    // {
    //     // Expression mode
    //     runExpression(argv[2]);
    // }
    // else if (option == "-c" && argc == 3)
    // {
    //     // Code mode
    //     runProgram(argv[2]);
    // }
    // else if (option[0] != '-')
    // {
    //     // File mode
    //     std::string source = readFile(argv[1]);
    //     if (!source.empty())
    //     {
    //         runProgram(source);
    //     }
    // }
    // else
    // {
    //     printUsage(argv[0]);
    //     return 1;
    // }

    return 0;
}