//为了减短GUI.cpp长度
//多数和Class不相关的函数都被放到这里

#pragma once

#include "resource.h"

#define BUTTON_IN(x,y) if(x == Hash(y))
#define Delta 10 //按钮渐变色速度

constexpr auto MAX_BUTTON = 5;
constexpr auto MAX_TEXT = 25;
constexpr auto MAX_LINE = 20;
constexpr auto MAX_STRING = 20;