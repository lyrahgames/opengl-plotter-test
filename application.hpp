#pragma once
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
//
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
//
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//
#include <glm/glm.hpp>
//
#include <glm/ext.hpp>

struct graph {
  graph(size_t n, const float* x, const float* y);
  ~graph();
  graph(const graph&) = delete;
  graph& operator=(const graph&) = delete;
  graph(graph&&) = delete;
  graph& operator=(graph&&) = delete;

  // float x_min, x_max;
  // float y_min, y_max;
  glm::vec2 min, max;
  size_t samples;
  gl::GLuint vertex_array;
  gl::GLuint vertex_buffer;
};

class application {
 public:
  application();
  ~application();
  void execute();

  void plot(size_t n, const float* x, const float* y);
  template <typename Function>
  void plot(Function&& function, float a, float b, size_t n);
  void fit_view();

 private:
  // We use table of all possible windows to transform the GLFW window pointer
  // into the this pointer because all callbacks have to be helper functions.
  static std::unordered_map<GLFWwindow*, application*> table;
  // We need to use static functions instead of lambda expressions as callbacks
  // to be able to access private variables.
  static void key_callback(GLFWwindow* window, int key, int scancode,
                           int action, int mods);
  static void char_callback(GLFWwindow* window, unsigned int codepoint);
  static void framebuffer_size_callback(GLFWwindow* window, int width,
                                        int height);
  static void scroll_callback(GLFWwindow* window, double x, double y);

 private:
  void create_opengl_context();
  void init_shaders();
  void init_opengl();
  void process_events();
  void update_view();
  void resize();
  void render();

 private:
  GLFWwindow* window;
  int width = 500;
  int height = 500;
  unsigned int plot_border = 100;

  gl::GLuint program;
  gl::GLuint point_program;
  gl::GLuint mvp_location;

  glm::vec2 origin{};
  glm::vec2 fov{1, 1};

  glm::vec2 mouse_pos{};
  glm::vec2 old_mouse_pos{};

  std::list<graph> graphs;
};

template <typename Function>
void application::plot(Function&& function, float a, float b, size_t n) {
  if (n < 2) return;
  std::vector<float> x_data(n);
  std::vector<float> y_data(n);
  for (size_t i = 0; i < n; ++i) {
    const auto scale = float(i) / (n - 1);
    x_data[i] = a * (1.0f - scale) + b * scale;
    y_data[i] = function(x_data[i]);
  }
  plot(n, x_data.data(), y_data.data());
}