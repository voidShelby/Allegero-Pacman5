#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>

using namespace std;

// Game Constants
const float FPS = 6.6f;
const int SCREEN_W = 460;
const int SCREEN_H = 550;
const int CELL_SIZE = 20;
const char WALL = '1';
const char DOT = '2';
const char EMPTY = '0';
const char KEY = '3';
const int TARGET_SCORE_LEVEL1 = 100;
const int TARGET_SCORE_LEVEL2 = 150;
const int TARGET_SCORE_LEVEL3 = 210;

// Raw map (24×24)
static const char RAW_MAP[24][24] = {
    "11111111111111111111111",
    "12222222222122222222221",
    "12111211112121111211121",
    "12111211112121111211121",
    "12222222222222222222221",
    "12111212111111121211121",
    "12222212222122221222221",
    "11111211110101111211111",
    "11111210000000001211111",
    "11111210111011101211111",
    "00000000111011100000000",
    "11111210111011101211111",
    "11111210111111101211111",
    "11111210000000001211111",
    "11111210111111101211111",
    "12222222222122222222221",
    "12111211112121111211121",
    "12221222222022222212221",
    "11121212111111121212111",
    "12222212222122221222221",
    "12111111112121111111121",
    "12222222222222222222221",
    "11111111111111111111111"
};

// Map wrapper
class Map {
    char grid[24][24];
public:
    Map() {
        for (int i = 0; i < 24; i++)
            for (int j = 0; j < 24; j++)
                grid[i][j] = RAW_MAP[i][j];
    }
    char get(int i, int j) const { return grid[i][j]; }
    void set(int i, int j, char v) { grid[i][j] = v; }
};

// Abstract Entity
class Entity {
protected:
    int gridX, gridY, posX, posY;
public:
    Entity(int gx, int gy)
        : gridX(gx), gridY(gy),
        posX(gy* CELL_SIZE), posY(gx* CELL_SIZE) {
    }
    virtual ~Entity() {}
    virtual void update(Map& map) = 0;
    virtual void draw() const = 0;
    int getGridX() const { return gridX; }
    int getGridY() const { return gridY; }
};

// Pacman
class Pacman : public Entity {
    int intent, previousIntent;
    int mouthToggle, lastMouthDir;
    ALLEGRO_BITMAP* bmp, * bmpUp, * bmpDown, * bmpLeft, * bmpRight, * bmpShut;
    int* bolaPtr, * scorePtr;
    bool hasKey;
public:
    Pacman(int gx, int gy,
        ALLEGRO_BITMAP* base, ALLEGRO_BITMAP* u, ALLEGRO_BITMAP* d,
        ALLEGRO_BITMAP* l, ALLEGRO_BITMAP* r, ALLEGRO_BITMAP* s,
        int* bola, int* score)
        : Entity(gx, gy),
        intent(0), previousIntent(0),
        mouthToggle(0), lastMouthDir(3),
        bmp(base), bmpUp(u), bmpDown(d),
        bmpLeft(l), bmpRight(r), bmpShut(s),
        bolaPtr(bola), scorePtr(score),
        hasKey(false)
    {
    }

    bool getHasKey() const { return hasKey; }
    void setHasKey(bool v) { hasKey = v; }

    void handleKey(int k) {
        previousIntent = intent;
        if (k == ALLEGRO_KEY_UP)    intent = 4;
        else if (k == ALLEGRO_KEY_DOWN)  intent = 2;
        else if (k == ALLEGRO_KEY_LEFT)  intent = 1;
        else if (k == ALLEGRO_KEY_RIGHT) intent = 3;
    }

