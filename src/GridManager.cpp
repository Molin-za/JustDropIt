#define NOGDI
#define NOUSER
#include "GridManager.h"
#include "raylib.h"

//创建空仓库
GridManager::GridManager() : isGridFull(false) {
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            grid[y][x] = nullptr;
        }
    }
    initGrid();
}

//关闭游戏时释放内存
GridManager::~GridManager() {
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            if (grid[y][x] != nullptr) {
                delete grid[y][x];
                grid[y][x] = nullptr;
            }
        }
    }
}

//仓库全部设为None
void GridManager::initGrid() {
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            if (grid[y][x] != nullptr) {
                delete grid[y][x];
            }
            grid[y][x] = new Block(BlockType::None);
        }
    }
    isGridFull = false;
}

//在仓库指定坐标(x, y)放置一个指定类型的新方块
bool GridManager::placeBlock(int x, int y, BlockType type) {
    if (x < 0 || x >= GRID_COLS || y < 0 || y >= GRID_ROWS) return false;

    if (grid[y][x] != nullptr) {
        delete grid[y][x];
    }

    grid[y][x] = new Block(type);
    updateGridFullStatus();
    return true;
}

//在仓库指定坐标(x, y)选择方块
void GridManager::toggleSelectBlock(int x, int y) {
    //获取方块
    Block* target = getBlock(x, y);
    if (target == nullptr || target->getType() == BlockType::None) return;
    //如未选中则选中，如已选中则取消
    bool currentStatus = target->getSelected();
    target->setSelected(!currentStatus);
}

//取出选中方块
std::vector<BlockType> GridManager::extractSelectedBlocks() {
    std::vector<BlockType> extractedList; 

    //收集全局所有被选中的方块坐标
    std::vector<std::pair<int, int>> selectedCoords;
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            if (grid[y][x] != nullptr && grid[y][x]->getSelected()) {
                selectedCoords.push_back({ x, y });
            }
        }
    }

    //如果没选中则直接返回空
    if (selectedCoords.empty()) return extractedList;

    //检验选中的方块是否完全连通
    std::set<std::pair<int, int>> visited;
    std::queue<std::pair<int, int>> q;

    //从第一个选中的方块开始作为感染源
    q.push(selectedCoords[0]);
    visited.insert(selectedCoords[0]);

    //使用十字扫描方向
    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        //探测当前方块的邻居
        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            //确认邻居是否在合法范围内
            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                //如果邻居也是被选中的，且之前没访问过
                Block* neighbor = getBlock(nx, ny);
                if (neighbor != nullptr && neighbor->getSelected()) {
                    if (visited.find({ nx, ny }) == visited.end()) {
                        visited.insert({ nx, ny });
                        q.push({ nx, ny }); //加入队列继续感染探测
                    }
                }
            }
        }
    }

    //连通性判断
    if (visited.size() < selectedCoords.size()) {
        //拒绝提取
        return extractedList;
    }

    //正式提取
    for (const auto& [x, y] : selectedCoords) {
        Block* target = grid[y][x];

        //记录类型
        extractedList.push_back(target->getType());

        //删除并原地替换为空方块 None
        delete target;
        grid[y][x] = new Block(BlockType::None);
    }

    //刷新爆仓状态
    updateGridFullStatus();

    return extractedList;
}

//返回爆仓状态
bool GridManager::getIsGridFull() const {
    return isGridFull;
}

//更新是否爆仓
void GridManager::updateGridFullStatus() {
    for (int x = 0; x < GRID_COLS; ++x) {
        if (grid[0][x] != nullptr && grid[0][x]->getType() != BlockType::None) {
            isGridFull = true;
            return;
        }
    }
    isGridFull = false;
}

//判断是否是最顶层
bool GridManager::isTopLayer(int x, int y) const{
    for (int curY = 0; curY < y; ++curY) {
        if (grid[curY][x] != nullptr && grid[curY][x]->getType() != BlockType::None) {
            return false;
        }
    }
    return true;
}

