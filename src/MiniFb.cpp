#define NOGDI
#define NOUSER
#include "raylib.h"
#include "MiniFb.h"
#include "GridManager.h"
#include "ShopManager.h"
#include "Block.h"
#include <cmath>
#include <cstdio>
#include <cstring>

struct MiniFb::Impl {
    Font font;
    bool fontLoaded;
    bool windowOpen;
    int width, height;
    Impl() : fontLoaded(false), windowOpen(false), width(960), height(780) { font = { 0 }; }
};

MiniFb::MiniFb() : m_impl(std::make_unique<Impl>()) {}

MiniFb::~MiniFb() {
    if (m_impl->fontLoaded && m_impl->font.texture.id > 0) UnloadFont(m_impl->font);
    if (m_impl->windowOpen) CloseWindow();
}

//初始化
bool MiniFb::init(int width, int height, const char* title) {
    m_impl->width = width;
    m_impl->height = height;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title);
    m_impl->windowOpen = true;
    SetTargetFPS(60);

    m_impl->font = GetFontDefault();

    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\consola.ttf",
        "C:\\Windows\\Fonts\\calibri.ttf",
        "assets/font.ttf"
    };
    for (int fi = 0; fi < 5; fi++) {
        if (FileExists(fontPaths[fi])) {
            int cpCount = 0;
            static int cps[256];
            for (int i = 32; i < 127; i++) cps[cpCount++] = i;
            Font loaded = LoadFontEx(fontPaths[fi], 32, cps, cpCount);
            if (loaded.texture.id != 0) {
                m_impl->font = loaded;
                m_impl->fontLoaded = true;
                SetTextureFilter(m_impl->font.texture, TEXTURE_FILTER_BILINEAR);
                break;
            }
        }
    }
    return true;
}

//主循环
bool MiniFb::shouldClose() const { return WindowShouldClose(); }
void MiniFb::beginDrawing() { BeginDrawing(); ClearBackground(BLACK); }
void MiniFb::endDrawing() { EndDrawing(); }
double MiniFb::getTime() const { return GetTime(); }

bool MiniFb::isMouseButtonPressed(FbMouseButton button) const { return IsMouseButtonPressed(button); }
bool MiniFb::isKeyPressed(FbKey key) const { return IsKeyPressed(key); }
int MiniFb::getMouseX() const { return GetMouseX(); }
int MiniFb::getMouseY() const { return GetMouseY(); }

//绘制基础
static ::Color toRay(FbColor c) { return { c.r, c.g, c.b, c.a }; }

void MiniFb::drawRectangle(int x, int y, int w, int h, FbColor color) {
    DrawRectangle(x, y, w, h, toRay(color));
}
void MiniFb::drawRectangleLines(int x, int y, int w, int h, FbColor color, int thickness) {
    DrawRectangleLinesEx({ (float)x, (float)y, (float)w, (float)h }, (float)thickness, toRay(color));
}
void MiniFb::drawText(const char* text, int x, int y, int fontSize, FbColor color) {
    DrawTextEx(m_impl->font, text, { (float)x, (float)y }, (float)fontSize, 1, toRay(color));
}
int MiniFb::measureText(const char* text, int fontSize) const {
    return (int)MeasureTextEx(m_impl->font, text, (float)fontSize, 1).x;
}
void MiniFb::drawLine(int x1, int y1, int x2, int y2, FbColor color) {
    DrawLine(x1, y1, x2, y2, toRay(color));
}

//方块外观
static FbColor getBlockColor(BlockType type) {
    switch (type) {
    case BlockType::SkyBlue:      return { 102, 191, 255, 255 };
    case BlockType::Silver:       return { 192, 192, 192, 255 };
    case BlockType::Gray:         return { 128, 128, 128, 255 };
    case BlockType::Black:        return { 40, 40, 40, 255 };
    case BlockType::White:        return { 240, 240, 240, 255 };
    case BlockType::Brown:        return { 139, 90, 43, 255 };
    case BlockType::Blue:         return { 0, 121, 241, 255 };
    case BlockType::Yellow:       return { 253, 249, 0, 255 };
    case BlockType::LightYellow:  return { 255, 255, 180, 255 };
    case BlockType::Transparent:  return { 200, 220, 255, 180 };
    case BlockType::Gold:         return { 255, 203, 0, 255 };
    case BlockType::Red:          return { 230, 41, 55, 255 };
    case BlockType::Green:        return { 0, 228, 48, 255 };
    case BlockType::Orange:       return { 255, 161, 0, 255 };
    case BlockType::Cyan:         return { 0, 230, 230, 255 };
    case BlockType::Jade:         return { 0, 168, 107, 255 };
    case BlockType::Steam:        return { 200, 220, 235, 200 };
    default:                      return { 30, 30, 30, 255 };
    }
}