    void update(Map& map) override {
        // Tunnel teleport
        if (intent != 1 && gridY >= 23) { gridX = 10; gridY = 0; }
        else if (intent != 3 && gridY < 0) { gridX = 10; gridY = 23; }

        auto tryEat = [&]() {
            char c = map.get(gridX, gridY);
            if (c == DOT) {
                map.set(gridX, gridY, EMPTY);
                (*bolaPtr)--; (*scorePtr)++;
            }
            else if (c == KEY) {
                map.set(gridX, gridY, EMPTY);
                hasKey = true;
                (*scorePtr) += 50;
            }
            };

        // Movement
        if (intent == 4 && map.get(gridX - 1, gridY) != WALL) {
            gridX--; lastMouthDir = 2; tryEat();
        }
        else if (intent == 2 && map.get(gridX + 1, gridY) != WALL) {
            gridX++; lastMouthDir = 0; tryEat();
        }
        else if (intent == 1 && map.get(gridX, gridY - 1) != WALL) {
            gridY--; lastMouthDir = 1; tryEat();
        }
        else if (intent == 3 && map.get(gridX, gridY + 1) != WALL) {
            gridY++; lastMouthDir = 3; tryEat();
        }

        // Animate
        mouthToggle++;
        if (mouthToggle % 2 == 0) bmp = bmpShut;
        else {
            switch (lastMouthDir) {
            case 0: bmp = bmpDown;  break;
            case 1: bmp = bmpLeft;  break;
            case 2: bmp = bmpUp;    break;
            case 3: bmp = bmpRight; break;
            }
        }

        posX = gridY * CELL_SIZE;
        posY = gridX * CELL_SIZE;
    }

    void draw() const override {
        al_draw_bitmap(bmp, posX, posY, 0);
    }

    void resetPosition(int gx, int gy) {
        gridX = gx;
        gridY = gy;
        posX = gy * CELL_SIZE;
        posY = gx * CELL_SIZE;
    }
};

// Ghost Base
class Ghost {
protected:
    int gridX, gridY, posX, posY;
    ALLEGRO_BITMAP* bmp;
    int delayFrames;
public:
    Ghost(int gx, int gy, ALLEGRO_BITMAP* b, int delay)
        : gridX(gx), gridY(gy),
        posX(gy* CELL_SIZE), posY(gx* CELL_SIZE),
        bmp(b), delayFrames(delay)
    {
    }
    virtual ~Ghost() {}
    virtual void moveAlgo(Map& M, const Pacman& p, int frameCount) = 0;
    int getGridX() const { return gridX; }
    int getGridY() const { return gridY; }
    void draw() const { al_draw_bitmap(bmp, posX, posY, 0); }
    void teleportCheck() {
        if (gridX == 10 && gridY < 0)  gridY = 23;
        if (gridX == 10 && gridY > 23) gridY = 0;
    }
    void syncPos() {
        posX = gridY * CELL_SIZE;
        posY = gridX * CELL_SIZE;
    }
};

// RandomGhost
class RandomGhost : public Ghost {
    int lastDir;
public:
    RandomGhost(int gx, int gy, ALLEGRO_BITMAP* b, int delay)
        : Ghost(gx, gy, b, delay), lastDir(-1) {
    }
    void moveAlgo(Map& M, const Pacman& p, int frameCount) override {
        if (frameCount < delayFrames) return;
        teleportCheck();
        int x = gridX, y = gridY;
        int choice = rand() % 4;
        if (choice == 0 && M.get(x - 1, y) != WALL && lastDir != 1) {
            x--; lastDir = 0;
        }
        else if (choice == 1 && M.get(x + 1, y) != WALL && lastDir != 0) {
            x++; lastDir = 1;
        }
        else if (choice == 2 && M.get(x, y + 1) != WALL && lastDir != 3) {
            y++; lastDir = 2;
        }
        else if (choice == 3 && M.get(x, y - 1) != WALL && lastDir != 2) {
            y--; lastDir = 3;
        }
        else {
            // fallback chase
            if (p.getGridX() > x && M.get(x + 1, y) != WALL && lastDir != 0) { x++; lastDir = 1; }
            else if (p.getGridX() < x && M.get(x - 1, y) != WALL && lastDir != 1) { x--; lastDir = 0; }
            else if (p.getGridY() > y && M.get(x, y + 1) != WALL && lastDir != 3) { y++; lastDir = 2; }
            else if (p.getGridY() < y && M.get(x, y - 1) != WALL && lastDir != 2) { y--; lastDir = 3; }
        }
        gridX = x; gridY = y;
        syncPos();
    }
};

