#include <application.hpp>
#include <cmath>
#include <vector>

using namespace std;

int main() {
  size_t samples = 100;
  float x_min = -10.0f;
  float x_max = 10.0f;
  vector<float> x_data(samples), y_data(samples);
  for (size_t i = 0; i < samples; ++i) {
    const auto scale = static_cast<float>(i) / (samples - 1);
    const auto x = x_min * (1.0f - scale) + x_max * scale;
    const auto y = (abs(x) < 1e-5f) ? (2.0f) : (sin(2 * x) / x);
    x_data[i] = x;
    y_data[i] = y;
  }

  application app{};
  app.plot(samples, x_data.data(), y_data.data());
  app.plot([](float x) { return sin(x); }, -5, 10, 100);
  app.plot([](float x) { return (x <= 0.0f) ? 0.0f : exp(-1.0f / x); }, -10, 10,
           100);
  app.fit_view();
  app.execute();
}