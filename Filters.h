#ifndef FILTERS_H
#define FILTERS_H

#include "FilterContext.h"
#include <QOpenGLTexture>  // 新增：用于纹理对象
#include <QImage>          // 新增：用于水印纹理生成
#include <QPainter>        // 新增：用于绘制水印文字
#include <QFont>           // 新增：用于设置字体

// ========== 1. 直通滤镜（无效果）==========
// 作用：直接输出原图，不做任何处理
// 用途：作为默认滤镜，或者关闭滤镜效果
class PassthroughFilter : public AbstractFilter {
protected:
    bool initShaders() override {
        m_program = new QOpenGLShaderProgram();

        // 顶点着色器：处理顶点位置和纹理坐标
        // aPos：顶点位置（-1到1的NDC坐标）
        // aTexCoord：纹理坐标（0到1）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                "#version 330 core\n"
                "layout(location = 0) in vec2 aPos;\n"
                "layout(location = 1) in vec2 aTexCoord;\n"
                "out vec2 TexCoord;\n"
                "void main() {\n"
                "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                "    TexCoord = aTexCoord;\n"
                "}");

        // 片段着色器：直接输出原图颜色
        // texture()：从纹理中采样颜色
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                "#version 330 core\n"
                "in vec2 TexCoord;\n"
                "uniform sampler2D inputTexture;\n"
                "out vec4 FragColor;\n"
                "void main() {\n"
                "    FragColor = texture(inputTexture, TexCoord);\n"
                "}");
        //编译着色器
        return m_program->link();
    }
};

// ========== 2. 灰度滤镜 ==========
// 作用：将彩色图像转换为灰度图像
// 原理：使用加权平均公式 Gray = 0.299*R + 0.587*G + 0.114*B
//      （这个公式考虑了人眼对不同颜色的敏感度）
class GrayscaleFilter : public AbstractFilter {
protected:
    bool initShaders() override {
        m_program = new QOpenGLShaderProgram();

        // 顶点着色器（与直通滤镜相同）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：灰度转换
        // 使用加权平均公式计算灰度值
        // 然后将 R、G、B 都设置为这个灰度值
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;\n"
                                           "out vec4 FragColor;\n"
                                           "void main() {\n"
                                           //从纹理单元0采样
           "    vec4 color = texture(inputTexture, TexCoord);\n"
                                           //计算灰度
            "    float gray = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;\n"
                                           //输出
            "    FragColor = vec4(gray, gray, gray, color.a);\n"
                                           "}");

        return m_program->link();
    }
};

// ========== 3. 反色滤镜 ==========
// 作用：颜色反转（类似底片效果）
// 原理：每个颜色分量用 1.0 减去原值
//      例如：红色(1,0,0) → 青色(0,1,1)
class InvertFilter : public AbstractFilter {
protected:
    bool initShaders() override {
        m_program = new QOpenGLShaderProgram();

        // 顶点着色器（与直通滤镜相同）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：颜色反转
        // 1.0 - color.rgb：对每个颜色分量取反
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;\n"
                                           "out vec4 FragColor;\n"
                                           "void main() {\n"
                                           "    vec4 color = texture(inputTexture, TexCoord);\n"
                                           "    FragColor = vec4(1.0 - color.rgb, color.a);\n"
                                           "}");

        return m_program->link();
    }
};

