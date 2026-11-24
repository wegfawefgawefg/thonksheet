#pragma once

#include "blob/blob.hpp"
#include "world/plug.hpp"

#include <cstdio>

// draw where each line sits in the sensor and motor regions, as text
void plug_print(const Plug& plug, const Blob& blob, FILE* f);
