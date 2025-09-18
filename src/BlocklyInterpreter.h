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
    int execFile(String);
    int triggerEntrance(String);
    void registerHandler(String, iBlocklyNodeHandler);
    bool error(String, String);
    void clearHandlers();
    bool isBusy();
    void killAll();
    bool _flag_stop;

private:
    std::map<String, iBlocklyNodeHandler> _handlers;
    std::map<String, JsonObject> _entrances;
    int runningEntrances;
};

#endif