// ========== 4. 模糊滤镜 ==========
// 作用：图像模糊效果
// 原理：3×3 盒式模糊（Box Blur）
//      对当前像素周围 9 个像素取平均值
//      这是一种简单的卷积操作
class BlurFilter : public AbstractFilter {
public:
    BlurFilter() {
        m_params["intensity"] = 1.0f;
    }
protected:
    bool initShaders() override {
        m_program = new QOpenGLShaderProgram();

        // 顶点着色器（与直通滤镜相同）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：3×3 模糊
        // resolution：纹理分辨率，用于计算偏移量
        // intensity：模糊强度参数
        // offset：一个像素的大小 × 强度
        // 对周围 9 个像素采样并求平均
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;\n"
                                           "uniform vec2 resolution;\n"
                                           "uniform float intensity;\n"  // 新增：强度参数
                                           "out vec4 FragColor;\n"
                                           "void main() {\n"
                                           "    vec2 offset = (1.0 / resolution) * intensity;\n"  // 强度影响偏移量
                                           "    vec4 sum = vec4(0.0);\n"
                                           "    for (int x = -1; x <= 1; x++) {\n"
                                           "        for (int y = -1; y <= 1; y++) {\n"
                                           "            sum += texture(inputTexture, TexCoord + vec2(x, y) * offset);\n"
                                           "        }\n"
                                           "    }\n"
                                           "    FragColor = sum / 9.0;\n"
                                           "}");

        return m_program->link();
    }
    void onSetUniforms() override {
        //着色器设置intensity的变量的值，模糊强度参数
        //用户拖动滑块到 15，intensity = 1.5f
        m_program->setUniformValue("intensity", m_params["intensity"].toFloat());
    }
};

// ========== 5. 边缘检测滤镜 ==========
// 作用：检测图像中的边缘
// 原理：Sobel 算子
//      使用两个 3×3 卷积核分别检测水平和垂直方向的边缘
//      然后计算梯度强度 sqrt(Gx² + Gy²)
class EdgeDetectFilter : public AbstractFilter {
protected:
    bool initShaders() override {
        m_program = new QOpenGLShaderProgram();

        // 顶点着色器（与直通滤镜相同）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：Sobel 边缘检测
        // 1. 先转换为灰度图
        // 2. 使用 Sobel 算子计算水平和垂直梯度
        // 3. 计算梯度强度
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;\n"
                                           "uniform vec2 resolution;\n"
                                           "out vec4 FragColor;\n"
                                           "void main() {\n"
                                           "    vec2 offset = 1.0 / resolution;\n"
                                           "    float Gx = 0.0, Gy = 0.0;\n"
                                           "    for (int x = -1; x <= 1; x++) {\n"
                                           "        for (int y = -1; y <= 1; y++) {\n"
                                           "            vec4 color = texture(inputTexture, TexCoord + vec2(x, y) * offset);\n"
                                           "            float gray = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;\n"
                                           "            Gx += gray * float(x);\n"
                                           "            Gy += gray * float(y);\n"
                                           "        }\n"
                                           "    }\n"
                                           "    float edge = sqrt(Gx * Gx + Gy * Gy);\n"
                                           "    FragColor = vec4(edge, edge, edge, 1.0);\n"
                                           "}");

        return m_program->link();
    }
};

// ========== 6. 锐化滤镜 ==========
// 作用：增强图像细节，使图像更清晰
// 原理：使用锐化卷积核
//      中心权重为 5，周围权重为 -1
//      这会增强边缘和细节
class SharpenFilter : public AbstractFilter {
protected:
    bool initShaders() override {
        m_program = new QOpenGLShaderProgram();

        // 顶点着色器（与直通滤镜相同）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：锐化卷积
        // 卷积核：
        //  0  -1   0
        // -1   5  -1
        //  0  -1   0
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;\n"
                                           "uniform vec2 resolution;\n"
                                           "out vec4 FragColor;\n"
                                           "void main() {\n"
                                           "    vec2 offset = 1.0 / resolution;\n"
                                           "    vec4 center = texture(inputTexture, TexCoord);\n"
                                           "    vec4 top = texture(inputTexture, TexCoord + vec2(0, offset.y));\n"
                                           "    vec4 bottom = texture(inputTexture, TexCoord - vec2(0, offset.y));\n"
                                           "    vec4 left = texture(inputTexture, TexCoord - vec2(offset.x, 0));\n"
                                           "    vec4 right = texture(inputTexture, TexCoord + vec2(offset.x, 0));\n"
                                           "    FragColor = 5.0 * center - top - bottom - left - right;\n"
                                           "}");

        //编译链接，生成可以执行的着色器
        return m_program->link();
    }
};

