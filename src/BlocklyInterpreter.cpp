#include <SD.h>
#include <Arduino.h>
#include <StreamUtils.h>

#include "BlocklyInterpreter.h"

BlocklyInterpreter::BlocklyInterpreter()
{
}

BlocklyInterpreter::~BlocklyInterpreter()
{
}

/// 传入一个Block，从该Block开始向后顺序执行。
/// 当一个Block的处理函数返回false时立即停止执行并返回false。 
int BlocklyInterpreter::exec(JsonObject json)
{
    if (json.containsKey("block"))
        json = json["block"]; // 兼容不同层级结构的Json
    if (json.containsKey("type"))
    {
        // 是单个block
        JsonObject current = json;

        // 顺序执行所有关联block
        do
        {
            // 当单个block要求跳出时停止执行其余的block并向上级汇报跳出
            if (!_handlers[json["type"]](current, this))
                return 0;
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
    }
    return 1;
}

/// 传入一个Block，执行它并获得它处理函数的返回值。
/// 注意：此函数不会执行连接在传入Block下方的其它Block。 
int BlocklyInterpreter::eval(JsonObject json)
{
    if (json.containsKey("type"))
    {
        return _handlers[json["type"]](json, this);
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
        log_i("Entrance triggered: %s", entrance.c_str());
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