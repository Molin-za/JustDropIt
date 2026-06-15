# 🎮 Just Drop It!

> **仓库经营 × 连锁反应 × 回合策略** — 一款基于 C++20 + raylib 的桌面策略拼图游戏

你经营着一间神秘仓库。每回合购买方块形状放入货架，满足顾客千奇百怪的订单来赚取金币——但小心：方块之间会发生爆炸、火焰蔓延和重力坍塌，一切可能在瞬间失控！

---

## 🎯 游戏玩法

```
┌──────────────────────────────────────────────┐
│  Gold: 20          Turn: 5    Just Drop It!   │  ← 状态栏
├───────────────┬──────────────┬───────────────┤
│               │              │               │
│   📦 订单面板  │   🧱 仓库    │               │
│   (6 个活跃   │   20×10 网格 │               │
│    订单)      │              │               │
│               │              │               │
│               │              │               │
├───────────────┴──────────────┤               │
│   🛒 商店面板（3 个方块组合）  │               │
├──────────────────────────────┤               │
│     ⏳ END TURN（反应引擎运行）│               │
└──────────────────────────────┴───────────────┘
```

1. **主菜单** → 开始游戏，随机放置 3~5 个初始方块
2. **每回合**：商店刷新 → 购买方块摆放到仓库 → 选中连通方块提交订单 → 结束回合
3. **结束回合**：扣除 2 金币 → 运行反应引擎（爆炸/火焰/重力） → 检查破产或爆仓
4. **破产（金币 ≤ 0）或爆仓（方块堆到顶）** → Game Over

---

## 🧱 17 种方块类型

| 方块 | 颜色 | 特性 |
|---|---|---|
| 🔵 **SkyBlue** 天蓝 | `#66BFFF` | 爆炸物：接触火焰引发 3×3 爆炸，连锁引爆 Silver |
| ⚪ **Silver** 银 | `#C0C0C0` | 爆炸物：接触火焰引发 3×3 爆炸；接触白色/顶层 → 变 Gray |
| 🔘 **Gray** 灰 | `#808080` | 接触火焰 → 变为 Silver |
| ⬛ **Black** 黑 | `#282828` | 接触白色 → 自燃并吞噬相邻白色方块 |
| ⬜ **White** 白 | `#F0F0F0` | 燃烧第 2 回合 → 变 Steam；被 Black 吞噬；+Yellow → LightYellow |
| 🟤 **Brown** 棕 | `#8B5A2B` | 接触 SkyBlue → 变为 Blue |
| 🔷 **Blue** 蓝 | `#0079F1` | 1 回合后消失；接触销毁 Silver/Black/Yellow |
| 🟡 **Yellow** 黄 | `#FDF900` | 接触 White → 变 LightYellow；燃烧第 2 回合 → LightYellow |
| 🟨 **LightYellow** 浅黄 | `#FFFFB4` | 不接触 Yellow → 变回 White；火焰自熄 |
| 🫧 **Transparent** 透明 | `#C8DCFF` | 向上浮动穿透方块 |
| 🥇 **Gold** 金 | `#FFCB00` | 贵重，高回报订单专用；不被同化 |
| 🔴 **Red** 红 | `#E62937` | 被移除时引燃相邻 Red（连锁）；火焰持续 2 回合 |
| 🟢 **Green** 绿 | `#00E430` | 沉底，与下方方块交换位置 |
| 🟠 **Orange** 橙 | `#FFA100` | 每回合随机将相邻非金方块同化为 Orange |
| 🩵 **Cyan** 青 | `#00E6E6` | 接触火焰 → 灭火并变 Steam |
| 💚 **Jade** 翡翠 | `#00A86B` | 贵重，计时器 = 额外奖金；火焰 -2 计时器 |
| 💨 **Steam** 蒸汽 | `#C8DCEB` | 1 回合后消失；熄灭周围 3×3 火焰 |

---

## 💥 反应引擎（每回合结束时运行）