// ========== 7. 美颜滤镜（磨皮）==========
// 作用：智能磨皮美白，只对皮肤区域生效
// 原理：双边滤波（Bilateral Filter）+ 皮肤检测
// 核心技术：
//   1. 皮肤检测：通过RGB阈值判断是否是皮肤
//   2. 双边滤波：空间权重 + 颜色权重，保留边缘的同时平滑皮肤
//   3. 自适应强度：皮肤区域强，非皮肤区域弱
class BeautySmoothFilter : public AbstractFilter {
public:
    BeautySmoothFilter() {
        m_params["intensity"] = 0.5f;  // 美颜强度（0.0-1.0）
    }

protected:
    bool initShaders() override {
        m_program = new QOpenGLShaderProgram();

        // 顶点着色器（标准）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：双边滤波 + 皮肤检测
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;\n"
                                           "uniform vec2 resolution;\n"
                                           "uniform float intensity;  // 美颜强度\n"
                                           "out vec4 FragColor;\n"
                                           "\n"
                                           "// ========== 皮肤检测算法 ==========\n"
                                           "// 原理：人类皮肤在RGB空间有特定的分布规律\n"
                                           "// 参考论文：Skin Detection using RGB color space\n"
                                           "bool isSkin(vec3 color) {\n"
                                           "    float r = color.r;\n"
                                           "    float g = color.g;\n"
                                           "    float b = color.b;\n"
                                           "    \n"
                                           "    // 皮肤检测条件（经验阈值）：\n"
                                           "    // 1. R > 0.37：红色分量要足够高\n"
                                           "    // 2. G > 0.21：绿色分量要足够高\n"
                                           "    // 3. B > 0.14：蓝色分量要足够高\n"
                                           "    // 4. R > G && R > B：红色是主导色\n"
                                           "    // 5. max(R,G,B) - min(R,G,B) > 0.058：有一定的色彩饱和度\n"
                                           "    return (r > 0.37 && g > 0.21 && b > 0.14 && \n"
                                           "            r > g && r > b && \n"
                                           "            (max(max(r, g), b) - min(min(r, g), b)) > 0.058);\n"
                                           "}\n"
                                           "\n"
                                           "void main() {\n"
                                           "    // 1. 采样中心像素\n"
                                           "    vec4 centerColor = texture(inputTexture, TexCoord);\n"
                                           "    \n"
                                           "    // 2. 如果强度为0，直接返回原图\n"
                                           "    if (intensity <= 0.01) {\n"
                                           "        FragColor = centerColor;\n"
                                           "        return;\n"
                                           "    }\n"
                                           "    \n"
                                           "    // 3. 检测当前像素是否是皮肤\n"
                                           "    float skinFactor = isSkin(centerColor.rgb) ? 1.0 : 0.0;\n"
                                           "    \n"
                                           "    // 4. 非皮肤区域也稍微处理一下（避免边界过于明显）\n"
                                           "    if (skinFactor < 0.5) skinFactor = 0.1;\n"
                                           "    \n"
                                           "    // 5. 计算模糊半径（根据强度动态调整）\n"
                                           "    float blurRadius = 3.0 + (intensity * 5.0);  // 3.0-8.0像素\n"
                                           "    \n"
                                           "    // 6. 计算纹理采样步长\n"
                                           "    vec2 texelSize = 1.0 / resolution;\n"
                                           "    \n"
                                           "    // ========== 双边滤波核心算法 ==========\n"
                                           "    // 原理：结合空间距离和颜色相似度的加权平均\n"
                                           "    // 空间权重：距离越远权重越小（高斯分布）\n"
                                           "    // 颜色权重：颜色差异越大权重越小（保留边缘）\n"
                                           "    \n"
                                           "    vec3 sumColor = vec3(0.0);  // 累加颜色\n"
                                           "    float sumWeight = 0.0;      // 累加权重\n"
                                           "    \n"
                                           "    // 7. 9×9采样（-4到+4）\n"
                                           "    for(int x = -4; x <= 4; x++) {\n"
                                           "        for(int y = -4; y <= 4; y++) {\n"
                                           "            // 7.1 计算采样偏移\n"
                                           "            vec2 offset = vec2(float(x), float(y)) * blurRadius * 0.5;\n"
                                           "            vec2 uv = TexCoord + offset * texelSize;\n"
                                           "            \n"
                                           "            // 7.2 采样邻域像素\n"
                                           "            vec3 sampleColor = texture(inputTexture, uv).rgb;\n"
                                           "            \n"
                                           "            // 7.3 计算空间权重（高斯分布）\n"
                                           "            // 公式：exp(-距离平方 / (2 * sigma^2))\n"
                                           "            float distSq = float(x*x + y*y);  // 距离平方\n"
                                           "            float spatialWeight = exp(-distSq / (2.0 * 4.0));\n"
                                           "            \n"
                                           "            // 7.4 计算颜色权重（颜色相似度）\n"
                                           "            // 公式：exp(-颜色差异平方 / (2 * sigma^2))\n"
                                           "            vec3 diff = sampleColor - centerColor.rgb;\n"
                                           "            float colorDistSq = dot(diff, diff);  // 颜色差异平方\n"
                                           "            float rangeSigma = 0.05 + (intensity * 0.2);  // 动态调整\n"
                                           "            float colorWeight = exp(-colorDistSq / (2.0 * rangeSigma * rangeSigma));\n"
                                           "            \n"
                                           "            // 7.5 计算总权重（空间权重 × 颜色权重）\n"
                                           "            float totalWeight = spatialWeight * colorWeight;\n"
                                           "            \n"
                                           "            // 7.6 累加加权颜色\n"
                                           "            sumColor += sampleColor * totalWeight;\n"
                                           "            sumWeight += totalWeight;\n"
                                           "        }\n"
                                           "    }\n"
                                           "    \n"
                                           "    // 8. 计算平均颜色（双边滤波结果）\n"
                                           "    vec3 smoothResult = sumColor / sumWeight;\n"
                                           "    \n"
                                           "    // 9. 根据皮肤因子和强度混合原图和磨皮结果\n"
                                           "    // skinFactor=1.0（皮肤）：完全应用磨皮\n"
                                           "    // skinFactor=0.1（非皮肤）：轻微应用磨皮\n"
                                           "    float finalMix = clamp(intensity * skinFactor, 0.0, 1.0);\n"
                                           "    FragColor = vec4(mix(centerColor.rgb, smoothResult, finalMix), centerColor.a);\n"
                                           "}");

