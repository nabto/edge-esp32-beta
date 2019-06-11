#pragma once

#include "logging.h"

#include <platform/np_logging.h>

void np_log_init(void);

void nm_esp32_log (uint32_t severity, uint32_t module, uint32_t line, const char* file, const char* fmt, va_list args);
void nm_esp32_log_buf(uint32_t severity, uint32_t module, uint32_t line, const char* file, const uint8_t* buf, size_t len);