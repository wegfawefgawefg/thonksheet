#pragma once

#include "world/task.hpp"

// inside the function battery: each kind fills the class of every input it can show
// (task_function_points.cpp, task_function_number.cpp); task_function.cpp draws and judges

constexpr int FN_PAST_A = 0, FN_PAST_B = 1, FN_PAST_ASK = 2;
constexpr int FN_SHEET_WIDTH = 8; // number kinds: x lines wrap at 8, in a snake

void points_classes(Task& task);
void past_classes(Task& task);
void number_classes(Task& task);
void pair_classes(Task& task);
// number: the x values never rewarded, drawn once per life
void number_hold(Task& task);
// number kinds: the shown value(s) onto the sheet, on the answer step
void number_sheet(const Task& task, TaskSense& sense);
// number kinds: the sheet's size
int number_sheet_w(const Task& task);
int number_sheet_h(const Task& task);
