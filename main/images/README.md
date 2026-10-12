把一张图片放在本文件夹，命名为 `test.png`、`test.jpg`、`test.jpeg` 或 `test.bmp`，只保留其中一个。

建议图片分辨率与所选屏幕一致（例如 H070B26 为 1024×600），避免拉伸。
编译会按所选 panel 的分辨率自动转换，尺寸不一致时拉伸到全屏。
首次加入、替换、删除图片或切换 panel 后执行 `idf.py build flash`；只烧录应用 bin 不会更新图片分区。
没有图片时跳过图片画面，红、绿、蓝、白、16 级灰阶各显示 1 秒，持续循环。
灰阶横屏显示竖条（左黑右白），竖屏显示横条（上黑下白），按屏幕宽高自动切换。

打开 `idf.py menuconfig → Display Test Demo → Run native LCD test instead of LVGL` 启用；关闭则运行原有 LVGL 界面。
有图片时编译使用 ESP-IDF Python 环境中的 Pillow：`python -m pip install Pillow`。
图片保存在新增的 4 MiB `lcd_image` 分区，不占用应用分区；工程需要至少 8 MB Flash。
