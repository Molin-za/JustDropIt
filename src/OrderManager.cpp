#include "OrderManager.h"
#include "Block.h"
#include <cstdlib>

//构造函数
OrderManager::OrderManager(int startingMoney) {
    playerMoney = startingMoney;
    turnCounter = 0;
    currentOrders.clear();
}

//拒绝订单
void OrderManager::rejectOrder(size_t index) {
    if (index < currentOrders.size()) {
        currentOrders.erase(currentOrders.begin() + index);
    }
}

//保留订单
void OrderManager::keepOrder(size_t index) {
    if (index < currentOrders.size()) {
        currentOrders[index].holdTurns++;
    }
}

//回合结算
void OrderManager::settleEndTurn(const std::vector<std::vector<Block*>>& grid) {
    for (auto it = currentOrders.begin(); it != currentOrders.end(); ) {
        if (it->isCancelled) {
            it = currentOrders.erase(it);
            continue;
        }
        it->applyDebuffFunc(*it);
        if (it->isCancelled) {
            it = currentOrders.erase(it);
        }
        else {
            ++it;
        }
    }
}

//提交指定索引的订单
bool OrderManager::submitOrder(size_t index, GridManager& gridManager) {
    if (index >= currentOrders.size()) {
        return false;
    }

    Order& targetOrder = currentOrders[index];

    std::vector<std::pair<int, int>> selectedCoords;
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Block* b = gridManager.getBlock(x, y);
            if (b != nullptr && b->getSelected()) {
                selectedCoords.push_back({ x, y });
            }
        }
    }

    if (selectedCoords.empty()) {
        return false;
    }

    std::set<std::pair<int, int>> visited;
    std::queue<std::pair<int, int>> q;

    q.push(selectedCoords[0]);
    visited.insert(selectedCoords[0]);

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                Block* neighbor = gridManager.getBlock(nx, ny);
                if (neighbor != nullptr && neighbor->getSelected()) {
                    if (visited.find({ nx, ny }) == visited.end()) {
                        visited.insert({ nx, ny });
                        q.push({ nx, ny });
                    }
                }
            }
        }
    }

    if (visited.size() < selectedCoords.size()) {
        return false;
    }

    std::vector<std::vector<Block*>> gridSnapshot(GRID_ROWS, std::vector<Block*>(GRID_COLS, nullptr));
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            gridSnapshot[y][x] = gridManager.getBlock(x, y);
        }
    }

    int payout = targetOrder.validateFunc(gridSnapshot, targetOrder.penaltyDiscount);

    if (payout > 0) {
        playerMoney += payout;
        gridManager.extractSelectedBlocks();
        gridManager.handleGravity();
        currentOrders.erase(currentOrders.begin() + index);
        return true;
    }
    return false;
}