static const char* getBlockName(BlockType type) {
    switch (type) {
    case BlockType::SkyBlue:      return "SkyBlue";
    case BlockType::Silver:       return "Silver";
    case BlockType::Gray:         return "Gray";
    case BlockType::Black:        return "Black";
    case BlockType::White:        return "White";
    case BlockType::Brown:        return "Brown";
    case BlockType::Blue:         return "Blue";
    case BlockType::Yellow:       return "Yellow";
    case BlockType::LightYellow:  return "LtYellow";
    case BlockType::Transparent:  return "Transparent";
    case BlockType::Gold:         return "Gold";
    case BlockType::Red:          return "Red";
    case BlockType::Green:        return "Green";
    case BlockType::Orange:       return "Orange";
    case BlockType::Cyan:         return "Cyan";
    case BlockType::Jade:         return "Jade";
    case BlockType::Steam:        return "Steam";
    default:                      return "?";
    }
}

static const char* getBlockDesc(BlockType type) {
    switch (type) {
    case BlockType::SkyBlue:      return "Explosive: 3x3 blast when on fire. Chains to Silver.";
    case BlockType::Silver:       return "Explosive: 3x3 blast when on fire. Top/White contact: -> Gray.";
    case BlockType::Gray:         return "Touching fire: turns Silver.";
    case BlockType::Black:        return "White contact: self-ignites, eats White tiles. Fire: 2 turns.";
    case BlockType::White:        return "Fire 2t: -> Steam. Eaten by Black. +Yellow -> LtYellow.";
    case BlockType::Brown:        return "SkyBlue contact: -> Blue.";
    case BlockType::Blue:         return "Gone in 1 turn. Contact destroys Silver/Black/Yellow.";
    case BlockType::Yellow:       return "White contact: -> LtYellow. Fire 2t: -> LtYellow.";
    case BlockType::LightYellow:  return "No Yellow nearby: -> White. Fire goes out.";
    case BlockType::Transparent:  return "Floats upward through blocks.";
    case BlockType::Gold:         return "Valuable. High payout.";
    case BlockType::Red:          return "When removed: ignites adjacent Red (chain). Fire 2 turns.";
    case BlockType::Green:        return "Sinks to bottom, swapping with block below.";
    case BlockType::Orange:       return "Randomly infects one neighbor (non-Gold) to Orange.";
    case BlockType::Cyan:         return "Extinguishes adjacent fire -> Steam.";
    case BlockType::Jade:         return "Valuable. Timer = bonus. Fire: -2 timer every 2 turns.";
    case BlockType::Steam:        return "Gone in 1 turn. Extinguishes fire in 3x3 area.";
    default:                      return "";
    }
}

//顶部信息栏
void MiniFb::drawHUD(int money, int turn) {
    drawRectangle(0, 0, m_impl->width, HUD_HEIGHT, FbColor(15, 15, 20, 255));
    drawRectangleLines(0, 0, m_impl->width, HUD_HEIGHT, FbColor(80, 80, 80, 255), 1);

    char buf[64];
    snprintf(buf, sizeof(buf), "Gold: %d", money);
    drawText(buf, 12, 4, 38, FbColor(255, 215, 0, 255));

    snprintf(buf, sizeof(buf), "Turn: %d", turn);
    drawText(buf, 300, 4, 38, FbColor(200, 200, 200, 255));

    drawText("Just Drop It!", m_impl->width / 2 - 80, 4, 38, FbColor(120, 120, 120, 255));
}

