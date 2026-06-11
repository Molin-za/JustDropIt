#pragma once
#include "Common.h"
#include "GridManager.h"
#include "OrderManager.h"
#include <vector>

struct ShopBlock {
    BlockType type;
    int relX;
    int relY;
};

//商店中的商品
struct ShopItem {
    std::vector<ShopBlock> blocks;  //方块列表
    int price = 0;                  //售价
};

class ShopManager {
private:
    std::vector<ShopItem> currentShopItems; //当前商店面板上的商品列表

    //根据当前回合数返回一个当前已解锁的方块类型池
    std::vector<BlockType> getUnlockedBlockTypes(int turnCounter);

public:
    //构造函数
    ShopManager();
    ~ShopManager() = default;

    //每回合刷新商店面板，随机生成若干个方块组合商品
    void refreshShop(int turnCounter);

    //购买商品接口
    bool buyItem(size_t index, int targetCol, OrderManager& orderManager, GridManager& gridManager);

    //获取当前商店商品列表
    const std::vector<ShopItem>& getCurrentShopItems() const { return currentShopItems; }
}; 
