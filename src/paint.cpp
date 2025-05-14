#include "paint.h"
#include "utils.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <SDL.h>

#include <iostream>
#include <fstream>
#include <algorithm>

#define MIN_SCALE_VAL 1

std::string default_config_path = "config.bin";

Paint::PaintConfigurable default_config = {
  .current_file_path = "Untitled.txt",
  .scale = 20,
  .camera_pos = { 0, 0 }
};

double max_stitch_length = 10;

SDL_Color clear_color = { 200, 200, 200, 255 };
SDL_Color grid_color = { 0, 0, 0, 255 };
SDL_Color stitch_color = { 255, 0, 0, 255 };
SDL_Color line_color = { 0, 0, 255, 255 };
SDL_Color new_line_color = { 0, 255, 0, 255 };

double zoom_speed = 1.1;
int stitch_size = 10; // Stitch size in pixels

void Paint::place_stitch()
{
  if(can_stitch_be_placed)
    add_stitch(mouse_pos_base);
}

void Paint::render_main()
{
  render_grid();
  render_stitches();
  if(stitches.size() > 0 && can_stitch_be_placed)
    render_new_stitch_line();
}

void Paint::render_grid()
{
  SDL_SetRenderDrawColor(renderer, grid_color.r, grid_color.g, grid_color.b, grid_color.a);

  long double integ;
  double x_start = std::modfl(camera_pos.x, &integ);
  double y_start = std::modfl(camera_pos.y, &integ);

  for(double i=-x_start * scale; i <= window_width; i += scale) // Lines along x axis
  {
    SDL_RenderDrawLine(renderer, i, 0, i, window_height);
  }
  for(double i=y_start * scale; i <= window_height; i += scale) // Lines along y axis
  {
    SDL_RenderDrawLine(renderer, 0, i, window_width, i);
  }
}

void Paint::render_GUI()
{
  // Start the Dear ImGui frame
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  {
    ImGui::Begin("Menu");                          

    Vec2 drawing_size = calculate_drawing_size();
    ImGui::Text("Current file name: %s\nDrawing size: x = %.2f y = %.2f\nStitches amount: %d\nScale: %.2f", current_file_path, drawing_size.x, drawing_size.y, stitches.size(), scale);
    if(ImGui::Button("Undo"))
    {
      undo();
    }
    if(!stitches.empty()) {
      const Vec2& last_stitch = stitches_abs.back();
      ImGui::Text("Relative cursor pos: x = %.2f; y = %.2f\n", mouse_pos_base.x - last_stitch.x, mouse_pos_base.y - last_stitch.y);
    }
    ImGui::InputText("File path", new_file_path, FILE_PATH_LENGTH);
    if(ImGui::Button("Save")) {
      if(strlen(new_file_path)) {
        save_stitches_to_file(new_file_path);
        strcpy(current_file_path, new_file_path);
      }
    }
    ImGui::SameLine();
    if(ImGui::Button("Load")) {
      if(strlen(new_file_path)) {
        bool is_loaded = load_stitches_from_file(new_file_path);
        if(is_loaded) {
          convert_coords(stitches, stitches_abs);
        }
      }
    }
  
    ImGui::End();
  }

  // Rendering
  ImGui::Render();
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
}

Vec2 Paint::calculate_drawing_size()
{
  auto cmp_x = [](const Vec2& a, const Vec2& b){ return a.x < b.x; }; // Comparison functions
  auto cmp_y = [](const Vec2& a, const Vec2& b){ return a.y < b.y; };
  double x, y;  
  x = std::max_element(stitches_abs.begin(), stitches_abs.end(), cmp_x)->x - std::min_element(stitches_abs.begin(), stitches_abs.end(), cmp_x)->x;
  y = std::max_element(stitches_abs.begin(), stitches_abs.end(), cmp_y)->y - std::min_element(stitches_abs.begin(), stitches_abs.end(), cmp_y)->y;

  return { x, y };
}

