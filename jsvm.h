#pragma once
#include "cdefs.h"
#include <stdint.h>
#include <string>
#include <compartment.h>

// Execute JavaScript bytecode using the Microvium VM.
int __cheri_compartment("jsvm") run_js_bytecode(const uint8_t *bytecode, size_t bytecode_len);
