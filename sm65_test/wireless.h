#pragma once
#include "quantum.h"

void wireless_init(void);
void wireless_send(uint8_t *data, size_t len);
size_t wireless_receive(uint8_t *buffer, size_t max_len);
