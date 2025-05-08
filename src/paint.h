#pragma once

#include "vec.h"

#include <list>
#include <string>
#include <SDL.h>

#define FILE_PATH_LENGTH 100

class Paint
{
public:
  enum Status
  {
    Success = 0,
    Fail,
  };
  
  Status init(int window_width, int window_height);
  Status start();

  struct PaintConfigurable {
    char current_file_path[FILE_PATH_LENGTH];
    double scale;
    Vec2 camera_pos;
  };
private:
  void handle_event(const SDL_Event* event);

  void render_main();
  void render_GUI();
  void render_grid();
  void render_stitches();
  void render_new_stitch_line();

  bool load_config_from_file(std::string path);
  bool save_config_to_file(std::string path);
  bool load_stitches_from_file(std::string path);
  bool save_stitches_to_file(std::string path);
  
  void add_stitch(const Vec2& coords);
  bool check_can_stitch_be_placed(const Vec2& coords);
  void place_stitch();
  bool undo();
  Vec2 calculate_drawing_size();

  SDL_Window* window; 
  SDL_Renderer* renderer;
  bool is_running;
  int window_width, window_height;
  char current_file_path[FILE_PATH_LENGTH]; // Current loaded file
  char new_file_path[FILE_PATH_LENGTH]; // File in file path input

  double scale;
  Vec2 camera_pos; // Offset to left top corner in base coord system
  Vec2 mouse_pos_base; // Mouse pos in base coords system
  std::list<Vec2> stitches; // In base coord system, each point is relative to previous point
  std::list<Vec2> stitches_abs; // In base coords systems, absolute coords of each point
  bool can_stitch_be_placed = false;
  Vec2 drawing_size = { 0, 0 };

};