反应引擎按以下顺序执行四个阶段：

### 第一阶段：系统衰减
- **Blue / Steam**：计时器 +1，到 1 时消失
- **Jade**：计时器 +1（计数值 = 额外奖金）
- **Silver**：接触白色或在最顶层时计时器 +1，到 2 时变为 Gray
- **Black**：接触白色时计时器 +1，到 1 时吞噬相邻白色并自燃

### 第二阶段：空间扰动
- **Transparent（透明）**：向上浮动，穿透上方方块
- **Green（绿色）**：向下沉底，与下方方块交换

### 第三阶段：接触反应
- **Brown + SkyBlue** → Brown 变 Blue
- **Blue** 接触 → 销毁相邻的 Silver/Black/Yellow
- **Yellow + White** → White 变 LightYellow
- **LightYellow** 不接触 Yellow → 变回 White
- **Orange** → 随机同化一个相邻非金方块
- **Cyan** 接触火焰 → 灭火并变 Steam

### 第四阶段：爆炸与火焰
- **SkyBlue/Silver** 自身或相邻着火 → 触发 3×3 爆炸
- **Gray** 自身或相邻着火 → 变 Silver
- 爆炸摧毁 3×3 范围方块，**SkyBlue/Silver 触发连锁爆炸**
- **火焰传播**：BFS 引燃相邻 Red 方块
- **火焰衰减**（第 2 回合）：
  - White → Steam
  - Yellow → LightYellow
  - Brown/Blue/Transparent/Orange/Green → 消失
  - Red/Black/LightYellow → 火焰熄灭
- **Steam** 熄灭周围 3×3 所有火焰

---

## 📦 26 种订单类型

订单系统是游戏核心驱动力，每回合随机生成 3~5 个新订单。你可以右键订单「保留」到下回合。

| # | 订单名 | 要求 | 基础奖励 | 机制 |
|---|---|---|---|---|
| 1 | 💣 爆炸迷 | 3+ SkyBlue/Silver | 4G | 额外 Gray/Silver +1G，Red/White -2G |
| 2 | 💎 珠宝商 | Gold 或 Jade | 0G | Gold +4G, Jade +(6+计时器)G |
| 3 | 🏴 叛逆者 | 1 SkyBlue + 1 Black + 1 White | 8G | Black 必须同时接触两者 |
| 4 | 📦 批发商 | 任意方块 | 变化 | 前 6 个 +1G，超过 -2G/个 |
| 5 | 🔥 火焰领主 | 4+ Red | 11G | 未着火的 Red -2G |
| 6 | 💰 收藏家 | 1+ Gold + 1+ White（无火） | 7G | 额外 Gold/White +2G |
| 7 | 🧪 化学家 | Brown + SkyBlue（相邻） | 5G | 额外配对 +4G |
| 8 | 🔥 纵火狂 | 1+ Red + 至少 1 个着火 | 6G | 额外燃烧方块 +2G |
| 9 | 🧯 消防员 | 1+ Cyan 或 Steam | 4G | 选中的燃烧方块 -3G |
| 10 | 🌿 环保者 | 3+ 连通 Green | 5G | 选中的 Red -2G |
| 11 | ⛏️ 矿工 | 2+ Gray | 4G | 额外 Gray +3G |
| 12 | 👻 幽灵 | 2+ Transparent | 5G | 无惩罚 |
| 13 | ✂️ 极简主义 | 恰好 2 个连通块 | 3G | 多了少了都不行 |
| 14 | 🌈 彩虹 | 不同颜色的连通块 | 2~20G | 6 色 = 20G！ |
| 15 | ☯️ 阴阳 | 黑白数量相等 | 7G | 每对 +3G，差异 -2G |
| 16 | 🧹 清道夫 | 横跨 4+ 列 | 5G | 额外列 +2G |
| 17 | 🌻 园丁 | LightYellow（无 Yellow 相邻） | 5G | 有 Yellow 邻居的 -2G |
| 18 | ⏳ 计时员 | Blue 或 Steam | 6G | 1 回合限时 |
| 19 | 🏗️ 建筑师 | 精确 2×2 正方形 | 10G | 1 回合限时 |
| 20 | 🎰 赌徒 | 任意连通块 | 随机 | 50% +12G / 50% -5G |
| 21 | 💀 末日使者 | 1+ SkyBlue 或 Silver | 15G | 1 回合限时，高风险 |
| 22 | 💯 完美主义 | 全部同类型 | 3~15+G | 4+ 块时+(n-3)×4G |
| 23 | 🔫 清算者 | 5+ 方块 | 3G | 超 10 个 +3G/个 |
| 24 | ⛓️ 链式大师 | 接触反应配对 | 8G | 额外配对 +4G |
| 25 | 🏕️ 幸存者 | 提交后无火残留 | 6G | 有火 -5G |
| 26 | 🍝 千层面 | 3+ 不同行 | 5G | 额外行 +2G |

