#if __has_include(<SFML/Graphics.hpp>)
#include <SFML/Graphics.hpp>
#else
#include <cstdint>
#include <string>
#include <iostream>
namespace sf {
    using Uint8 = std::uint8_t;
    struct Color {
        Uint8 r, g, b, a;
        Color(Uint8 rr = 0, Uint8 gg = 0, Uint8 bb = 0, Uint8 aa = 255) : r(rr), g(gg), b(bb), a(aa) {}
        static const Color White;
    };
    const Color Color::White = Color(255,255,255,255);
    struct Font { bool loadFromFile(const std::string&) { return true; } };
    struct FloatRect {
        float left=0, top=0, width=0, height=0;
        FloatRect() {}
        FloatRect(float l, float t, float w, float h) : left(l), top(t), width(w), height(h) {}
        bool intersects(const FloatRect& o) const {
            return !(left + width < o.left || o.left + o.width < left || top + height < o.top || o.top + o.height < top);
        }
    };
    struct Vector2f { float x=0, y=0; Vector2f(){} Vector2f(float a, float b):x(a),y(b){} };
    struct Vector2i { int x=0, y=0; Vector2i(){} Vector2i(int a,int b):x(a),y(b){} };
    struct CircleShape { CircleShape(float){} void setPosition(float, float){} void setFillColor(Color){} };
    struct RectangleShape {
        RectangleShape(const Vector2f&){}
        RectangleShape(std::initializer_list<float> l){}
        RectangleShape(float, float){}
        void setPosition(float, float){}
        void setFillColor(Color){}
        void setOutlineColor(Color){}
        void setOutlineThickness(float){}
    };
    struct Text { Text(const std::string&, const Font&, unsigned int){} void setPosition(float, float){} void setFillColor(Color){} enum Style { Bold = 1 }; void setStyle(int){} };
    struct Event { enum EventType { Closed, MouseButtonPressed }; EventType type; struct MouseButtonEvent { int x=0,y=0; } mouseButton; };
    struct VideoMode { VideoMode(int, int) {} };
    namespace Style { enum { None = 0, Titlebar = 1, Resize = 2, Close = 4, Fullscreen = 8 }; }
    struct RenderWindow {
        bool _open = true;
        RenderWindow(const VideoMode&, const std::string&, unsigned int) {}
        void setFramerateLimit(int) {}
        void setMouseCursorVisible(bool) {}
        bool isOpen() { return _open; }
        void close() { _open = false; }
        bool pollEvent(Event& e) { return false; }
        void clear(Color) {}
        void draw(const CircleShape&) {}
        void draw(const RectangleShape&) {}
        void draw(const Text&) {}
        void display() {}
    };
    struct Mouse { static Vector2i getPosition(const RenderWindow&) { return Vector2i(0,0); } };
    struct Time { float s = 0.f; float asSeconds() const { return s; } };
    struct Clock { Time restart() { return Time{0.f}; } };
}
#endif
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;
using namespace sf;

// ── Palette ─────────────────────────────────────────────────────
const Color BG{ 12,12,25 };
const Color GLOW_CYAN{ 0,255,220 };
const Color GLOW_PINK{ 255,60,150 };
const Color GLOW_GOLD{ 255,200,40 };
const Color GLOW_PURP{ 160,80,255 };
const Color TEXT_COL{ 210,210,240 };
const Color DIM{ 80,80,120 };

Font gFont;

// ── Star (background twinkle) ───────────────────────────────────
struct Star { float x, y, brightness, speed; };

// ── Trail particle ──────────────────────────────────────────────
struct Particle { float x, y, vx, vy, life; Color col; };

// ── Brick ───────────────────────────────────────────────────────
struct Brick {
    FloatRect rect;
    Color col;
    bool alive = true;
    int hits = 1; // hits remaining
};

// ── Game ────────────────────────────────────────────────────────
class Game {
    static const int W = 640, H = 700;
    // Paddle
    float px, pw = 100, ph = 14, py = H - 50;
    // Ball
    float bx, by, bvx, bvy, br = 7;
    bool launched = false;
    // Bricks
    vector<Brick> bricks;
    // Particles & stars
    vector<Particle> particles;
    vector<Star> stars;
    // State
    int score = 0, lives = 3, level = 1;
    bool gameOver = false, won = false;
    Clock clk;

