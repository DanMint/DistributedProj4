#ifndef PEER_H
#define PEER_H

struct Peer {
    int next;
    int prev;
    int delay;

    Peer() {
        next = -1;
        prev = -1;
    }
};

#endif