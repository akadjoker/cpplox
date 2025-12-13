#include "vm.h"
#include <cstdio>
#include <cstdarg>

CallFrame::CallFrame()
    : function(nullptr), ip(nullptr), slots(nullptr) {}

VM::VM() : stackTop_(stack_), frameCount_(0)
{
    natives_.registerBuiltins();
}

VM::~VM()
{
    for (Function *func : functions_)
    {
        delete func;
    }
}

uint16_t VM::registerFunction(const std::string &name, Function *func)
{
    // Valida
    if (!func)
    {
        runtimeError("Cannot register null function");
        return 0;
    }

 
    auto it = functionNames_.find(name);

    if (it != functionNames_.end())
    {
       
        fprintf(stderr, "Warning: Function '%s' already registered at index %d\n",
                name.c_str(), it->second);
        return it->second;

        /* ANTES (bugado):
        fprintf(stderr, "Warning: Function '%s' already exists, overwriting\n", name.c_str());
        uint16_t oldIdx = it->second;
        delete functions_[oldIdx];  // ← BUG! Deleta mas caller ainda tem ponteiro!
        functions_[oldIdx] = func;
        return oldIdx;
        */
    }
 
    if (functions_.size() >= 65535)
    {
        runtimeError("Too many functions (max 65535)");
        return 0;
    }

    uint16_t index = static_cast<uint16_t>(functions_.size());
    functions_.push_back(func);
    functionNames_[name] = index;

    return index;
}

Function *VM::getFunction(const char *name)
{
    auto it = functionNames_.find(name);
    if (it != functionNames_.end())
    {
        return functions_[it->second];
    }
    runtimeError("Undefined function '%s'", name);
    return nullptr;
}

Function *VM::getFunction(uint16_t index)
{
    if (index < functions_.size())
    {
        return functions_[index];
    }
    runtimeError("Function index out of range");
    return nullptr;
}

void VM::resetStack()
{
    stackTop_ = stack_;
    frameCount_ = 0;
}

