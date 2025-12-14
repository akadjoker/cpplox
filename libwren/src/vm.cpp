#include "vm.h"
#include "stringpool.h"
#include "table.h"
#include "compiler.h"
#include "process.h"
#include "process_manager.h"
#include <cstdio>
#include <cstdarg>

CallFrame::CallFrame()
    : function(nullptr), ip(nullptr), slots(nullptr) {}

VM::VM() : stackTop_(stack_), frameCount_(0), hasFatalError_(false)
{
    compiler = new Compiler(this);
    globals_ = new Table();
    currentProcess_ = nullptr;
    processManager_ = new ProcessManager();

    global_cache_.invalidate();
    natives_.registerBuiltins(this);


    globals_->define("PI", Value::makeDouble(3.141592653589793));

  
}

VM::~VM()
{

    delete processManager_;
    delete globals_;
    delete compiler;
    StringPool::instance().clear();
    for (Function *func : functions_)
    {
        delete func;
    }
}

uint16_t VM::registerFunction(const std::string &name, Function *func)
{
    if (!func)
    {
        runtimeError("Cannot register null function");
        return 0;
    }

    // Intern name ONCE
    const char *internedName = StringPool::instance().intern(name);

    auto it = functionNames_.find(internedName);
    if (it != functionNames_.end())
    {
        fprintf(stderr, "Warning: Function '%s' already registered at index %d\n",
                internedName, it->second);
        return it->second;
    }

    if (functions_.size() >= 65535)
    {
        runtimeError("Too many functions (max 65535)");
        return 0;
    }

    uint16_t index = static_cast<uint16_t>(functions_.size());
    functions_.push_back(func);
    functionNames_[internedName] = index;

    return index;
}

bool VM::canRegisterFunction(const std::string &name)
{
    const char *internedName = StringPool::instance().intern(name);
    return functionNames_.find(internedName) == functionNames_.end();
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
    hasFatalError_ = false;
    stackTop_ = stack_;
    frameCount_ = 0;
}

void VM::registerNative(const char *name, int arity, NativeFunction fn)
{
     

    // if (natives_.hasFunction(internedName))
    // {
    //     runtimeError("Native function '%s' already registered", name);
    //     return;
    // }

    uint16_t func = natives_.registerFunction(name, arity, fn);
    if (!globals_->define(name, std::move(Value::makeNative(func))))
    {
        runtimeError("Native function '%s' already registered", name);
    }
    else
    {

       // printf("Registering native function '%s' at index %d\n", name, func);
    }
}

uint16_t VM::getFunctionId(const char *name)
{
    const char *internedName = StringPool::instance().intern(name);
    auto it = functionNames_.find(internedName);
    if (it != functionNames_.end())
    {
        return it->second;
    }
    printf("Undefined function '%s'\n", internedName);
    return 0;
}