//执行重力下落
void GridManager::handleGravity() {
    bool anyBlockMoved = true;
    while (anyBlockMoved) {
        anyBlockMoved = false;

        //创建一个布尔数组标记可下落（false=不可下落）
        bool canFall[GRID_ROWS][GRID_COLS] = { false };

        //扫描浮空方块
        for (int y = 0; y < GRID_ROWS - 1; ++y) { 
            for (int x = 0; x < GRID_COLS; ++x) {
                Block* current = grid[y][x];

                if (current != nullptr && current->getType() != BlockType::None) {
                    //检查下方是否是空的
                    Block* down = grid[y + 1][x];
                    if (down != nullptr && down->getType() == BlockType::None) {
                        canFall[y][x] = true; //标记记录为可下降
                    }
                }
            }
        }

        //统一下移
        for (int y = GRID_ROWS - 2; y >= 0; --y) {
            for (int x = 0; x < GRID_COLS; ++x) {
                if (canFall[y][x]) {
                    Block* current = grid[y][x];
                    Block* down = grid[y + 1][x];

                    grid[y + 1][x] = current;
                    grid[y][x] = down;

                    anyBlockMoved = true;
                }
            }
        }
    }
    //刷新爆仓状态
    updateGridFullStatus();
}

//红色引燃检测函数
void GridManager::checkAndIgniteRedNeighbors(int startX, int startY) {
    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    //用一个队列记录需要扫描的红色方块
    std::queue<std::pair<int, int>> q;
    q.push({ startX, startY });

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            Block* neighbor = getBlock(nx, ny);
            if (neighbor != nullptr && neighbor->getType() == BlockType::Red && neighbor->getEnvStatus() != Environment::Fire) {
                neighbor->setEnvStatus(Environment::Fire);
                neighbor->setFireTimer(0);
                q.push({ nx, ny });
            }
        }
    }
}

//爆炸处理函数
void GridManager::executeExplosion(int startX, int startY) {
    addExplosionEvent(startX, startY);
    //一个队列收集可引发二次爆炸的天蓝色和银色方块
    std::vector<std::pair<int, int>> chainReactions;

    //扫描3x3范围
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = startX + dx;
            int ny = startY + dy;
            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                Block* target = grid[ny][nx];
                if (target == nullptr || target->getType() == BlockType::None) continue;
                BlockType type = target->getType();
                //天蓝色和银色方块准备再次触发爆炸
                if (type == BlockType::SkyBlue || type == BlockType::Silver) {
                    chainReactions.push_back({ nx, ny });
                    delete target;
                    grid[ny][nx] = new Block(BlockType::None);
                    continue;
                }
                //灰色方块变为银色
                if (type == BlockType::Gray) {
                    target->setType(BlockType::Silver);
                    checkAndIgniteRedNeighbors(nx, ny); //红色引燃
                    continue;
                }

                delete target;
                grid[ny][nx] = new Block(BlockType::None);
                checkAndIgniteRedNeighbors(nx, ny); //红色引燃
            }
        }
    }
    handleGravity();
    //连环引爆天蓝色和银色方块
    for (const auto& [cx, cy] : chainReactions) {
        Block* b = getBlock(cx, cy);
        if (b != nullptr && b->getType() != BlockType::None) {
            executeExplosion(cx, cy);
        }
    }
}

