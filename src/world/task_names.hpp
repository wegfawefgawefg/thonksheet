#pragma once

#include "world/task.hpp"

// Names of a task's lines, for people (the viewer's task panel and the marks on its sheet).
// Each returns nullptr when the task has no name of its own for that line; the caller then
// calls it by kind and number ("line 3", "number 1", "button 2"). A new task needs nothing
// here to work; a case in task_names.cpp only makes the viewer read better.

// on/off line i (0 is the task's first single, after good and bad)
const char* task_single_name(const Task& task, int i);
// number i
const char* task_number_name(const Task& task, int i);
// motor line (button) i
const char* task_motor_name(const Task& task, int i);
