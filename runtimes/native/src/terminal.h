#pragma once
#include <stdbool.h>
#include <stdint.h>

extern bool TERMINAL;

void w4_terminalBoot (const char* title);
void w4_terminalComposite (const uint32_t* palette, const uint8_t* framebuffer);