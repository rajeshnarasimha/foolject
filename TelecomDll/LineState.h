#ifndef LINE_STATE_H
#define LINE_STATE_H

enum TLineState
{
    LINESTATE_CLOSED = 0,
    LINESTATE_UNINITIALIZED,
    LINESTATE_IDLE,
    LINESTATE_BLOCKING,
    LINESTATE_OUT_OF_SERVICE,
    LINESTATE_ACTIVE,
    
    LINESTATE_QTY
};

extern const char *LineStateName[LINESTATE_QTY];

#endif