//每回合刷新
void OrderManager::generateTurnOrders() {
    currentOrders.erase(
        std::remove_if(currentOrders.begin(), currentOrders.end(),[](const Order& o) {return o.holdTurns == 0;}),
        currentOrders.end()
    );

    for (auto& o : currentOrders) {
        o.holdTurns = 0;
    }

    if (currentOrders.size() >= 6) {
        return;
    }

    int currentSpace = 6 - static_cast<int>(currentOrders.size());

    int newOrderCount = 3 + (rand() % 3);
    if (newOrderCount > currentSpace) {
        newOrderCount = currentSpace;
    }

    for (int i = 0; i < newOrderCount; ++i) {
        Order newOrder;

        int orderType = rand() % 26;

        switch (orderType) {
            //订单1：爆炸物爱好者
        case 0: {
            newOrder.orderText = "Explosives Fan: Need 3+ SkyBlue/Silver. Base 4G. -2 per missing. Bonus +1 per extra Gray/Silver (max +2). -2 per White/Red. -1 per turn, cancel after 2.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int targetCount = 0, bonusCount = 0, badCount = 0;
                bool anySelected = false;

                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || b->getType() == BlockType::None || !b->getSelected()) continue;

                        anySelected = true;
                        BlockType t = b->getType();

                        if (t == BlockType::SkyBlue || t == BlockType::Silver) {
                            if (targetCount < 3) { targetCount++; }
                            else { bonusCount++; }
                        }
                        else if (t == BlockType::Gray) {
                            bonusCount++;
                        }
                        else if (t == BlockType::White || t == BlockType::Red) {
                            badCount++;
                        }
                    }
                }

                if (!anySelected) return 0;

                int score = 4;

                if (targetCount < 3) {
                    score -= (3 - targetCount) * 2;
                }

                int bonusMoney = bonusCount * 1;
                if (bonusMoney > 2) bonusMoney = 2;
                score += bonusMoney;

                score -= badCount * 2;

                int finalPay = score - penalty;
                return (finalPay > 0) ? finalPay : 0; 
                };

            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 2) o.isCancelled = true;
                };
            break;
        }

              //订单2：高价值珠宝商
        case 1: {
            newOrder.orderText = "Jeweler: Need Gold or Jade. Base 0G. +4 per Gold, +(6+timer) per Jade. -2 per other block. First 2 turns free, then -2/turn. Cancel after 6.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int score = 0;
                bool hasTarget = false;

                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || b->getType() == BlockType::None || !b->getSelected()) continue;

                        BlockType t = b->getType();
                        if (t == BlockType::Gold) {
                            score += 4;
                            hasTarget = true;
                        }
                        else if (t == BlockType::Jade) {
                            score += (6 + b->getTimer());
                            hasTarget = true;
                        }
                        else {
                            score -= 2;
                        }
                    }
                }
                if (!hasTarget) return 0;

                int finalPay = score - penalty;
                return (finalPay > 0) ? finalPay : 0;
                };

            newOrder.applyDebuffFunc = [](Order& o) {
                if (o.penaltyDiscount == 0)       o.penaltyDiscount = -1;
                else if (o.penaltyDiscount == -1) o.penaltyDiscount = -2;
                else if (o.penaltyDiscount == -2) o.penaltyDiscount = 2;
                else if (o.penaltyDiscount > 0)   o.penaltyDiscount += 2;

                if (o.penaltyDiscount == 8) o.isCancelled = true;
                };
            break;
        }

              //订单3：叛乱分子
        case 2: {
            newOrder.orderText = "Rebel: Need 1 SkyBlue + 1 Black + 1 White, Black must touch both. Reward 8G. -4 if not touching, -2 per missing type. One-shot only.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                bool hasSky = false, hasBlack = false, hasWhite = false;
                int bx = -1, by = -1;
                int selectCount = 0;

                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || b->getType() == BlockType::None || !b->getSelected()) continue;

                        selectCount++;
                        BlockType t = b->getType();
                        if (t == BlockType::SkyBlue) hasSky = true;
                        if (t == BlockType::White)   hasWhite = true;
                        if (t == BlockType::Black) {
                            hasBlack = true;
                            bx = x; by = y;
                        }
                    }
                }
                if (selectCount == 0) return 0;

                int score = 8;

                int missing = (!hasSky) + (!hasBlack) + (!hasWhite);
                score -= missing * 2;

                if (hasSky && hasBlack && hasWhite) {
                    bool touchSky = false;
                    bool touchWhite = false;
                    int dx[] = { 0, 0, -1, 1 };
                    int dy[] = { -1, 1, 0, 0 };

                    for (int i = 0; i < 4; ++i) {
                        int nx = bx + dx[i];
                        int ny = by + dy[i];
                        if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                            Block* neighbor = grid[ny][nx];
                            if (neighbor && neighbor->getSelected()) {
                                if (neighbor->getType() == BlockType::SkyBlue) touchSky = true;
                                if (neighbor->getType() == BlockType::White) touchWhite = true;
                            }
                        }
                    }
                    if (!touchSky || !touchWhite) score -= 4;
                }
                else {
                    score -= 4;
                }

                return (score > 0) ? score : 0;
                };

            newOrder.applyDebuffFunc = [](Order& o) {
                o.isCancelled = true;
                };
            break;
        }

              //订单4：大众批发
        case 3: {
            newOrder.orderText = "Wholesale: Any blocks. Base 0G. First 6: +1 each (max 6). After 6: -2 each. -1 per delayed turn.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int totalSelected = 0;

                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        if (grid[y][x] && grid[y][x]->getType() != BlockType::None && grid[y][x]->getSelected()) {
                            totalSelected++;
                        }
                    }
                }
                if (totalSelected == 0) return 0;

                int score = 0;
                if (totalSelected <= 6) {
                    score += totalSelected * 1;
                }
                else {
                    score += 6;
                    score -= (totalSelected - 6) * 2;
                }

                int finalPay = score - penalty;
                return (finalPay > 0) ? finalPay : 0;
                };

            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                };
            break;
        }

              //订单5：火焰使者
        case 4: {
            newOrder.orderText = "Fire Lord: Need 4+ Red. Reward 11G. +3 per extra Red. -2 for every Red NOT on fire. -3 per turn, cancel after 1.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int redCount = 0;
                int firePenalty = 0;

                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (b && b->getType() == BlockType::Red && b->getSelected()) {
                            redCount++;
                            if (b->getEnvStatus() == Environment::Fire) {
                                firePenalty += 2;
                            }
                        }
                    }
                }
                if (redCount < 4) return 0;

                int score = 11;
                if (redCount > 4) {
                    score += (redCount - 4) * 3;
                }
                score -= firePenalty;

                int finalPay = score - penalty;
                return (finalPay > 0) ? finalPay : 0;
                };

            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 3;
                o.isCancelled = true;
                };
            break;
        }

              //订单6：谨慎收藏家
        case 5: {
            newOrder.orderText = "Collector: Need 1+ Gold + 1+ White, none on fire. Base 7G. -5 missing Gold, -3 missing White. +2 per extra Gold/White. -3 per other block. Cancel after 5.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int goldCount = 0, whiteCount = 0, otherCount = 0;
                bool anySelected = false;

                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || b->getType() == BlockType::None) continue;

                        if (b->getSelected()) {
                            anySelected = true;

                            if (b->getEnvStatus() == Environment::Fire) {
                                return 0;
                            }

                            BlockType t = b->getType();
                            if (t == BlockType::Gold) goldCount++;
                            else if (t == BlockType::White) whiteCount++;
                            else otherCount++;
                        }
                    }
                }
                if (!anySelected) return 0;

                int score = 7;

                if (goldCount == 0)  score -= 5;
                if (whiteCount == 0) score -= 3;

                if (goldCount > 1)   score += (goldCount - 1) * 2;
                if (whiteCount > 1)  score += (whiteCount - 1) * 2;

                score -= otherCount * 3;

                return (score > 0) ? score : 0;
                };

            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount -= 1;
                if (o.penaltyDiscount <= -5) {
                    o.isCancelled = true;
                }
            };
            break;
        }
              //订单7：化学家
        case 6: {
            newOrder.orderText = "Chemist: Need Brown + SkyBlue adjacent. Base 5G. -3 if not adjacent. +4 per extra pair. -1/turn, cancel after 3.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int brownCount = 0, pairCount = 0;
                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || b->getType() != BlockType::Brown || !b->getSelected()) continue;
                        brownCount++;
                        int dx[] = { 0,0,-1,1 }, dy[] = { -1,1,0,0 };
                        for (int i = 0; i < 4; ++i) {
                            int nx = x + dx[i], ny = y + dy[i];
                            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                                Block* nb = grid[ny][nx];
                                if (nb && nb->getSelected() && nb->getType() == BlockType::SkyBlue) { pairCount++; break; }
                            }
                        }
                    }
                }
                if (brownCount == 0) return 0;
                int score = 5;
                if (pairCount == 0) score -= 3;
                else if (pairCount > 1) score += (pairCount - 1) * 4;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 3) o.isCancelled = true;
            };
            break;
        }
              //订单8：纵火狂
        case 7: {
            newOrder.orderText = "Pyromaniac: Need 1+ Red + at least 1 block ON FIRE. Base 6G. -4 if no fire. +2 per extra burning block. Cancel after 1 turn.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                bool hasRed = false, hasFire = false;
                int fireCount = 0;
                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected()) continue;
                        if (b->getType() == BlockType::Red) hasRed = true;
                        if (b->getEnvStatus() == Environment::Fire) { hasFire = true; fireCount++; }
                    }
                }
                if (!hasRed) return 0;
                int score = 6;
                if (!hasFire) score -= 4;
                else if (fireCount > 1) score += (fireCount - 1) * 2;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) { o.isCancelled = true; };
            break;
        }
              //订单9：救火队
        case 8: {
            newOrder.orderText = "Firefighter: Need 1+ Cyan or Steam. Base 4G. +2 per extra. -3 per burning block selected. Cancel after 2 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int target = 0, fireBlocks = 0;
                for (int y = 0; y < GRID_ROWS; ++y) {
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected()) continue;
                        if (b->getType() == BlockType::Cyan || b->getType() == BlockType::Steam) target++;
                        if (b->getEnvStatus() == Environment::Fire) fireBlocks++;
                    }
                }
                if (target == 0) return 0;
                int score = 4 + (target - 1) * 2 - fireBlocks * 3;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 2) o.isCancelled = true;
            };
            break;
        }
              //订单10：环保主义者
        case 9: {
            newOrder.orderText = "Eco-Warrior: Need 3+ connected Green. Base 5G. +2 per extra Green. -2 per Red selected. -1/turn.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int green = 0, red = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected()) continue;
                        if (b->getType() == BlockType::Green) green++;
                        if (b->getType() == BlockType::Red) red++;
                    }
                if (green < 3) return 0;
                int score = 5 + (green - 3) * 2 - red * 2;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) { o.penaltyDiscount += 1; };
            break;
        }
              //订单11：矿工
        case 10: {
            newOrder.orderText = "Miner: Need 2+ Gray. Base 4G. +3 per extra Gray. Cancel after 3 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int grayCount = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (b && b->getSelected() && b->getType() == BlockType::Gray) grayCount++;
                    }
                if (grayCount < 2) return 0;
                int score = 4 + (grayCount - 2) * 3;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 3) o.isCancelled = true;
            };
            break;
        }
              //订单12：透明人
        case 11: {
            newOrder.orderText = "Ghost: Need 2+ Transparent. Base 5G. +2 per extra. No penalty.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int transCount = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (b && b->getSelected() && b->getType() == BlockType::Transparent) transCount++;
                    }
                if (transCount < 2) return 0;
                int score = 5 + (transCount - 2) * 2;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {};
            break;
        }
              //订单13：极简主义
        case 12: {
            newOrder.orderText = "Minimalist: Need EXACTLY 2 connected blocks. Base 3G. Cancel after 3 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int total = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x)
                        if (grid[y][x] && grid[y][x]->getSelected()) total++;
                if (total != 2) return 0;
                return (3 - penalty > 0) ? 3 - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 3) o.isCancelled = true;
            };
            break;
        }
              //订单14：彩虹收藏家
        case 13: {
            newOrder.orderText = "Rainbow: Need connected blocks of different colors. 2 colors=2G, 3=5G, 4=9G, 5=14G, 6=20G. Cancel after 2 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                bool seen[18] = { false };
                int colorCount = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected() || b->getType() == BlockType::None) continue;
                        int t = (int)b->getType();
                        if (!seen[t]) { seen[t] = true; colorCount++; }
                    }
                if (colorCount < 2) return 0;
                int score = 0;
                if (colorCount == 2) score = 2;
                else if (colorCount == 3) score = 5;
                else if (colorCount == 4) score = 9;
                else if (colorCount == 5) score = 14;
                else score = 20;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 2) o.isCancelled = true;
            };
            break;
        }
              //订单15：阴阳大师
        case 14: {
            newOrder.orderText = "Yin-Yang: Black and White count must be equal. Base 7G. +3 per pair. -2 per difference. Cancel after 2 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int black = 0, white = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected()) continue;
                        if (b->getType() == BlockType::Black) black++;
                        else if (b->getType() == BlockType::White) white++;
                    }
                if (black == 0 && white == 0) return 0;
                int score = 7;
                int diff = abs(black - white);
                if (diff == 0 && black > 0) score += black * 3;
                else score -= diff * 2;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 2) o.isCancelled = true;
            };
            break;
        }
              //订单16：清道夫
        case 15: {
            newOrder.orderText = "Sweeper: Need blocks spanning 4+ columns. Base 5G. +2 per extra column. Cancel after 2 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int minCol = 99, maxCol = -1;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x)
                        if (grid[y][x] && grid[y][x]->getSelected()) { if (x < minCol) minCol = x; if (x > maxCol) maxCol = x; }
                if (minCol > maxCol) return 0;
                int span = maxCol - minCol + 1;
                if (span < 4) return 0;
                int score = 5 + (span - 4) * 2;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 2) o.isCancelled = true;
            };
            break;
        }
              //订单17：园艺师
        case 16: {
            newOrder.orderText = "Gardener: Need LtYellow, no Yellow adjacent. Base 5G. +3 per extra. -2 per LtYellow with Yellow neighbor. Cancel after 3 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int ltCount = 0, tainted = 0;
                int dx[] = { 0,0,-1,1 }, dy[] = { -1,1,0,0 };
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected() || b->getType() != BlockType::LightYellow) continue;
                        ltCount++;
                        for (int i = 0; i < 4; ++i) {
                            int nx = x + dx[i], ny = y + dy[i];
                            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS)
                                if (grid[ny][nx] && grid[ny][nx]->getType() == BlockType::Yellow) { tainted++; break; }
                        }
                    }
                if (ltCount == 0) return 0;
                int score = 5 + (ltCount - 1) * 3 - tainted * 2;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 3) o.isCancelled = true;
            };
            break;
        }
              //订单18：计时员
        case 17: {
            newOrder.orderText = "Timekeeper: Need Blue or Steam. Base 6G. +2 per extra. -1 per non-timed block. Cancel after 1 turn.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int good = 0, other = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected()) continue;
                        if (b->getType() == BlockType::Blue || b->getType() == BlockType::Steam) good++;
                        else other++;
                    }
                if (good == 0) return 0;
                int score = 6 + (good - 1) * 2 - other * 1;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) { o.isCancelled = true; };
            break;
        }
              //订单19：建筑师
        case 18: {
            newOrder.orderText = "Architect: Need a 2x2 square of blocks. Base 10G. Cancel after 1 turn.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                for (int y = 0; y < GRID_ROWS - 1; ++y)
                    for (int x = 0; x < GRID_COLS - 1; ++x) {
                        Block* a = grid[y][x], *b = grid[y][x+1], *c = grid[y+1][x], *d = grid[y+1][x+1];
                        if (a && b && c && d && a->getSelected() && b->getSelected() && c->getSelected() && d->getSelected())
                            return (10 - penalty > 0) ? 10 - penalty : 0;
                    }
                return 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) { o.isCancelled = true; };
            break;
        }
              //订单20：赌徒
        case 19: {
            newOrder.orderText = "Gambler: Any connected blocks. Random: 50% +12G, 50% -5G. Cancel after 1 turn.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int count = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x)
                        if (grid[y][x] && grid[y][x]->getSelected()) count++;
                if (count == 0) return 0;
                int roll = rand() % 2;
                return (roll == 0 ? 12 : -5) - penalty;
            };
            newOrder.applyDebuffFunc = [](Order& o) { o.isCancelled = true; };
            break;
        }
              //订单21：末日信徒
        case 20: {
            newOrder.orderText = "Doomsayer: Need 1+ SkyBlue or Silver. Base 15G. Cancel after 1 turn.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int bomb = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (b && b->getSelected() && (b->getType() == BlockType::SkyBlue || b->getType() == BlockType::Silver)) bomb++;
                    }
                if (bomb == 0) return 0;
                return (15 - penalty > 0) ? 15 - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) { o.isCancelled = true; };
            break;
        }
              //订单22：完美主义
        case 21: {
            newOrder.orderText = "Perfectionist: Need all SAME type. Base: 1=3G, 2=8G, 3=15G, 4+ each extra +4G. Cancel after 2 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                BlockType firstType = BlockType::None;
                int count = 0;
                bool mismatch = false;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected() || b->getType() == BlockType::None) continue;
                        if (firstType == BlockType::None) firstType = b->getType();
                        else if (b->getType() != firstType) mismatch = true;
                        count++;
                    }
                if (count == 0 || mismatch) return 0;
                int score = 0;
                if (count == 1) score = 3;
                else if (count == 2) score = 8;
                else if (count == 3) score = 15;
                else score = 15 + (count - 3) * 4;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 2) o.isCancelled = true;
            };
            break;
        }
              //订单23：清算人
        case 22: {
            newOrder.orderText = "Liquidator: Need 5+ blocks. Base 3G. Over 10: +3 per extra. -2/turn, cancel after 3.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int count = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x)
                        if (grid[y][x] && grid[y][x]->getSelected()) count++;
                if (count < 5) return 0;
                int score = 3;
                if (count > 10) score += (count - 10) * 3;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 2;
                if (o.penaltyDiscount >= 6) o.isCancelled = true;
            };
            break;
        }
              //订单24：连锁大师
        case 23: {
            newOrder.orderText = "Chainmaster: Need pairs that react on contact. Base 8G. +4 per extra pair. Cancel after 2 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int pairs = 0;
                int dx[] = { 0,0,-1,1 }, dy[] = { -1,1,0,0 };
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (!b || !b->getSelected()) continue;
                        for (int i = 0; i < 4; ++i) {
                            int nx = x + dx[i], ny = y + dy[i];
                            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                                Block* nb = grid[ny][nx];
                                if (!nb || !nb->getSelected()) continue;
                                BlockType t1 = b->getType(), t2 = nb->getType();
                                if ((t1 == BlockType::Brown && t2 == BlockType::SkyBlue) || (t1 == BlockType::SkyBlue && t2 == BlockType::Brown) ||
                                    (t1 == BlockType::Yellow && t2 == BlockType::White) || (t1 == BlockType::White && t2 == BlockType::Yellow) ||
                                    (t1 == BlockType::Blue && (t2 == BlockType::Silver || t2 == BlockType::Black || t2 == BlockType::Yellow)) ||
                                    ((t1 == BlockType::Silver || t1 == BlockType::Black || t1 == BlockType::Yellow) && t2 == BlockType::Blue))
                                    pairs++;
                            }
                        }
                    }
                pairs /= 2;
                if (pairs == 0) return 0;
                int score = 8 + (pairs - 1) * 4;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 2) o.isCancelled = true;
            };
            break;
        }
              //订单25：幸存者
        case 24: {
            newOrder.orderText = "Survivor: Submit any blocks. After extraction, NO fire can remain. Base 6G. If fire remains after: -5G. Cancel after 3 turns.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                int count = 0;
                bool hasFire = false;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x) {
                        Block* b = grid[y][x];
                        if (b && b->getSelected()) count++;
                        if (b && b->getEnvStatus() == Environment::Fire && !b->getSelected()) hasFire = true;
                    }
                if (count == 0) return 0;
                int score = 6;
                if (hasFire) score -= 5;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {
                o.penaltyDiscount += 1;
                if (o.penaltyDiscount >= 3) o.isCancelled = true;
            };
            break;
        }
              //订单26：千层酥
        case 25: {
            newOrder.orderText = "Lasagna: Need blocks in 3+ different rows. Base 5G. +2 per extra row. No penalty.";

            newOrder.validateFunc = [](const std::vector<std::vector<Block*>>& grid, int penalty) -> int {
                bool rows[20] = { false };
                int rowCount = 0;
                for (int y = 0; y < GRID_ROWS; ++y)
                    for (int x = 0; x < GRID_COLS; ++x)
                        if (grid[y][x] && grid[y][x]->getSelected()) { if (!rows[y]) { rows[y] = true; rowCount++; } }
                if (rowCount < 3) return 0;
                int score = 5 + (rowCount - 3) * 2;
                return (score - penalty > 0) ? score - penalty : 0;
            };
            newOrder.applyDebuffFunc = [](Order& o) {};
            break;
        }
        }
        currentOrders.push_back(newOrder);
    }
}
