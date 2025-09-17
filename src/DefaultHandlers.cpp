#include <Arduino.h>
#include "DefaultHandlers.h"

/*
 * 默认的IF块行为
 * 从前到后判断IFn，条件满足执行DOn, 任意条件满足不再执行后续判断。
 * 所有条件不满足则执行ELSE
 *
 * 请注意，对于一个操作Block，返回false将阻止连接在它下方的块继续执行。
 */
int __BH_IF(JsonObject jb, BlocklyInterpreter b)
{
    bool hit = false;
    for (int i = 0; jb["inputs"].containsKey("IF" + String(i)); i++)
    {
        if (b.eval(jb["inputs"]["IF" + String(i)]["block"]))
        {
            hit = true;
            if (jb["inputs"].containsKey("DO" + String(i)))
                return b.exec(jb["inputs"]["DO" + String(i)]["block"]);
            break;
        }
    }
    if (!hit && jb["inputs"].containsKey("ELSE"))
        return b.exec(jb["inputs"]["ELSE"]["block"]);
    return true;
}

void RegisterDefaultHandlers(BlocklyInterpreter* bi)
{
    bi->registerHandler("controls_if", __BH_IF);
}