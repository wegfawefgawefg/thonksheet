#pragma once
// The project's colours for the panels; the same as shaders/common.glsl and the diagram.

#include <imgui.h>

inline const ImVec4 AMBER(0.949f, 0.722f, 0.294f, 1.0f);  // excitatory #f2b84b
inline const ImVec4 BLUE(0.290f, 0.639f, 1.000f, 1.0f);   // inhibitory #4aa3ff
inline const ImVec4 GREEN(0.373f, 0.827f, 0.553f, 1.0f);  // sensor #5fd38d
inline const ImVec4 PINK(1.000f, 0.420f, 0.604f, 1.0f);   // motor #ff6b9a
inline const ImVec4 VIOLET(0.690f, 0.549f, 1.000f, 1.0f); // far links #b08cff
inline const ImVec4 DIM(0.55f, 0.58f, 0.64f, 1.0f);
inline const ImVec4 ALARM(1.0f, 0.35f, 0.35f, 1.0f);

inline const char* const kind_names[4] = {"excitatory", "inhibitory", "sensor", "motor"};
inline const ImVec4 kind_colors[4] = {AMBER, BLUE, GREEN, PINK};
