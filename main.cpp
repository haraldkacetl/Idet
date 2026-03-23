#include <ncurses.h>
#include <fstream>
#include <vector>
#include <string>

#define CTRL_KEY(k) ((k) & 0x1f)

int lineNumberScheme = 1; // 1 or 2
int contentScheme = 3;    // 3 or 4
bool selectionActive = false;
int selStartY = 0, selStartX = 0;
int selEndY = 0, selEndX = 0;
std::string clipboard;

std::vector<std::string> buffer;

void loadFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        buffer.push_back(line);
    }
}
void draw(int cursorY, int cursorX, int rowOffset, const std::string& filename,
          int lineNumberScheme, int contentScheme) {
    erase();

    int lineNumberWidth = std::to_string(buffer.size()).length() + 2;
    int maxRows = LINES - 2; // leave last line for status bar

    // header
    attron(A_BOLD);
    mvhline(COLOR_WHITE, COLOR_BLACK, ' ', COLS); // fill header background
    mvprintw(0, 0, "Idet-Editor - File: %s", filename.c_str());
    attroff(A_BOLD);

    // draw file content with full background
    for (int i = 0; i < maxRows; ++i) {
        int fileLine = rowOffset + i;
        if (selectionActive) {
            int startY = std::min(selStartY, selEndY);
            int endY = std::max(selStartY, selEndY);

            for (int y = startY; y <= endY; y++) {
                int lineStartX = (y == selStartY) ? selStartX : 0;
                int lineEndX = (y == selEndY) ? selEndX : buffer[y].size();

                // highlight selection
                for (int x = lineStartX; x < lineEndX; x++) {
                    mvchgat(y - rowOffset + 1, x + lineNumberWidth, 1, A_REVERSE, contentScheme, NULL);
                }
            }
        }
        // line number column
        attron(COLOR_PAIR(lineNumberScheme));
        mvhline(i + 1, 0, ' ', lineNumberWidth); // fill column
        if (fileLine < (int)buffer.size()) {
            mvprintw(i + 1, 0, "%*d", lineNumberWidth - 1, fileLine + 1);
        }
        attroff(COLOR_PAIR(lineNumberScheme));

        // file content column
        attron(COLOR_PAIR(contentScheme));
        mvhline(i + 1, lineNumberWidth, ' ', COLS - lineNumberWidth); // fill line
        if (fileLine < (int)buffer.size()) {
            mvprintw(i + 1, lineNumberWidth, "%s", buffer[fileLine].c_str());
        }
        attroff(COLOR_PAIR(contentScheme));
    }

    // status bar
    attron(A_REVERSE);
    mvhline(LINES - 1, 0, ' ', COLS); // fill status bar background
    mvprintw(LINES - 1, 0, "CTRL+S=Save | CTRL+Q=Quit | Line %d/%d | Column %d/%d",
             cursorY + 1, (int)buffer.size(),
             cursorX + 1, (int)buffer[cursorY].size() + 1);
    attroff(A_REVERSE);

    // move cursor
    move(cursorY - rowOffset + 1, cursorX + lineNumberWidth);
    refresh();
}




void saveFile(const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& line : buffer) {
        file << line << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    loadFile(argv[1]);
    if (buffer.empty()) buffer.push_back("");

    // init ncurses
    initscr();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_CYAN);  // Line numbers default
    init_pair(2, COLOR_WHITE, COLOR_BLUE);  // Alternate line number scheme
    init_pair(3, COLOR_WHITE, COLOR_BLACK); // File content default
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Alternate file content scheme
    use_default_colors();
    init_pair(1, COLOR_BLACK, COLOR_CYAN); // line number background
    raw();
    keypad(stdscr, TRUE);
    noecho();

    int cursorX = 0, cursorY = 0;
    int rowOffset = 0; // scrolling offset
    int ch;