// BlinkyGhost (Red Ghost)
class BlinkyGhost : public Ghost {
    int prevX, prevY;
    double dist(int x1, int y1, int x2, int y2) {
        return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }
public:
    BlinkyGhost(int gx, int gy, ALLEGRO_BITMAP* b, int delay)
        : Ghost(gx, gy, b, delay), prevX(gx), prevY(gy) {
    }
    void moveAlgo(Map& M, const Pacman& p, int frameCount) override {
        if (frameCount < delayFrames) return;
        int x = gridX, y = gridY;
        if (x == 8 && y == 11) {
            if (p.getGridY() <= y) y--; else y++;
        }
        else {
            struct Opt { double d; int x, y; };
            Opt opts[4] = {
                { dist(x + 1, y, p.getGridX(), p.getGridY()), x + 1, y },
                { dist(x, y - 1, p.getGridX(), p.getGridY()), x, y - 1 },
                { dist(x - 1, y, p.getGridX(), p.getGridY()), x - 1, y },
                { dist(x, y + 1, p.getGridX(), p.getGridY()), x, y + 1 }
            };
            double best = 1e9; int pick = -1;
            for (int i = 0; i < 4; i++) {
                if (opts[i].d < best &&
                    M.get(opts[i].x, opts[i].y) != WALL &&
                    !(opts[i].x == prevX && opts[i].y == prevY))
                {
                    best = opts[i].d;
                    pick = i;
                }
            }
            prevX = x; prevY = y;
            if (pick == 0) x++;
            else if (pick == 1) y--;
            else if (pick == 2) x--;
            else if (pick == 3) y++;
        }
        gridX = x; gridY = y;
        teleportCheck();
        syncPos();
    }
};

// PinkyGhost (Pink Ghost)
class PinkyGhost : public Ghost {
    int lastDir;
public:
    PinkyGhost(int gx, int gy, ALLEGRO_BITMAP* b, int delay)
        : Ghost(gx, gy, b, delay), lastDir(-1) {
    }
    void moveAlgo(Map& M, const Pacman& p, int frameCount) override {
        if (frameCount < delayFrames) return;
        teleportCheck();
        int x = gridX, y = gridY;
        int tx = p.getGridX() + 2;
        int ty = p.getGridY() + 2;
        if (abs(tx - x) > abs(ty - y)) {
            if (tx > x && M.get(x + 1, y) != WALL && lastDir != 0) { x++; lastDir = 1; }
            else if (tx < x && M.get(x - 1, y) != WALL && lastDir != 1) { x--; lastDir = 0; }
        }
        else {
            if (ty > y && M.get(x, y + 1) != WALL && lastDir != 3) { y++; lastDir = 2; }
            else if (ty < y && M.get(x, y - 1) != WALL && lastDir != 2) { y--; lastDir = 3; }
        }
        gridX = x; gridY = y;
        syncPos();
    }
};

// Game Class
class Game {
    ALLEGRO_DISPLAY* display = nullptr;
    ALLEGRO_TIMER* timer = nullptr;
    ALLEGRO_EVENT_QUEUE* evq = nullptr;
    ALLEGRO_FONT* font = nullptr;

    // Bitmaps
    ALLEGRO_BITMAP* bmpMap, * bmpMapLevel3, * bmpDots, * bmpKey;
    ALLEGRO_BITMAP* bmpPac, * bmpPUp, * bmpPDown, * bmpPLeft, * bmpPRight, * bmpPShut;
    ALLEGRO_BITMAP* bmpBlue, * bmpYellow, * bmpRed, * bmpGreen, * bmpPink;

    Map map;
    Pacman* pac = nullptr;
    vector<Ghost*> ghosts;

    int bola = 0, score = 0, frameCount = 0;
    bool gameover = false, exitGame = false, redraw = false, begun = false;
    int currentLevel = 1;
    int lives = 0;
    bool keyAvailable = false;
    bool hasExtraLife = false;

    // Count dots in RAW_MAP
    int countDots() {
        int cnt = 0;
        for (int i = 0; i < 24; i++)
            for (int j = 0; j < 24; j++)
                if (RAW_MAP[i][j] == DOT)
                    cnt++;
        return cnt;
    }

