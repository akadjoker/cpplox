#pragma once

#include "value.h"

#define MAX_PROCESSES 1024

struct Process {
    enum State {
        FREE,              // Slot vazio
        RUNNING,           // Executando
        WAITING_FRAME,     // Esperando próximo frame
        DEAD               // Morto (reciclar)
    } state;
    
    // Execution context
    uint16_t function_id;      // Qual função executar
    int pc;                    // Program counter (onde parou)
    Value locals[256];         // Stack local salvo
    int local_count;           // Quantas locals tem
    
    // Frame control
    int frame_percentage;      // 0-∞ (velocidade)
    int frame_counter;         // Contador de frames
    
    // Game data
    int id;                    // ID único
    int x, y;                  // Posição
    int sprite_id;             // Sprite (-1 = sem sprite)
    int type;                  // Tipo (para collision)
    
    Process() : state(FREE), id(-1), pc(0), local_count(0),
                frame_percentage(100), frame_counter(0),
                x(0), y(0), sprite_id(-1), type(0) {}
    
    // Quantas vezes executar este frame?
    int timesToRun() const {
        if (frame_percentage == 0) {
            return 0;  // Pausado
        }
        
        if (frame_percentage >= 100) {
            // Rápido: múltiplas execuções
            return frame_percentage / 100;
        } else {
            // Lento: skip frames
            int skip_rate = 100 / frame_percentage;
            if (skip_rate == 0) skip_rate = 1;
            return (frame_counter % skip_rate) == 0 ? 1 : 0;
        }
    }
};