//火焰传播和衰减
void GridManager::handleFireSpreadAndDecay() {
    bool needGravity = false;
    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    //收集所有需要处理的蒸汽方块9x9范围
    std::vector<std::pair<int, int>> steamPositions;

    //遍历所有方块处理燃烧状态和相邻关系
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Block* b = grid[y][x];
            if (b == nullptr || b->getType() == BlockType::None) continue;

            BlockType type = b->getType();
            Environment env = b->getEnvStatus();
            int fireAge = b->getFireTimer();

            //记录蒸汽方块的位置
            if (type == BlockType::Steam) {
                steamPositions.push_back({ x, y });
            }

            //方块不处于燃烧状态则强制将燃烧计数器归零
            if (env != Environment::Fire) { 
                b->setFireTimer(0);
                continue;
            }

            //燃烧第1回合：黑色方块
            if (fireAge == 1 && type == BlockType::Black) {
                if ((rand() % 100) < 40) { 
                    std::vector<std::pair<int, int>> neighbors;
                    for (int i = 0; i < 4; ++i) {
                        int nx = x + dx[i]; int ny = y + dy[i];
                        if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                            if (grid[ny][nx] != nullptr && grid[ny][nx]->getType() != BlockType::None) {
                                neighbors.push_back({ nx, ny });
                            }
                        }
                    }
                    if (!neighbors.empty()) {
                        auto [rx, ry] = neighbors[rand() % neighbors.size()];
                        grid[ry][rx]->setEnvStatus(Environment::Fire); 
                    }
                }
            }

            //燃烧第2回合
            if (fireAge == 2) {
                //黑色、浅黄、红色：去除燃烧状态
                if (type == BlockType::Black || type == BlockType::LightYellow || type == BlockType::Red) {
                    b->setEnvStatus(Environment::None);
                }
                //白色：删除自己并生成蒸汽
                else if (type == BlockType::White) {
                    delete b;
                    grid[y][x] = new Block(BlockType::Steam);
                    grid[y][x]->setTimer(0);
                    checkAndIgniteRedNeighbors(x, y);
                }
                //棕色、蓝色、透明、橙色、绿色：删除自己
                else if (type == BlockType::Brown || type == BlockType::Blue ||
                    type == BlockType::Transparent || type == BlockType::Orange ||
                    type == BlockType::Green) {
                    delete b;
                    grid[y][x] = new Block(BlockType::None);
                    needGravity = true;
                    checkAndIgniteRedNeighbors(x, y);
                }
                //黄色：删除自己并生成浅黄
                else if (type == BlockType::Yellow) {
                    delete b;
                    grid[y][x] = new Block(BlockType::LightYellow);
                    checkAndIgniteRedNeighbors(x, y);
                }
                //翡翠：扣除2点计时器
                else if (type == BlockType::Jade) {
                    if (b->getTimer() >= 2) {
                        b->setTimer(b->getTimer() - 2);
                        b->setEnvStatus(Environment::None);
                    }
                    else {
                        delete b;
                        grid[y][x] = new Block(BlockType::None);
                        needGravity = true;
                        checkAndIgniteRedNeighbors(x, y);
                    }
                }
            }
        }
    }

    //全局所有着火的方块燃烧计数器+1
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            if (grid[y][x] != nullptr && grid[y][x]->getEnvStatus() == Environment::Fire) {
                grid[y][x]->setFireTimer(grid[y][x]->getFireTimer() + 1);
            }
        }
    }

    //蒸汽：熄灭周围9x9范围内的所有火焰
    for (const auto& [sx, sy] : steamPositions) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = sx + dx; int ny = sy + dy;
                if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                    if (grid[ny][nx] != nullptr && grid[ny][nx]->getEnvStatus() == Environment::Fire) {
                        grid[ny][nx]->setEnvStatus(Environment::None);
                        grid[ny][nx]->setFireTimer(0);
                    }
                }
            }
        }
    }
    if (needGravity) {
        handleGravity();
    }
}

//一回合内反应顺序
void GridManager::runReactionEngine() {
    //系统衰减
    phase4_SystemDecay();

    //空间扰动
    phase3_SpaceDisturbance();

    //接触反应
    phase2_ContactReactions();

    //爆炸与火焰
    phase1_ExplosionsAndFires();

    //刷新爆仓状态
    updateGridFullStatus();
}

