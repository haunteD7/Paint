#pragma once

template<typename T>
struct Vec2t
{
  T x,y;
};
using Vec2d = Vec2t<double>;
using Vec2u = Vec2t<unsigned int>;
using Vec2i = Vec2t<int>;
using Vec2 = Vec2d;
