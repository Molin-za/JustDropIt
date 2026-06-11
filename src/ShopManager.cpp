#include "ShopManager.h"
#include <cstdlib>
#include <ctime>

ShopManager::ShopManager() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

//根据回合数解锁方块类型池
std::vector<BlockType> ShopManager::getUnlockedBlockTypes(int turnCounter) {
    std::vector<BlockType> pool;

    //0回合即可刷出
    pool.push_back(BlockType::SkyBlue);
    pool.push_back(BlockType::Black);
    pool.push_back(BlockType::White);

    //3回合后增加
    if (turnCounter >= 3) {
        pool.push_back(BlockType::Silver);
        pool.push_back(BlockType::Gray);
        pool.push_back(BlockType::Brown);
        pool.push_back(BlockType::Blue);
        pool.push_back(BlockType::Yellow);
        pool.push_back(BlockType::LightYellow);
    }

    //5回合后增加
    if (turnCounter >= 5) {
        pool.push_back(BlockType::Transparent);
        pool.push_back(BlockType::Red);
        pool.push_back(BlockType::Green);
        pool.push_back(BlockType::Orange);
        pool.push_back(BlockType::Cyan);
    }

    //8回合后增加
    if (turnCounter >= 8) {
        pool.push_back(BlockType::Gold);
        pool.push_back(BlockType::Jade);
    }

    return pool;
}

//给商品添加一个方块
void addShopBlock(ShopItem& item, int rx, int ry, BlockType type) {
    ShopBlock sb = { type, rx, ry };
    item.blocks.push_back(sb);
}

