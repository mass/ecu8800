#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "stm32h7xx.h"
#include "system_stm32h7xx.h"
#pragma GCC diagnostic pop

// Function definitions
int main(void);
int init_flash_option_bytes(void);

//TODO: Interrupt handlers
