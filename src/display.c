#include "display.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

uint8_t gfx[DISPLAY_HEIGHT][DISPLAY_WIDTH];

void sleep_ms(int ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms);
#endif
}

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

void init_display() {
  memset(gfx, 0, sizeof(gfx));
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