    // Reset for Level 2
    void setupLevel2() {
        currentLevel = 2;
        map = Map();
        bola = countDots();
        pac->setHasKey(false);
        keyAvailable = true;
        lives = 0;
        hasExtraLife = false;
        // Key placed in central accessible location
        map.set(10, 11, KEY);

        delete pac;
        for (auto g : ghosts) delete g;
        ghosts.clear();

        pac = new Pacman(17, 11,
            bmpPac, bmpPUp, bmpPDown,
            bmpPLeft, bmpPRight, bmpPShut,
            &bola, &score
        );

        ghosts.push_back(new RandomGhost(8, 11, bmpYellow, 0));
        ghosts.push_back(new RandomGhost(9, 11, bmpBlue, 70));
        ghosts.push_back(new RandomGhost(10, 11, bmpRed, 140));
        ghosts.push_back(new BlinkyGhost(11, 11, bmpGreen, 210));
        ghosts.push_back(new PinkyGhost(8, 9, bmpPink, 280));
    }

    // Setup for Level 3 (updated with 2 keys in accessible positions)
    void setupLevel3() {
        currentLevel = 3;
        map = Map();
        bola = countDots();
        pac->setHasKey(false);
        keyAvailable = true;  // Enable keys for this level
        lives = 0;
        hasExtraLife = false;

        // Add 2 keys to the map in accessible positions
        map.set(10, 5, KEY);    // First key position (top-center)
        map.set(10, 17, KEY);   // Second key position (bottom-center)

        delete pac;
        for (auto g : ghosts) delete g;
        ghosts.clear();

        pac = new Pacman(17, 11,
            bmpPac, bmpPUp, bmpPDown,
            bmpPLeft, bmpPRight, bmpPShut,
            &bola, &score
        );

        // Add more ghosts for increased difficulty
        ghosts.push_back(new RandomGhost(8, 11, bmpYellow, 0));
        ghosts.push_back(new RandomGhost(9, 11, bmpBlue, 70));
        ghosts.push_back(new RandomGhost(10, 11, bmpRed, 140));
        ghosts.push_back(new BlinkyGhost(11, 11, bmpGreen, 210));
        ghosts.push_back(new PinkyGhost(8, 9, bmpPink, 280));
        ghosts.push_back(new RandomGhost(7, 11, bmpYellow, 350)); // Additional ghost
    }

    // Function to reset Pacman and ghosts without resetting the map
    void resetEntities() {
        pac->resetPosition(17, 11);

        for (auto g : ghosts) delete g;
        ghosts.clear();

        if (currentLevel == 1) {
            ghosts.push_back(new RandomGhost(8, 11, bmpYellow, 0));
            ghosts.push_back(new RandomGhost(9, 11, bmpBlue, 70));
            ghosts.push_back(new RandomGhost(10, 11, bmpRed, 140));
            ghosts.push_back(new BlinkyGhost(11, 11, bmpGreen, 210));
        }
        else if (currentLevel == 2) {
            ghosts.push_back(new RandomGhost(8, 11, bmpYellow, 0));
            ghosts.push_back(new RandomGhost(9, 11, bmpBlue, 70));
            ghosts.push_back(new RandomGhost(10, 11, bmpRed, 140));
            ghosts.push_back(new BlinkyGhost(11, 11, bmpGreen, 210));
            ghosts.push_back(new PinkyGhost(8, 9, bmpPink, 280));
        }
        else if (currentLevel == 3) {
            ghosts.push_back(new RandomGhost(8, 11, bmpYellow, 0));
            ghosts.push_back(new RandomGhost(9, 11, bmpBlue, 70));
            ghosts.push_back(new RandomGhost(10, 11, bmpRed, 140));
            ghosts.push_back(new BlinkyGhost(11, 11, bmpGreen, 210));
            ghosts.push_back(new PinkyGhost(8, 9, bmpPink, 280));
            ghosts.push_back(new RandomGhost(7, 11, bmpYellow, 350));
        }
    }

public:
    bool loadBMP(const char* path, ALLEGRO_BITMAP*& bmp) {
        bmp = al_load_bitmap(path);
        if (!bmp) {
            cerr << "ERROR: failed to load bitmap: " << path << "\n";
            return false;
        }
        return true;
    }