    // ── Helpers ─────────────────────────────────────────────────
    void spawnBricks() {
        bricks.clear();
        Color rows[] = { GLOW_PINK, GLOW_PURP, GLOW_CYAN, GLOW_GOLD,
                      Color(80,220,120), Color(255,130,60) };
        int cols = 10, rowCount = 5 + level;
        if (rowCount > 8) rowCount = 8;
        float bw = (W - 80) / cols, bh = 22, x0 = 40, y0 = 70;
        for (int r = 0;r < rowCount;r++) for (int c = 0;c < cols;c++) {
            Brick b;
            b.rect = { x0 + c * bw + 2, y0 + r * (bh + 4) + 2, bw - 4, bh - 2 };
            b.col = rows[r % 6];
            b.hits = (r < 2 && level>1) ? 2 : 1; // top 2 rows are tough on lvl 2+
            if (b.hits == 2) b.col = Color(b.col.r / 2, b.col.g / 2, b.col.b / 2);
            b.alive = true;
            bricks.push_back(b);
        }
    }

    void resetBall() {
        bx = px + pw / 2; by = py - br - 2;
        bvx = 0; bvy = 0; launched = false;
    }

    void spawnParticles(float x, float y, Color col, int n = 12) {
        for (int i = 0;i < n;i++) {
            float a = (rand() % 360) * 3.14159f / 180.f;
            float sp = 80 + rand() % 120;
            particles.push_back({ x,y,cos(a) * sp,sin(a) * sp,0.5f + rand() % 50 / 100.f,col });
        }
    }

    void initStars() {
        for (int i = 0;i < 80;i++)
            stars.push_back({ (float)(rand() % W),(float)(rand() % H),
                             0.3f + rand() % 70 / 100.f, 0.2f + rand() % 30 / 100.f });
    }

public:
    Game() {
        srand((unsigned)time(0));
        px = W / 2 - pw / 2;
        initStars();
        spawnBricks();
        resetBall();
    }

