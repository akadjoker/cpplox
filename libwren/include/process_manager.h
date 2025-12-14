
#pragma once

#include "process.h"

#include <cstdio>

class VM;

class ProcessManager
{
private:
    Process processes_[MAX_PROCESSES];
    int next_id_;

public:
    ProcessManager() : next_id_(0)
    {
        for (int i = 0; i < MAX_PROCESSES; i++)
        {
            processes_[i].state = Process::FREE;
        }
    }

    // Spawna novo processo
    int spawn(uint16_t function_id, int x, int y)
    {
        // Procura slot livre
        for (int i = 0; i < MAX_PROCESSES; i++)
        {
            if (processes_[i].state == Process::FREE)
            {
                Process &proc = processes_[i];

                proc.state = Process::RUNNING;
                proc.id = next_id_++;
                proc.function_id = function_id;
                proc.pc = 0;
                proc.x = x;
                proc.y = y;
                proc.sprite_id = -1;
                proc.type = 0;
                proc.local_count = 0;
                proc.frame_percentage = 100;
                proc.frame_counter = 0;

                return proc.id;
            }
        }

        fprintf(stderr, "ERROR: Max processes (%d) reached!\n", MAX_PROCESSES);
        return -1;
    }

    // Get processo por ID
    Process *get(int id)
    {
        for (int i = 0; i < MAX_PROCESSES; i++)
        {
            if (processes_[i].id == id && processes_[i].state != Process::FREE)
            {
                return &processes_[i];
            }
        }
        return nullptr;
    }

    // Mata processo
    void kill(int id)
    {
        Process *proc = get(id);
        if (proc)
        {
            proc->state = Process::DEAD;
        }
    }

    // Limpa processos mortos
    void cleanup()
    {
        for (int i = 0; i < MAX_PROCESSES; i++)
        {
            if (processes_[i].state == Process::DEAD)
            {
                processes_[i].state = Process::FREE;
            }
        }
    }

    // Update - executa todos os processos
    void update(VM *vm); // Implementa no .cpp

    // Debug
    int count() const
    {
        int total = 0;
        for (int i = 0; i < MAX_PROCESSES; i++)
        {
            if (processes_[i].state != Process::FREE)
            {
                total++;
            }
        }
        return total;
    }

    // Acesso direto ao array (para render, collision, etc)
    Process *getAll()
    {
        return processes_;
    }
};