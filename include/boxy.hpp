#ifndef BOXY_HPP__
#define BOXY_HPP__

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
  Boxy() {
    // TODO
  }

  void spin(double dx_2d, double dy_2d) {
    // TODO
  }

  void render(int term_h, int term_w) {
    if (term_h < 0 || term_w < 0) return;

    // TODO
  }

private:
  void spin(double spin_mat[3][3]) {
    // TODO;
  }

  // TODO
};

#endif