void Paint::render_stitches()
{
  SDL_Rect rect;
  rect.w = stitch_size;
  rect.h = stitch_size;

  if(stitches_abs.size() > 1) { // render lines
    auto it = stitches_abs.begin();
    for(int i=1; i<stitches_abs.size(); i++) {
      Vec2i current = base_to_screen(*(it), scale, camera_pos);
      Vec2i next = base_to_screen(*(++it), scale, camera_pos);
      SDL_SetRenderDrawColor(renderer, line_color.r, line_color.g, line_color.b, line_color.a);
      SDL_RenderDrawLine(renderer, current.x, current.y, next.x, next.y);
    }
  }
  for(auto it = stitches_abs.begin(); it != stitches_abs.end(); it++) { // Render points
    Vec2i current = base_to_screen(*it, scale, camera_pos);
    rect.x = current.x - stitch_size / 2;
    rect.y = current.y - stitch_size / 2;
    SDL_SetRenderDrawColor(renderer, stitch_color.r, stitch_color.g, stitch_color.b, stitch_color.a);
    SDL_RenderFillRect(renderer, &rect);
  }
}

void Paint::render_new_stitch_line()
{
  Vec2i mouse_pos;
  Vec2i last_point_pos_screen = base_to_screen(stitches_abs.back(), scale, camera_pos);
  mouse_pos = base_to_screen(mouse_pos_base, scale, camera_pos);

  SDL_SetRenderDrawColor(renderer, new_line_color.r, new_line_color.g, new_line_color.b, new_line_color.a);
  SDL_RenderDrawLine(renderer, mouse_pos.x, mouse_pos.y, last_point_pos_screen.x, last_point_pos_screen.y);
}

bool Paint::check_can_stitch_be_placed(const Vec2& cooords)
{
  if(stitches.empty())
    return true;

  const Vec2& last_stitch = stitches_abs.back();
  double distance_sqr = std::powl(last_stitch.x - cooords.x, 2) + std::powl(last_stitch.y - cooords.y, 2);

  return (distance_sqr <= std::powl(max_stitch_length, 2));
}

bool Paint::load_config_from_file(std::string path)
{
  std::fstream fs;
  fs.open(path, std::ios::in | std::ios::binary);
  if(!fs.is_open()) {
    strcpy(current_file_path, default_config.current_file_path);
    strcpy(new_file_path, default_config.current_file_path);
    scale = default_config.scale;
    camera_pos = default_config.camera_pos;
    return false;
  }

  PaintConfigurable conf;
  fs.read((char*)&conf, sizeof(conf));

  strcpy(current_file_path, conf.current_file_path);
  strcpy(new_file_path, conf.current_file_path);
  scale = conf.scale;
  camera_pos = conf.camera_pos;

  fs.close();
  return true;
}
bool Paint::save_config_to_file(std::string path)
{
  std::fstream fs;
  fs.open(path, std::ios::out | std::ios::binary);
  if(!fs.is_open())
    return false;
  
  PaintConfigurable conf;
  conf.scale = scale;
  strcpy(conf.current_file_path, current_file_path);
  conf.camera_pos = camera_pos;

  fs.write((char*)&conf, sizeof(conf));

  fs.close();
  return true;
}
bool Paint::load_stitches_from_file(std::string path)
{
  std::fstream fs;
  fs.open(path, std::ios::in);
  if(!fs.is_open())
    return false;

  if(!stitches.empty())
    stitches.clear();

  std::string line;
  while(std::getline(fs, line)) {
    size_t space_pos = line.find(" ");

    Vec2 stitch;
    stitch.x = std::stod(line.substr(0, space_pos));
    stitch.y = std::stod(line.substr(space_pos, line.size()));

    stitches.push_back(stitch);
  }
  strcpy(current_file_path, path.c_str());

  fs.close();
  return true;
}
bool Paint::save_stitches_to_file(std::string path)
{
  std::fstream fs;
  fs.open(path, std::ios::out);
  if(!fs.is_open())
    return false;

  for(const auto& stitch : stitches) {
    fs << stitch.x << " " << stitch.y << "\n";
  }

  fs.close();
  return true;
}

