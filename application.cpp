#include <application.hpp>

using namespace std;
using namespace gl;

graph::graph(size_t n, const float* x, const float* y) {
  min = glm::vec2{*min_element(x, x + n), *min_element(y, y + n)};
  max = glm::vec2{*max_element(x, x + n), *max_element(y, y + n)};
  samples = n;

  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, 2 * n * sizeof(float), nullptr, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, n * sizeof(float), x);
  glBufferSubData(GL_ARRAY_BUFFER, n * sizeof(float), n * sizeof(float), y);

  glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float),
                        (void*)(n * sizeof(float)));
  glEnableVertexAttribArray(1);
}

graph::~graph() {
  // glDeleteBuffers(1, &vertex_buffer);
  // glDeleteVertexArrays(1, &vertex_array);
}

unordered_map<GLFWwindow*, application*> application::table{};

application::application() {
  create_opengl_context();
  update_view();
  init_shaders();
  init_opengl();
}

application::~application() {
  table.erase(window);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void application::create_opengl_context() {
  glfwSetErrorCallback([](int error, const char* description) {
    throw runtime_error{"GLFW Error "s + to_string(error) + ": " + description};
  });
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);

  window =
      glfwCreateWindow(width, height, "OpenGL Plotter Test", nullptr, nullptr);
  glfwGetFramebufferSize(window, &width, &height);
  table.emplace(window, this);

  glfwMakeContextCurrent(window);
  glbinding::initialize(glfwGetProcAddress);

  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);
}

