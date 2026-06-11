#pragma once
#include "Block.h"

class GridManager {
private:
    //仓库网格
    Block* grid[GRID_ROWS][GRID_COLS];

    //爆仓状态标志
    bool isGridFull;

    bool isTopLayer(int x, int y) const; //判断是否是最顶层

public:
    //初始化
    GridManager();//创建空仓库
    ~GridManager();//关闭游戏时释放内存
    void initGrid();//仓库全部设为None

    //基础操作
    bool placeBlock(int x, int y, BlockType type);//在仓库指定坐标(x, y)放置一个指定类型的新方块
    void toggleSelectBlock(int x, int y);//在仓库指定坐标(x, y)选择方块
    std::vector<BlockType> extractSelectedBlocks();//取出选中方块

    //爆仓检测
    bool getIsGridFull() const;//返回爆仓状态
    void updateGridFullStatus();//更新是否爆仓

    //反应系统
    void runReactionEngine();//执行反应顺序
    void handleGravity();//执行重力下落
    void checkAndIgniteRedNeighbors(int x, int y);//红色引燃检测函数
    void executeExplosion(int startX, int startY);//爆炸处理函数
    void handleFireSpreadAndDecay();//火焰传播和状态衰减函数

    //四阶段反应
    void phase1_ExplosionsAndFires();//爆炸与火焰
    void phase2_ContactReactions();//接触反应
    void phase3_SpaceDisturbance();//空间扰动
    void phase4_SystemDecay();//系统衰减

    Block* getBlock(int x, int y) const;//获取指定坐标的方块指针
    void debugPrint() const;//临时字符串输出

    //提供给外部只读获取仓库指针数组的接口
    const Block* const (*getGrid() const)[GRID_COLS];
    BlockType getBlockTypeAt(int x, int y) const { return (x >= 0 && x < GRID_COLS && y >= 0 && y < GRID_ROWS && grid[y][x]) ? grid[y][x]->getType() : BlockType::None; }

    //爆炸特效系统
    struct ExplosionEvent { double time; int x; int y; };
    void addExplosionEvent(int x, int y);
    const std::vector<ExplosionEvent>& getExplosionEvents() const { return m_explosions; }
    void cleanupExplosionEvents(double now);
private:
    std::vector<ExplosionEvent> m_explosions;
};
