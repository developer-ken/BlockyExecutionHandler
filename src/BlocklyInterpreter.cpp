#include <SD.h>
#include <Arduino.h>
#include <StreamUtils.h>

#include "BlocklyInterpreter.h"

BlocklyInterpreter::BlocklyInterpreter()
{
    runningEntrances = 0;
    _flag_stop = false;
}

BlocklyInterpreter::~BlocklyInterpreter()
{
}

/// 传入一个Block，从该Block开始向后顺序执行。
/// 当一个Block的处理函数返回false时立即停止执行并返回false。
int BlocklyInterpreter::exec(JsonObject json)
{
    log_i("exec(Xjson)");
    if (_flag_stop)
    {
        log_w("Exec stopped by _flag_stop. return false.");
        return false;
    }
    log_i("no flagstop");
    if (json.isNull())
    {
        error("NULL", "Exec hit a null block.");
        return false;
    }
    log_i("not null");
    if (json.containsKey("block"))
    {
        log_i("block - Go in");
        json = json["block"]; // 兼容不同层级结构的Json
    }
    else if (json.containsKey("shadow"))
    {
        log_i("shadow - Go in");
        json = json["shadow"]; // 兼容shadow block
    }
    log_i("JsonStruct handled. Type check...");
    if (json.containsKey("type"))
    {
        // 是单个block

        JsonObject current = json;
        log_i("*Valid block*");
        // 顺序执行所有关联block
        do
        {
            if (current.containsKey("block"))
            {
                log_i("block - Go in");
                current = current["block"]; // 兼容不同层级结构的Json
            }
            else if (current.containsKey("shadow"))
            {
                log_i("shadow - Go in");
                current = current["shadow"]; // 兼容shadow block
            }
            log_i("Digest block: %s", current["id"].as<const char *>());
            uint32_t typehash = strHash(current["type"].as<const char *>());
            if (_handlers.find(typehash) == _handlers.end())
            {
                log_e("Block %s(%d) is undefined.", current["type"].as<const char *>(), typehash);
                return error(current["id"], "Exec hit an undefined block.");
            }
            log_i("Feed into executor");
            // 当单个block要求跳出时停止执行其余的block并向上级汇报跳出
            if (!_handlers[typehash](current, this))
            {
                log_i("Block request an abort.");
                return false;
            }
            if (current.containsKey("next"))
            {
                current = current["next"];
                log_i("-->Next block");
            }
            else
            {
                break;
            }
            taskYIELD();
        } while (true);

        log_i("Block sequence end");
    }
    else if (json.containsKey("blocks"))
    {
        log_i("*Valid blockly entrance file*");
        // 是整个Blocky文件，将每个根block注册为入口
        EntranceBlocks = json["blocks"]["blocks"];
        log_i("%d enterances found", EntranceBlocks.size());
        for (int ii = 0; ii < EntranceBlocks.size(); ii++)
        {
            log_i("Registered entrance: %s", EntranceBlocks[ii]["type"].as<const char *>());
            _entrances.emplace(strHash(EntranceBlocks[ii]["type"]), EntranceBlocks[ii]);
        }
        return true;
    }
    else
    {
        error("NULL", "Eval hit a invalid block.");
        return false;
    }
    return 1;
}

/// 计算单个block及其依赖的block的值
int BlocklyInterpreter::eval(JsonObject json)
{
    if (_flag_stop)
    {
        log_w("Eval stopped by _flag_stop. return false.");
        return false;
    }
    if (json.isNull())
    {
        error("NULL", "Eval hit a null block.");
        return false;
    }
    if (json.containsKey("block"))
        json = json["block"]; // 兼容不同层级结构的Json
    else if (json.containsKey("shadow"))
        json = json["shadow"]; // 兼容shadow block
    if (json.containsKey("type"))
    {
        uint32_t typehash = strHash(json["type"]);
        log_i("eval_%s_%d(%s)", json["type"].as<const char *>(), typehash, json["id"].as<const char *>());
        if (_handlers.find(typehash) != _handlers.end())
        {
            int val = _handlers[typehash](json, this);
            log_i("result: %d", val);
            return val;
        }
        else
        {
            error(json["id"], "Eval hit an undefined block.");
            return false;
        }
    }
    else
    {
        log_e("Eval accept only single rootblock.");
    }
}

int BlocklyInterpreter::execFile(const char *filename)
{

    log_i("call execFile(%s)", filename);
    File file = SD.open(filename);
    log_i("fopen FI");
    if (file)
    {
        ReadBufferingStream bufferedFile{file, 64};
        JsonDocument _json;
        log_i("start deserilize...");
        DeserializationError error = deserializeJson(_json, bufferedFile);
        file.close();
        if (error)
        {
            log_e("deserializeJson() failed: %s", error.c_str());
            return -1;
        }
        return exec(_json.as<JsonObject>());
    }
    else
    {
        log_e("Failed to open file: %s", filename);
        return -1;
    }
}

int BlocklyInterpreter::triggerEntrance(const char *entrance)
{
    uint32_t strhashkey = strHash(entrance);
    if (_entrances.find(strhashkey) != _entrances.end())
    {
        runningEntrances++;
        int iid = runningEntrances;
        log_i("Entrance %s triggered with iid %d", entrance, iid);
        int retval = exec(_entrances[strhashkey]);
        log_i("Entrance %s ended with iid %d", entrance, iid);
        runningEntrances--;
        log_i("There is currently %d entrances running.", runningEntrances);
        return retval;
    }
    else
    {
        log_e("Entrance not found: %s", entrance);
        return -1;
    }
}

void BlocklyInterpreter::registerHandler(const char *type, iBlocklyNodeHandler handler)
{
    uint32_t typehash = strHash(type);
    log_i("Register handler for %s(%d)", type, typehash);
    _handlers.emplace(typehash, handler);
}

void BlocklyInterpreter::clearHandlers()
{
    log_i("Clear all handlers");
    _handlers.clear();
}

bool BlocklyInterpreter::isBusy()
{
    return runningEntrances > 0;
}

void BlocklyInterpreter::killAll()
{
    log_i("killAll set _flag_stop and waiting for all runningEntrances to stop.");
    _flag_stop = true;
    while (isBusy())
        delay(1);
    _flag_stop = false;
}

inline bool BlocklyInterpreter::error(const char *blockid, const char *msg)
{
    log_e("Block interpreter failed at block %s:", blockid);
    log_e("%s", msg);
    return false;
}

uint32_t BlocklyInterpreter::strHash(const char *str)
{
    uint32_t hash = 0;
    for (; *str != '\0'; str++)
    {
        hash = hash * 31 + *str; // 使用31乘以之前的哈希值并添加当前字符的ASCII值
    }
    return hash;
}