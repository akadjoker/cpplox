#include "vm.h"
#include <cstdio>
#include <cstdarg>

CallFrame::CallFrame()
    : function(nullptr), ip(nullptr), slots(nullptr) {}

VM::VM() : stackTop_(stack_), frameCount_(0)
{
    natives_.registerBuiltins();
}

void VM::resetStack()
{
    stackTop_ = stack_;
    frameCount_ = 0;
}

void VM::registerNative(const std::string &name, int arity, NativeFunction fn)
{
    natives_.registerFunction(name, arity, fn);
}

bool VM::callNative(const std::string &name, int argCount)
{
    NativeFn *native = natives_.getFunction(name);

    if (!native)
    {
        runtimeError("Undefined native function '%s'", name.c_str());
        return false;
    }

    if (native->arity != -1 && argCount != native->arity)
    {
        runtimeError("%s() expects %d arguments but got %d",
                     name.c_str(), native->arity, argCount);
        return false;
    }

    Value *args = stackTop_ - argCount;
    Value result = native->function(this, argCount, args);

    stackTop_ -= argCount;
    push(result);

    return true;
}

InterpretResult VM::interpret(Function *function)
{
    push(Value::makeFunction(0));

    CallFrame *frame = &frames_[frameCount_++];
    frame->function = function;
    frame->ip = function->chunk.code.data();
    frame->slots = stack_;

    return run() ? InterpretResult::OK : InterpretResult::RUNTIME_ERROR;
}

bool VM::run()
{
    CallFrame *frame = &frames_[frameCount_ - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
#define READ_STRING() (READ_CONSTANT().asString()->c_str())

    for (;;)
    {
        uint8_t instruction = READ_BYTE();

        switch (instruction)
        {
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

        case OP_ADD:
        {
            Value b = pop();
            Value a = pop();

            if (a.isString() && b.isString())
            {
                String result = *a.asString() + *b.asString();
                push(Value::makeString(result));
            }
            else if (a.isInt() && b.isInt())
            {
                push(Value::makeInt(a.asInt() + b.asInt()));
            }
            else if (a.isDouble() || b.isDouble())
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeDouble(da + db));
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
            else
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeDouble(da - db));
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
            else
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeDouble(da * db));
            }
            break;
        }

        case OP_DIVIDE:
        {
            Value b = pop();
            Value a = pop();

            int bInt = b.isInt() ? b.asInt() : (int)b.asDouble();
            if (bInt == 0)
            {
                runtimeError("Division by zero");
                return false;
            }

            if (a.isInt() && b.isInt())
            {
                push(Value::makeInt(a.asInt() / b.asInt()));
            }
            else
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeDouble(da / db));
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
            else
            {
                push(Value::makeDouble(-a.asDouble()));
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
                push(Value::makeBool(*a.asString() == *b.asString()));
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
                push(Value::makeBool(*a.asString() != *b.asString()));
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
            else
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeBool(da > db));
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
            else
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeBool(da >= db));
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
            else
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeBool(da < db));
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
            else
            {
                double da = a.isInt() ? a.asInt() : a.asDouble();
                double db = b.isInt() ? b.asInt() : b.asDouble();
                push(Value::makeBool(da <= db));
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
            std::string name = READ_STRING();
            Value value = pop();

            if (globals_.count(name))
            {
                runtimeError("Variable '%s' already defined", name.c_str());
                return false;
            }

            globals_[name] = value;
            break;
        }

        case OP_GET_GLOBAL:
        {
            std::string name = READ_STRING();

            auto it = globals_.find(name);
            if (it == globals_.end())
            {
                runtimeError("Undefined variable '%s'", name.c_str());
                return false;
            }

            push(it->second);
            break;
        }

        case OP_SET_GLOBAL:
        {
            std::string name = READ_STRING();

            if (!globals_.count(name))
            {
                runtimeError("Undefined variable '%s'", name.c_str());
                return false;
            }

            globals_[name] = peek(0); // não faz pop!
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
            if (!peek(0).asBool())
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
            std::string name = READ_STRING();
            uint8_t argCount = READ_BYTE();

            if (!callNative(name, argCount))
            {
                return false;
            }
            break;
        }

        case OP_RETURN:
        {
            Value result = pop();
            frameCount_--;

            if (frameCount_ == 0)
            {
                stackTop_ = stack_;
                return true;
            }

            stackTop_ = frame->slots;
            push(result);
            frame = &frames_[frameCount_ - 1];
            break;
        }

        default:
            runtimeError("Unknown opcode: %d", instruction);
            return false;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
}

void VM::push(Value value)
{
    if (stackTop_ >= stack_ + STACK_MAX)
    {
        runtimeError("Stack overflow");
        return;
    }
    *stackTop_ = value;
    stackTop_++;
}

Value VM::pop()
{
    if (stackTop_ <= stack_)
    {
        runtimeError("Stack underflow");
        return Value::makeNull();
    }
    stackTop_--;
    return *stackTop_;
}

Value VM::peek(int distance)
{
    return stackTop_[-1 - distance];
}

void VM::runtimeError(const char *format, ...)
{
    fprintf(stderr, "Runtime Error: ");

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputs("\n", stderr);

    for (int i = frameCount_ - 1; i >= 0; i--)
    {
        CallFrame *frame = &frames_[i];
        Function *function = frame->function;

        size_t instruction = frame->ip - function->chunk.code.data() - 1;

        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);

        if (function->name.empty())
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name.c_str());
        }
    }

    resetStack();
}