//仓库网格
void MiniFb::drawGrid(const GridManager& gm, double time) {
    int gw = GRID_COLS * CELL_SIZE;
    int gh = GRID_ROWS * CELL_SIZE;
    drawRectangle(GRID_ORIGIN_X - 2, GRID_ORIGIN_Y - 2, gw + 4, gh + 4, FbColor(50, 50, 54, 255));

    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            int sx = GRID_ORIGIN_X + x * CELL_SIZE;
            int sy = GRID_ORIGIN_Y + y * CELL_SIZE;
            Block* b = gm.getBlock(x, y);
            if (!b) continue;

            BlockType type = b->getType();

            if (type == BlockType::None) {
                drawRectangle(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(60, 60, 64, 255));
            } else {
                FbColor bc = getBlockColor(type);
                drawRectangle(sx, sy, CELL_SIZE, CELL_SIZE, bc);

                if (b->getSelected()) {
                    float sp = (float)(0.4f + 0.6f * fabs(sin(time * 6.0)));
                    unsigned char sa = (unsigned char)(80 + (int)(sp * 120));
                    drawRectangle(sx + 2, sy + 2, CELL_SIZE - 4, CELL_SIZE - 4, FbColor(255, 255, 255, sa));
                    drawRectangleLines(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(255, 215, 0, 255), 3);
                }

                if (b->getEnvStatus() == Environment::Fire) {
                    float pulse = (float)(0.3f + 0.7f * fabs(sin(time * 8.0)));
                    unsigned char alpha = (unsigned char)(120 + (int)(pulse * 135));
                    drawRectangle(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(255, 30, 0, alpha));
                    float p2 = (float)(0.5f + 0.5f * sin(time * 12.0 + 1.5f));
                    unsigned char a2 = (unsigned char)(200 + (int)(p2 * 55));
                    drawRectangleLines(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(255, 100, 0, a2), 3);
                    drawRectangleLines(sx + 2, sy + 2, CELL_SIZE - 4, CELL_SIZE - 4, FbColor(255, 200, 50, 100), 1);
                }

                if (type == BlockType::SkyBlue || type == BlockType::Silver || type == BlockType::Gray) {
                    bool nearFire = (b->getEnvStatus() == Environment::Fire);
                    if (!nearFire) {
                        int dx[] = { 0, 0, -1, 1 };
                        int dy[] = { -1, 1, 0, 0 };
                        for (int i = 0; i < 4 && !nearFire; ++i) {
                            Block* nb = gm.getBlock(x + dx[i], y + dy[i]);
                            if (nb && nb->getEnvStatus() == Environment::Fire) nearFire = true;
                        }
                    }
                    if (nearFire) {
                        float bp = (float)(0.3f + 0.7f * fabs(sin(time * 14.0)));
                        unsigned char ba = (unsigned char)(120 + (int)(bp * 135));
                        drawRectangle(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(255, 140, 0, ba));
                        float bp2 = (float)(0.5f + 0.5f * sin(time * 16.0 + 2.0f));
                        unsigned char ba2 = (unsigned char)(180 + (int)(bp2 * 75));
                        drawRectangleLines(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(255, 255, 50, ba2), 4);
                        float bp3 = (float)(0.5f + 0.5f * sin(time * 20.0));
                        unsigned char ba3 = (unsigned char)(100 + (int)(bp3 * 155));
                        drawRectangleLines(sx + 2, sy + 2, CELL_SIZE - 4, CELL_SIZE - 4, FbColor(255, 50, 0, ba3), 2);
                    }
                }

                if (type == BlockType::Jade && b->getTimer() > 0) {
                    char tb[8];
                    snprintf(tb, sizeof(tb), "%d", b->getTimer());
                    drawText(tb, sx + 4, sy + 4, 30, FbColor(255, 255, 255, 255));
                }

                if ((type == BlockType::Steam || type == BlockType::Blue) && b->getTimer() > 0) {
                    char tb[8];
                    snprintf(tb, sizeof(tb), "%d", b->getTimer());
                    drawText(tb, sx + CELL_SIZE / 2 - 8, sy + CELL_SIZE / 2 - 15, 30, FbColor(255, 255, 255, 255));
                }
            }
            drawRectangleLines(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(55, 55, 55, 255), 1);
        }
    }

    const auto& explosions = gm.getExplosionEvents();
    for (const auto& ev : explosions) {
        float age = (float)(time - ev.time);
        if (age > 0.6f) continue;
        float progress = age / 0.6f;
        float radius = (float)(CELL_SIZE * (1 + progress * 4));
        float alpha = 1.0f - progress;
        int cx = GRID_ORIGIN_X + ev.x * CELL_SIZE + CELL_SIZE / 2;
        int cy = GRID_ORIGIN_Y + ev.y * CELL_SIZE + CELL_SIZE / 2;
        int ri = (int)radius;
        drawRectangleLines(cx - ri, cy - ri, ri * 2, ri * 2, FbColor(255, 180, 30, (unsigned char)(alpha * 200)), (int)(4 * (1.0f - progress)));
        drawRectangleLines(cx - ri / 2, cy - ri / 2, ri, ri, FbColor(255, 100, 0, (unsigned char)(alpha * 150)), (int)(3 * (1.0f - progress)));
    }
}