        return m_program->link();
    }

    void onSetUniforms() override {
        //设置美颜强度参数
        m_program->setUniformValue("intensity", m_params["intensity"].toFloat());
    }
};

// ========== 8. LUT滤镜（色彩风格）==========
// 作用：通过3D LUT实现复杂的色彩风格转换
// 原理：使用512×512的LUT纹理作为"颜色翻译字典"
//      输入RGB → 查LUT表 → 输出新RGB
// 优势：支持任意复杂的色彩映射，性能高（GPU硬件加速）
class LutFilter : public AbstractFilter {
public:
    // 预设风格枚举
    enum Preset {
        Origin = 0,     // 原图（不做任何处理）
        Cool,           // 清凉（提亮、加蓝、减红）
        Pink,           // 粉嫩（加红紫、减绿、柔和对比度）
        Vintage,        // 怀旧（棕褐色调）
        BlackWhite,     // 黑白（高对比度灰度）
        Cyberpunk       // 赛博朋克（暗部偏青、亮部偏洋红）
    };

    LutFilter() {
        m_params["intensity"] = 1.0f;  // 滤镜强度（0.0-1.0）
        // 默认生成原图LUT（不做任何处理）
        switchPreset(Origin);
    }

    ~LutFilter() {
        if (m_lutTexture) delete m_lutTexture;
    }

