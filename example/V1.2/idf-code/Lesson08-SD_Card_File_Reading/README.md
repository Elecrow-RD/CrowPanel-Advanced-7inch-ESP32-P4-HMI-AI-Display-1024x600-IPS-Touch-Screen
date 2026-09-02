# ESP32-P4 SD卡测试项目

这是一个精简的ESP32-P4项目，专门用于测试SD卡功能。

## 项目结构

```
Lesson08-SD/
├── main/
│   ├── main.c              # 主程序文件
│   ├── include/
│   │   └── main.h          # 主程序头文件
│   └── CMakeLists.txt      # 主程序构建配置
├── peripheral/
│   └── bsp_sd/             # SD卡BSP组件
│       ├── bsp_sd.c        # SD卡实现
│       ├── include/
│       │   └── bsp_sd.h    # SD卡头文件
│       └── CMakeLists.txt  # SD卡组件构建配置
├── CMakeLists.txt          # 项目构建配置
└── README.md               # 项目说明
```

## 功能特性

- **SD卡初始化**: 自动检测和挂载SD卡
- **文件操作**: 支持创建、读取、写入文件
- **SD卡信息**: 显示SD卡详细信息
- **错误处理**: 完善的错误处理机制

## 硬件连接

SD卡连接到ESP32-P4的以下GPIO引脚：
- CLK: GPIO_NUM_43
- CMD: GPIO_NUM_44  
- D0: GPIO_NUM_39

## 使用方法

1. 确保ESP-IDF环境已正确安装和配置
2. 将SD卡插入开发板
3. 编译并烧录程序：
   ```bash
   idf.py build
   idf.py flash monitor
   ```

## 程序功能

程序启动后会：
1. 初始化SD卡
2. 显示SD卡信息
3. 创建一个名为"hello.txt"的文件
4. 向文件写入"Hello World from SD Card!"
5. 读取并显示文件内容
6. 完成测试后退出

## 主要函数

### SD卡BSP组件 (bsp_sd.h)

- `sd_init()`: 初始化SD卡
- `get_sd_card_info()`: 获取SD卡信息
- `write_string_file()`: 写入字符串到文件
- `read_string_file()`: 从文件读取字符串
- `create_file()`: 创建文件
- `write_file()`: 写入二进制数据
- `read_file()`: 读取二进制数据
- `format_sd_card()`: 格式化SD卡

## 注意事项

- 确保SD卡格式为FAT32
- 如果挂载失败，程序会自动格式化SD卡
- 程序使用1线SDIO模式
- 支持最大5个同时打开的文件

## 精简说明

本项目已从原始的多组件项目中精简而来，移除了：
- 所有无关的BSP组件（音频、摄像头、显示、USB等）
- 所有宏定义条件编译
- 复杂的任务管理系统
- 不必要的依赖项

现在项目只专注于SD卡功能，代码简洁易懂。