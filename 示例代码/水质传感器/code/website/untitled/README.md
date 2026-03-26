# 水上福寿螺监测与清除系统

## 项目概述
这是一个基于Vue.js和Flask的水上福寿螺监测与清除系统，集成了地图显示、异常警报、水质监测等功能模块，旨在提供直观的福寿螺分布监测和清除作业管理解决方案。

## 功能特点
- **地图可视化**：集成高德地图API，展示福寿螺分布位置和密度热力图
- **船行轨迹**：实时显示和回放清除作业船的航行轨迹
- **异常警报**：监测福寿螺聚集区域并发出警报
- **水质监测**：显示监测区域的水质参数
- **数据统计**：提供福寿螺数量变化趋势图表

## 技术栈

### 前端
- Vue.js 3：用于构建用户界面的渐进式框架
- TypeScript：提供类型安全的JavaScript开发体验
- Vite：快速的前端构建工具
- Element Plus：UI组件库
- ECharts：数据可视化图表库
- Axios：HTTP客户端
- 高德地图API：地图显示与交互

### 后端
- Flask：轻量级Python Web框架
- SQLAlchemy：ORM数据库工具
- MySQL：关系型数据库
- Flask-CORS：处理跨域请求

## 项目结构
```
untitled/
├── src/
│   ├── components/        # Vue组件
│   │   ├── MapDisplay.vue     # 地图显示组件
│   │   ├── AnomalyAlerts.vue  # 异常警报组件
│   │   ├── Dashboard.vue      # 仪表盘组件
│   │   └── ...
│   ├── api/               # API调用
│   ├── router/            # 路由配置
│   └── main.ts            # 入口文件
├── FlaskProject/          # 后端项目
│   ├── app.py             # Flask应用入口
│   ├── models/            # 数据库模型
│   └── routes/            # API路由
├── index.html             # HTML入口
└── package.json           # 项目依赖
```

## 安装与启动

### 前端
```bash
# 安装依赖
npm install

# 开发模式运行
npm run dev

# 构建生产版本
npm run build

# 预览生产版本
npm run preview
```

### 后端
```bash
# 进入后端目录
cd FlaskProject

# 安装依赖
pip install flask flask-sqlalchemy flask-cors pymysql

# 运行Flask应用
python app.py
```

## 配置说明
1. 高德地图API密钥：在`MapDisplay.vue`中配置
2. 数据库连接：在Flask项目的`app.py`中配置数据库连接字符串

## 功能模块详解

### 地图显示模块
- 显示福寿螺位置标记
- 切换显示福寿螺热力图
- 显示和回放船行轨迹
- 点击标记查看福寿螺图片

### 异常警报模块
- 显示福寿螺数量异常区域
- 按时间顺序展示警报信息
- 提供警报位置的经纬度信息

### 水质监测模块
- 实时显示水质参数
- 提供水质变化趋势图表

## 使用指南
1. 启动前端和后端服务
2. 在浏览器中访问前端页面
3. 使用地图控件切换不同图层显示
4. 查看异常警报和水质监测数据
5. 分析福寿螺分布趋势