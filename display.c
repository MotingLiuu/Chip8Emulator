#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

static uint8_t gfx[DISPLAY_HEIGHT][DISPLAY_WIDTH];

void init_console() {
  // \033[2J: 清空整个屏幕
  // \033[?25l: 隐藏控制台光标 (防止闪烁)
  printf("\033[2J\033[?25l");
  fflush(stdout);
}

void restore_console() {
  // \033[?25h: 恢复显示光标
  printf("\033[?25h\n");
}

void draw_display() {
  // \033[H: 将光标移动到屏幕左上角 (0, 0)
  // 这样做比每次 clear 屏幕能有效避免画面闪烁
  printf("\033[H");

  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      if (gfx[y][x]) {
        printf("██");
      } else {
        printf("  ");
      }
    }
    printf("\n");
  }
  fflush(stdout);
}

int main() {
  memset(gfx, 0, sizeof(gfx));
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

    SLEEP_MS(16);
  }

  restore_console();
  return 0;
}
