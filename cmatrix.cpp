// cmatrix.cpp - efeito Matrix no terminal (Windows / Linux / macOS)
// Windows (MSVC):  cl /EHsc /O2 cmatrix.cpp
// Windows (MinGW): g++ -O2 -o cmatrix.exe cmatrix.cpp
// Linux/macOS:     g++ -O2 -o cmatrix cmatrix.cpp
// Sair: q, ESC ou Ctrl+C

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
  #include <conio.h>
  #include <windows.h>
#else
  #include <sys/ioctl.h>
  #include <sys/select.h>
  #include <termios.h>
  #include <unistd.h>
#endif

static volatile std::sig_atomic_t g_running = 1;
static void onSignal(int) { g_running = 0; }

#ifndef _WIN32
static termios g_oldTerm;
#endif

static void getSize(int& w, int& h) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    winsize ws{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    w = ws.ws_col ? ws.ws_col : 80;
    h = ws.ws_row ? ws.ws_row : 24;
#endif
}

static void initTerminal() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(out, &mode);
    SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    tcgetattr(STDIN_FILENO, &g_oldTerm);
    termios raw = g_oldTerm;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
    std::fputs("\x1b[?1049h\x1b[?25l\x1b[2J", stdout); // tela alternativa, esconde cursor
    std::fflush(stdout);
}

static void restoreTerminal() {
    std::fputs("\x1b[0m\x1b[?25h\x1b[?1049l", stdout);
    std::fflush(stdout);
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSANOW, &g_oldTerm);
#endif
}

static bool quitPressed() {
#ifdef _WIN32
    while (_kbhit()) {
        int c = _getch();
        if (c == 'q' || c == 'Q' || c == 27) return true;
    }
    return false;
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{0, 0};
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0 && (c == 'q' || c == 'Q' || c == 27)) return true;
    }
    return false;
#endif
}

struct Column {
    int head = 0;   // linha da "cabeça" da gota
    int len = 0;    // tamanho do rastro
    int speed = 1;  // frames por passo (menor = mais rápido)
    int tick = 0;
};

int main() {
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    std::mt19937 rng{std::random_device{}()};
    auto rnd = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); };
    const std::string charset =
        "abcdefghijklmnopqrstuvwxyz0123456789@#$%&*+=<>?/\\|{}[]";
    auto randChar = [&]() { return charset[rnd(0, (int)charset.size() - 1)]; };

    initTerminal();

    int w = 0, h = 0;
    std::vector<Column> cols;
    std::vector<char> grid;

    auto resetCol = [&](Column& c, bool initial) {
        c.len = rnd(5, std::max(6, h * 2 / 3));
        c.speed = rnd(1, 3);
        c.tick = 0;
        c.head = initial ? -rnd(0, h * 2) : -rnd(0, h / 2);
    };

    auto resize = [&]() {
        getSize(w, h);
        cols.assign(w, Column{});
        grid.assign((size_t)w * h, ' ');
        for (auto& c : cols) resetCol(c, true);
        for (auto& ch : grid) ch = randChar();
        std::fputs("\x1b[2J", stdout);
    };
    resize();

    std::string frame;
    while (g_running) {
        if (quitPressed()) break;

        int nw, nh;
        getSize(nw, nh);
        if (nw != w || nh != h) resize();

        frame.clear();
        frame += "\x1b[H";
        const char* last = "";

        // atualiza colunas
        for (int x = 0; x < w; ++x) {
            Column& c = cols[x];
            if (++c.tick >= c.speed) {
                c.tick = 0;
                ++c.head;
                if (c.head >= 0 && c.head < h) grid[(size_t)c.head * w + x] = randChar();
                if (c.head - c.len > h) resetCol(c, false);
            }
        }
        // cintilação aleatória
        for (int i = 0; i < w * h / 40; ++i) grid[rnd(0, w * h - 1)] = randChar();

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const Column& c = cols[x];
                int d = c.head - y;
                const char* color;
                char ch;
                if (d == 0)                 { color = "\x1b[1;97m"; ch = grid[(size_t)y * w + x]; }
                else if (d > 0 && d < c.len) {
                    ch = grid[(size_t)y * w + x];
                    if (d < c.len / 4)      color = "\x1b[1;92m";
                    else if (d < c.len / 2) color = "\x1b[32m";
                    else                    color = "\x1b[2;32m";
                } else                      { color = ""; ch = ' '; }

                if (*color && color != last) { frame += color; last = color; }
                frame += ch;
            }
            if (y < h - 1) frame += "\r\n";
        }
        std::fwrite(frame.data(), 1, frame.size(), stdout);
        std::fflush(stdout);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    restoreTerminal();
    return 0;
}