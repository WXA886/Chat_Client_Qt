#ifndef BROADCAST_H
#define BROADCAST_H

#include "common.h"

void broadcast_message(
    const char *msg,
    int sender_fd
);

#endif