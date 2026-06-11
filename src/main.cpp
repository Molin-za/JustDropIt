#define NOGDI
#define NOUSER
#include "raylib.h"
#include "Common.h"
#include "GridManager.h"
#include "OrderManager.h"
#include "ShopManager.h"
#include "MiniFb.h"
#include <vector>

static std::vector<std::vector<Block*>> buildGridSnapshot(const GridManager& gm) {
    std::vector<std::vector<Block*>> snap(GRID_ROWS, std::vector<Block*>(GRID_COLS, nullptr));
    for (int y = 0; y < GRID_ROWS; ++y)
        for (int x = 0; x < GRID_COLS; ++x)
            snap[y][x] = gm.getBlock(x, y);
    return snap;
}

int main() {
    MiniFb fb;
    if (!fb.init(1560, 1280, "Just Drop It!")) return 1;

    GameState screen = GameState::MainMenu;
    GridManager gm;
    OrderManager om(20);
    ShopManager sm;

    int turn = 0;
    int pendingBuyIndex = -1;
    int pendingBuyCol = 4;
    const char* statusMsg = nullptr;
    double statusTime = 0;

    while (!fb.shouldClose()) {
        int mx = fb.getMouseX();
        int my = fb.getMouseY();

        switch (screen) {

        case GameState::MainMenu: {
            if (fb.isStartButtonHovered(mx, my) && fb.isMouseButtonPressed(FB_MOUSE_LEFT)) {
                gm.initGrid();
                om = OrderManager(20);
                turn = 0;
                pendingBuyIndex = -1;

                //随机放置3-5个初始基础方块
                int initCount = 3 + (rand() % 3);
                BlockType basic[] = {BlockType::Red, BlockType::Yellow, BlockType::Blue, BlockType::Green, BlockType::White, BlockType::Black};
                for (int n = 0; n < initCount; n++) {
                    int attempts = 0;
                    while (attempts < 200) {
                        int rx = rand() % GRID_COLS;
                        int ry = 5 + (rand() % (GRID_ROWS - 8));
                        //检查该位置及邻居是否为空
                        bool clear = true;
                        for (int dy = -1; dy <= 1 && clear; dy++)
                            for (int dx = -1; dx <= 1 && clear; dx++)
                                if (gm.getBlockTypeAt(rx+dx, ry+dy) != BlockType::None) clear = false;
                        if (clear) {
                            gm.placeBlock(rx, ry, basic[rand() % 6]);
                            break;
                        }
                        attempts++;
                    }
                }

                sm.refreshShop(turn);
                om.generateTurnOrders();
                gm.runReactionEngine();

                screen = GameState::Playing;
            }
            break;
        }

        case GameState::Playing: {
            gm.cleanupExplosionEvents(fb.getTime());
            int gx, gy;

            if (pendingBuyIndex >= 0) {
                if (fb.isMouseButtonPressed(FB_MOUSE_RIGHT) || fb.isKeyPressed(FB_KEY_ESCAPE)) {
                    pendingBuyIndex = -1;
                }
                if (fb.getGridCell(mx, my, gx, gy)) {
                    pendingBuyCol = gx;
                    if (pendingBuyIndex >= 0 && (size_t)pendingBuyIndex < sm.getCurrentShopItems().size()) {
                        int maxRelX = 0;
                        for (const auto& sb : sm.getCurrentShopItems()[pendingBuyIndex].blocks)
                            if (sb.relX > maxRelX) maxRelX = sb.relX;
                        if (pendingBuyCol + maxRelX >= GRID_COLS)
                            pendingBuyCol = GRID_COLS - 1 - maxRelX;
                    }
                }
            }

            if (fb.getGridCell(mx, my, gx, gy) && fb.isMouseButtonPressed(FB_MOUSE_LEFT)) {
                if (pendingBuyIndex >= 0) {
                    const auto& shopItems = sm.getCurrentShopItems();
                    if ((size_t)pendingBuyIndex < shopItems.size() && om.getPlayerMoney() < shopItems[pendingBuyIndex].price) {
                        statusMsg = "Not enough gold!";
                        statusTime = fb.getTime();
                    } else if (sm.buyItem(pendingBuyIndex, gx, om, gm)) {
                        pendingBuyIndex = -1;
                    }
                } else {
                    gm.toggleSelectBlock(gx, gy);
                }
            }

            if (pendingBuyIndex < 0) {
                const auto& orders = om.getCurrentOrders();
                int oi = fb.getOrderIndexAt(mx, my, (int)orders.size());
                if (oi >= 0 && fb.isMouseButtonPressed(FB_MOUSE_LEFT)) {
                    if (!om.submitOrder(oi, gm)) {
                        statusMsg = "Submit failed: select connected correct blocks";
                        statusTime = fb.getTime();
                    }
                }
                if (oi >= 0 && fb.isMouseButtonPressed(FB_MOUSE_RIGHT)) {
                    auto& mutableOrders = om.getMutableOrders();
                    if (mutableOrders[oi].holdTurns > 0) {
                        mutableOrders[oi].holdTurns = 0;
                        statusMsg = "Order un-kept";
                        statusTime = fb.getTime();
                    } else {
                        om.keepOrder(oi);
                        statusMsg = "Order kept";
                        statusTime = fb.getTime();
                    }
                }
            }

            if (pendingBuyIndex < 0) {
                const auto& items = sm.getCurrentShopItems();
                int si = fb.getShopIndexAt(mx, my, (int)items.size());
                if (si >= 0 && fb.isMouseButtonPressed(FB_MOUSE_LEFT)) {
                    pendingBuyIndex = si;
                }
            }

            if (pendingBuyIndex < 0 && fb.isEndTurnButtonHovered(mx, my) && fb.isMouseButtonPressed(FB_MOUSE_LEFT)) {
                om.settleEndTurn(buildGridSnapshot(gm));
                om.addMoney(-2);
                turn++;
                sm.refreshShop(turn);
                om.generateTurnOrders();
                gm.runReactionEngine();

                if (gm.getIsGridFull() || om.getPlayerMoney() <= 0) {
                    screen = GameState::GameOver;
                }
            }
            break;
        }

        case GameState::GameOver: {
            if (fb.isRestartButtonHovered(mx, my) && fb.isMouseButtonPressed(FB_MOUSE_LEFT)) {
                screen = GameState::MainMenu;
            }
            break;
        }

        default:
            break;
        }

        fb.beginDrawing();

        switch (screen) {

        case GameState::MainMenu:
            fb.drawMainMenu();
            break;

        case GameState::Playing: {
            double time = fb.getTime();

            fb.drawHUD(om.getPlayerMoney(), turn);
            fb.drawGrid(gm, time);

            const auto& orders = om.getCurrentOrders();
            int hoveredOrder = fb.getOrderIndexAt(mx, my, (int)orders.size());
            fb.drawOrders(om, hoveredOrder);

            const auto& items = sm.getCurrentShopItems();
            int hoveredShop = fb.getShopIndexAt(mx, my, (int)items.size());
            fb.drawShop(sm, turn, hoveredShop);

            fb.drawEndTurnButton();

            int gx, gy;
            if (fb.getGridCell(mx, my, gx, gy)) {
                fb.drawBlockTooltip(gx, gy, gm, mx, my);
            }

            if (hoveredOrder >= 0 && hoveredOrder < (int)orders.size()) {
                fb.drawOrderTooltip(orders[hoveredOrder], mx, my);
            }

            if (pendingBuyIndex >= 0) {
                fb.drawShopPlacementPreview(sm, pendingBuyIndex, pendingBuyCol, gm);
            }

            if (statusMsg && time - statusTime < 3.0) {
                fb.drawStatusMessage(statusMsg);
            }
            break;
        }

        case GameState::GameOver:
            fb.drawGameOver(turn, om.getPlayerMoney());
            break;

        default:
            break;
        }

        fb.endDrawing();
    }

    return 0;
}