void VM::registerNative(const std::string &name, int arity, NativeFunction fn)
{
    if (natives_.hasFunction(name))
    {
        runtimeError("Native function '%s' already registered", name.c_str());
        return;
    }

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

bool VM::callFunction(Function *function, int argCount)
{
    // Verifica arity
    if (argCount != function->arity)
    {
        runtimeError("Function '%s' expects %d arguments but got %d",
                     function->name.c_str(), function->arity, argCount);
        return false;
    }

    // Verifica overflow de frames
    if (frameCount_ >= FRAMES_MAX)
    {
        runtimeError("Stack overflow - too many nested calls");
        return false;
    }

    // Cria novo frame
    CallFrame *frame = &frames_[frameCount_++];
    frame->function = function;
    frame->ip = function->chunk.code.data();
    frame->slots = stackTop_ - argCount; // args já estão na stack

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

bool VM::isTruthy(const Value &value)
{
    switch (value.type)
    {
    case VAL_NULL:
        return false;
    case VAL_BOOL:
        return value.asBool();
    case VAL_INT:
        return value.asInt() != 0;
    case VAL_DOUBLE:
        return value.asDouble() != 0.0;
    default:
        return true; // strings, functions são truthy
    }
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
                double da = a.asDouble();
                double db = b.asDouble();
                push(Value::makeDouble(da + db));
            }
            else if (a.isDouble() || b.isInt())
            {
                double fa = a.asDouble();
                int fb = b.isInt();
                push(Value::makeDouble(fa + fb));
            }
            else if (a.isInt() || b.isDouble())
            {
                int fa = a.isInt();
                int fb = b.asDouble();
                push(Value::makeInt(fa + fb));
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
            std::string name = READ_STRING();
            uint8_t argCount = READ_BYTE();

            if (!callNative(name, argCount))
            {
                return false;
            }
            break;
        }
        case OP_CALL:
        {
            std::string name = READ_STRING();
            uint8_t argCount = READ_BYTE();

            // Lookup função
            auto it = functionNames_.find(name);
            if (it == functionNames_.end())
            {
                runtimeError("Undefined function '%s'", name.c_str());
                return false;
            }

            Function *function = functions_[it->second];

            if (!callFunction(function, argCount))
            {
                return false;
            }

            // Atualiza frame pointer
            frame = &frames_[frameCount_ - 1];
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

const Value &VM::peek(int distance)
{

    int stackSize = stackTop_ - stack_;

    if (distance < 0 || distance >= stackSize)
    {
        runtimeError("Stack peek out of bounds: distance=%d, size=%d",
                     distance, stackSize);

        static const Value null = Value::makeNull();
        return null;
    }

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

// ============================================
// PUBLIC API
// ============================================

// Stack manipulation
const Value &VM::Peek(int index)
{
    if (index < 0)
    {
        int dist = -1 - index;
        if (dist >= GetTop())
        {
            runtimeError("Stack index out of bounds");
            static Value null = Value::makeNull(); // static para retornar ref
            return null;
        }
        return peek(dist); // peek também deve retornar ref!
    }
    else
    {
        if (index >= GetTop())
        {
            runtimeError("Stack index out of bounds");
            static Value null = Value::makeNull();
            return null;
        }
        return stack_[index];
    }
}

void VM::Push(Value value)
{
    push(value); // wrapper
}

Value VM::Pop()
{
    return pop(); // wrapper
}

void VM::PushInt(int n)
{
    push(Value::makeInt(n));
}

void VM::PushDouble(double d)
{
    push(Value::makeDouble(d));
}

void VM::PushString(const char *s)
{
    push(Value::makeString(s));
}

void VM::PushBool(bool b)
{
    push(Value::makeBool(b));
}

void VM::PushNull()
{
    push(Value::makeNull());
}

// Type conversions
int VM::ToInt(int index)
{
    Value v = Peek(index);
    if (!v.isInt())
    {
        runtimeError("Expected int at index %d", index);
        return 0;
    }
    return v.asInt();
}

double VM::ToDouble(int index)
{
    Value v = Peek(index);

    if (v.isDouble())
    {
        return v.asDouble();
    }
    else if (v.isInt())
    {
        return (double)v.asInt();
    }

    runtimeError("Expected number at index %d", index);
    return 0.0;
}

const char *VM::ToString(int index)
{
    const Value &v = Peek(index);
    if (!v.isString())
    {
        runtimeError("Expected string at index %d", index);
        return "";
    }
    return v.asString()->c_str();
}

bool VM::ToBool(int index)
{
    Value v = Peek(index);
    return isTruthy(v);
}

// Stack info
int VM::GetTop()
{
    return stackTop_ - stack_;
}

void VM::SetTop(int index)
{
    if (index < 0 || index > STACK_MAX)
    {
        runtimeError("Invalid stack index");
        return;
    }
    stackTop_ = stack_ + index;
}

void VM::Remove(int index)
{
    Value *slot;

    if (index < 0)
    {
        slot = stackTop_ + index;
    }
    else
    {
        slot = stack_ + index;
    }

    if (slot < stack_ || slot >= stackTop_)
    {
        runtimeError("Invalid stack index");
        return;
    }

    // Shift down
    for (Value *p = slot; p < stackTop_ - 1; p++)
    {
        *p = *(p + 1);
    }
    stackTop_--;
}

void VM::Insert(int index)
{
    Value top = Pop();
    Value *slot;

    if (index < 0)
    {
        slot = stackTop_ + index + 1;
    }
    else
    {
        slot = stack_ + index;
    }

    if (slot < stack_ || slot > stackTop_)
    {
        runtimeError("Invalid stack index");
        Push(top);
        return;
    }

    // Shift up
    for (Value *p = stackTop_; p > slot; p--)
    {
        *p = *(p - 1);
    }

    *slot = top;
    stackTop_++;
}

void VM::Replace(int index)
{
    Value top = Pop();
    Value *slot;

    if (index < 0)
    {
        slot = stackTop_ + index + 1;
    }
    else
    {
        slot = stack_ + index;
    }

    if (slot < stack_ || slot >= stackTop_)
    {
        runtimeError("Invalid stack index");
        Push(top);
        return;
    }

    *slot = top;
}

void VM::Copy(int from, int to)
{
    Value v = Peek(from);

    Value *destSlot;
    if (to < 0)
    {
        destSlot = stackTop_ + to;
    }
    else
    {
        destSlot = stack_ + to;
    }

    if (destSlot < stack_ || destSlot >= stackTop_)
    {
        runtimeError("Invalid destination index");
        return;
    }

    *destSlot = v;
}

// Type checking
ValueType VM::GetType(int index)
{
    return Peek(index).type;
}

bool VM::IsInt(int index)
{
    return Peek(index).type == VAL_INT;
}

bool VM::IsDouble(int index)
{
    return Peek(index).type == VAL_DOUBLE;
}

bool VM::IsString(int index)
{
    return Peek(index).type == VAL_STRING;
}

bool VM::IsBool(int index)
{
    return Peek(index).type == VAL_BOOL;
}

bool VM::IsNull(int index)
{
    return Peek(index).type == VAL_NULL;
}

bool VM::IsFunction(int index)
{
    return Peek(index).type == VAL_FUNCTION;
}

// Globals
void VM::SetGlobal(const char *name)
{
    Value value = Pop();
    globals_[name] = value;
}

void VM::GetGlobal(const char *name)
{
    if (globals_.find(name) == globals_.end())
    {
        runtimeError("Undefined global '%s'", name);
        PushNull();
        return;
    }
    Push(globals_[name]);
}

void VM::Call(int argCount, int resultCount)
{
    // Stack layout:
    // [...] [function] [arg1] [arg2] ... [argN] <- stackTop
    //       ^-argCount-1              ^

    // 1.  função do stack
    const Value &funcVal = Peek(-argCount - 1);

    if (!funcVal.isFunction())
    {
        runtimeError("Attempt to call a non-function value (type: %s)",
                     TypeName(funcVal.type));
        return;
    }

    uint16_t funcIdx = funcVal.asFunctionIdx();
    Function *func = getFunction(funcIdx);

    if (!func)
    {
        runtimeError("Invalid function index: %d", funcIdx);
        return;
    }

    // 2. Remove função da stack (shift args para baixo)
    // Antes: [func] [arg1] [arg2]
    // Depois: [arg1] [arg2]
    Value *funcSlot = stackTop_ - argCount - 1;
    for (Value *p = funcSlot; p < stackTop_ - 1; p++)
    {
        *p = *(p + 1);
    }
    stackTop_--;

    // 3. Guarda frameCount antes da call
    int beforeCall = frameCount_;

    // 4. Cria frame (usa callFunction interno)
    if (!callFunction(func, argCount))
    {
        return; // Erro (arity mismatch, stack overflow, etc)
    }

    // 5. Executa até esta função retornar
    if (!executeUntilReturn(beforeCall))
    {
        return; // Erro de runtime
    }

    // 6. Resultado está no topo
    if (resultCount == 0)
    {
        Pop();
    }
}

bool VM::executeUntilReturn(int targetFrameCount)
{
    while (frameCount_ > targetFrameCount)
    {
        CallFrame *frame = &frames_[frameCount_ - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
#define READ_STRING() (READ_CONSTANT().asString()->c_str())
        uint8_t instruction = *frame->ip++;
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
                double da = a.asDouble();
                double db = b.asDouble();
                push(Value::makeDouble(da + db));
            }
            else if (a.isDouble() || b.isInt())
            {
                double fa = a.asDouble();
                int fb = b.isInt();
                push(Value::makeDouble(fa + fb));
            }
            else if (a.isInt() || b.isDouble())
            {
                int fa = a.isInt();
                int fb = b.asDouble();
                push(Value::makeInt(fa + fb));
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
            std::string name = READ_STRING();
            uint8_t argCount = READ_BYTE();

            if (!callNative(name, argCount))
            {
                return false;
            }
            break;
        }
        case OP_CALL:
        {
            std::string name = READ_STRING();
            uint8_t argCount = READ_BYTE();

            // Lookup função
            auto it = functionNames_.find(name);
            if (it == functionNames_.end())
            {
                runtimeError("Undefined function '%s'", name.c_str());
                return false;
            }

            Function *function = functions_[it->second];

            if (!callFunction(function, argCount))
            {
                return false;
            }

            // Atualiza frame pointer
            frame = &frames_[frameCount_ - 1];
            break;
        }

        case OP_RETURN:
        {
            Value result = pop();
            frameCount_--;

            if (frameCount_ <= targetFrameCount)
            {

                if (frameCount_ == 0)
                {
                    stackTop_ = stack_;
                }
                else
                {
                    stackTop_ = frames_[frameCount_].slots;
                }

                push(result);

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
                return true;
            }

            // Retornou de subcall, continua execução
            stackTop_ = frame->slots;
            push(result);
            frame = &frames_[frameCount_ - 1];
            break;
        }

        default:
            runtimeError("Unknown opcode: %d", instruction);
            return false;
        }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
    }

    return true;
}

// Debug
void VM::DumpStack()
{
    printf("=== Stack Dump (size: %d) ===\n", GetTop());

    if (GetTop() == 0)
    {
        printf("  (empty)\n");
        return;
    }

    for (int i = 0; i < GetTop(); i++)
    {
        Value v = Peek(i);
        printf("  [%2d] ", i);

        switch (v.type)
        {
        case VAL_NULL:
            printf("null\n");
            break;
        case VAL_BOOL:
            printf("bool: %s\n", v.asBool() ? "true" : "false");
            break;
        case VAL_INT:
            printf("int: %d\n", v.asInt());
            break;
        case VAL_DOUBLE:
            printf("double: %.2f\n", v.asDouble());
            break;
        case VAL_STRING:
            printf("string: \"%s\"\n", v.asString()->c_str());
            break;
        case VAL_FUNCTION:
            printf("function: %d\n", v.asFunctionIdx());
            break;
        }
    }

    printf("===========================\n");
}

void VM::DumpGlobals()
{
    printf("=== Globals Dump (count: %zu) ===\n", globals_.size());

    if (globals_.empty())
    {
        printf("  (none)\n");
        return;
    }

    for (const auto &[name, value] : globals_)
    {
        printf("  %s = ", name.c_str());
        printValue(value);
        printf("\n");
    }

    printf("================================\n");
}

const char *VM::TypeName(ValueType type)
{
    switch (type)
    {
    case VAL_NULL:
        return "null";
    case VAL_BOOL:
        return "bool";
    case VAL_INT:
        return "int";
    case VAL_DOUBLE:
        return "double";
    case VAL_STRING:
        return "string";
    case VAL_FUNCTION:
        return "function";
    default:
        return "unknown";
    }
}
