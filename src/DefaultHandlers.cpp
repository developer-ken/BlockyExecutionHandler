#include <Arduino.h>
#include "DefaultHandlers.h"

bool __DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK = false;

/*
 * controls_if
 * 默认的IF块行为
 * 从前到后判断IFn，条件满足执行DOn, 任意条件满足不再执行后续判断。
 * 所有条件不满足则执行ELSE
 *
 * 请注意，对于一个操作(Action)Block，返回false将阻止连接在它下方的块继续执行。
 */
int _A_IF(JsonObject jb, BlocklyInterpreter b)
{
    bool hit = false;
    for (int i = 0; jb["inputs"].containsKey("IF" + String(i)); i++)
    {
        if (b.eval(jb["inputs"]["IF" + String(i)]))
        {
            hit = true;
            if (jb["inputs"].containsKey("DO" + String(i)))
                return b.exec(jb["inputs"]["DO" + String(i)]);
            break;
        }
    }
    if (!hit && jb["inputs"].containsKey("ELSE"))
        return b.exec(jb["inputs"]["ELSE"]);
    return true;
}

/*
 * logic_compare
 * COMPARE块行为
 *
 * 这是一个求值(Value)Block，其返回值被传到上一级Block。
 */
int _V_COMPARE(JsonObject jb, BlocklyInterpreter b)
{
    if (!jb.containsKey("inputs"))
    {
        if (b.error(jb["id"], "No input found."))
            return false;
    }
    if (!jb["inputs"].containsKey("A"))
    {
        if (b.error(jb["id"], "Input field A not found."))
            return false;
    }
    if (!jb["inputs"].containsKey("B"))
    {
        if (b.error(jb["id"], "Input field B not found."))
            return false;
    }
    char *op = jb["fields"]["OP"].as<char *>();
    if (String("EQ").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) == b.eval(jb["inputs"]["B"]);
    }
    if (String("NEQ").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) != b.eval(jb["inputs"]["B"]);
    }
    if (String("LT").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) < b.eval(jb["inputs"]["B"]);
    }
    if (String("LTE").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) <= b.eval(jb["inputs"]["B"]);
    }
    if (String("GT").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) > b.eval(jb["inputs"]["B"]);
    }
    if (String("GTE").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) >= b.eval(jb["inputs"]["B"]);
    }
    return false;
}

/*
 * math_number
 * 数值常量块
 */
int _V_CONST_NUMBER(JsonObject jb, BlocklyInterpreter b)
{
    return jb["fields"]["NUM"].as<int>();
}

/*
 * math_arithmetic
 * 基础计算块
 */
int _V_ARITHMETIC(JsonObject jb, BlocklyInterpreter b)
{
    if (!jb.containsKey("inputs"))
    {
        if (b.error(jb["id"], "No input found."))
            return false;
    }
    if (!jb["inputs"].containsKey("A"))
    {
        if (b.error(jb["id"], "Input field A not found."))
            return false;
    }
    if (!jb["inputs"].containsKey("B"))
    {
        if (b.error(jb["id"], "Input field B not found."))
            return false;
    }
    char *op = jb["fields"]["OP"].as<char *>();
    if (String("ADD").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) + b.eval(jb["inputs"]["B"]);
    }
    if (String("MINUS").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) - b.eval(jb["inputs"]["B"]);
    }
    if (String("MULTIPLY").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) * b.eval(jb["inputs"]["B"]);
    }
    if (String("DIVIDE").equals(op))
    {
        return b.eval(jb["inputs"]["A"]) / b.eval(jb["inputs"]["B"]);
    }
    if (String("POWER").equals(op))
    {
        return pow(b.eval(jb["inputs"]["A"]), b.eval(jb["inputs"]["B"]));
    }
    return false;
}

/*
 * controls_whileUntil
 * 条件循环块
 */
int _A_WHILELOOP(JsonObject jb, BlocklyInterpreter b)
{
    __DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK = false;
    if (!jb.containsKey("inputs"))
    {
        if (b.error(jb["id"], "No input found."))
            return false;
    }
    if (!jb["inputs"].containsKey("MODE"))
    {
        if (b.error(jb["id"], "Input field MODE not found."))
            return false;
    }
    char *op = jb["fields"]["MODE"].as<char *>();
    bool retval = true;
    if (String("WHILE").equals(op))
    {
        while (b.eval(jb["inputs"]["BOOL"]))
        {
            if (!b.exec(jb["inputs"]["DO"]))
            {
                if (__DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK)
                {
                    __DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK = false;
                    break;
                }
            }
        }
    }
    else if (String("UNTIL").equals(op))
    {
        do
        {
            if (!b.exec(jb["inputs"]["DO"]))
            {
                if (__DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK)
                {
                    __DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK = false;
                    break;
                }
            }
        } while (!b.eval(jb["inputs"]["BOOL"]));
    }
    return true;
}

/*
 * controls_repeat_ext
 * 计次循环
 */
int _A_COUNTED_LOOP(JsonObject jb, BlocklyInterpreter b)
{
    int count = b.eval(jb["inputs"]["TIMES"]);
    __DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK = false;
    for (int i = 0; i < count; i++)
    {
        if (!b.exec(jb["inputs"]["DO"]))
        {
            if (__DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK)
            {
                __DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK = false;
                break;
            }
        }
    }
    return true;
}

/*
 * controls_flow_statements
 * 循环跳出
 *
 * 此块在循环中执行时，其导致循环break或continue.
 * 此块在循环外执行时，其导致程序立即结束.
 */
int _A_BREAK_CONTINUE(JsonObject jb, BlocklyInterpreter b)
{
    char *op = jb["fields"]["FLOW"].as<char *>();
    if (String("BREAK").equals(op))
    {
        __DEFAULT_BLOCKLY_HANDLERS_LOOPBREAK = true;
    }
    return false;
}

void RegisterDefaultHandlers(BlocklyInterpreter *bi)
{
    bi->registerHandler("controls_if", _A_IF);
    bi->registerHandler("logic_compare", _V_COMPARE);
    bi->registerHandler("math_number", _V_CONST_NUMBER);
    bi->registerHandler("math_arithmetic", _V_ARITHMETIC);
    bi->registerHandler("controls_whileUntil", _A_WHILELOOP);
    bi->registerHandler("controls_repeat_ext", _A_COUNTED_LOOP);
    bi->registerHandler("controls_flow_statements", _A_BREAK_CONTINUE);
}