    bool init() {
        if (!al_init()) {
            cerr << "ERROR: al_init() failed\n"; return false;
        }
        if (!al_install_keyboard()) {
            cerr << "ERROR: al_install_keyboard() failed\n"; return false;
        }
        al_init_image_addon();
        al_init_font_addon();
        al_init_ttf_addon();

        display = al_create_display(SCREEN_W, SCREEN_H);
        if (!display) { cerr << "ERROR: al_create_display() failed\n"; return false; }
        timer = al_create_timer(1.0 / FPS);
        if (!timer) { cerr << "ERROR: al_create_timer() failed\n";   return false; }
        evq = al_create_event_queue();
        if (!evq) { cerr << "ERROR: al_create_event_queue() failed\n"; return false; }

        // Load bitmaps
        if (!loadBMP("assets/maps/map.bmp", bmpMap))  return false;
        if (!loadBMP("assets/maps/map23.bmp", bmpMapLevel3))  return false;
        if (!loadBMP("assets/maps/bolas.png", bmpDots)) return false;
        if (!loadBMP("assets/maps/key.png", bmpKey))  return false;
        if (!loadBMP("assets/characters/pacman/pacman.png", bmpPac))   return false;
        if (!loadBMP("assets/characters/pacman/pac_up.png", bmpPUp))   return false;
        if (!loadBMP("assets/characters/pacman/pac_down.png", bmpPDown)) return false;
        if (!loadBMP("assets/characters/pacman/pac_left.png", bmpPLeft)) return false;
        if (!loadBMP("assets/characters/pacman/pac_right.png", bmpPRight))return false;
        if (!loadBMP("assets/characters/pacman/shutup.png", bmpPShut)) return false;
        if (!loadBMP("assets/characters/ghosts/amarelo.png", bmpYellow)) return false;
        if (!loadBMP("assets/characters/ghosts/azul.png", bmpBlue))   return false;
        if (!loadBMP("assets/characters/ghosts/blinky.png", bmpRed))    return false;
        if (!loadBMP("assets/characters/ghosts/gburro1.png", bmpGreen)) return false;
        if (!loadBMP("assets/characters/ghosts/rosa.png", bmpPink))   return false;

        // Load font
        font = al_load_ttf_font("/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf", 28, 0);
        if (!font) {
            font = al_load_ttf_font("C:/Windows/Fonts/OCRAEXT.ttf", 28, 0);
            if (!font) {
                cerr << "ERROR: failed to load any TTF font\n";
                return false;
            }
        }

        // Level 1 setup
        bola = countDots();
        pac = new Pacman(17, 11,
            bmpPac, bmpPUp, bmpPDown,
            bmpPLeft, bmpPRight, bmpPShut,
            &bola, &score
        );

        ghosts.push_back(new RandomGhost(8, 11, bmpYellow, 0));
        ghosts.push_back(new RandomGhost(9, 11, bmpBlue, 70));
        ghosts.push_back(new RandomGhost(10, 11, bmpRed, 140));
        ghosts.push_back(new BlinkyGhost(11, 11, bmpGreen, 210));

        // Register events
        al_register_event_source(evq, al_get_display_event_source(display));
        al_register_event_source(evq, al_get_timer_event_source(timer));
        al_register_event_source(evq, al_get_keyboard_event_source());

        al_start_timer(timer);
        return true;
    }

    void run() {
        while (!exitGame) {
            ALLEGRO_EVENT ev;
            al_wait_for_event(evq, &ev);

            if (ev.type == ALLEGRO_EVENT_TIMER) {
                frameCount++;
                pac->update(map);
                for (auto g : ghosts) g->moveAlgo(map, *pac, frameCount);

                // Check if key was collected
                if ((currentLevel == 2 || currentLevel == 3) && pac->getHasKey()) {
                    hasExtraLife = true;
                    pac->setHasKey(false);
                    keyAvailable = (currentLevel == 3 && map.get(10, 5) == KEY) ||
                        (currentLevel == 3 && map.get(10, 17) == KEY);
                    lives = 1;
                }

                // Level transitions
                if (score >= TARGET_SCORE_LEVEL1 && currentLevel == 1) {
                    setupLevel2();
                    al_rest(1.0);
                    continue;
                }
                else if (score >= TARGET_SCORE_LEVEL2 && currentLevel == 2) {
                    setupLevel3();
                    al_rest(1.0);
                    continue;
                }

                // Win condition for Level 3
                if (bola == 0 && currentLevel == 3) {
                    al_draw_bitmap(bmpMapLevel3, 0, 0, 0);
                    al_flip_display();
                    al_rest(4.0);
                    exitGame = true;
                    continue;
                }

                // Collision check
                for (auto g : ghosts) {
                    if (g->getGridX() == pac->getGridX() &&
                        g->getGridY() == pac->getGridY())
                    {
                        if (hasExtraLife) {
                            // Use the extra life
                            hasExtraLife = false;
                            lives = 0;
                            resetEntities();
                            al_rest(1.0);
                        }
                        else {
                            // Game over
                            gameover = true;
                        }
                    }
                }
                redraw = true;
            }
            else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                exitGame = true;
            }
            else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                pac->handleKey(ev.keyboard.keycode);
                if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                    exitGame = true;
            }

