#ifndef __STREAM_H
#define __STREAM_H

#include <stdbool.h>

void stream_tx_start(void);
void stream_rx_start(void);

int stream_q_put(uint8_t *data, uint8_t len);

bool stream_q_empty(void);

#endif // __STREAM_H