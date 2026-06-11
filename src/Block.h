#pragma once
#include "Common.h"

class Block {
private:
    BlockType type;          //方块类型
    Environment envStatus;   //当前环境状态
    int timer;               //通用回合计数器
    int fireTimer;           //燃烧回合计数器
    bool isSelected;         //标记当前方块是否被选中
public:
    //构造函数
    Block(BlockType t);

    //虚析构函数
    virtual ~Block() = default;

    //类型读写
    BlockType getType() const;
    void setType(BlockType t);

    //特殊环境状态读写
    Environment getEnvStatus() const;
    void setEnvStatus(Environment env);

    //通用计数器读写和操作
    int getTimer() const;
    void setTimer(int value);
    void resetTimer();       //通用计数器归零
    void incrementTimer();   //通用计数器加 1

    //燃烧计数器读写
    int getFireTimer() const { return fireTimer; }
    void setFireTimer(int time) { fireTimer = time; }

    //选中状态读写
    bool getSelected() const { return isSelected; }
    void setSelected(bool sel) { isSelected = sel; }
};