//订单面板
void MiniFb::drawOrders(const OrderManager& om, int hoveredOrder) {
    const auto& orders = om.getCurrentOrders();
    drawRectangle(4, 58, 926, 540, FbColor(25, 25, 30, 255));
    drawRectangleLines(4, 58, 926, 540, FbColor(80, 80, 80, 255), 1);
    drawText("ORDERS", 14, 62, 28, FbColor(200, 200, 200, 255));
    drawLine(14, 94, 916, 94, FbColor(80, 80, 80, 255));

    for (size_t i = 0; i < orders.size(); ++i) {
        int oy = 100 + (int)i * 72;
        FbColor bg = ((int)i == hoveredOrder) ? FbColor(60, 60, 70, 255) : FbColor(35, 35, 40, 255);
        drawRectangle(8, oy, 910, 68, bg);
        drawRectangleLines(8, oy, 910, 68, FbColor(100, 100, 100, 255), 1);

        char ib[16];
        snprintf(ib, sizeof(ib), "#%zu", i + 1);
        drawText(ib, 16, oy + 6, 26, FbColor(150, 150, 150, 255));

        std::string dt = orders[i].orderText;
        if (dt.length() > 55) dt = dt.substr(0, 53) + "...";
        drawText(dt.c_str(), 60, oy + 6, 24, FbColor(200, 200, 200, 255));

        if (orders[i].penaltyDiscount > 0) {
            char pb[32];
            snprintf(pb, sizeof(pb), "Penalty: %d", orders[i].penaltyDiscount);
            drawText(pb, 16, oy + 40, 24, FbColor(255, 100, 100, 255));
        }
        if (orders[i].isCancelled) {
            drawText("[CANCEL]", 160, oy + 40, 24, FbColor(255, 50, 50, 255));
        }
        if (orders[i].holdTurns > 0) {
            char kb[32];
            snprintf(kb, sizeof(kb), "[Kept %d turns]", orders[i].holdTurns);
            drawText(kb, 300, oy + 40, 24, FbColor(100, 255, 100, 255));
            drawRectangleLines(8, oy, 910, 68, FbColor(0, 255, 0, 150), 2);
        }
    }
}

//商店面板
void MiniFb::drawShop(const ShopManager& sm, int turn, int hoveredItem) {
    const auto& items = sm.getCurrentShopItems();
    int st = 610;
    drawRectangle(4, st, 926, 640, FbColor(25, 25, 30, 255));
    drawRectangleLines(4, st, 926, 640, FbColor(80, 80, 80, 255), 1);
    drawText("SHOP", 14, st + 6, 28, FbColor(200, 200, 200, 255));
    drawLine(14, st + 36, 916, st + 36, FbColor(80, 80, 80, 255));

    for (size_t i = 0; i < items.size(); ++i) {
        int iy = st + 44 + (int)i * 192;
        FbColor bg = ((int)i == hoveredItem) ? FbColor(60, 60, 70, 255) : FbColor(35, 35, 40, 255);
        drawRectangle(8, iy, 910, 186, bg);
        drawRectangleLines(8, iy, 910, 186, FbColor(100, 100, 100, 255), 1);

        int px = 14, py = iy + 14;
        for (const auto& sb : items[i].blocks) {
            int bx = px + sb.relX * 38;
            int by = py + sb.relY * 38;
            drawRectangle(bx, by, 34, 34, getBlockColor(sb.type));
            drawRectangleLines(bx, by, 34, 34, FbColor(150, 150, 150, 255), 1);
        }

        char pbuf[32];
        snprintf(pbuf, sizeof(pbuf), "Price: %d G", items[i].price);
        drawText(pbuf, 180, iy + 10, 28, FbColor(255, 215, 0, 255));

        snprintf(pbuf, sizeof(pbuf), "%zu blocks", items[i].blocks.size());
        drawText(pbuf, 180, iy + 46, 24, FbColor(180, 180, 180, 255));

        std::string ts = "Types: ";
        for (size_t j = 0; j < items[i].blocks.size(); ++j) {
            if (j > 0) ts += ", ";
            ts += getBlockName(items[i].blocks[j].type);
        }
        drawText(ts.c_str(), 180, iy + 78, 22, FbColor(160, 160, 160, 255));
    }

    char ub[64];
    snprintf(ub, sizeof(ub), "Turn: %d | Tier: %s", turn,
        turn >= 10 ? "All" : (turn >= 6 ? "High" : (turn >= 3 ? "Mid" : "Basic")));
    drawText(ub, 14, st + 610, 22, FbColor(120, 120, 120, 255));
}