---

## 🛒 商店系统

每回合生成 **3 个方块组合**，每个组合包含 2~4 个方块的固定形状：
- **2 块**：横条 / 竖条
- **3 块**：L 形 / 拐角 / 横竖条
- **4 块**：田字、T 形、Z 形、3+1 等各种形状

定价按稀有度 1~4G/块 + 数量加成。

随着回合推进，解锁新的方块类型：

| 回合 | 解锁 Tier | 新方块 |
|---|---|---|
| 0 | Basic | SkyBlue, Black, White |
| ≥ 3 | Mid | Silver, Gray, Brown, Blue, Yellow, LightYellow |
| ≥ 5 | High | Transparent, Red, Green, Orange, Cyan |
| ≥ 8 | All | Gold, Jade |

---

## 🎮 操作说明

| 操作 | 按键 |
|---|---|
| 选择/取消方块 | **左键** 点击仓库中方块 |
| 购买商店方块 | **左键** 点击商店物品 → 移动鼠标到仓库选择列 → **左键** 确认 |
| 取消购买 | **右键** 或 **Esc** |
| 提交订单 | **左键** 点击订单（需先选中正确的连通方块） |
| 保留/取消保留订单 | **右键** 点击订单 |
| 结束回合 | 点击左下角 **END TURN** 按钮 |
| 悬停查看 | 鼠标悬停在方块/订单上查看详情 |

**提交规则**：选中的方块必须**全部连通**（4 方向），否则拒绝提取。

---

## 🔨 构建与运行

### 前置要求
- **CMake** ≥ 3.15
- **C++20** 编译器（GCC 11+ / Clang 14+ / MSVC 2022+）
- **raylib** 系统依赖（如 Ubuntu 上 `apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev`）
- 字体文件：将字体放到 `assets/font.ttf`（可选，自动回退到 raylib 默认字体）

### 构建步骤

```bash
git clone https://github.com/Molin-za/JustDropIt.git
cd JustDropIt
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./JustDropIt
```

CMake 会自动通过 `FetchContent` 下载 raylib 5.5，无需手动安装。

---

## 🏗️ 项目结构

```
JustDropIt/
├── assets/
│   └── font.ttf              # 可选字体文件
├── src/
│   ├── main.cpp              # 主循环、状态机
│   ├── Common.h              # 常量、枚举定义
│   ├── Block.h / Block.cpp   # 方块类
│   ├── GridManager.h/.cpp    # 仓库网格、反应引擎、重力
│   ├── OrderManager.h/.cpp   # 订单生成、验证、结算
│   ├── ShopManager.h/.cpp    # 商店生成、购买逻辑
│   └── MiniFb.h / MiniFb.cpp # 图形渲染（raylib 封装）
├── CMakeLists.txt            # CMake 构建配置
└── LICENSE                   # MIT 许可证
```

---

## 📄 许可证

MIT License — 详见 [LICENSE](LICENSE)

---

*「你可以精打细算，但连锁反应永远出乎意料。」*