void Paint::handle_event(const SDL_Event* event)
{
  ImGui_ImplSDL2_ProcessEvent(event);

  const auto& io = ImGui::GetIO(); 
  bool GUI_occupied = io.WantCaptureMouse;
  switch (event->type) {
    case SDL_QUIT:
    {
      is_running = false;
      break;
    }
    case SDL_WINDOWEVENT:
    {
      if(event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) // Window resize
      {
        window_width = event->window.data1;
        window_height = event->window.data2;
      }
      break;
    }
    case SDL_MOUSEWHEEL:
    {
      if(!GUI_occupied) { 
        double new_scale;
        if(event->wheel.preciseY > 0) { // Zoom
          new_scale = scale * zoom_speed;
        } else {
          new_scale = scale / zoom_speed;
        }
        if(new_scale > MIN_SCALE_VAL) 
          scale = new_scale;
        else
          scale = MIN_SCALE_VAL;
        Vec2i mouse_pos;
        SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
        mouse_pos_base = screen_to_base(mouse_pos, scale, camera_pos);
        can_stitch_be_placed = check_can_stitch_be_placed(mouse_pos_base);
      }
      break;
    }
    case SDL_MOUSEMOTION: // Camera movement
    {
      if(!GUI_occupied) {
        Vec2i mouse_pos;
        SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
        mouse_pos_base = screen_to_base(mouse_pos, scale, camera_pos);
        mouse_pos_base.x = std::round(mouse_pos_base.x);
        mouse_pos_base.y = std::round(mouse_pos_base.y);
        can_stitch_be_placed = check_can_stitch_be_placed(mouse_pos_base);

        bool rmb_pressed = SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK; // Is right mouse button pressed
        if(rmb_pressed)
        {
          Vec2i motion_screen = { event->motion.xrel, event->motion.yrel };
          Vec2 motion_base = screen_to_base(motion_screen, scale, {0});
          camera_pos = { camera_pos.x - motion_base.x, camera_pos.y - motion_base.y };
        }
      }
      break;
    }
    case SDL_MOUSEBUTTONDOWN:
    {
      if(!GUI_occupied) {
        if(event->button.button == SDL_BUTTON_LEFT)
        {
          place_stitch();
        }
      }
      break;
    }
    case SDL_KEYDOWN:
    {
      if(!GUI_occupied) {
        switch (event->key.keysym.sym) {
          case SDLK_RETURN:
          {
            place_stitch();
            break;
          }
          case SDLK_ESCAPE:
          {
            is_running = false;
            break;
          }
        }
      }
      break;
    }
  }
}
void Paint::add_stitch(const Vec2& coords)
{
  if(stitches.empty())
  {
    stitches.push_back(coords);
    stitches_abs.push_back(coords);
  }
  else {
    Vec2& last_stitch_coords = stitches_abs.back(); 
    stitches.push_back({ coords.x - last_stitch_coords.x, coords.y - last_stitch_coords.y });
    stitches_abs.push_back(coords);
  }
}
bool Paint::undo()
{
  if(stitches.empty())
    return false;
  stitches.pop_back();
  stitches_abs.pop_back();
  
  return true;
}

Paint::Status Paint::init(int window_width, int window_height)
{
  this->window_width = window_width;
  this->window_height = window_height;

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    printf("Error: %s\n", SDL_GetError());
    return Status::Fail;
  }

  // Create window with SDL_Renderer graphics context
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window = SDL_CreateWindow("Paint", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, window_flags);
  if (window == nullptr)
  {
    std::cout << "Error: SDL_CreateWindow(): " << SDL_GetError() << "\n";
    return Status::Fail;
  }
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr)
  {
    std::cout << "Error creating SDL_Renderer;\n";
    return Status::Fail;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); 
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  return Status::Success;
}

Paint::Status Paint::start()
{
  load_config_from_file(default_config_path);
  if(load_stitches_from_file(current_file_path)) {
    convert_coords(stitches, stitches_abs);
  } else {
    strcpy(current_file_path, default_config.current_file_path);
    strcpy(new_file_path, default_config.current_file_path);
  }

  is_running = true;
  while (is_running) // Main loop
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      handle_event(&event);
    }

    SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    SDL_RenderClear(renderer);

    render_main();
    render_GUI();

    SDL_RenderPresent(renderer);
  }

  // Cleanup
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  save_config_to_file(default_config_path);
  for(const auto& stitch : stitches) { // Print stitches' coords
    std::cout << "(" << stitch.x << "; " << stitch.y << ")\n";
  }

  return Success;
}
