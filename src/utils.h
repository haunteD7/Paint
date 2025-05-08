#pragma once

#include "vec.h"

#include <list>

Vec2 screen_to_base(const Vec2& coords, double scale, const Vec2& camera_pos)
{
  return { coords.x / scale + camera_pos.x, -coords.y / scale + camera_pos.y };
}
Vec2 screen_to_base(const Vec2i& coords, double scale, const Vec2& camera_pos)
{
  return { (double)coords.x / scale + camera_pos.x, -(double)coords.y / scale + camera_pos.y };
}
Vec2i base_to_screen(const Vec2& coords, double scale, const Vec2& camera_pos)
{
  return { (coords.x - camera_pos.x) * scale, -(coords.y - camera_pos.y) * scale };
}
void convert_coords(const std::list<Vec2>& coords, std::list<Vec2>& new_coords) // Convert from relative to absolute coords
{
  if(coords.empty())
    return;

  if(!new_coords.empty())
    new_coords.clear();
  Vec2 current_coords;
  auto it = coords.begin();
  while(it != coords.end()) {
    if(it == coords.begin()) {
      current_coords = *it; 
    } else {
      current_coords.x += it->x; 
      current_coords.y += it->y; 
    }
    new_coords.push_back(current_coords);
    it++;
  }
}