    // 切换预设风格
    void switchPreset(int presetIndex) {
        // 如果已经是当前预设，且纹理已存在，直接返回
        if (m_currentPreset == presetIndex && m_lutTexture) return;
        
        m_currentPreset = presetIndex;
        generateLut(static_cast<Preset>(presetIndex));
    }

protected:
    // ========== 核心算法：生成512×512的LUT纹理 ==========
    // 这是CPU端的算法，运行一次后纹理就生成好了
    // GPU端只需要查表，不需要重复计算
    void generateLut(Preset preset) {
        // 1. 删除旧纹理
        if (m_lutTexture) {
            delete m_lutTexture;
            m_lutTexture = nullptr;
        }

        // 2. 创建512×512的图像
        QImage img(512, 512, QImage::Format_RGB32);

        // 3. 填充LUT数据
        // 外层循环：遍历512×512的每个像素
        for (int y = 0; y < 512; ++y) {
            for (int x = 0; x < 512; ++x) {
                // ========== 步骤A：计算当前像素对应的原始RGB值 ==========
                // 3D LUT的存储方式：把64×64×64的立方体切成64个64×64的切片
                // 平铺成8×8的网格（8×64=512）
                
                // 计算当前像素属于哪个切片（B通道）
                int blueBlockX = x / 64;             // 0-7（横向第几个切片）
                int blueBlockY = y / 64;             // 0-7（纵向第几个切片）
                int blueIdx = blueBlockY * 8 + blueBlockX; // 0-63（B通道索引）

                // 计算切片内的位置（R和G通道）
                int inBlockX = x % 64; // 0-63（切片内横坐标，对应R通道）
                int inBlockY = y % 64; // 0-63（切片内纵坐标，对应G通道）

                // 归一化到0.0-1.0
                float r = inBlockX / 63.0f;  // R通道
                float g = inBlockY / 63.0f;  // G通道
                float b = blueIdx / 63.0f;   // B通道

                // ========== 步骤B：根据预设应用色彩变换算法 ==========
                float nr = r, ng = g, nb = b;  // 新的RGB值

                switch (preset) {
                case Origin:
                    // 原图：不做任何改变
                    // nr = r, ng = g, nb = b（已经赋值了）
                    break;

                case Cool: // 清凉风格
                    // 算法：提亮、加蓝、减红
                    nr = r * 0.9f;    // 减少红色
                    ng = g * 1.05f;   // 稍微增加绿色
                    nb = b * 1.2f;    // 增加蓝色
                    // Gamma校正：提亮整体画面
                    nr = pow(nr, 0.9f);
                    ng = pow(ng, 0.9f);
                    nb = pow(nb, 0.9f);
                    break;

                case Pink: // 粉嫩风格
                    // 算法：加红、加蓝（形成紫色调）、减绿
                    nr = r * 1.1f;    // 增加红色
                    ng = g * 0.95f;   // 减少绿色
                    nb = b * 1.05f;   // 稍微增加蓝色
                    // 柔和对比度：让画面更柔和
                    nr = 0.5f + (nr - 0.5f) * 0.9f;
                    ng = 0.5f + (ng - 0.5f) * 0.9f;
                    nb = 0.5f + (nb - 0.5f) * 0.9f;
                    break;

                case Vintage: // 怀旧风格
                    // 算法：经典的棕褐色（Sepia）变换矩阵
                    // 这是电影行业的标准算法
                    nr = r * 0.393f + g * 0.769f + b * 0.189f;
                    ng = r * 0.349f + g * 0.686f + b * 0.168f;
                    nb = r * 0.272f + g * 0.534f + b * 0.131f;
                    break;

                case BlackWhite: // 黑白风格
                {
                    // 算法：加权灰度转换 + S型曲线增加对比度
                    // 加权公式考虑了人眼对不同颜色的敏感度
                    float gray = r * 0.299f + g * 0.587f + b * 0.114f;
                    
                    // S型曲线：让暗部更暗，亮部更亮（增加对比度）
                    gray = (gray > 0.5f) ? (1.0f - 2.0f * (1.0f - gray) * (1.0f - gray))
                                         : (2.0f * gray * gray);
                    
                    nr = ng = nb = gray;  // 三个通道都用同一个灰度值
                }
                break;

                case Cyberpunk: // 赛博朋克风格
                    // 算法：暗部偏青色、亮部偏洋红色
                    nr = r * 1.2f;  // 整体偏红
                    ng = g * 0.8f;  // 减绿
                    nb = b * 1.3f;  // 强蓝
                    
                    // 暗部加蓝（形成青色调）
                    if (r + g + b < 1.0f) {
                        nb += 0.2f;
                    }
                    break;
                }

                // ========== 步骤C：钳制到合法范围 ==========
                // 防止颜色值超出0.0-1.0范围
                nr = qBound(0.0f, nr, 1.0f);
                ng = qBound(0.0f, ng, 1.0f);
                nb = qBound(0.0f, nb, 1.0f);

                // ========== 步骤D：写入图像 ==========
                img.setPixelColor(x, y, QColor::fromRgbF(nr, ng, nb));
            }
        }

        // 4. 创建OpenGL纹理
        m_lutTexture = new QOpenGLTexture(img);
        m_lutTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_lutTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    }

