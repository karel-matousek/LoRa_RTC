#ifndef LORA_LISTEN_H
#define LORA_LISTEN_H

#include "stm32wlxx_hal.h"
#include "radio_driver.h"

void lora_init(void);
void lora_continual_receive(void);

#endif