void application::key_callback(GLFWwindow* window, int key, int scancode,
                               int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void application::char_callback(GLFWwindow* window, unsigned int codepoint) {
  // if (codepoint == 'e') application::table.at(window)->plot_border = 50;
  // if (codepoint == 's') application::table.at(window)->plot_border = 100;
  // cout << "Codepoint = " << static_cast<char>(codepoint) << '\n';
}

void application::scroll_callback(GLFWwindow* window, double x, double y) {
  table.at(window)->fov *= exp(-0.1f * y);
  // fov = glm::vec2{clamp(fov.x, 1e-5f, 10.0f),clamp(fov.y, 1e-5f, 10.0f) };
  table.at(window)->update_view();
}

void application::process_events() {
  // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  //   glfwSetWindowShouldClose(window, true);
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  old_mouse_pos = mouse_pos;
  mouse_pos = {xpos, ypos};

  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  if (state == GLFW_PRESS) {
    const auto mouse_move = mouse_pos - old_mouse_pos;
    origin -= mouse_move * fov /
              glm::vec2{float(width - 2 * plot_border),
                        -float(height - 2 * plot_border)};
    update_view();
  }
}

void application::framebuffer_size_callback(GLFWwindow* window, int width,
                                            int height) {
  table.at(window)->width = width;
  table.at(window)->height = height;
  table.at(window)->resize();
}

void application::plot(size_t n, const float* x, const float* y) {
  graphs.emplace_back(n, x, y);
}

void application::init_shaders() {
  const char* vertex_shader_src =
      "#version 330\n"
      "layout (location = 0) in float x_data;"
      "layout (location = 1) in float y_data;"
      "layout (std140) uniform MVP {"
      "  mat4 mvp;"
      "};"
      // "uniform mat4 mvp;"
      "void main(){"
      "  gl_Position = mvp * vec4(x_data, y_data, 0.0, 1.0);"
      "}";
  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
  glCompileShader(vertex_shader);

  const char* fragment_shader_src =
      "#version 330\n"
      "out vec4 color;"
      "void main(){"
      "  color = vec4(0.1, 0.1, 0.1, 1.0);"
      // "  float alpha = 1.0;"
      // "  vec2 tmp = 2.0 * gl_PointCoord - 1.0;"
      // "  vec2 tmp = gl_PointCoord;"
      // "  if (tmp.y < 0.0) alpha = 0.0;"
      // "  float r = tmp.y;"
      // "  float delta = fwidth(abs(r));"
      // "  alpha = 1.0 - smoothstep(0.0 - delta, 0.0 + delta, r);"
      // "  gl_FragColor = vec4(0.5, 0.1, 0.1, alpha);"
      "}";
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
  glCompileShader(fragment_shader);

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  const char* point_fragment_shader_src =
      "#version 330\n"
      // "#extension GL_OES_standard_derivatives : enable\n"
      // "out vec4 color;"
      "void main(){"
      "  float alpha = 1.0;"
      "  vec2 tmp = 2.0 * gl_PointCoord - 1.0;"
      // "  if (dot(tmp, tmp) > 1.0) alpha = 0.0;"
      // "  if ((abs(tmp.x) + abs(tmp.y)) > 1.0) discard;"
      "  float r = dot(tmp, tmp);"
      "  float delta = fwidth(0.5 * r);"
      "  alpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, r);"
      "  gl_FragColor = vec4(0.1, 0.3, 0.7, alpha);"
      "}";
  auto point_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(point_fragment_shader, 1, &point_fragment_shader_src, nullptr);
  glCompileShader(point_fragment_shader);

  point_program = glCreateProgram();
  glAttachShader(point_program, vertex_shader);
  glAttachShader(point_program, point_fragment_shader);
  glLinkProgram(point_program);

  // mvp_location = glGetUniformLocation(program, "mvp");
  // point_mvp_location = glGetUniformLocation(point_program, "mvp");

  glUniformBlockBinding(program, glGetUniformBlockIndex(program, "MVP"), 0);
  glUniformBlockBinding(point_program,
                        glGetUniformBlockIndex(point_program, "MVP"), 0);

  glGenBuffers(1, &mvp_location);
  glBindBuffer(GL_UNIFORM_BUFFER, mvp_location);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glBindBufferRange(GL_UNIFORM_BUFFER, 0, mvp_location, 0, sizeof(glm::mat4));
}

void application::init_opengl() {
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void application::execute() {
  while (!glfwWindowShouldClose(window)) {
    process_events();
    render();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

void application::fit_view() {
  if (graphs.size() == 0) return;

  glm::vec2 bound_min{INFINITY, INFINITY};
  glm::vec2 bound_max{-INFINITY, -INFINITY};
  for (const auto& graph : graphs) {
    bound_min = min(bound_min, graph.min);
    bound_max = max(bound_max, graph.max);
  }

  constexpr float scale_factor = 1.2f;
  origin = 0.5f * (bound_max + bound_min);
  fov = scale_factor * (bound_max - bound_min);
  update_view();
}

void application::resize() {
  glViewport(0, 0, width, height);
  update_view();
}

void application::update_view() {
  auto view_min = origin - 0.5f * fov;
  auto view_max = origin + 0.5f * fov;
  const auto border =
      (view_max - view_min) * float(plot_border) /
      glm::vec2{width - 2 * plot_border, height - 2 * plot_border};
  view_max += border;
  view_min -= border;

  glm::mat4 mvp(1.0f);
  mvp = glm::translate(mvp, {0, 0, -1});
  mvp =
      glm::ortho(view_min.x, view_max.x, view_min.y, view_max.y, 0.1f, 100.0f) *
      mvp;

  // glUseProgram(program);
  // glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

  // glUseProgram(point_program);
  // glUniformMatrix4fv(point_mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

  glBindBuffer(GL_UNIFORM_BUFFER, mvp_location);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(mvp));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void application::render() {
  glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glLineWidth(2.0f);
  glPointSize(7.0f);

  for (const auto& graph : graphs) {
    glBindVertexArray(graph.vertex_array);
    glUseProgram(program);
    glDrawArrays(GL_LINE_STRIP, 0, graph.samples);
    glUseProgram(point_program);
    glDrawArrays(GL_POINTS, 0, graph.samples);
  }
}