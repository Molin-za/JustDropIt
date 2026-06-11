#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <queue>
#include <set>

const int GRID_ROWS = 20;     //仓库行数
const int GRID_COLS = 10;     //仓库列数
const int CELL_SIZE = 60;     //每个格子的像素大小

//17种方块类型
enum class BlockType {
    None = 0,         //空格
    SkyBlue = 1,      //天蓝色
    Silver = 2,       //银色
    Gray = 3,         //灰色
    Black = 4,        //黑色
    White = 5,        //白色
    Brown = 6,        //棕色
    Blue = 7,         //蓝色
    Yellow = 8,       //黄色
    LightYellow = 9,  //浅黄色
    Transparent = 10, //透明
    Gold = 11,        //金色
    Red = 12,         //红色
    Green = 13,       //绿色
    Orange = 14,      //橙色
    Cyan = 15,        //青色
    Jade = 16,        //翡翠
    Steam = 17        //蒸汽
};

//特殊环境状态
enum class Environment {
    None,       //无
    Fire,       //燃烧状态
};

//游戏状态枚举
enum class GameState {
    MainMenu,   //主菜单
    Playing,    //游戏中
    Paused,     //暂停
    GameOver    //游戏结束
};
