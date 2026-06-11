#pragma once
#include "Common.h"
#include "GridManager.h"
#include <functional>
#include <string>
#include <vector>

//订单管理数据结构
struct Order {
    std::string orderText;     //展示给玩家的订单文案
    int baseReward = 0;        //单次提交的基础奖金
    int penaltyDiscount = 0;   //累计延期导致的扣款
    bool isCancelled = false;  //该订单是否已被强制取消
    int holdTurns = 0;         //玩家已经连续保留了多少个回合该订单

    //结算函数
    std::function<int(const std::vector<std::vector<class Block*>>&, int penalty)> validateFunc;

    //每个回合结束时调用，让玩家选择保留该订单，否则触发惩罚函数改变订单自身状态
    std::function<void(Order&)> applyDebuffFunc;
};

class OrderManager {
private:
    std::vector<Order> currentOrders; //当前板上挂着的订单
    int playerMoney;                  //玩家当前资金
    int turnCounter;                  //全局回合计数器

public:
    //构造函数
    OrderManager(int startingMoney = 5);
    ~OrderManager() = default;

    //每回合开始重新生成 3~5 个全新的订单
    void generateTurnOrders();

    //拒绝指定索引的订单
    void rejectOrder(size_t index);

    //正式标记保留指定索引的订单
    void keepOrder(size_t index);

    //结算延时
    void settleEndTurn(const std::vector<std::vector<class Block*>>& grid);

    //提交订单
    bool submitOrder(size_t index, GridManager& gridManager);

    //接口

    //资金
    int getPlayerMoney() const { return playerMoney; }
    void addMoney(int amount) { playerMoney += amount; }

    //检测是否破产
    bool checkBankruptcy() const { return playerMoney <= 0; }

    //获取当前订单列表
    const std::vector<Order>& getCurrentOrders() const { return currentOrders; }

    //获取可修改的订单列表用于刷新回合时重置状态
    std::vector<Order>& getMutableOrders() { return currentOrders; }
};
