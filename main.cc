#include <fstream>
#include <cstring>
#include <ncurses.h>
#include <string>

static void new_map();
static void start_screen();
static void ask_dimensions(int *width, int *height, int *depth);

int main() {
  initscr();
  cbreak();             // give keys w/o pressing return
  noecho();             // don't echo pressed keys to terminal
  keypad(stdscr, TRUE); // allow using more keys (F1, arrow keys etc.)

  start_screen();

  endwin();
}

static void start_screen() {
  nocbreak();
  echo();
  clear();
  mvaddstr(0, 0, "Welcome to the Level Editor");
  mvaddstr(2, 0, "Type in a number or letter:");
  mvaddstr(3, 0, "1) Create a [N]ew map");
  mvaddstr(4, 0, "2) [O]pen existing map to edit it (not working)");
  mvaddstr(5, 0, "3) [E]xit");
  mvaddstr(7, 0, "> ");
  refresh();

get_choice:
  char input[512];
  getnstr(input, 512);
  switch (input[0]) {
    case '1': case 'N': case 'n':
      new_map();
      break;
    case '2': case 'O': case 'o':
    case '3': case 'E': case 'e':
      break;
    default:
      mvaddstr(8, 0, "Unrecognized choice. Try \"1\", \"O\" or \"e\".");
      move(7, 0);
      clrtoeol();
      mvaddstr(7, 0, ">  ");
      move(7, 2);
      refresh();
      goto get_choice;
      break;
  }
}

static void new_map() {
  int width, height, depth;
  ask_dimensions(&width, &height, &depth);

  noecho();
  cbreak();
  clear();
  curs_set(0); // set terminal cursor to invisible

  int map[depth][height][width];

  for (int z = 0; z < depth; z++)
    for (int y = 0; y < height; y++)
      for (int x = 0; x < width; x++)
        map[z][y][x] = 0;

  const int xof = 1, yof = 2;
  WINDOW *mapw = newwin(depth + 2, width * 2 + 2 + 1, yof, xof);
  box(mapw, 0, 0);
  struct { int x, y; } cursor = { 0, 0 };

  const int help_text_lines = 8;
  const char help_text[help_text_lines][256] = {
    "Instructions:",
    "arrow keys - move in current height",
    "j/k        - up/down height level",
    "space/t    - toggle set/unset tile",
    "z          - set",
    "x          - erase",
    "e          - export",
    "q          - quit"
  };
  for (int i = 0; i < help_text_lines; ++i)
    mvaddstr(yof + i, xof + width * 2 + 2 + 1 + 2, help_text[i]);

  int height_level = 0, input = 0;

  while (input != 'q') {
    mvprintw(0, 1, "Height level: %d", height_level, cursor.x, cursor.y);

    for (int z = 0; z < depth; ++z) {
      mvwprintw(mapw, z + 1, 0, "%-2d", z);
      for (int x = 0; x < width; ++x) {
        int tile = map[z][height_level][x];
        if (tile)
          wattron(mapw, A_REVERSE);
        mvwaddstr(mapw, z + 1, x * 2 + 2, "  ");
        if (tile)
          wattroff(mapw, A_REVERSE);
      }
    }
    for (int x = 0; x < width; ++x)
      mvwprintw(mapw, 0, x * 2 + 2, "%-2d", x);

    // draw cursor
    const int tile = map[cursor.y][height_level][cursor.x];
    if (tile)
      wattron(mapw, A_REVERSE);
    mvwaddstr(mapw, cursor.y + 1, cursor.x * 2 + 2, "##");
    if (tile)
      wattroff(mapw, A_REVERSE);

    move(yof - 1, 1);
    clrtoeol();
    mvaddstr(yof - 1, cursor.x * 2 + xof + 2, "vv");
    for (int z = 0; z < depth; ++z)
      mvaddch(yof + z + 1, xof - 1, ' ');
    mvaddch(cursor.y + yof + 1, xof - 1, '>');

    refresh();
    wrefresh(mapw);

    input = getch();
    switch (input) {
      // Moving =====================================================
      case KEY_LEFT:
        if (cursor.x > 0)
          cursor.x--;
        break;
      case KEY_RIGHT:
        if (cursor.x < width - 1)
          cursor.x++;
        break;
      case KEY_UP:
        if (cursor.y > 0)
          cursor.y--;
        break;
      case KEY_DOWN:
        if (cursor.y < depth - 1)
          cursor.y++;
        break;
      case 'j':
        if (height_level > 0)
          height_level--;
        break;
      case 'k':
        if (height_level < height - 1)
          height_level++;
        break;
        // Modifying ==================================================
      case 32: // space
      case 't':
        map[cursor.y][height_level][cursor.x]
          = !map[cursor.y][height_level][cursor.x];
        break;
      case 'z':
        map[cursor.y][height_level][cursor.x] = 1;
        break;
      case 'x':
        map[cursor.y][height_level][cursor.x] = 0;
        break;
        // Else =======================================================
      case 'e': {
        curs_set(1);
        echo();
        mvaddstr(yof + depth + 2, 0,
            "Enter filename (leave empty for level.vxl): ");
        char filename[50];
        getnstr(filename, 50);
        if (strcmp(filename, "\n") == 0)
          strcpy(filename, "level.vxl");
        if (strcmp(filename, ".vxl") >= 0)
          strncat(filename, ".vxl", 50-strlen(filename)-1);
        noecho();
        curs_set(0);

        std::ofstream ofs(filename, std::ofstream::out);
        if (!ofs) {
          mvaddstr(yof + depth + 3, 0, "Error opening file.");
          refresh();
          break;
        }
        ofs << "# Automatically generated by level_editor" << std::endl;
        ofs << std::endl;
        for (int z = 0; z < depth; z++)
          for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
              if (map[z][y][x])
                ofs << "v " << x << " " << y << " " << z
                  << std::endl;
        ofs.close();
        mvaddstr(yof+depth+3, 0, "Save successful.");
        break;
      }
      default:
        break;
    }
  }

  delwin(mapw);
  curs_set(1);
}

static void ask_dimensions(int *width, int *height, int *depth) {
  nocbreak();
  echo();
  clear();
  mvaddstr(0, 0, "Enter dimensions for the level");
  mvaddstr(1, 0, "Width (x axis), Height (y axis) and Depth (-z axis)");
  mvaddstr(2, 0, "> ");
  refresh();

get_choice:
  char scan_buffer[512];
  mvgetnstr(2, 2, scan_buffer, 512);
  int scanned = sscanf(scan_buffer, "%d %d %d", width, height, depth);
  if (scanned != 3) {
    mvaddstr(4, 0, "Failed to parse.");
    move(2, 0);
    clrtoeol();
    mvaddstr(2, 0, ">  ");
    move(2, 2);
    refresh();
    goto get_choice;
  }
}

