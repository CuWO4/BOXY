#include "include/boxy.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <csignal>
#include <cstring>
#include <time.h>

static constexpr int fps = 50;

static struct termios original_termios;

void cleanup() {
  const char disable_mouse[] = "\033[?1003l\033[?1006l";
  write(STDOUT_FILENO, disable_mouse, sizeof(disable_mouse) - 1);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void clear() {
  static char clear[] = "\033[2J\033[H";
  write(STDOUT_FILENO, clear, sizeof(clear) - 1);
  fflush(stdout);
}

void switch_display_buffer() {
  static bool current_state = false;
  if (current_state) {
    printf("\033[?1049h");
  } else {
    printf("\033[?1049l");
  }
  current_state = !current_state;
}

void initialize() {
  /* save terminal state */
  tcgetattr(STDIN_FILENO, &original_termios);

  /* register signal handling */
  static struct sigaction sa;
  sa.sa_handler = +[](int sig) {
    cleanup();
    static struct sigaction sa_default;
    sa_default.sa_handler = SIG_DFL;
    sigemptyset(&sa_default.sa_mask);
    sa_default.sa_flags = 0;
    signal(sig, SIG_DFL);
    raise(sig);
  };
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);   /* Ctrl+C */
  sigaction(SIGTERM, &sa, NULL);  /* terminate */
  sigaction(SIGHUP, &sa, NULL);   /* terminal hang up */
  sigaction(SIGQUIT, &sa, NULL);  /* Ctrl+`\` */
  sigaction(SIGABRT, &sa, NULL);  /* abort */
  sigaction(SIGSEGV, &sa, NULL);  /* segment fault */
  sigaction(SIGPIPE, &sa, NULL);  /* pipe closed */

  /* register cleanup at exit */
  atexit(cleanup);

  /* set terminal to raw mode */
  struct termios raw = original_termios;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  /* enable mouse report */
  const char enable_mouse[] = "\033[?1003h\033[?1006h";
  write(STDOUT_FILENO, enable_mouse, sizeof(enable_mouse) - 1);
  fflush(stdout);

  /* hide cursor */
  const char hide_cursor[] = "\033[?25l";
  write(STDOUT_FILENO, hide_cursor, sizeof(hide_cursor) - 1);
  fflush(stdout);

  clear();
}

int has_input() {
  int bytes_available;
  ioctl(STDIN_FILENO, FIONREAD, &bytes_available);
  return bytes_available > 0;
}

static constexpr int left_drag_event = 32; /* may differ from XTerm or GTK terminal */
static constexpr int right_drag_event = 34;
bool get_next_mouse_event(int& event, int& col, int& row, bool& is_release) {
  if (!has_input()) return false;
  char buf[1];
  while (read(STDIN_FILENO, buf, 1) == 1) /* ignore other inputs */ {
    if (buf[0] == '\033') break;
    if (!has_input()) return false;
  }
  if (buf[0] != '\033') return false;
  if (read(STDIN_FILENO, buf, 1) != 1 || buf[0] != '[') return false;
  if (read(STDIN_FILENO, buf, 1) != 1 || buf[0] != '<') return false;

  static char seq[32];
  int seq_len = 0;
  while (seq_len + 1 < sizeof(seq) && read(STDIN_FILENO, &seq[seq_len], 1) == 1) {
    seq_len++;
    if (seq[seq_len - 1] == 'M' || seq[seq_len - 1] == 'm') break;
  }
  seq[seq_len] = '\0';
  if (sscanf(seq, "%d;%d;%d", &event, &col, &row) != 3) return false;
  is_release = seq[seq_len - 1] == 'm';
  return true;
}

int get_terminal_size(int& nr_rows, int& nr_cols) {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != 0) return -1;
  nr_rows = w.ws_row; nr_cols = w.ws_col;
  return 0;
}

time_t get_us() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void loop() {
  Boxy boxy;

  time_t last_time_us = 0;
  int last_col, last_row = -1;
  while (1) {
    time_t cur_time_us;
    while ((cur_time_us = get_us()) - last_time_us < 1000000 / fps) {
      usleep(2000);
    }
    last_time_us = cur_time_us;

    int frame_col = -1, frame_row = -1, frame_release;
    int event, col, row;
    int light_set_col = -1, light_set_row = -1;
    bool is_release;
    while(get_next_mouse_event(event, col, row, is_release)) {
      if (event == left_drag_event) {
        frame_col = col; frame_row = row; frame_release = false;
      } else {
        frame_col = frame_row = -1; frame_release = true;
      }

      if (event == right_drag_event) {
        light_set_col = col; light_set_row = row;
      }
    }

    if (frame_release) {
      last_row = last_col = -1;
    }

    int dx, dy;
    if (frame_col == -1 || frame_row == -1) {
      dx = dy = 0;
    } else {
      if (last_col == -1 || last_row == -1) {
        dx = dy = 0;
      } else {
        dx = frame_row - last_row;
        dy = frame_col - last_col;
      }
      last_row = frame_row; last_col = frame_col;
    }

    boxy.spin(dx * 2, dy); // the character is not a square
    int term_h = -1, term_w = -1;
    get_terminal_size(term_h, term_w);
    if (light_set_col >= 0) {
      float light[3];
      light[0] = light_set_row - (float) term_h / 2;
      light[1] = light_set_col - (float) term_w / 2;
      light[2] = 0;
      boxy.set_light(light);
    }

    switch_display_buffer();
    boxy.render(term_h, term_w);
  }
}

int main() {
  initialize();
  loop();
  return 0;
}