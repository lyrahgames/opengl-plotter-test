#include <application.hpp>

using namespace std;
using namespace gl;

graph::graph(size_t n, const float* x, const float* y) {
  x_min = *min_element(x, x + n);
  x_max = *max_element(x, x + n);
  y_min = *min_element(y, y + n);
  y_max = *max_element(y, y + n);

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
  glfwSetErrorCallback([](int error, const char* description) {
    throw runtime_error{"GLFW Error "s + to_string(error) + ": " + description};
  });
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);

  window = glfwCreateWindow(800, 600, "OpenGL Plotter Test", nullptr, nullptr);
  glfwGetFramebufferSize(window, &width, &height);

  table.emplace(window, this);

  glfwMakeContextCurrent(window);
  glbinding::initialize(glfwGetProcAddress);

  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);
}

application::~application() {
  table.erase(window);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void application::key_callback(GLFWwindow* window, int key, int scancode,
                               int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void application::char_callback(GLFWwindow* window, unsigned int codepoint) {
  if (codepoint == 'e') application::table.at(window)->plot_border = 50;
  if (codepoint == 's') application::table.at(window)->plot_border = 100;
  // cout << "Codepoint = " << static_cast<char>(codepoint) << '\n';
}

void application::scroll_callback(GLFWwindow* window, double x, double y) {
  table.at(window)->fov *= exp(-0.1f * y);
  // fov = glm::vec2{clamp(fov.x, 1e-5f, 10.0f),clamp(fov.y, 1e-5f, 10.0f) };
  // table.at(window)->fit_view();
}

void application::process_events() {
  // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  //   glfwSetWindowShouldClose(window, true);
}

void application::framebuffer_size_callback(GLFWwindow* window, int width,
                                            int height) {
  application::table.at(window)->width = width;
  application::table.at(window)->height = height;
  cout << "Framebuffer Size = " << width << " x " << height << '\n';
}

void application::plot(size_t n, const float* x, const float* y) {
  graphs.emplace_back(n, x, y);

  if (graphs.size() == 1) {
    x_min = graphs.back().x_min;
    x_max = graphs.back().x_max;
    y_min = graphs.back().y_min;
    y_max = graphs.back().y_max;
  } else {
    x_min = min(x_min, graphs.back().x_min);
    x_max = max(x_max, graphs.back().x_max);
    y_min = min(y_min, graphs.back().y_min);
    y_max = max(y_max, graphs.back().y_max);
  }
}

void application::execute() {
  vector<float> border_x_data = {x_min, x_max, x_max, x_min};
  vector<float> border_y_data = {y_min, y_min, y_max, y_max};

  glGenVertexArrays(1, &border_vertex_array);
  glBindVertexArray(border_vertex_array);

  GLuint border_x_data_buffer;
  glGenBuffers(1, &border_x_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, border_x_data_buffer);
  glBufferData(GL_ARRAY_BUFFER, border_x_data.size() * sizeof(float),
               border_x_data.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  GLuint border_y_data_buffer;
  glGenBuffers(1, &border_y_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, border_y_data_buffer);
  glBufferData(GL_ARRAY_BUFFER, border_y_data.size() * sizeof(float),
               border_y_data.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), nullptr);
  glEnableVertexAttribArray(1);

  const char* vertex_shader_src =
      "#version 330\n"
      "layout (location = 0) in float x_data;"
      "layout (location = 1) in float y_data;"
      "uniform mat4 mvp;"
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

  const char* point_vertex_shader_src =
      "#version 330\n"
      "layout (location = 0) in float x_data;"
      "layout (location = 1) in float y_data;"
      "uniform mat4 mvp;"
      "void main(){"
      "  gl_Position = mvp * vec4(x_data, y_data, 0.0, 1.0);"
      "}";
  auto point_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(point_vertex_shader, 1, &point_vertex_shader_src, nullptr);
  glCompileShader(point_vertex_shader);

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
  glAttachShader(point_program, point_vertex_shader);
  glAttachShader(point_program, point_fragment_shader);
  glLinkProgram(point_program);

  mvp_location = glGetUniformLocation(program, "mvp");
  point_mvp_location = glGetUniformLocation(point_program, "mvp");

  glEnable(GL_MULTISAMPLE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  while (!glfwWindowShouldClose(window)) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    old_mouse_pos = mouse_pos;
    mouse_pos = {xpos, ypos};

    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS) {
      const auto mouse_move = mouse_pos - old_mouse_pos;
      origin -= mouse_move * fov / glm::vec2{float(width), -float(height)};
      // fit_view();
    }

    process_events();
    render();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

void application::fit_view() {
  constexpr float scale_factor = 1.2f;
  origin = 0.5f * glm::vec2{x_min + x_max, y_min + y_max};
  fov = scale_factor * glm::vec2(x_max - x_min, y_max - y_min);
  // const auto dx = 0.5f * (x_max - x_min);
  // x_max += scale_factor * dx;
  // x_min -= scale_factor * dx;
  // const auto dy = 0.5f * (y_max - y_min);
  // y_max += scale_factor * dy;
  // y_min -= scale_factor * dy;
}

void application::render() {
  float x_min = origin.x - 0.5f * fov.x;
  float x_max = origin.x + 0.5f * fov.x;
  float y_min = origin.y - 0.5f * fov.y;
  float y_max = origin.y + 0.5f * fov.y;

  glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(plot_border, plot_border, width - 2 * plot_border,
             height - 2 * plot_border);
  glm::mat4 mvp(1.0f);
  mvp = glm::translate(mvp, {0, 0, -1});
  mvp = glm::ortho(x_min, x_max, y_min, y_max, 0.1f, 100.0f) * mvp;
  glLineWidth(2.0f);
  glPointSize(7.0f);

  for (const auto& graph : graphs) {
    glBindVertexArray(graph.vertex_array);
    glUseProgram(program);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    glDrawArrays(GL_LINE_STRIP, 0, samples);
    glUseProgram(point_program);
    glUniformMatrix4fv(point_mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    glDrawArrays(GL_POINTS, 0, samples);
  }

  // x_min = -0.5f * fov.x;
  // x_max = +0.5f * fov.x;
  // y_min = -0.5f * fov.y;
  // y_max = +0.5f * fov.y;

  // const auto x_border =
  //     (x_max - x_min) * plot_border / (width - 2 * plot_border);
  // const auto y_border =
  //     (y_max - y_min) * plot_border / (height - 2 * plot_border);
  // glViewport(0, 0, width, height);
  // glm::mat4 outer_mvp(1.0f);
  // outer_mvp = glm::translate(outer_mvp, {0, 0, -1});
  // outer_mvp = glm::ortho(x_min - x_border, x_max + x_border, y_min -
  // y_border,
  //                        y_max + y_border, 0.1f, 100.0f) *
  //             outer_mvp;
  // glLineWidth(5.0f);
  // glPointSize(5.0f);
  // glUseProgram(program);
  // glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(outer_mvp));
  // glBindVertexArray(border_vertex_array);
  // glDrawArrays(GL_LINE_LOOP, 0, 4);
  // glUseProgram(point_program);
  // glUniformMatrix4fv(point_mvp_location, 1, GL_FALSE,
  //                    glm::value_ptr(outer_mvp));
  // glDrawArrays(GL_POINTS, 0, 4);
}