//系统衰减
void GridManager::phase4_SystemDecay() {
    //遍历整个仓库
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Block* b = grid[y][x];
            //空位直接跳过
            if (b == nullptr || b->getType() == BlockType::None) continue;

            BlockType type = b->getType();

            //蓝色和蒸汽：每回合计时器+1，到1后消失
            if (type == BlockType::Blue || type == BlockType::Steam) {
                //检查当前计时器是否为1
                if (b->getTimer() == 1) {
                    //为1直接消失
                    delete b;
                    grid[y][x] = new Block(BlockType::None);
                    checkAndIgniteRedNeighbors(x, y);//方块变化通知红色引燃
                }
                else {
                    //否则计数器+1
                    b->setTimer(b->getTimer() + 1);
                }
                continue;
            }

            //翡翠：通用计时器加一
            if (type == BlockType::Jade) {
                b->setTimer(b->getTimer() + 1);
                continue;
            }

            //银色和黑色：检查是否接触白色方块，使用十字扫描探测邻居
            bool touchWhite = false;
            int dx[] = { 0, 0, -1, 1 };
            int dy[] = { -1, 1, 0, 0 };

            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                //确保邻居存在
                if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                    if (grid[ny][nx] != nullptr && grid[ny][nx]->getType() == BlockType::White) {
                        touchWhite = true;
                        break; 
                    }
                }
            }

            //银色：接触白色或最顶层时计时器+1，到2后变灰色
            if (type == BlockType::Silver) {
                if (touchWhite || isTopLayer(x, y)) {
                    //检查当前计时器是否为 2
                    if (b->getTimer() == 2) {
                        //变为灰色
                        Environment currentEnv = b->getEnvStatus();
                        delete b;
                        grid[y][x] = new Block(BlockType::Gray);
                        grid[y][x]->setEnvStatus(currentEnv);
                        checkAndIgniteRedNeighbors(x, y);//方块变化通知红色引燃
                    }
                    else {
                        b->setTimer(b->getTimer() + 1);
                    }
                }
                else {
                    //不满足条件，计时器归零
                    b->setTimer(0);
                }
                continue;
            }

            //黑色：接触白色时计时器+1，到1后吃掉周围白色方块并点燃自己
            if (type == BlockType::Black) {
                if (touchWhite) {
                    //检查当前计时器是否为1
                    if (b->getTimer() == 1) {
                        //将周围的白色方块删除
                        for (int i = 0; i < 4; ++i) {
                            int nx = x + dx[i];
                            int ny = y + dy[i];
                            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                                if (grid[ny][nx] != nullptr && grid[ny][nx]->getType() == BlockType::White) {
                                    delete grid[ny][nx];
                                    grid[ny][nx] = new Block(BlockType::None); 
                                    checkAndIgniteRedNeighbors(nx, ny);//方块变化通知红色引燃
                                }
                            }
                        }
                        //为自己施加燃烧状态并重置计时器
                        b->setEnvStatus(Environment::Fire);
                        b->setTimer(0);
                        checkAndIgniteRedNeighbors(x, y);//方块变化通知红色引燃
                    }
                    else {
                        b->setTimer(b->getTimer() + 1);
                    }
                }
                else {
                    //不满足条件，计时器归零
                    b->setTimer(0);
                }
                continue;
            }
            b = nullptr;
        }
    }
    handleGravity();
}

//空间扰动
void GridManager::phase3_SpaceDisturbance() {
    //透明方块向上浮动
    bool transparentProcessed[GRID_ROWS][GRID_COLS] = { false };
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Block* b = grid[y][x];
            if (b == nullptr || b->getType() != BlockType::Transparent || transparentProcessed[y][x]) continue;
            if (!isTopLayer(x, y)) {
                int targetY = y;
                //只有当上方不是空且也不是另一个透明时进行交换
                while (targetY > 0 && grid[targetY - 1][x] != nullptr &&
                    grid[targetY - 1][x]->getType() != BlockType::None &&
                    grid[targetY - 1][x]->getType() != BlockType::Transparent) {

                    Block* temp = grid[targetY][x];
                    grid[targetY][x] = grid[targetY - 1][x];
                    grid[targetY - 1][x] = temp;

                    transparentProcessed[targetY - 1][x] = true; //标记被交换上去的透明方块本回合不再重复处理
                    targetY--;
                }
            }
        }
    }
    handleGravity();

    //绿色方块沉底
    bool greenProcessed[GRID_ROWS][GRID_COLS] = { false };
    for (int y = GRID_ROWS - 1; y >= 0; --y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Block* b = grid[y][x];
            if (b == nullptr || b->getType() != BlockType::Green || greenProcessed[y][x]) continue; 

            //检查下方是否有方块
            if (y + 1 < GRID_ROWS) {
                Block* downBlock = grid[y + 1][x];
                //下方有方块且不是空且下方那个方块本回合没被处理过
                if (downBlock != nullptr && downBlock->getType() != BlockType::None && !greenProcessed[y + 1][x]) {
                    //交换位置
                    grid[y][x] = downBlock;
                    grid[y + 1][x] = b;

                    greenProcessed[y][x] = true;
                    greenProcessed[y + 1][x] = true;
                }
            }
        }
    }
    handleGravity();
}