    bool initShaders() override {
        // 如果纹理未生成，先生成默认LUT
        if (!m_lutTexture) generateLut(Origin);

        m_program = new QOpenGLShaderProgram();
        
        // 顶点着色器（标准）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：3D LUT查找算法
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;  // 输入图像\n"
                                           "uniform sampler2D lutTexture;    // LUT纹理\n"
                                           "uniform float intensity;         // 滤镜强度\n"
                                           "out vec4 FragColor;\n"
                                           "\n"
                                           "// ========== 核心算法：3D LUT查找 ==========\n"
                                           "vec4 lookup(vec4 textureColor) {\n"
                                           "    // 1. 计算B通道对应的切片索引\n"
                                           "    //    B值范围0.0-1.0，映射到0-63\n"
                                           "    float blueColor = textureColor.b * 63.0;\n"
                                           "    \n"
                                           "    // 2. 计算两个相邻切片的位置（用于插值）\n"
                                           "    //    floor(blueColor)：下界切片\n"
                                           "    //    ceil(blueColor)：上界切片\n"
                                           "    vec2 quad1;  // 下界切片的位置\n"
                                           "    quad1.y = floor(floor(blueColor) / 8.0);  // 纵向第几行\n"
                                           "    quad1.x = floor(blueColor) - (quad1.y * 8.0);  // 横向第几列\n"
                                           "    \n"
                                           "    vec2 quad2;  // 上界切片的位置\n"
                                           "    quad2.y = floor(ceil(blueColor) / 8.0);\n"
                                           "    quad2.x = ceil(blueColor) - (quad2.y * 8.0);\n"
                                           "    \n"
                                           "    // 3. 计算切片内的纹理坐标\n"
                                           "    //    每个切片占512/8=64像素\n"
                                           "    //    0.125 = 1/8（每个切片占总纹理的1/8）\n"
                                           "    vec2 texPos1;\n"
                                           "    texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);\n"
                                           "    texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);\n"
                                           "    \n"
                                           "    vec2 texPos2;\n"
                                           "    texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);\n"
                                           "    texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);\n"
                                           "    \n"
                                           "    // 4. 从LUT纹理中采样两个颜色\n"
                                           "    vec4 newColor1 = texture(lutTexture, texPos1);\n"
                                           "    vec4 newColor2 = texture(lutTexture, texPos2);\n"
                                           "    \n"
                                           "    // 5. 在两个颜色之间插值（三线性插值）\n"
                                           "    //    fract(blueColor)：B值的小数部分，用作插值权重\n"
                                           "    vec4 newColor = mix(newColor1, newColor2, fract(blueColor));\n"
                                           "    return newColor;\n"
                                           "}\n"
                                           "\n"
                                           "void main() {\n"
                                           "    // 1. 采样输入图像\n"
                                           "    vec4 px = texture(inputTexture, TexCoord);\n"
                                           "    \n"
                                           "    // 2. 查LUT表，获取新颜色\n"
                                           "    vec4 lutColor = lookup(px);\n"
                                           "    \n"
                                           "    // 3. 根据强度混合原图和LUT颜色\n"
                                           "    //    intensity=0.0：完全是原图\n"
                                           "    //    intensity=1.0：完全是LUT效果\n"
                                           "    FragColor = mix(px, vec4(lutColor.rgb, px.a), intensity);\n"
                                           "}");