    void update(float dt) {
        if (gameOver || won) return;

        // Stars twinkle
        for (auto& s : stars) {
            s.brightness += s.speed * dt;
            if (s.brightness > 1) s.speed = -fabs(s.speed);
            if (s.brightness < 0.2f) s.speed = fabs(s.speed);
        }

        // Particles fade
        for (auto& p : particles) {
            p.x += p.vx * dt; p.y += p.vy * dt;
            p.life -= dt;
        }
        particles.erase(remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0; }), particles.end());

        if (!launched) { bx = px + pw / 2; by = py - br - 2; return; }

        // Move ball
        bx += bvx * dt; by += bvy * dt;

        // Wall bounces
        if (bx - br < 0) { bx = br; bvx = fabs(bvx); }
        if (bx + br > W) { bx = W - br; bvx = -fabs(bvx); }
        if (by - br < 0) { by = br; bvy = fabs(bvy); }

        // Fell off bottom
        if (by > H + br) {
            lives--;
            if (lives <= 0) gameOver = true;
            else resetBall();
            return;
        }

        // Paddle collision
        if (bvy > 0 && by + br >= py && by + br <= py + ph && bx >= px && bx <= px + pw) {
            bvy = -fabs(bvy);
            // Spin: hit position affects x velocity
            float hit = (bx - px) / pw; // 0..1
            bvx = (hit - 0.5f) * 500;
            by = py - br - 1;
            spawnParticles(bx, py, GLOW_CYAN, 6);
        }

        // Brick collision
        bool allDead = true;
        for (auto& b : bricks) {
            if (!b.alive) continue;
            allDead = false;
            FloatRect br_rect(bx - br, by - br, br * 2, br * 2);
            if (b.rect.intersects(br_rect)) {
                b.hits--;
                if (b.hits <= 0) {
                    b.alive = false;
                    score += 10 * level;
                    spawnParticles(b.rect.left + b.rect.width / 2,
                        b.rect.top + b.rect.height / 2, b.col, 15);
                }
                else {
                    // Brighten on crack
                    {
                        int nr = int(b.col.r) * 2; if (nr > 255) nr = 255;
                        int ng = int(b.col.g) * 2; if (ng > 255) ng = 255;
                        int nb = int(b.col.b) * 2; if (nb > 255) nb = 255;
                        b.col = Color((Uint8)nr, (Uint8)ng, (Uint8)nb);
                    }
                    spawnParticles(bx, by, b.col, 5);
                }
                // Simple bounce logic
                float cx = b.rect.left + b.rect.width / 2;
                float cy = b.rect.top + b.rect.height / 2;
                if (fabs(bx - cx) / (b.rect.width / 2) > fabs(by - cy) / (b.rect.height / 2))
                    bvx = -bvx;
                else
                    bvy = -bvy;
                break;
            }
        }

        // Level cleared
        if (allDead) {
            level++;
            if (level > 5) { won = true; return; }
            spawnBricks();
            resetBall();
        }
    }

    void draw(RenderWindow& win) {
        win.clear(BG);

        // Stars
        for (auto& s : stars) {
            CircleShape c(1.5f);
            c.setPosition(s.x, s.y);
            Uint8 a = (Uint8)(s.brightness * 180);
            c.setFillColor(Color(200, 200, 255, a));
            win.draw(c);
        }

        // Particles (glow dots)
        for (auto& p : particles) {
            float r = 3 * p.life;
            CircleShape c(r);
            c.setPosition(p.x - r, p.y - r);
            Uint8 a = (Uint8)(p.life * 255);
            c.setFillColor(Color(p.col.r, p.col.g, p.col.b, a));
            win.draw(c);
        }

        // Bricks
        for (auto& b : bricks) {
            if (!b.alive) continue;
            RectangleShape r({ b.rect.width,b.rect.height });
            r.setPosition(b.rect.left, b.rect.top);
            r.setFillColor(b.col);
            // Glow outline
            r.setOutlineColor(Color(b.col.r, b.col.g, b.col.b, 80));
            r.setOutlineThickness(1.5f);
            win.draw(r);
        }

        // Paddle (neon glow effect with 2 layers)
        RectangleShape glow({ pw + 6,ph + 6 });
        glow.setPosition(px - 3, py - 3);
        glow.setFillColor(Color(0, 255, 220, 40));
        win.draw(glow);
        RectangleShape pad({ pw,ph });
        pad.setPosition(px, py);
        pad.setFillColor(GLOW_CYAN);
        win.draw(pad);

        // Ball + trail glow
        CircleShape bg2(br + 4);
        bg2.setPosition(bx - br - 4, by - br - 4);
        bg2.setFillColor(Color(255, 255, 255, 30));
        win.draw(bg2);
        CircleShape ball(br);
        ball.setPosition(bx - br, by - br);
        ball.setFillColor(Color::White);
        win.draw(ball);

        // HUD
        Text sc("Score: " + to_string(score), gFont, 20);
        sc.setPosition(15, 10); sc.setFillColor(GLOW_GOLD); win.draw(sc);
        Text lv("Level: " + to_string(level), gFont, 20);
        lv.setPosition(W / 2 - 40, 10); lv.setFillColor(GLOW_CYAN); win.draw(lv);
        Text li("Lives: " + to_string(lives), gFont, 20);
        li.setPosition(W - 120, 10); li.setFillColor(GLOW_PINK); win.draw(li);

        // Messages
        if (!launched && !gameOver && !won) {
            Text t("Click to Launch", gFont, 22);
            t.setPosition(W / 2 - 80, H / 2 + 40);
            t.setFillColor(DIM); win.draw(t);
        }
        if (gameOver) {
            Text t("GAME OVER", gFont, 48);
            t.setPosition(W / 2 - 145, H / 2 - 60);
            t.setFillColor(GLOW_PINK); t.setStyle(Text::Bold); win.draw(t);
            Text t2("Click to Restart", gFont, 20);
            t2.setPosition(W / 2 - 80, H / 2 + 10);
            t2.setFillColor(DIM); win.draw(t2);
        }
        if (won) {
            Text t("YOU WIN!", gFont, 48);
            t.setPosition(W / 2 - 120, H / 2 - 60);
            t.setFillColor(GLOW_GOLD); t.setStyle(Text::Bold); win.draw(t);
            Text t2("Score: " + to_string(score), gFont, 24);
            t2.setPosition(W / 2 - 60, H / 2 + 10);
            t2.setFillColor(GLOW_CYAN); win.draw(t2);
        }

        win.display();
    }

    void handleClick(int mx, int my) {
        if (gameOver || won) {
            score = 0; lives = 3; level = 1; gameOver = false; won = false;
            spawnBricks(); resetBall();
            return;
        }
        if (!launched) {
            launched = true;
            bvx = (rand() % 200 - 100); bvy = -350;
        }
    }

    void movePaddle(float mouseX) {
        px = mouseX - pw / 2;
        if (px < 0) px = 0;
        if (px + pw > W) px = W - pw;
    }

    int getW() const { return W; }
    int getH() const { return H; }
};

// ── Main ────────────────────────────────────────────────────────
int main() {
    if (!gFont.loadFromFile("arial.ttf"))
        gFont.loadFromFile("C:/Windows/Fonts/arial.ttf");

    Game game;
    RenderWindow win(VideoMode(game.getW(), game.getH()),
        "Neon Breakout", Style::Titlebar | Style::Close);
    win.setFramerateLimit(60);
    win.setMouseCursorVisible(false);
    Clock clk;

    while (win.isOpen()) {
        float dt = clk.restart().asSeconds();
        Event e;
        while (win.pollEvent(e)) {
            if (e.type == Event::Closed) win.close();
            if (e.type == Event::MouseButtonPressed)
                game.handleClick(e.mouseButton.x, e.mouseButton.y);
        }
        game.movePaddle((float)Mouse::getPosition(win).x);
        game.update(dt);
        game.draw(win);
    }
    return 0;
}