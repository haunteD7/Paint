#include "paint.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main (int argc, char *argv[]) {
  
  Paint paint;
  paint.init(WINDOW_WIDTH, WINDOW_HEIGHT);
  paint.start();

  return 0;
}