while (true) {
    // Draw screen
    draw(cursorY, cursorX, rowOffset, argv[1], lineNumberScheme, contentScheme);

    // Get input
    ch = getch();

    // Quit
    if (ch == CTRL_KEY('q')) {
        break;
    }
    // Save
    else if (ch == CTRL_KEY('s')) {
        saveFile(argv[1]);
    }
    // Start/stop selection
    else if (ch == KEY_F(3)) {
        if (!selectionActive) {
            selectionActive = true;
            selStartY = selEndY = cursorY;
            selStartX = selEndX = cursorX;
        } else {
            selectionActive = false;
        }
    }

    // ← Step 5: Copy selected text
    else if (ch == CTRL_KEY('c') && selectionActive) {
        clipboard.clear();
        int startY = std::min(selStartY, selEndY);
        int endY = std::max(selStartY, selEndY);
        for (int y = startY; y <= endY; y++) {
            int lineStartX = (y == selStartY) ? selStartX : 0;
            int lineEndX = (y == selEndY) ? selEndX : buffer[y].size();
            clipboard += buffer[y].substr(lineStartX, lineEndX - lineStartX);
            if (y != endY) clipboard += "\n";
        }
        selectionActive = false;
    }

    // ← Step 6: Paste clipboard
    else if (ch == CTRL_KEY('v') && !clipboard.empty()) {
        size_t pos = 0, prev = 0;
        while ((pos = clipboard.find('\n', prev)) != std::string::npos) {
            std::string line = clipboard.substr(prev, pos - prev);
            buffer[cursorY].insert(cursorX, line);
            cursorY++;
            buffer.insert(buffer.begin() + cursorY, "");
            cursorX = 0;
            prev = pos + 1;
        }
        // last line
        std::string lastLine = clipboard.substr(prev);
        buffer[cursorY].insert(cursorX, lastLine);
        cursorX += lastLine.size();
    }
        // if F1 show help menu
        if (ch == KEY_F(1)) {
            clear();
            mvprintw(0, 0, "Help - Idet-Editor");
            mvprintw(2, 0, "CTRL+S: Save file");
            mvprintw(3, 0, "CTRL+Q: Quit editor");
            mvprintw(4, 0, "Arrow Keys: Move cursor");
            mvprintw(5, 0, "HOME/END: Move to line start/end");
            mvprintw(7, 0, "Press any key to return...");
            refresh();
            getch();
            continue;
        }
        else if (ch == KEY_F(3)) {
    // toggle selection mode
        if (!selectionActive) {
            selectionActive = true;
            selStartY = selEndY = cursorY;
            selStartX = selEndX = cursorX;
        } else {
            selectionActive = false; // finish selection
        }
        }
        // if F2 change color scheme
        else if (ch == KEY_F(2)) {
            // toggle line number scheme
            lineNumberScheme = (lineNumberScheme == 1) ? 2 : 1;

            // toggle content scheme
            contentScheme = (contentScheme == 3) ? 4 : 3;
        }
        if (ch == CTRL_KEY('q')) {
            break;
        }
        else if (ch == CTRL_KEY('s')) {
            saveFile(argv[1]);
        }
        else if (ch == KEY_HOME) {
            cursorX = 0;
        }
        else if (ch == KEY_END) {
            cursorX = buffer[cursorY].size();
        }
        else if (ch == KEY_UP) {
            if (cursorY > 0) cursorY--;
            if (selectionActive) {
        selEndY = cursorY;
        selEndX = cursorX;
        }
        }
        else if (ch == KEY_DOWN) {
            if (cursorY < (int)buffer.size() - 1) cursorY++;
            if (selectionActive) {
            selEndY = cursorY;
            selEndX = cursorX;
            }
        }
        else if (ch == KEY_LEFT) {
            if (cursorX > 0) cursorX--;
            if (selectionActive) {
            selEndY = cursorY;
            selEndX = cursorX;
            }
        }
        else if (ch == KEY_RIGHT) {
            if (cursorX < (int)buffer[cursorY].size()) cursorX++;
            if (selectionActive) {
            selEndY = cursorY;
            selEndX = cursorX;
            }
        }
        else if (ch == 10) { // ENTER
            std::string newLine = buffer[cursorY].substr(cursorX);
            buffer[cursorY] = buffer[cursorY].substr(0, cursorX);
            buffer.insert(buffer.begin() + cursorY + 1, newLine);
            cursorY++;
            cursorX = 0;
        }
        else if (ch == KEY_BACKSPACE || ch == 127) {
            if (cursorX > 0) {
                buffer[cursorY].erase(cursorX - 1, 1);
                cursorX--;
            }
            else if (cursorY > 0) { // merge lines
                cursorX = buffer[cursorY - 1].size();
                buffer[cursorY - 1] += buffer[cursorY];
                buffer.erase(buffer.begin() + cursorY);
                cursorY--;
            }
        }
        else if (ch >= 32 && ch <= 126) { // printable
            buffer[cursorY].insert(cursorX, 1, ch);
            cursorX++;
        }
    }

    endwin();
    return 0;
}