        return m_program->link();
    }

    //每帧都要调用，绑定纹理到纹理单元
    void onSetUniforms() override {
        // 绑定LUT纹理到纹理单元1
        if (m_lutTexture) {
            glActiveTexture(GL_TEXTURE0 + 1);  // 切换到纹理单元1
            m_lutTexture->bind();//绑定lUT纹理
            m_program->setUniformValue("lutTexture", 1);  // 告诉着色器用单元1（LUT纹理）
        }
        
        // 传递强度参数
        m_program->setUniformValue("intensity", m_params["intensity"].toFloat());
    }

private:
    QOpenGLTexture* m_lutTexture = nullptr;  // LUT纹理对象
    int m_currentPreset = -1;                // 当前预设索引
};

// ========== 9. 水印叠加滤镜 ==========
// 作用：在视频上叠加水印（文字/图片）
// 原理：多纹理叠加 + Alpha混合
// 核心技术：
//   1. QPainter动态生成水印纹理
//   2. 多纹理单元（GL_TEXTURE2）
//   3. 屏幕空间坐标映射
//   4. Alpha通道混合
class WatermarkFilter : public AbstractFilter {
public:
    WatermarkFilter() {
        m_params["scale"] = 0.3f;      // 水印缩放（0.1-1.0）
        m_params["x"] = 0.05f;         // 水印X位置（0.0-1.0，0.0=左，1.0=右）
        m_params["y"] = 0.05f;         // 水印Y位置（0.0-1.0，0.0=上，1.0=下）
        m_params["opacity"] = 0.8f;    // 水印不透明度（0.0-1.0）
    }

    ~WatermarkFilter() {
        if (m_wmTexture) delete m_wmTexture;
    }

    // 创建默认水印（使用QPainter绘制文字）
    void createDefaultWatermark() {
        if (m_wmTexture) return;  // 已经创建过了
        
        // 1. 创建透明背景的图像
        QImage img(300, 128, QImage::Format_ARGB32);
        img.fill(Qt::transparent);  // 填充透明色
        
        // 2. 使用QPainter绘制文字
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);  // 抗锯齿
        
