# 项目阶段划分（精简版）

## Phase 0：规范与目标对齐

**目标**
- 明确 GIF 文件中哪些部分与“左右翻转”直接相关
- 明确哪些部分可以暂时忽略

**完成标志**
- 能在纸上画出 GIF Data Stream 的结构
- 明确：真正需要动的是 **Image Data（LZW 解码后的像素索引顺序）**

---

## Phase 1：GIF 结构解析（到 Image Data 为止）

**目标**
- 用顺序读取 + `fread` / pointer 运算，完整解析 GIF 结构
- 不解码，只正确“走完”文件

**内容**
- Header
- Logical Screen Descriptor
- Global Color Table（若存在）
- 各类 Extension（只识别类型并跳过）
- Image Descriptor
- Image Data 的 sub-block 边界

**完成标志**
- 能正确识别每个 Image
- 能准确定位每一帧的 image data 起止位置
- 文件能被完整解析到 Trailer

---

## Phase 2：LZW 解码（独立于图像操作）

**目标**
- 实现 **GIF 语境下的 LZW 解码**
- 得到一维的像素 index 流

**内容**
- LZW minimum code size
- Clear Code / End Code
- 动态 code width 增长
- 字典重建逻辑

**完成标志**
- 对任意 GIF image data：
  - 能输出正确数量的 index
  - index 值始终落在 color table 范围内
- 对同一 GIF，多次解码结果一致

---

## Phase 3：像素矩阵重建

**目标**
- 将一维 index 流映射为二维图像

**内容**
- 宽 × 高 的 row-major 重排
- 处理 interlace（若存在）

**完成标志**
- 像素矩阵维度与 Image Descriptor 一致
- 行列顺序与 GIF 规范一致

---

## Phase 4：左右翻转（核心目标）

**目标**
- 在“像素 index 矩阵”层面完成左右翻转

**内容**
- 每一行执行反转
- 不改动 color table
- 不涉及重新压缩

**完成标志**
- 翻转后矩阵逻辑正确
- 与原图仅左右镜像差异

---

## Phase 5（可选）：重新编码或验证输出

**方向 A：只验证**
- 将翻转结果输出为：
  - PPM
  - 或调试用文本 / 可视化

**方向 B：完整闭环**
- 将翻转后的 index 流重新 LZW 编码
- 写回合法 GIF 文件

**完成标志**
- 图像可被标准 GIF 查看器正确显示
- 翻转效果肉眼可见

---

## Phase 6（可选）：扩展与健壮性

- 多帧 GIF
- 局部 color table
- 不同 code size 的鲁棒性
- 错误 GIF 的容错

