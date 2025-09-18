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
    if (_flag_stop)
    {
        log_w("Exec stopped by _flag_stop. return false.");
        return false;
    }
    if (json.isNull())
    {
        error("NULL", "Exec hit a null block.");
        return false;
    }
    if (json.containsKey("block"))
        json = json["block"]; // 兼容不同层级结构的Json
    else if (json.containsKey("shadow"))
        json = json["shadow"]; // 兼容shadow block
    if (json.containsKey("type"))
    {
        // 是单个block
        JsonObject current = json;

        // 顺序执行所有关联block
        do
        {
            if (_handlers.find(json["type"]) == _handlers.end())
            {
                return error(json["id"], "Exec hit an undefined block.");
            }
            // 当单个block要求跳出时停止执行其余的block并向上级汇报跳出
            if (!_handlers[json["type"]](current, this))
                return false;
            current = current["next"];
        } while (current.containsKey("next"));
    }
    else if (json.containsKey("blocks"))
    {
        // 是整个Blocky文件，将每个根block注册为入口
        JsonArray blocks = json["blocks"]["blocks"].to<JsonArray>();
        for (JsonObject block : blocks)
        {
            log_i("Registered entrance: %s", block["type"].as<const char *>());
            _entrances.emplace(block["type"], block);
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

/// 传入一个Block，执行它并获得它处理函数的返回值。
/// 注意：此函数不会执行连接在传入Block下方的其它Block。
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
        if (_handlers.find(json["type"]) != _handlers.end())
            return _handlers[json["type"]](json, this);
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

int BlocklyInterpreter::execFile(String filename)
{
    File file = SD.open(filename, FILE_READ);
    if (file)
    {
        ReadBufferingStream bufferedFile{file, 64};
        JsonDocument _json;
        DeserializationError error = deserializeJson(_json, bufferedFile);
        if (error)
        {
            log_e("deserializeJson() failed: %s", error.c_str());
            return -1;
        }
        return exec(_json.as<JsonObject>());
        file.close();
    }
    else
    {
        log_e("Failed to open file: %s", filename.c_str());
        return -1;
    }
}

int BlocklyInterpreter::triggerEntrance(String entrance)
{
    if (_entrances.find(entrance) != _entrances.end())
    {
        runningEntrances++;
        int iid = runningEntrances;
        log_i("Entrance %s triggered with iid %d", entrance.c_str(), iid);
        int retval = exec(_entrances[entrance]);
        log_i("Entrance %s ended with iid %d", entrance.c_str(), iid);
        runningEntrances--;
        log_i("There is currently %d entrances running.", runningEntrances);
        return retval;
    }
    else
    {
        log_e("Entrance not found: %s", entrance.c_str());
        return -1;
    }
}

void BlocklyInterpreter::registerHandler(String type, iBlocklyNodeHandler handler)
{
    _handlers.emplace(type, handler);
}

void BlocklyInterpreter::clearHandlers()
{
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

inline bool BlocklyInterpreter::error(String blockid, String msg)
{
    log_e("Block interpreter failed at block %s:", blockid);
    log_e("%s", msg);
    return false;
}