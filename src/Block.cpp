#include "Block.h"

//构造函数
Block::Block(BlockType t) {
    type = t;
    envStatus = Environment::None; //默认没有特殊状态
    timer = 0;                     //通用计数器初始化
    fireTimer = 0;                 //燃烧计数器初始化
    isSelected = false;            //默认不选中
}

//获取当前方块类型
BlockType Block::getType() const{
    return type;
}

//修改当前方块类型
void Block::setType(BlockType t){
    type = t;
}

//获取当前特殊环境状态
Environment Block::getEnvStatus() const {
    return envStatus;
}

//修改特殊环境状态
void Block::setEnvStatus(Environment env) {
    envStatus = env;
}

//读取通用计时器
int Block::getTimer() const {
    return timer;
}

//直接设置通用计时器值
void Block::setTimer(int value) {
    timer = value;
}

//通用计时器归零
void Block::resetTimer() {
    timer = 0;
}

//通用计时器加1
void Block::incrementTimer() {
    timer++;
}
