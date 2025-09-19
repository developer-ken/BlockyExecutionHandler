#ifndef _BLOCKY_INDEXER_H_
#define _BLOCKY_INDEXER_H_

#include <map>
#include <Arduino.h>
#include <ArduinoJson.h>

class BlocklyInterpreter;
typedef int (*iBlocklyNodeHandler)(JsonObject, BlocklyInterpreter *);
class BlocklyInterpreter
{
public:
    BlocklyInterpreter();
    ~BlocklyInterpreter();
    int exec(JsonObject);
    int eval(JsonObject);
    int execFile(const char *);
    int triggerEntrance(const char *);
    void registerHandler(const char *, iBlocklyNodeHandler);
    bool error(const char *, const char *);
    void clearHandlers();
    bool isBusy();
    void killAll();
    bool _flag_stop;

private:
    std::map<uint32_t, iBlocklyNodeHandler> _handlers;
    std::map<uint32_t, JsonObject> _entrances;
    int runningEntrances;
    JsonArray EntranceBlocks;
    uint32_t strHash(const char *);
};

#endif