//接触反应
void GridManager::phase2_ContactReactions() {
    //临时的处理队列，记录哪些橙色方块本回合已经反应过
    bool orangeProcessed[GRID_ROWS][GRID_COLS] = { false };

    //在反应开始前，先记录全局所有方块的类型
    BlockType currentTypes[GRID_ROWS][GRID_COLS];
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            currentTypes[y][x] = grid[y][x] ? grid[y][x]->getType() : BlockType::None;
        }
    }

    //使用十字扫描方向
    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Block* b = grid[y][x];
            if (b == nullptr || b->getType() == BlockType::None) continue;

            //获取方块在本回合开始时的原始类型
            BlockType originalType = currentTypes[y][x];

            //棕色：周围有天蓝色时变蓝色
            if (originalType == BlockType::Brown) {
                bool hasSkyBlue = false;
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i]; int ny = y + dy[i];
                    if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                        //使用原始类型判断
                        if (currentTypes[ny][nx] == BlockType::SkyBlue) {
                            hasSkyBlue = true; break;
                        }
                    }
                }
                if (hasSkyBlue) {
                    b->setType(BlockType::Blue);
                    checkAndIgniteRedNeighbors(x, y);
                }
                continue;
            }

            //蓝色：接触银色/黑色/黄色时将其销毁
            if (originalType == BlockType::Blue) {
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i]; int ny = y + dy[i];
                    if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                        BlockType nt = currentTypes[ny][nx]; //使用原始类型
                        if (nt == BlockType::Silver || nt == BlockType::Black || nt == BlockType::Yellow) {
                            if (grid[ny][nx] != nullptr) {
                                delete grid[ny][nx];
                                grid[ny][nx] = new Block(BlockType::None);
                            }
                        }
                    }
                }
                continue;
            }

            //黄色：接触白色时将其变为浅黄
            if (originalType == BlockType::Yellow) {
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i]; int ny = y + dy[i];
                    if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                        if (grid[ny][nx] != nullptr && currentTypes[ny][nx] == BlockType::White) {
                            grid[ny][nx]->setType(BlockType::LightYellow);
                            checkAndIgniteRedNeighbors(nx, ny);
                        }
                    }
                }
                continue;
            }

            //浅黄：未接触黄色时变回白色
            if (originalType == BlockType::LightYellow) {
                bool touchYellow = false;
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i]; int ny = y + dy[i];
                    if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                        if (currentTypes[ny][nx] == BlockType::Yellow) {
                            touchYellow = true; break;
                        }
                    }
                }
                if (!touchYellow) {
                    b->setType(BlockType::White);
                    checkAndIgniteRedNeighbors(x, y);
                }
                continue;
            }

            //橙色：随机将相邻非金色方块同化为橙色
            if (originalType == BlockType::Orange && !orangeProcessed[y][x]) {
                std::vector<std::pair<int, int>> validNeighbors;
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i]; int ny = y + dy[i];
                    if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                        BlockType nt = currentTypes[ny][nx];
                        if (nt != BlockType::Gold && nt != BlockType::None && nt != BlockType::Orange) {
                            validNeighbors.push_back({ nx, ny });
                        }
                    }
                }
                if (!validNeighbors.empty()) {
                    int randomIndex = rand() % validNeighbors.size();
                    auto [targetX, targetY] = validNeighbors[randomIndex];

                    grid[targetY][targetX]->setType(BlockType::Orange);
                    currentTypes[targetY][targetX] = BlockType::Orange;
                    orangeProcessed[targetY][targetX] = true; //标记被同化的橙色方块本回合不再继续感染
                    checkAndIgniteRedNeighbors(targetX, targetY);
                }
                continue;
            }

            //青色：接触燃烧状态的方块时熄灭火焰并变为蒸汽
            if (originalType == BlockType::Cyan) {
                bool quenchedFire = false;
                for (int i = 0; i < 4; ++i) {
                    int nx = x + dx[i]; int ny = y + dy[i];
                    if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                        Block* neighbor = grid[ny][nx];
                        if (neighbor != nullptr && neighbor->getEnvStatus() == Environment::Fire) {
                            neighbor->setEnvStatus(Environment::None);
                            quenchedFire = true;
                            checkAndIgniteRedNeighbors(nx, ny);
                        }
                    }
                }
                if (quenchedFire) {
                    b->setType(BlockType::Steam);
                    currentTypes[y][x] = BlockType::Steam;
                    b->setTimer(0);
                    checkAndIgniteRedNeighbors(x, y);
                }
                continue;
            }
        }
    }
    handleGravity();
}