            if (redraw && al_is_event_queue_empty(evq)) {
                redraw = false;
                al_clear_to_color(al_map_rgb(0, 0, 0));

                // Draw the appropriate map based on level
                if (currentLevel == 3) {
                    al_draw_bitmap(bmpMapLevel3, 0, 0, 0);
                }
                else {
                    al_draw_bitmap(bmpMap, 0, 0, 0);
                }

                // Draw dots & key(s)
                for (int i = 0; i < 24; i++)
                    for (int j = 0; j < 24; j++) {
                        char c = map.get(i, j);
                        if (c == DOT)
                            al_draw_bitmap(bmpDots, j * CELL_SIZE, i * CELL_SIZE, 0);
                        else if (c == KEY && keyAvailable)
                            al_draw_bitmap(bmpKey, j * CELL_SIZE, i * CELL_SIZE, 0);
                    }

                pac->draw();
                for (auto g : ghosts) g->draw();

                al_draw_textf(font, al_map_rgb(200, 200, 200),
                    0, 505, 0,
                    "Score: %d/%d | Level: %d | Lives: %d",
                    score,
                    currentLevel == 1 ? TARGET_SCORE_LEVEL1 :
                    (currentLevel == 2 ? TARGET_SCORE_LEVEL2 : TARGET_SCORE_LEVEL3),
                    currentLevel,
                    lives);

                if (currentLevel == 2 || currentLevel == 3) {
                    if (hasExtraLife)
                        al_draw_text(font, al_map_rgb(0, 255, 0),
                            200, 505, 0, "Life Available!");
                    else if (keyAvailable)
                        al_draw_text(font, al_map_rgb(255, 255, 0),
                            200, 505, 0, "Find the Key!");
                }

                if (currentLevel == 3) {
                    al_draw_text(font, al_map_rgb(255, 0, 0),
                        350, 505, 0, "FINAL LEVEL!");
                }

                al_flip_display();

                if (!begun) {
                    al_rest(3.1);
                    begun = true;
                }
                if (gameover) {
                    al_rest(2.0);
                    exitGame = true;
                }
            }
        }
    }

    void cleanup() {
        delete pac;
        for (auto g : ghosts) delete g;

        al_destroy_display(display);
        al_destroy_timer(timer);
        al_destroy_event_queue(evq);
        al_destroy_font(font);

        // Destroy bitmaps
        al_destroy_bitmap(bmpMap);
        al_destroy_bitmap(bmpMapLevel3);
        al_destroy_bitmap(bmpDots);
        al_destroy_bitmap(bmpKey);
        al_destroy_bitmap(bmpPac);
        al_destroy_bitmap(bmpPUp);
        al_destroy_bitmap(bmpPDown);
        al_destroy_bitmap(bmpPLeft);
        al_destroy_bitmap(bmpPRight);
        al_destroy_bitmap(bmpPShut);
        al_destroy_bitmap(bmpBlue);
        al_destroy_bitmap(bmpYellow);
        al_destroy_bitmap(bmpRed);
        al_destroy_bitmap(bmpGreen);
        al_destroy_bitmap(bmpPink);
    }
};

int main() {
    srand((unsigned)time(nullptr));
    Game game;
    if (!game.init()) {
        cerr << "Initialization failed\n";
        return -1;
    }
    game.run();
    game.cleanup();
    return 0;
}