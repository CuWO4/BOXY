#ifndef BOXY_HPP__
#define BOXY_HPP__

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>

/*
 *           y
 *  z o------>
 *    |
 *    |
 *  x V
 *
 *    (shorter side)
 *  |<------ 2 ------>|
 *  |                 |
 *  +-----------------+ -----------
 *  |                 |          ^
 *  |   |<-- 1 -->|   |          |
 *  |   +---------+ --+----      |
 *  |   |         |   |  ^    adaptive
 *  |   |    .    |   |  1  (longer side)
 *  |   |  (0,0)  |   |  V       |
 *  |   +---------+ --+----      |
 *  |                 |          |
 *  |                 |          V
 *  +-----------------+ -----------
 *
 */

class Boxy {
public:
  Boxy() {}

  void spin(float dx_2d, float dy_2d) {
    float len = sqrt(dx_2d * dx_2d + dy_2d * dy_2d);
    float cos_theta = len != 0 ? dy_2d / len : 0,
          sin_theta = len != 0 ? -dx_2d / len : 0,
          phi = len / 50 * 3.14;
    float mat[3][3] = {};
    mat[0][0] = 1 - (1 - cos(phi)) * sin_theta * sin_theta;
    mat[0][1] = (1 - cos(phi)) * sin_theta * cos_theta;
    mat[0][2] = -sin(phi) * sin_theta;
    mat[1][0] = (1 - cos(phi)) * sin_theta * cos_theta;
    mat[1][1] = 1 - (1 - cos(phi)) * cos_theta * cos_theta;
    mat[1][2] = sin(phi) * cos_theta;
    mat[2][0] = sin(phi) * sin_theta;
    mat[2][1] = -sin(phi) * cos_theta;
    mat[2][2] = cos(phi);
    spin(mat);
  }

  void render(int term_h, int term_w) {
    if (term_h < 0 || term_w < 0) return;

    puts("\033[2J\033[H");

    float step = 2. / std::min(term_h, term_w);

    srand(axies[0][0] * 10237 + axies[1][1] * 126 + axies[2][2] * 1236876);

    for (int i = 0; i < term_h; i++) {
      for (int j = 0; j < term_w; j++) {
        int hit_face_idx;
        if (is_hit(hit_face_idx, step * (i - term_h / 2.), step * (j - term_w / 2.) / 2)) {
          float norm[3];
          memcpy(norm, axies[hit_face_idx], 3 * sizeof(float));
          if (norm[2] < 0) {
            for (int i = 0; i < 3; i++) norm[i] = -norm[i];
          }
          float brightness = 
            base_lightness[hit_face_idx] * (dot(light, norm) + 0.1)
            + ((float) rand() / RAND_MAX / brightness_level - 0.5 / brightness_level);
          if (brightness < 0) brightness = 0;
          if (brightness >= 1) brightness = 0.999;
          char ch = brightness_char[(size_t) (brightness * brightness_level)];
          putc(ch, stdout);
        } else {
          putc(' ', stdout);
        }
      }
      putc('\n', stdout);
    }
  }

  void set_light(float light[3]) {
    float len = sqrt(dot(light, light));
    if (len <= 1e-3) return;
    this->light[0] = light[0] / len;
    this->light[1] = light[1] / len;
    this->light[2] = light[2] / len;
  }

private:
  void spin(float spin_mat[3][3]) {
    float new_axies[3][3] = {};
    for (int k = 0; k < 3; k++) {
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
          new_axies[k][i] += spin_mat[i][j] * axies[k][j];
        }
      }
    }
    memcpy(axies, new_axies, 3 * 3 * sizeof(new_axies[0][0]));
  }

  float dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  bool is_hit(int& hit_face_idx, float x, float y) {
    float ori[3] = {x, y, 0.0f};
    float dir[3] = {0.0f, 0.0f, -1.0f};
    float extent = 0.5f;

    float t_entry = -std::numeric_limits<float>::infinity();
    float t_exit = std::numeric_limits<float>::infinity();

    int entry_face_axis_index = -1;

    for (int i = 0; i < 3; ++i) {
      const float* axis = axies[i];
      float denom = dot(dir, axis);
      float nom = dot(ori, axis);
      if (std::abs(denom) < 1e-6f) {
        if (-nom - extent > 0 || -nom + extent < 0) return false;
      } else {
        float t1 = (extent - nom) / denom;
        float t2 = (-extent - nom) / denom;

        float near = std::min(t1, t2);
        float far = std::max(t1, t2);

        if (near > t_entry) {
          t_entry = near;
          entry_face_axis_index = i;
        }
        if (far < t_exit) {
          t_exit = far;
        }
      }
    }

    if (t_entry >= t_exit) return false;

    if (entry_face_axis_index != -1) {
      hit_face_idx = entry_face_axis_index;
      return true;
    }

    return false;
  }


  float light[3] = { 0, 1, 0 };
  float axies[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };

  float base_lightness[3] = { 0.8, 0.9, 1.0 };
  const char* brightness_char = "```````.''''::_,,,^^^===;>>><!rc/zzzLv)|i{3lnZya2wwwwww6dVObXXXXH8R#BgMMNQQQ%%&&@@@@@@@@@@@@@@@@@@@";
  int brightness_level = strlen(brightness_char);
};

#endif
