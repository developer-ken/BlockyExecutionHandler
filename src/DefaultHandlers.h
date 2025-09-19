#ifndef _DEFAULTHANDLERS_H_
#define _DEFAULTHANDLERS_H_

#include "BlocklyInterpreter.h"

void RegisterDefaultHandlers(BlocklyInterpreter*);

int _E_COMMON_NOP(JsonObject jb, BlocklyInterpreter *b);

#endif