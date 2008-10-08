#ifndef CALLBACK_H
#define CALLBACK_H

#include "LogLevel.h"

typedef void (__stdcall *EVENT_HANDLER)(unsigned line, const char *evt_msg);
typedef void (__stdcall *LOG_PROC)(TLogLevel level, int line, char *msg);

#endif