//主菜单
void MiniFb::drawMainMenu() {
    drawRectangle(0, 0, m_impl->width, m_impl->height, FbColor(15, 15, 25, 255));

    const char* title = "Just Drop It!";
    int tw = measureText(title, 100);
    drawText(title, (m_impl->width - tw) / 2, 240, 100, FbColor(255, 215, 0, 255));

    const char* sub = "Warehouse + Chain Reaction Puzzle";
    int sw = measureText(sub, 42);
    drawText(sub, (m_impl->width - sw) / 2, 360, 42, FbColor(180, 180, 180, 255));

    int bx = (m_impl->width - 360) / 2, by = 480;
    drawRectangle(bx, by, 360, 70, FbColor(40, 80, 40, 255));
    drawRectangleLines(bx, by, 360, 70, FbColor(80, 160, 80, 255), 2);
    const char* st = "START GAME";
    int stw = measureText(st, 48);
    drawText(st, bx + (360 - stw) / 2, by + 10, 48, FbColor(200, 255, 200, 255));

    const char* inst[] = {
        "L-click Block: Select/Unselect",
        "L-click Order: Submit | R-click Order: Keep",
        "L-click Shop: Buy (then click grid column)",
        "R-click / Esc: Cancel Buy | End Turn: bottom-left"
    };
    int iy = 620;
    for (int i = 0; i < 4; ++i) {
        int iw = measureText(inst[i], 30);
        drawText(inst[i], (m_impl->width - iw) / 2, iy, 30, FbColor(140, 140, 140, 255));
        iy += 42;
    }
}

//游戏结束
void MiniFb::drawGameOver(int finalTurn, int finalMoney) {
    drawRectangle(0, 0, m_impl->width, m_impl->height, FbColor(25, 10, 10, 255));

    const char* title = "GAME OVER";
    int tw = measureText(title, 100);
    drawText(title, (m_impl->width - tw) / 2, 240, 100, FbColor(255, 50, 50, 255));

    char buf[64];
    snprintf(buf, sizeof(buf), "Turns: %d", finalTurn);
    int bw = measureText(buf, 48);
    drawText(buf, (m_impl->width - bw) / 2, 360, 48, FbColor(220, 220, 220, 255));

    snprintf(buf, sizeof(buf), "Gold: %d", finalMoney);
    bw = measureText(buf, 48);
    drawText(buf, (m_impl->width - bw) / 2, 420, 48, FbColor(255, 215, 0, 255));

    if (finalMoney <= 0) {
        drawText("Bankrupt!", (m_impl->width - measureText("Bankrupt!", 42)) / 2, 480, 42, FbColor(255, 120, 120, 255));
    } else {
        drawText("Warehouse Full!", (m_impl->width - measureText("Warehouse Full!", 42)) / 2, 480, 42, FbColor(255, 120, 120, 255));
    }

    int bx = (m_impl->width - 360) / 2, by = 560;
    drawRectangle(bx, by, 360, 70, FbColor(60, 40, 40, 255));
    drawRectangleLines(bx, by, 360, 70, FbColor(160, 80, 80, 255), 2);
    const char* rt = "MAIN MENU";
    int rtw = measureText(rt, 48);
    drawText(rt, bx + (360 - rtw) / 2, by + 10, 48, FbColor(255, 180, 180, 255));
}

