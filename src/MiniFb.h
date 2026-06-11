#pragma once
#include "Common.h"
#include "OrderManager.h"
#include <string>
#include <memory>

struct FbColor {
    unsigned char r, g, b, a;
    FbColor(unsigned char r_ = 255, unsigned char g_ = 255, unsigned char b_ = 255, unsigned char a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}
};

enum FbMouseButton { FB_MOUSE_LEFT = 0, FB_MOUSE_RIGHT = 1, FB_MOUSE_MIDDLE = 2 };
enum FbKey { FB_KEY_ESCAPE = 256, FB_KEY_ENTER = 257, FB_KEY_SPACE = 32, FB_KEY_R = 82, FB_KEY_LEFT = 263, FB_KEY_RIGHT = 262 };

class GridManager;
class ShopManager;

class MiniFb {
public:
    MiniFb();
    ~MiniFb();

    bool init(int width, int height, const char* title);
    bool shouldClose() const;
    void beginDrawing();
    void endDrawing();
    double getTime() const;

    bool isMouseButtonPressed(FbMouseButton button) const;
    bool isKeyPressed(FbKey key) const;
    int getMouseX() const;
    int getMouseY() const;

    void drawRectangle(int x, int y, int w, int h, FbColor color);
    void drawRectangleLines(int x, int y, int w, int h, FbColor color, int thickness = 1);
    void drawText(const char* text, int x, int y, int fontSize, FbColor color);
    int measureText(const char* text, int fontSize) const;
    void drawLine(int x1, int y1, int x2, int y2, FbColor color);

    void drawHUD(int money, int turn);
    void drawGrid(const GridManager& gm, double time);
    void drawOrders(const OrderManager& om, int hoveredOrder);
    void drawShop(const ShopManager& sm, int turn, int hoveredItem);
    void drawMainMenu();
    void drawGameOver(int finalTurn, int finalMoney);
    void drawBlockTooltip(int gridX, int gridY, const GridManager& gm, int mouseX, int mouseY);
    void drawOrderTooltip(const Order& order, int mouseX, int mouseY);
    void drawShopPlacementPreview(const ShopManager& sm, int itemIndex, int targetCol, const GridManager& gm);
    void drawEndTurnButton();
    void drawStatusMessage(const char* msg);

    void beginDrawingDiag();
    void drawDiag();
    void endDrawingDiag();

    bool getGridCell(int screenX, int screenY, int& outGridX, int& outGridY) const;
    int getOrderIndexAt(int screenX, int screenY, int orderCount) const;
    int getShopIndexAt(int screenX, int screenY, int shopCount) const;
    bool isEndTurnButtonHovered(int screenX, int screenY) const;
    bool isStartButtonHovered(int screenX, int screenY) const;
    bool isRestartButtonHovered(int screenX, int screenY) const;

    static constexpr int GRID_ORIGIN_X = 940;
    static constexpr int GRID_ORIGIN_Y = 66;
    static constexpr int HUD_HEIGHT = 50;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
