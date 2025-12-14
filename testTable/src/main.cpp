#include <iostream>
#include <chrono>
#include <unordered_map>
#include <random>
#include <iomanip>
#include <vector>
#include <string>
#include "table.h"
#include "symboltable_table.h"

// Cores para output
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

using namespace std::chrono;

class Timer
{
    high_resolution_clock::time_point start;
public:
    Timer() : start(high_resolution_clock::now()) {}
    
    double elapsed_ms() const
    {
        auto end = high_resolution_clock::now();
        return duration_cast<microseconds>(end - start).count() / 1000.0;
    }
    
    void reset() { start = high_resolution_clock::now(); }
};

void print_header(const std::string& title)
{
    std::cout << "\n" << BOLD << CYAN << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  " << title << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << "\n";
}

void print_result(const std::string& test_name, double table_time, double std_time)
{
    double speedup = std_time / table_time;
    std::string color = speedup > 1.0 ? GREEN : (speedup > 0.8 ? YELLOW : RED);
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  " << std::setw(35) << std::left << test_name;
    std::cout << "  Table: " << CYAN << std::setw(10) << table_time << "ms" << RESET;
    std::cout << "  Std: " << std::setw(10) << std_time << "ms";
    std::cout << "  " << color << std::setw(8) << speedup << "x" << RESET;
    
    if (speedup > 1.0)
        std::cout << " " << GREEN << "✓" << RESET;
    else if (speedup < 0.8)
        std::cout << " " << RED << "⚠" << RESET;
    
    std::cout << "\n";
}

 
 
// ============================================================================
// MAIN
// ============================================================================
int main()
{
    std::cout << BOLD << MAGENTA;
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       LUA-STYLE TABLE vs STD::UNORDERED_MAP       ║\n";
    std::cout << "║            Performance Benchmark Suite            ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    std::cout << RESET;
     
 
    
    
    std::cout << "\n" << BOLD << GREEN;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  BENCHMARK COMPLETED\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << RESET;
    
    return 0;
}