//方块信息弹窗
void MiniFb::drawBlockTooltip(int gridX, int gridY, const GridManager& gm, int mouseX, int mouseY) {
    Block* b = gm.getBlock(gridX, gridY);
    if (!b || b->getType() == BlockType::None) return;

    BlockType type = b->getType();
    int tw = 360, th = 180;
    int tx = mouseX + 15, ty = mouseY + 15;
    if (tx + tw > m_impl->width) tx = mouseX - tw - 15;
    if (ty + th > m_impl->height) ty = mouseY - th - 15;

    drawRectangle(tx, ty, tw, th, FbColor(20, 20, 30, 240));
    drawRectangleLines(tx, ty, tw, th, FbColor(150, 150, 150, 255), 1);

    FbColor bc = getBlockColor(type);
    drawRectangle(tx + 4, ty + 4, 22, 22, bc);
    drawRectangleLines(tx + 4, ty + 4, 22, 22, FbColor(200, 200, 200, 255), 1);

    char nb[64];
    snprintf(nb, sizeof(nb), "%s (ID: %d)", getBlockName(type), (int)type);
    drawText(nb, tx + 30, ty + 4, 20, FbColor(255, 255, 255, 255));

    int lineY = ty + 28;
    if (b->getEnvStatus() == Environment::Fire) {
        drawText("[ON FIRE]", tx + 30, lineY, 18, FbColor(255, 80, 80, 255));
        lineY += 22;
    }
    if (b->getTimer() > 0) {
        char tb[32];
        snprintf(tb, sizeof(tb), "Timer: %d", b->getTimer());
        drawText(tb, tx + 4, lineY, 18, FbColor(180, 180, 180, 255));
        lineY += 22;
    }
    if (b->getFireTimer() > 0) {
        char fb[32];
        snprintf(fb, sizeof(fb), "Fire age: %d", b->getFireTimer());
        drawText(fb, tx + 4, lineY, 18, FbColor(255, 150, 100, 255));
        lineY += 22;
    }

    const char* desc = getBlockDesc(type);
    if (strlen(desc) > 0) {
        int mid = (int)strlen(desc) / 2;
        std::string l1(desc, mid);
        std::string l2(desc + mid);
        drawText(l1.c_str(), tx + 4, lineY, 16, FbColor(200, 200, 200, 255));
        drawText(l2.c_str(), tx + 4, lineY + 22, 16, FbColor(200, 200, 200, 255));
    }
}

//订单详情弹窗
void MiniFb::drawOrderTooltip(const Order& order, int mouseX, int mouseY) {
    int tw = 520, th = 180;
    int tx = mouseX + 15, ty = mouseY + 15;
    if (tx + tw > m_impl->width) tx = mouseX - tw - 15;
    if (ty + th > m_impl->height) ty = mouseY - th - 15;

    drawRectangle(tx, ty, tw, th, FbColor(20, 20, 35, 245));
    drawRectangleLines(tx, ty, tw, th, FbColor(150, 150, 150, 255), 1);
    drawText("Order Details:", tx + 4, ty + 3, 20, FbColor(255, 215, 0, 255));

    const std::string& ft = order.orderText;
    int cpl = 38, lineY = ty + 26;
    for (size_t pos = 0; pos < ft.length() && lineY < ty + th - 20; ) {
        size_t len = (pos + cpl < ft.length()) ? cpl : ft.length() - pos;
        std::string line = ft.substr(pos, len);
        drawText(line.c_str(), tx + 4, lineY, 18, FbColor(200, 200, 200, 255));
        pos += len;
        lineY += 22;
    }

    if (order.penaltyDiscount != 0) {
        char pb[32];
        snprintf(pb, sizeof(pb), "Penalty: %d", order.penaltyDiscount);
        drawText(pb, tx + 4, ty + th - 20, 18, FbColor(255, 120, 120, 255));
    }
}

