#include "../src/display.h"

int main() {
  init_display();
  init_console();

  for (int x = 0; x < DISPLAY_WIDTH; x++) {
    gfx[0][x] = 1;
    gfx[DISPLAY_HEIGHT - 1][x] = 1;
  }
  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    gfx[y][0] = 1;
    gfx[y][DISPLAY_WIDTH - 1] = 1;
  }

  int test_x = 2;
  int direction = 1;

  for (int frame = 0; frame < 200; frame++) {
    gfx[15][test_x] = 0;

    test_x += direction;
    if (test_x >= DISPLAY_WIDTH - 2 || test_x <= 1) {
      direction *= -1; 
    }

    gfx[15][test_x] = 1;

    draw_display();

    sleep_ms(16);
  }

  restore_console();
  return 0;
}
