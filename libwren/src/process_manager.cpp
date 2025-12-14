
#include "process_manager.h"
#include "vm.h"

void ProcessManager::update(VM *vm)
{
    // Loop por todos os processos
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        Process &proc = processes_[i];

        // Skip se não está ativo
        if (proc.state == Process::FREE || proc.state == Process::DEAD)
        {
            continue;
        }

        // Calcula quantas vezes executar
        int times = proc.timesToRun();

        // Executa N vezes
        for (int t = 0; t < times; t++)
        {
            // Executa até OP_FRAME ou terminar
            vm->executeProcess(&proc);

            // Se morreu, para
            if (proc.state == Process::DEAD)
            {
                break;
            }

            // Se yielded, reset para próxima iteração
            if (proc.state == Process::WAITING_FRAME)
            {
                proc.state = Process::RUNNING;
            }
        }

        // Incrementa contador
        proc.frame_counter++;
    }

    // Limpa mortos
    cleanup();
}