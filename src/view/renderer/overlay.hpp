#pragma once
// A colour per cell of the sheet, built on the CPU each frame and read by shaders/sheet.frag
// (binding 3). One word per cell: red, green, blue, then a byte that says how it is used:
//   bit 7 set     the colour replaces the cell's kind colour (dim when quiet, lit when it
//                 fires): the buttons' cells
//   bits 0..6     how much of the colour is laid over the cell, 0..127: the evidence
//                 picture and the readability tint
// A zero word leaves the cell as it was.

#include <cstdint>
#include <vector>

struct ViewFrame;
struct ViewSettings;

// false when nothing is laid over the sheet (the shader then skips the buffer)
bool build_overlay(std::vector<uint32_t>& cells, const ViewFrame& f, const ViewSettings& v);

// shared with the panels: the readability scale, 0..1 from a tile's number
float readable_shade(float value);
void readable_rgb(float shade, float* rgb);