//爆炸与火焰
void GridManager::phase1_ExplosionsAndFires() {
    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    std::vector<std::pair<int, int>> bombsToTrigger;

    //判断天蓝色、银色、灰色方块自身或周围是否有火
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Block* b = grid[y][x];
            if (b == nullptr || b->getType() == BlockType::None) continue;

            BlockType type = b->getType();

            if (type == BlockType::SkyBlue || type == BlockType::Silver || type == BlockType::Gray) {
                bool trigger = false;

                //检查自身是否着火
                if (b->getEnvStatus() == Environment::Fire) {
                    trigger = true;
                }
                else {
                    //检查周围四个方向是否有火
                    for (int i = 0; i < 4; ++i) {
                        int nx = x + dx[i]; int ny = y + dy[i];
                        if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                            if (grid[ny][nx] != nullptr && grid[ny][nx]->getEnvStatus() == Environment::Fire) {
                                trigger = true; break;
                            }
                        }
                    }
                }

                if (trigger) {
                    if (type == BlockType::Gray) {
                        //灰色变银色
                        b->setType(BlockType::Silver);
                        checkAndIgniteRedNeighbors(x, y);
                    }
                    else {
                        bombsToTrigger.push_back({ x, y });
                    }
                }
            }
        }
    }

    for (const auto& [bx, by] : bombsToTrigger) {
        if (grid[by][bx] != nullptr && grid[by][bx]->getType() != BlockType::None) {
            executeExplosion(bx, by);
        }
    }
    handleFireSpreadAndDecay();
}

//获取指定坐标的方块指针
Block* GridManager::getBlock(int x, int y) const {
    if (x < 0 || x >= GRID_COLS || y < 0 || y >= GRID_ROWS) return nullptr;
    return grid[y][x];
}

//临时字符串输出
void GridManager::debugPrint() const {
    std::cout << "\n=== [仓库当前状态] ===" << (isGridFull ? " (仓库爆仓!!)" : "") << "\n";
    for (int y = 0; y < GRID_ROWS; ++y) {
        std::cout << (y < 10 ? "0" : "") << y << " | ";
        for (int x = 0; x < GRID_COLS; ++x) {
            // 格子内容
            if (grid[y][x] == nullptr || grid[y][x]->getType() == BlockType::None) {
                std::cout << ". ";
            }
            else {
                std::cout << (int)grid[y][x]->getType()
                    << (grid[y][x]->getEnvStatus() == Environment::Fire ? "*" : " ");
            }
        }
        std::cout << "\n";
    }
    std::cout << "---------------------------\n";
}

//提供给外部只读获取仓库指针数组
const Block* const (*GridManager::getGrid() const)[GRID_COLS] {
    return (const Block* const (*)[GRID_COLS])grid;
}

void GridManager::addExplosionEvent(int x, int y) {
    m_explosions.push_back({ GetTime(), x, y });
}

void GridManager::cleanupExplosionEvents(double now) {
    m_explosions.erase(
        std::remove_if(m_explosions.begin(), m_explosions.end(),
            [now](const ExplosionEvent& e) { return now - e.time > 0.6; }),
        m_explosions.end());
}
