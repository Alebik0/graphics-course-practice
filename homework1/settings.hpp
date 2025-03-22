#include <array>
#include <cmath>

// Project settings:
const int W = 100;
const int H = 100;
const float X_MIN = 0.f;
const float Y_MIN = 0.f;
const float X_MAX = 1.f;
const float Y_MAX = 1.f;
const float PI = 3.1415f;
const float MAX_VALUE = 2.f;
const float MIN_VALUE = -2.f;
const float CHANGE_VALUE = 0.f;
const float C_ARRAY[4] = { -0.66f, -0.33f, 0.33f, 0.66 };
const float C = 0.5f;
const float ISOLINES_MIN = -1.f;
const float ISOLINES_MAX = 1.f;

float f(const float x, const float y, const float t) {
    return sin(4 * PI * x * y + 1234 + t);
}