//商店放置预览
void MiniFb::drawShopPlacementPreview(const ShopManager& sm, int itemIndex, int targetCol, const GridManager& gm) {
    if (itemIndex < 0 || itemIndex >= (int)sm.getCurrentShopItems().size()) return;
    const auto& item = sm.getCurrentShopItems()[itemIndex];

    for (const auto& sb : item.blocks) {
        int gx = targetCol + sb.relX;
        int gy = sb.relY;
        if (gx < 0 || gx >= GRID_COLS || gy < 0 || gy >= GRID_ROWS) continue;

        int sx = GRID_ORIGIN_X + gx * CELL_SIZE;
        int sy = GRID_ORIGIN_Y + gy * CELL_SIZE;

        bool free = (gm.getBlockTypeAt(gx, gy) == BlockType::None);
        FbColor color = getBlockColor(sb.type);
        color.a = 150;
        drawRectangle(sx, sy, CELL_SIZE, CELL_SIZE, color);

        if (free) {
            drawRectangleLines(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(0, 255, 0, 200), 2);
        } else {
            drawRectangleLines(sx, sy, CELL_SIZE, CELL_SIZE, FbColor(255, 0, 0, 200), 2);
            drawLine(sx, sy, sx + CELL_SIZE, sy + CELL_SIZE, FbColor(255, 0, 0, 200));
            drawLine(sx + CELL_SIZE, sy, sx, sy + CELL_SIZE, FbColor(255, 0, 0, 200));
        }
    }
}

void MiniFb::drawStatusMessage(const char* msg) {
    int fontSize = 36;
    int tw = measureText(msg, fontSize);
    int pw = tw + 60;
    int ph = 70;
    int tx = (m_impl->width - pw) / 2;
    int ty = m_impl->height / 2 - ph / 2;
    drawRectangle(tx + 4, ty + 4, pw, ph, FbColor(0, 0, 0, 180));
    drawRectangle(tx, ty, pw, ph, FbColor(40, 10, 10, 240));
    drawRectangleLines(tx, ty, pw, ph, FbColor(255, 60, 60, 255), 3);
    drawText(msg, tx + (pw - tw) / 2, ty + 16, fontSize, FbColor(255, 200, 200, 255));
}

//结束回合按钮
void MiniFb::drawEndTurnButton() {
    int bx = 4, by = 1254, bw = 926, bh = 34;
    drawRectangle(bx, by, bw, bh, FbColor(50, 50, 30, 255));
    drawRectangleLines(bx, by, bw, bh, FbColor(120, 120, 80, 255), 1);
    const char* tx = "END TURN (reaction runs next turn)";
    int tw = measureText(tx, 26);
    drawText(tx, bx + (bw - tw) / 2, by + 2, 26, FbColor(220, 220, 180, 255));
}

int MiniFb::getOrderIndexAt(int screenX, int screenY, int orderCount) const {
    if (screenX < 8 || screenX > 920 || orderCount <= 0) return -1;
    int idx = (screenY - 100) / 72;
    if (idx >= 0 && idx < orderCount && screenY >= 100) return idx;
    return -1;
}

int MiniFb::getShopIndexAt(int screenX, int screenY, int shopCount) const {
    if (screenX < 8 || screenX > 920 || shopCount <= 0) return -1;
    int st = 610 + 44;
    int idx = (screenY - st) / 192;
    if (idx >= 0 && idx < shopCount && screenY >= st) return idx;
    return -1;
}

bool MiniFb::isEndTurnButtonHovered(int screenX, int screenY) const {
    return screenX >= 4 && screenX <= 930 && screenY >= 1254 && screenY <= 1288;
}

bool MiniFb::getGridCell(int screenX, int screenY, int& outGridX, int& outGridY) const {
    int gx = (screenX - GRID_ORIGIN_X) / CELL_SIZE;
    int gy = (screenY - GRID_ORIGIN_Y) / CELL_SIZE;
    if (gx >= 0 && gx < GRID_COLS && gy >= 0 && gy < GRID_ROWS) {
        outGridX = gx; outGridY = gy; return true;
    }
    return false;
}

bool MiniFb::isStartButtonHovered(int screenX, int screenY) const {
    int bx = (m_impl->width - 360) / 2, by = 480;
    return screenX >= bx && screenX <= bx + 360 && screenY >= by && screenY <= by + 70;
}

bool MiniFb::isRestartButtonHovered(int screenX, int screenY) const {
    int bx = (m_impl->width - 360) / 2, by = 560;
    return screenX >= bx && screenX <= bx + 360 && screenY >= by && screenY <= by + 70;
}

void MiniFb::beginDrawingDiag() { BeginDrawing(); ClearBackground(RED); }
void MiniFb::drawDiag() { DrawText("CRITICAL TEST", 10, 10, 20, WHITE); }
void MiniFb::endDrawingDiag() { EndDrawing(); }