        // 3. 绘制主文字（白色）
        p.setFont(QFont("Arial", 40, QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(img.rect(), Qt::AlignCenter, "新年快乐");
        
        // 4. 绘制阴影（半透明黑色，偏移2像素）
        p.setPen(QColor(0, 0, 0, 128));
        p.drawText(img.rect().translated(2, 2), Qt::AlignCenter, "新年快乐");
        
        // 5. 创建OpenGL纹理
        //这时候就会有新的纹理id
        m_wmTexture = new QOpenGLTexture(img);
        m_wmTexture->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
        m_wmTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    }

protected:
    bool initShaders() override {
        // 初始化时创建水印纹理
        createDefaultWatermark();
        
        m_program = new QOpenGLShaderProgram();
        
        // 顶点着色器（标准）
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                           "#version 330 core\n"
                                           "layout(location = 0) in vec2 aPos;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n"
                                           "out vec2 TexCoord;\n"
                                           "void main() {\n"
                                           "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                                           "    TexCoord = aTexCoord;\n"
                                           "}");

        // 片段着色器：多纹理叠加
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                           "#version 330 core\n"
                                           "in vec2 TexCoord;\n"
                                           "uniform sampler2D inputTexture;  // 输入视频\n"
                                           "uniform sampler2D wmTexture;     // 水印纹理\n"
                                           "uniform vec2 position;           // 水印位置（0.0-1.0）\n"
                                           "uniform float scale;             // 水印缩放\n"
                                           "uniform float opacity;           // 水印不透明度\n"
                                           "uniform float imgAspect;         // 视频宽高比\n"
                                           "uniform float wmAspect;          // 水印宽高比\n"
                                           "uniform bool isInputFlipped;     // 输入是否翻转\n"
                                           "out vec4 FragColor;\n"
                                           "\n"
                                           "void main() {\n"
                                           "    // 1. 采样输入视频\n"
                                           "    vec4 base = texture(inputTexture, TexCoord);\n"
                                           "    \n"
                                           "    // ========== 坐标系归一化处理 ==========\n"
                                           "    // 问题：摄像头输入和FBO输入的坐标系不同\n"
                                           "    // 解决：构造统一的screenUV，(0,0)永远在视觉左上角\n"
                                           "    \n"
                                           "    vec2 screenUV = TexCoord;\n"
                                           "    if (!isInputFlipped) {\n"
                                           "        // FBO输入：OpenGL标准坐标，原点在左下\n"
                                           "        // 需要翻转Y轴，让(0,0)变成左上角\n"
                                           "        screenUV.y = 1.0 - screenUV.y;\n"
                                           "    } else {\n"
                                           "        // 摄像头输入：已经用翻转Quad处理过\n"
                                           "        // (0,0)已经是视觉左上角，无需变动\n"
                                           "    }\n"
                                           "    \n"
                                           "    // ========== 计算水印的UV坐标 ==========\n"
                                           "    // 2. 计算水印的实际缩放（考虑宽高比）\n"
                                           "    float realScaleX = scale;\n"
                                           "    float realScaleY = scale * (imgAspect / wmAspect);\n"
                                           "    \n"
                                           "    // 3. 计算水印的UV坐标\n"
                                           "    // 公式：(当前坐标 - 水印位置) / 水印尺寸\n"
                                           "    vec2 wmUV = (screenUV - position) / vec2(realScaleX, realScaleY);\n"
                                           "    \n"
                                           "    // 4. 判断当前像素是否在水印区域内\n"
                                           "    if (wmUV.x >= 0.0 && wmUV.x <= 1.0 && wmUV.y >= 0.0 && wmUV.y <= 1.0) {\n"
                                           "        // 5. 采样水印纹理（垂直翻转修正）\n"
                                           "        vec2 flippedWmUV = vec2(wmUV.x, 1.0 - wmUV.y);\n"
                                           "        vec4 wmColor = texture(wmTexture, flippedWmUV);\n"
                                           "        \n"
                                           "        // 6. Alpha混合（根据水印的Alpha通道和不透明度参数）\n"
                                           "        // 公式：base * (1 - alpha) + watermark * alpha\n"
                                           "        // 使用mix函数简化：mix(base, watermark, alpha)\n"
                                           "        FragColor = mix(base, vec4(wmColor.rgb, 1.0), wmColor.a * opacity);\n"
                                           "    } else {\n"
                                           "        // 不在水印区域，直接输出原图\n"
                                           "        FragColor = base;\n"
                                           "    }\n"
                                           "}");

        return m_program->link();
    }

    void onSetUniforms() override {
        // 绑定水印纹理到纹理单元2
        if (m_wmTexture) {
            glActiveTexture(GL_TEXTURE0 + 2);  // 切换到纹理单元2
            m_wmTexture->bind();
            m_program->setUniformValue("wmTexture", 2);
            
            // 传递宽高比参数
            m_program->setUniformValue("imgAspect", 16.0f / 9.0f);  // 假设视频是16:9
            m_program->setUniformValue("wmAspect", (float)m_wmTexture->width() / m_wmTexture->height());
        }
        
        // 传递其他参数
        m_program->setUniformValue("scale", m_params["scale"].toFloat());
        m_program->setUniformValue("opacity", m_params["opacity"].toFloat());
        m_program->setUniformValue("position", QVector2D(m_params["x"].toFloat(), m_params["y"].toFloat()));
    }

private:
    QOpenGLTexture* m_wmTexture = nullptr;  // 水印纹理对象
};

#endif // FILTERS_H
