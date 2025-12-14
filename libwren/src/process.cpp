#include "vm.h"
#include "process.h"
#include "stringpool.h"
#include "table.h"
#include "compiler.h"
#include <cstdio>
#include <cstdarg>

bool VM::executeProcess(Process *process)
{

    currentProcess_ = process;
    Function *func = functions_[process->function_id];
    if (!func)
    {
        runtimeError("Invalid function ID");
        currentProcess_ = nullptr;
        return false;
    }

    // Cria frame
    if (frameCount_ >= FRAMES_MAX)
    {
        runtimeError("Stack overflow in process");
        currentProcess_ = nullptr;
        return false;
    }

    CallFrame *frame = &frames_[frameCount_++];
    frame->function = func;
    frame->ip = func->chunk.code.data() + process->pc; // Restaura PC
    frame->slots = stackTop_;

    // Restaura locals
    for (int i = 0; i < process->local_count; i++)
    {
        push(process->locals[i]);
    }

    // ══════════════════════════════════════════════════

    for (;;)
    {
        if (hasFatalError_)
            return false;

        frame = &frames_[frameCount_ - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
#define READ_STRING_PTR() (frame->function->chunk.getStringPtr(READ_BYTE()))

        uint8_t instruction = READ_BYTE();

        switch (instruction)
        {
            // ════════════════════════════════════════
            // OPCODES ESPECIAIS DE PROCESSO
            // ════════════════════════════════════════

        case OP_FRAME:
        {
            Value percentVal = pop();
            int percentage = percentVal.isInt() ? percentVal.asInt() : 100;
            if (percentage < 0)
                percentage = 0;

            process->frame_percentage = percentage;
            process->pc = (int)(frame->ip - func->chunk.code.data());

            process->local_count = (int)(stackTop_ - frame->slots);
            for (int i = 0; i < process->local_count; i++)
            {
                process->locals[i] = frame->slots[i];
            }

            stackTop_ = frame->slots;
            process->state = Process::WAITING_FRAME;

            frameCount_--;
            currentProcess_ = nullptr;

            printf("[DEBUG] OP_FRAME executado! Process ID=%d\n", process->id);

            return true; // YIELD!
        }

        case OP_END_PROCESS:
        {
            process->state = Process::DEAD;
            stackTop_ = frame->slots;
            frameCount_--;
            currentProcess_ = nullptr;

            printf("[DEBUG] OP_END_PROCESS executado! Process ID=%d\n", process->id);

            return true;
        }

            // ════════════════════════════════════════
            // RETURN - Retorna de função dentro de processo
            // ════════════════════════════════════════

        case OP_RETURN:
        {
            Value result = pop();

            stackTop_ = frame->slots;
            frameCount_--;

            // Se não há mais frames, processo terminou
            if (frameCount_ == 0)
            {
                process->state = Process::DEAD;
                currentProcess_ = nullptr;

// #undef READ_BYTE
// #undef READ_SHORT
// #undef READ_CONSTANT
// #undef READ_STRING_PTR

                return true;
            }

            // Retorna de função chamada, continua
            push(result);
            frame = &frames_[frameCount_ - 1];
            break;
        }

        // ════════════════════════════════════════
        // OPCODES normais
        // ════════════════════════════════════════
        case OP_CONSTANT:
        {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }

        case OP_NIL:
            push(Value::makeNull());
            break;

        case OP_TRUE:
            push(Value::makeBool(true));
            break;

        case OP_FALSE:
            push(Value::makeBool(false));
            break;

        case OP_POP:
            pop();
            break;
        case OP_NOT:
        {
            Value v = pop();
            push(Value::makeBool(!isTruthy(v)));
            break;
        }
        case OP_ADD:
        {
            Value b = pop();
            Value a = pop();

            // String concatenation
            if (a.isString() && b.isString())
            {
                const char *result = StringPool::instance().concat(a.asString(), b.asString());
                push(Value::makeString(result));
            }
            // Int + Int = Int
            else if (a.isInt() && b.isInt())
            {
                push(Value::makeInt(a.asInt() + b.asInt()));
            }
            // Double + Double = Double
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeDouble(a.asDouble() + b.asDouble()));
            }
            // Int + Double = Double
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeDouble(a.asInt() + b.asDouble()));
            }
            // Double + Int = Double
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeDouble(a.asDouble() + b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers or strings");
                return false;
            }
            break;
        }

        case OP_SUBTRACT:
        {
            Value b = pop();
            Value a = pop();

            if (a.isInt() && b.isInt())
            {
                push(Value::makeInt(a.asInt() - b.asInt()));
            }
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeDouble(a.asDouble() - b.asDouble()));
            }
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeDouble(a.asInt() - b.asDouble()));
            }
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeDouble(a.asDouble() - b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers");
                return false;
            }
            break;
        }

        case OP_MULTIPLY:
        {
            Value b = pop();
            Value a = pop();

            if (a.isInt() && b.isInt())
            {
                push(Value::makeInt(a.asInt() * b.asInt()));
            }
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeDouble(a.asDouble() * b.asDouble()));
            }
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeDouble(a.asInt() * b.asDouble()));
            }
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeDouble(a.asDouble() * b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers");
                return false;
            }
            break;
        }

        case OP_DIVIDE:
        {
            Value b = pop();
            Value a = pop();

            // Check division by zero
            if ((b.isInt() && b.asInt() == 0) ||
                (b.isDouble() && b.asDouble() == 0.0))
            {
                runtimeError("Division by zero");
                return false;
            }

            if (a.isInt() && b.isInt())
            {
                push(Value::makeInt(a.asInt() / b.asInt()));
            }
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeDouble(a.asDouble() / b.asDouble()));
            }
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeDouble(a.asInt() / b.asDouble()));
            }
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeDouble(a.asDouble() / b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers");
                return false;
            }
            break;
        }
        case OP_MODULO:
        {
            Value b = pop();
            Value a = pop();
            if (a.isInt() && b.isInt())
            {
                push(Value::makeInt(a.asInt() % b.asInt()));
            }
            else
            {
                runtimeError("Operands must be integers");
                return false;
            }
            break;
        }

        case OP_NEGATE:
        {
            Value a = pop();
            if (a.isInt())
            {
                push(Value::makeInt(-a.asInt()));
            }
            else if (a.isDouble())
            {
                push(Value::makeDouble(-a.asDouble()));
            }
            else
            {
                runtimeError("Operand must be a number");
                return false;
            }
            break;
        }

        case OP_EQUAL:
        {
            Value b = pop();
            Value a = pop();

            if (a.type != b.type)
            {
                push(Value::makeBool(false));
            }
            else if (a.isInt())
            {
                push(Value::makeBool(a.asInt() == b.asInt()));
            }
            else if (a.isBool())
            {
                push(Value::makeBool(a.asBool() == b.asBool()));
            }
            else if (a.isNull())
            {
                push(Value::makeBool(true));
            }
            else if (a.isString())
            {
                push(Value::makeBool(a.asString() == b.asString()));
            }
            else if (a.isDouble())
            {
                push(Value::makeBool(a.asDouble() == b.asDouble()));
            }
            else
            {
                push(Value::makeBool(false));
            }
            break;
        }

        case OP_NOT_EQUAL:
        {
            Value b = pop();
            Value a = pop();

            if (a.type != b.type)
            {
                push(Value::makeBool(true));
            }
            else if (a.isInt())
            {
                push(Value::makeBool(a.asInt() != b.asInt()));
            }
            else if (a.isBool())
            {
                push(Value::makeBool(a.asBool() != b.asBool()));
            }
            else if (a.isString())
            {
                push(Value::makeBool(a.asString() != b.asString()));
            }
            else if (a.isDouble())
            {
                push(Value::makeBool(a.asDouble() != b.asDouble()));
            }
            else
            {
                push(Value::makeBool(false));
            }
            break;
        }

        case OP_GREATER:
        {
            Value b = pop();
            Value a = pop();

            if (a.isInt() && b.isInt())
            {
                push(Value::makeBool(a.asInt() > b.asInt()));
            }
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeBool(a.asDouble() > b.asDouble()));
            }
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeBool(a.asInt() > b.asDouble()));
            }
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeBool(a.asDouble() > b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers");
                return false;
            }
            break;
        }

        case OP_GREATER_EQUAL:
        {
            Value b = pop();
            Value a = pop();

            if (a.isInt() && b.isInt())
            {
                push(Value::makeBool(a.asInt() >= b.asInt()));
            }
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeBool(a.asDouble() >= b.asDouble()));
            }
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeBool(a.asInt() >= b.asDouble()));
            }
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeBool(a.asDouble() >= b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers");
                return false;
            }
            break;
        }

        case OP_LESS:
        {
            Value b = pop();
            Value a = pop();

            if (a.isInt() && b.isInt())
            {
                push(Value::makeBool(a.asInt() < b.asInt()));
            }
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeBool(a.asDouble() < b.asDouble()));
            }
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeBool(a.asInt() < b.asDouble()));
            }
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeBool(a.asDouble() < b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers");
                return false;
            }
            break;
        }

        case OP_LESS_EQUAL:
        {
            Value b = pop();
            Value a = pop();

            if (a.isInt() && b.isInt())
            {
                push(Value::makeBool(a.asInt() <= b.asInt()));
            }
            else if (a.isDouble() && b.isDouble())
            {
                push(Value::makeBool(a.asDouble() <= b.asDouble()));
            }
            else if (a.isInt() && b.isDouble())
            {
                push(Value::makeBool(a.asInt() <= b.asDouble()));
            }
            else if (a.isDouble() && b.isInt())
            {
                push(Value::makeBool(a.asDouble() <= b.asInt()));
            }
            else
            {
                runtimeError("Operands must be numbers");
                return false;
            }
            break;
        }

        case OP_PRINT:
        {
            printValue(pop());
            printf("\n");
            break;
        }

        case OP_GET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }

        case OP_SET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }

        case OP_DEFINE_GLOBAL:
        {
            const char *name = READ_STRING_PTR();
            Value value = pop();

            if (!globals_->define(name, value))
            {
                runtimeError("Variable '%s' already defined", name);
                return false;
            }

            // ✅ Invalida cache (nova variável pode ter mudado índices)
            global_cache_.invalidate();

            break;
        }

        case OP_GET_GLOBAL:
        {
            const char *name = READ_STRING_PTR();

            // ✅ Cache hit? (comparação de pointers!)
            if (global_cache_.name == name)
            {
                push(*global_cache_.value_ptr);
                break;
            }

            // Cache miss - lookup normal
            Value *value = globals_->get_ptr(name);
            if (value == nullptr)
            {
                runtimeError("Undefined variable '%s'", name);
                return false;
            }

            // ✅ Atualiza cache
            global_cache_.name = name;
            global_cache_.value_ptr = value;

            push(*value);
            break;
        }

        case OP_SET_GLOBAL:
        {
            const char *name = READ_STRING_PTR();

            // ✅ Cache hit?
            if (global_cache_.name == name && global_cache_.value_ptr != nullptr)
            {
                *global_cache_.value_ptr = peek(0);
                break;
            }

            // Cache miss
            if (!globals_->set_if_exists(name, peek(0)))
            {
                runtimeError("Undefined variable '%s'", name);
                return false;
            }

            // ✅ Atualiza cache
            global_cache_.name = name;
            global_cache_.value_ptr = globals_->get_ptr(name);

            break;
        }

        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }

        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (!isTruthy(peek(0)))
            {
                frame->ip += offset;
            }
            break;
        }

        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }

        case OP_CALL_NATIVE:
        {
            const char *name = READ_STRING_PTR();
            uint8_t argCount = READ_BYTE();

            if (!callNative(name, argCount))
            {
                return false;
            }

            break;
        }
        case OP_CALL:
        {
            uint8_t argCount = READ_BYTE();
            const Value &funcVal = peek(argCount);
            if (!funcVal.isFunction())
            {
                runtimeError("Attempt to call a non-function value (type: %s)",
                             TypeName(funcVal.type));
                return false;
            }
            uint16_t funcIdx = funcVal.asFunctionIdx();
            Function *function = getFunction(funcIdx);
            if (!function)
            {
                runtimeError("Invalid function index: %d", funcIdx);
                return false;
            }
            // Remove função da stack (shift args para baixo)
            Value *funcSlot = stackTop_ - argCount - 1;
            for (Value *p = funcSlot; p < stackTop_ - 1; p++)
                *p = *(p + 1);
            stackTop_--;
            // Faz a call
            if (!callFunction(function, argCount))
                return false;
            // Atualiza frame pointer
            frame = &frames_[frameCount_ - 1];
            break;
        }

        // case OP_RETURN:
        // {
        //     // DumpStack();
        //     Value result = pop();
        //     Value *resultSlot = frame->slots;
        //     frameCount_--;

        //     if (frameCount_ == 0)
        //     {
        //         // printf("=== Script finished ===\n");

        //         // Script principal terminou
        //         stackTop_ = stack_;
        //         push(result); // Resultado fica na stack para quem chamar Pop()
        //     }
        //     else
        //     {
        //         // Retornou de função
        //         stackTop_ = resultSlot;
        //         push(result);

        //         // printf("DEBUG: Returning value: ");
        //         // printValue(result);
        //         // printf("\n");
        //         // DumpStack();  // See stack after return
        //         frame = &frames_[frameCount_ - 1];
        //     }
        //     break;
        // }
        case OP_RETURN_NIL:

        {
            Value *resultSlot = frame->slots; // Save BEFORE decrementing

            frameCount_--;
            if (frameCount_ == 0)
            {
                stackTop_ = stack_;
                push(Value::makeNull());
            }
            else
            {
                stackTop_ = resultSlot; // Use saved value
                push(Value::makeNull());
                frame = &frames_[frameCount_ - 1];
            }
            break;
        }

        default:
            runtimeError("Unknown opcode %d", instruction);
            goto error_exit;
        }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING_PTR
    }

error_exit:
    process->state = Process::DEAD;
    frameCount_--;
    currentProcess_ = nullptr;
    return false;
}