//商品生成
void ShopManager::refreshShop(int turnCounter) {
    currentShopItems.clear();
    std::vector<BlockType> pool = getUnlockedBlockTypes(turnCounter);
    if (pool.empty()) return;

    const int ITEM_COUNT = 3; //每次生成商品数

    for (int i = 0; i < ITEM_COUNT; ++i) {
        ShopItem newItem;

        //随机选择2-4中任意数量生成方块组合
        int blockCount = 2 + (std::rand() % 3);

        //从池子里随机抽取每种方块的颜色(按比例权重)
        std::vector<BlockType> chosenTypes;
        for (int b = 0; b < blockCount; ++b) {
            // Build weighted list based on turn tier
            std::vector<BlockType> weightedPool;
            int baseW, tier3W, tier5W, tier8W;
            if (turnCounter >= 8)      { baseW = 40; tier3W = 30; tier5W = 20; tier8W = 10; }
            else if (turnCounter >= 5) { baseW = 50; tier3W = 30; tier5W = 20; tier8W = 0; }
            else if (turnCounter >= 3) { baseW = 60; tier3W = 40; tier5W = 0;  tier8W = 0; }
            else                       { baseW = 100; tier3W = 0; tier5W = 0;  tier8W = 0; }
            for (BlockType t : pool) {
                if (t == BlockType::SkyBlue || t == BlockType::Black || t == BlockType::White)
                    for (int w = 0; w < baseW / 3; w++) weightedPool.push_back(t);
                else if (t == BlockType::Silver || t == BlockType::Gray || t == BlockType::Brown || t == BlockType::Blue || t == BlockType::Yellow || t == BlockType::LightYellow)
                    for (int w = 0; w < tier3W / 6; w++) weightedPool.push_back(t);
                else if (t == BlockType::Transparent || t == BlockType::Red || t == BlockType::Green || t == BlockType::Orange || t == BlockType::Cyan)
                    for (int w = 0; w < tier5W / 5; w++) weightedPool.push_back(t);
                else if (t == BlockType::Gold || t == BlockType::Jade)
                    for (int w = 0; w < tier8W / 2; w++) weightedPool.push_back(t);
            }
            chosenTypes.push_back(weightedPool[std::rand() % weightedPool.size()]);
        }

        //根据方块数量随机选择组合方式生成形状
        if (blockCount == 2) {
            int shapeType = std::rand() % 2; // 0: 竖, 1: 横
            if (shapeType == 0) { //竖
                addShopBlock(newItem, 0, 0, chosenTypes[0]);
                addShopBlock(newItem, 0, 1, chosenTypes[1]);
            }
            else { //横
                addShopBlock(newItem, 0, 0, chosenTypes[0]);
                addShopBlock(newItem, 1, 0, chosenTypes[1]);
            }
        }
        else if (blockCount == 3) {
            int shapeType = std::rand() % 6; //0: 竖, 1: 横, 2-5: 四种拐角形
            if (shapeType == 0) { //竖
                addShopBlock(newItem, 0, 0, chosenTypes[0]);
                addShopBlock(newItem, 0, 1, chosenTypes[1]);
                addShopBlock(newItem, 0, 2, chosenTypes[2]);
            }
            else if (shapeType == 1) { //横
                addShopBlock(newItem, 0, 0, chosenTypes[0]);
                addShopBlock(newItem, 1, 0, chosenTypes[1]);
                addShopBlock(newItem, 2, 0, chosenTypes[2]);
            }
            else { //4种拐角形
                int rot = shapeType - 2;
                addShopBlock(newItem, 0, 0, chosenTypes[0]);
                if (rot == 0) { addShopBlock(newItem, 1, 0, chosenTypes[1]); addShopBlock(newItem, 0, 1, chosenTypes[2]); }
                else if (rot == 1) { addShopBlock(newItem, 1, 0, chosenTypes[1]); addShopBlock(newItem, 1, 1, chosenTypes[2]); }
                else if (rot == 2) { addShopBlock(newItem, 0, 1, chosenTypes[1]); addShopBlock(newItem, 1, 1, chosenTypes[2]); }
                else { addShopBlock(newItem, 1, 0, chosenTypes[0]); addShopBlock(newItem, 0, 1, chosenTypes[1]); addShopBlock(newItem, 1, 1, chosenTypes[2]); }
            }
        }
        else if (blockCount == 4) {
            int shapeType = std::rand() % 15; //0:竖, 1:横, 2:正方形, 3-6:3+1形状, 7-10:2+2形状, 11-14:T形

            if (shapeType == 0) { //竖
                for (int b = 0; b < 4; ++b) addShopBlock(newItem, 0, b, chosenTypes[b]);
            }
            else if (shapeType == 1) { //横
                for (int b = 0; b < 4; ++b) addShopBlock(newItem, b, 0, chosenTypes[b]);
            }
            else if (shapeType == 2) { //正方形
                addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 1, 0, chosenTypes[1]);
                addShopBlock(newItem, 0, 1, chosenTypes[2]); addShopBlock(newItem, 1, 1, chosenTypes[3]);
            }
            else if (shapeType >= 3 && shapeType <= 6) { //四种"3+1"形状旋转
                int rot = shapeType - 3;
                addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 0, 1, chosenTypes[1]); addShopBlock(newItem, 0, 2, chosenTypes[2]);
                if (rot == 0)      addShopBlock(newItem, 1, 0, chosenTypes[3]);
                else if (rot == 1) addShopBlock(newItem, 1, 2, chosenTypes[3]);
                else if (rot == 2) { newItem.blocks.clear(); addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 1, 0, chosenTypes[1]); addShopBlock(newItem, 2, 0, chosenTypes[2]); addShopBlock(newItem, 0, 1, chosenTypes[3]); }
                else { newItem.blocks.clear(); addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 1, 0, chosenTypes[1]); addShopBlock(newItem, 2, 0, chosenTypes[2]); addShopBlock(newItem, 2, 1, chosenTypes[3]); }
            }
            else if (shapeType >= 7 && shapeType <= 10) { //四种"2+2"形状旋转
                int rot = shapeType - 7;
                if (rot == 0 || rot == 1) {
                    addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 1, 0, chosenTypes[1]);
                    addShopBlock(newItem, (rot == 0 ? 1 : 0), 1, chosenTypes[2]); addShopBlock(newItem, (rot == 0 ? 2 : 1), 1, chosenTypes[3]);
                }
                else {
                    addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 0, 1, chosenTypes[1]);
                    addShopBlock(newItem, 1, (rot == 2 ? 1 : 0), chosenTypes[2]); addShopBlock(newItem, 1, (rot == 2 ? 2 : 1), chosenTypes[3]);
                }
            }
            else { //四种"T"形旋转
                int rot = shapeType - 11;
                if (rot == 0) {
                    addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 1, 0, chosenTypes[1]); addShopBlock(newItem, 2, 0, chosenTypes[2]); addShopBlock(newItem, 1, 1, chosenTypes[3]);
                }
                else if (rot == 1) {
                    addShopBlock(newItem, 1, 0, chosenTypes[0]); addShopBlock(newItem, 0, 1, chosenTypes[1]); addShopBlock(newItem, 1, 1, chosenTypes[2]); addShopBlock(newItem, 2, 1, chosenTypes[3]);
                }
                else if (rot == 2) {
                    addShopBlock(newItem, 1, 0, chosenTypes[0]); addShopBlock(newItem, 0, 1, chosenTypes[1]); addShopBlock(newItem, 1, 1, chosenTypes[2]); addShopBlock(newItem, 1, 2, chosenTypes[3]);
                }
                else {
                    addShopBlock(newItem, 0, 0, chosenTypes[0]); addShopBlock(newItem, 0, 1, chosenTypes[1]); addShopBlock(newItem, 0, 2, chosenTypes[2]); addShopBlock(newItem, 1, 1, chosenTypes[3]);
                }
            }
        }

        //售价计算
        int totalPrice = 0;
        for (const auto& sb : newItem.blocks) {
            int baseValue = static_cast<int>(sb.type);
            if (baseValue <= 7)       totalPrice += 1;
            else if (baseValue <= 11) totalPrice += 2;
            else if (baseValue <= 14) totalPrice += 3;
            else                      totalPrice += 4;
        }

        //数量加价
        if (blockCount == 4)      totalPrice += 2;
        else if (blockCount == 3) totalPrice += 1;

        newItem.price = totalPrice;
        currentShopItems.push_back(newItem);
    }
}

bool ShopManager::buyItem(size_t index, int targetCol, OrderManager& orderManager, GridManager& gridManager) {
    //检查索引是否合法
    if (index >= currentShopItems.size()) {
        return false;
    }

    const ShopItem& item = currentShopItems[index];

    //检查玩家的钱够不够
    if (orderManager.getPlayerMoney() < item.price) {
        return false;
    }

    for (const auto& sb : item.blocks) {
        int finalCol = targetCol + sb.relX;
        int finalRow = sb.relY;

        //检查列边界
        if (finalCol < 0 || finalCol >= GRID_COLS) {
            return false;
        }

        //检查行边界
        if (finalRow < 0 || finalRow >= GRID_ROWS) {
            return false;
        }

        //检查目标格子是否已经被非空方块占用
        BlockType currentT = gridManager.getBlockTypeAt(finalCol, finalRow);
        if (currentT != BlockType::None) {
            return false;
        }
    }

    //扣钱
    orderManager.addMoney(-item.price);

    //放置新方块
    for (const auto& sb : item.blocks) {
        int finalCol = targetCol + sb.relX;
        int finalRow = sb.relY;

        gridManager.placeBlock(finalCol, finalRow, sb.type);
    }

    gridManager.handleGravity();

    currentShopItems.erase(currentShopItems.begin() + index);

    return true;
}