bool VM::callNative(const char *name, int argCount)
{
    NativeFn *native = natives_.getFunction(name);

    if (!native)
    {
        runtimeError("Undefined native function '%s'", name);
        return false;
    }

    if (native->arity != -1 && argCount != native->arity)
    {
        runtimeError("%s() expects %d arguments but got %d",
                     name, native->arity, argCount);
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

    if (function->isProcess)
    {
        return true;
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

    // push(Value::makeNull());

    CallFrame *frame = &frames_[frameCount_++];
    frame->function = function;
    frame->ip = function->chunk.code.data();
    frame->slots = stack_;

    return run() ? InterpretResult::OK : InterpretResult::RUNTIME_ERROR;
}

InterpretResult VM::interpret(const std::string &source)
{
    

    Function *function = compiler->compile(source, this);
    if (!function)
    {
        return InterpretResult::COMPILE_ERROR;
    }
    // push(Value::makeNull());

    CallFrame *frame = &frames_[frameCount_++];
    frame->function = function;
    frame->ip = function->chunk.code.data();
    frame->slots = stack_;

    bool status = run();
    if (!status)
    {
        delete function;
        return InterpretResult::RUNTIME_ERROR;
    }
    else
    {
        delete function;
        return InterpretResult::OK;
    }
}

InterpretResult VM::interpretExpression(const std::string &source)
{
    Function *function = compiler->compileExpression(source, this);
    if (!function)
    {
        return InterpretResult::COMPILE_ERROR;
    }
    push(Value::makeNull());

    CallFrame *frame = &frames_[frameCount_++];
    frame->function = function;
    frame->ip = function->chunk.code.data();
    frame->slots = stack_;

    bool status = run();
    if (!status)
    {
        delete function;
        return InterpretResult::RUNTIME_ERROR;
    }
    else
    {
        delete function;
        return InterpretResult::OK;
    }
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
    hasFatalError_ = true;
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
    return v.asString();
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
    // if (!globals_->contains(name))
    // {
    //     runtimeError("Global '%s' already exists", name);
    //     return;
    // }
    Value value = Pop();
    if (!globals_->define(name, value))
    {
        runtimeError("Global '%s' already exists", name);
    }
}

void VM::GetGlobal(const char *name)
{
    const char *interned = StringPool::instance().intern(name);

    // if (!globals_->contains(interned))
    // {
    //     runtimeError("Undefined global '%s'", interned);
    //     PushNull();
    //     return;
    // }
    // Push(globals_->get(interned));

    Value *value = globals_->get_ptr(interned);

    if (value)
    {
        Push(*value);
    }
    else
    {
        runtimeError("Undefined global '%s'", interned);
        PushNull();
    }
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
            printf("string: \"%s\"\n", v.asString());
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

bool VM::isNativeFunction(const char *name) const
{
    return natives_.hasFunction(name);
}

// ============================================
// RUN: Loop principal da VM
// ============================================
bool VM::run()
{
    CallFrame *frame = &frames_[frameCount_ - 1];

    while (frameCount_ > 0)
    {
        if (!executeInstruction(frame))
        {
            return false; // Erro fatal
        }
    }

    return true;
}

// ============================================
// EXECUTE UNTIL RETURN: Para API pública (Call)
// ============================================
bool VM::executeUntilReturn(int targetFrameCount)
{
    while (frameCount_ > targetFrameCount)
    {
        CallFrame *frame = &frames_[frameCount_ - 1];

        if (!executeInstruction(frame))
        {
            return false; // Erro fatal
        }
    }

    return true;
}

// ============================================
// CORE: Executa UMA instrução
// ============================================
bool VM::executeInstruction(CallFrame *&frame)
{
    if (hasFatalError_)
        return false;

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
#define READ_STRING_PTR() (frame->function->chunk.getStringPtr(READ_BYTE()))

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



        global_cache_.invalidate();

        break;
    }

    case OP_GET_GLOBAL:
    {
        const char *name = READ_STRING_PTR();

        

     
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
        //globals_->dump();
        //printf("CALL\n");
        uint8_t argCount = READ_BYTE();
        const Value &funcVal = peek(argCount);
        printValue(funcVal);
        if (!funcVal.isFunction())
        {
            if (!callNative("clock", argCount))
            {
                return false;
            }
           // runtimeError("Attempt to call a non-function value (type: %s)",
             //            TypeName(funcVal.type));
            break;
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

    case OP_RETURN:
    {
        // DumpStack();
        Value result = pop();
        Value *resultSlot = frame->slots;
        frameCount_--;

        if (frameCount_ == 0)
        {
            // printf("=== Script finished ===\n");

            // Script principal terminou
            stackTop_ = stack_;
            push(result); // Resultado fica na stack para quem chamar Pop()
        }
        else
        {
            // Retornou de função
            stackTop_ = resultSlot;
            push(result);

            // printf("DEBUG: Returning value: ");
            // printValue(result);
            // printf("\n");
            // DumpStack();  // See stack after return
            frame = &frames_[frameCount_ - 1];
        }
        break;
    }
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
        runtimeError("Unknown opcode: %d", instruction);
        return false;
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_STRING_PTR

    return true;
}
