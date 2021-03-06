#include <fstream>
#include <cstring>
#include <ncurses.h>
#include <string>
#include <vector>

typedef std::vector<std::vector<std::vector<int>>> map_t;

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

  map_t map(depth, std::vector<std::vector<int>>(height, std::vector<int>(width, 0)));

  const int xof = 0, yof = 0;
  WINDOW *mapw = newwin(depth + 2, width * 2 + 2 + 1, yof + 2, xof + 1);
  box(mapw, 0, 0);
  struct { int x, y; } cursor = { 0, 0 };

  // draw help text
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
    mvaddstr(yof + i + 2, xof + width * 2 + 6, help_text[i]);

  int height_level = 0, input = 0;

  while (input != 'q') {
    mvprintw(yof, xof + 1, "Height level: %d  " // extra spaces on purpose
        , height_level, cursor.x, cursor.y);

    // draw map and numbers to the left
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

    // draw numbers at the top
    for (int x = 0; x < width; ++x)
      mvwprintw(mapw, 0, x * 2 + 2, "%-2d", x);

    // draw cursor
    const int tile = map[cursor.y][height_level][cursor.x];
    if (tile)
      wattron(mapw, A_REVERSE);
    mvwaddstr(mapw, cursor.y + 1, cursor.x * 2 + 2, "##");
    if (tile)
      wattroff(mapw, A_REVERSE);

    // draw cursor arrows at the sides
    move(yof + 1, 1);
    clrtoeol();
    mvaddstr(yof + 1, cursor.x * 2 + xof + 3, "vv");
    for (int z = 0; z < depth; ++z)
      mvaddch(yof + z + 3, xof, ' ');
    mvaddch(cursor.y + yof + 3, xof, '>');

    refresh();
    wrefresh(mapw);

    input = getch();
    switch (input) {
      // Moving =====================================================
      case KEY_LEFT:  if (cursor.x > 0)         --cursor.x; break;
      case KEY_RIGHT: if (cursor.x < width - 1) ++cursor.x; break;
      case KEY_UP:    if (cursor.y > 0)         --cursor.y; break;
      case KEY_DOWN:  if (cursor.y < depth - 1) ++cursor.y; break;
      case 'j':
        if (height_level > 0)
          --height_level;
        break;
      case 'k':
        if (height_level < height - 1)
          ++height_level;
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
        mvaddstr(yof + depth + 4, 0,
            "Enter filename without extension (leave empty for \"level\"): ");
        char filename[256];
        getnstr(filename, 256);
        if (strcmp(filename, "\n") == 0 || strcmp(filename, "") == 0)
          strcpy(filename, "level");
        std::string filename_str(filename);

#if 0
        // .vxl text format
        std::ofstream ofs_vxl(filename_str + ".vxl", std::ofstream::binary);
        if (!ofs_vxl) {
          mvaddstr(yof + depth + 5, 0, "Error opening file.");
          refresh();
          break;
        }
        ofs_vxl << "# Automatically generated by level_editor" << std::endl;
        ofs_vxl << std::endl;
        for (int z = 0; z < depth; z++)
          for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
              if (map[z][y][x])
                ofs_vxl << "v " << x << " " << y << " " << z
                  << std::endl;
        ofs_vxl << std::endl;
        ofs_vxl.close();
#endif

        // .qkm binary format
        std::ofstream ofs_bin(filename_str + ".qkm", std::ofstream::binary);
        if (!ofs_bin) {
          mvaddstr(yof + depth + 5, 0, "Error opening file.");
          refresh();
          break;
        }
        struct {
          char magic[9] = "QEIKEMAP";
          uint8_t version = 1;
          uint32_t tiles = 0;
          uint16_t width, height, depth;
        } header;
        header.width = width;
        header.height = height;
        header.depth = depth;
        for (int z = 0; z < depth; z++)
          for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
              if (map[z][y][x])
                ++header.tiles;
        ofs_bin.write((char*)&header, sizeof(header));
        for (int z = 0; z < depth; z++)
          for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
              if (map[z][y][x]) {
                uint8_t x8 = x, y8 = y, z8 = z;
                ofs_bin.write((char*)&x8, sizeof(x8));
                ofs_bin.write((char*)&y8, sizeof(y8));
                ofs_bin.write((char*)&z8, sizeof(z8));
              }
        ofs_bin.close();

        mvaddstr(yof + depth + 5, 0, "Save successful.");

        noecho();
        curs_set(0);
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

