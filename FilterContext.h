#ifndef FILTERCONTEXT_H
#define FILTERCONTEXT_H
#include <QOpenGLFunctions_3_3_Core>//opengl封装的函数
#include <QOpenGLShaderProgram>//控制着色器
#include <QMap>
#include <QVariant>
#include <QVector2D>

// ========== 抽象滤镜基类 ==========
// 所有滤镜都继承这个类
// 提供统一的接口和通用功能
   //四个轮子（VAO/VBO 顶点数据）
   // - 发动机接口（process() 方法）
   // - 油箱（参数存储 m_params）

class AbstractFilter : protected QOpenGLFunctions_3_3_Core{
public:
    AbstractFilter() = default;

    virtual ~AbstractFilter() {
        // 清理 OpenGL 资源
        if (m_program) delete m_program;
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        // 清理翻转Quad资源
        if (m_vaoFlip) glDeleteVertexArrays(1, &m_vaoFlip);
        if (m_vboFlip) glDeleteBuffers(1, &m_vboFlip);
    }

    // 初始化滤镜（调用一次）
    virtual bool init() {
        initializeOpenGLFunctions();  // 初始化 OpenGL 函数
        initQuad();                    // 初始化顶点数据
        return initShaders();          // 初始化着色器（子类实现）
    }

    // 执行每帧渲染，cpu绑定纹理到gpu插槽
    // inputTexId：输入纹理 ID
    // width, height：纹理尺寸
    // yFlip：是否使用Y轴翻转的Quad（摄像头输入需要翻转）
    virtual void process(GLuint inputTexId, int width, int height, bool yFlip = false) {
        if (!m_program) return;

        // 1. 启用着色器
        m_program->bind();

        // 2. 绑定输入纹理到纹理单元 0
        //切换到目标单元
        glActiveTexture(GL_TEXTURE0);  //gpu激活纹理单元0
        //绑定纹理到当前单元
        glBindTexture(GL_TEXTURE_2D, inputTexId);
        //告诉着色器用哪个单元
        m_program->setUniformValue("inputTexture", 0);

        // 3. 告诉着色器宽高
        m_program->setUniformValue("resolution", QVector2D(width, height));

        // 4. 子类设置自己的 uniform 参数
        onSetUniforms();//如果LUT纹理就绑定到单元1，使用单元1

        // 5. 根据输入源选择正确的Quad
        // yFlip = true:  使用翻转Quad（摄像头输入，纹理数据是Top-Down）
        // yFlip = false: 使用标准Quad（FBO输入，OpenGL标准坐标）
        if (yFlip) {
            renderQuadFlipped();
        } else {
            renderQuad();
        }

        // 6. 释放着色器
        m_program->release();
    }

    // 设置滤镜参数
    virtual void setParameter(const QString& key, const QVariant& value) {
        m_params[key] = value;
    }

    // 获取滤镜参数
    QVariant getParameter(const QString& key) const {
        return m_params.value(key);
    }

protected:
    // ========== 子类必须实现的纯虚函数 ==========

    // 初始化着色器（每个滤镜有自己的着色器代码）
    virtual bool initShaders() = 0;

    // ========== 子类可选实现的虚函数 ==========

    // 设置 uniform 参数（在 process 中被调用）
    //让子类设置自己特有的着色器参数
    virtual void onSetUniforms() {}

    // ========== 通用辅助函数 ==========

    // 使用已有的 VAO 和着色器绘制画面
    void renderQuad() {
        //绑定vao（激活顶点数据）
        glBindVertexArray(m_vao);
        //绘制四个顶点，绘制模式：三角形条带，从0开始画4个。
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        //解绑vao
        glBindVertexArray(0);
    }

    // 绘制翻转四边形（用于摄像头到FBO）
    void renderQuadFlipped() {
        glBindVertexArray(m_vaoFlip);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    // 上传顶点数据到GPU
    void initQuad() {
        // ========== 第1步：准备标准Quad的顶点数据 ==========
        // 标准Quad：用于 FBO → FBO 或 FBO → 屏幕
        // OpenGL标准纹理坐标：(0,0)在左下角，(1,1)在右上角
        float vertices[] = {
            // 位置          纹理坐标
            -1.0f, -1.0f,   0.0f, 0.0f,  // 左下 → UV(0,0) 对应纹理底部
             1.0f, -1.0f,   1.0f, 0.0f,  // 右下 → UV(1,0)
            -1.0f,  1.0f,   0.0f, 1.0f,  // 左上 → UV(0,1) 对应纹理顶部
             1.0f,  1.0f,   1.0f, 1.0f   // 右上 → UV(1,1)
        };

        // ========== 第2步：创建标准VAO ==========
        glGenVertexArrays(1, &m_vao);//生成一个vao对象
        glBindVertexArray(m_vao); //绑定这个vao

        // ========== 第3步：创建标准VBO并上传数据 ==========
        glGenBuffers(1, &m_vbo); //生成一个vbo
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);//绑定vbo到顶点数据缓冲区
        //上传数据到gpu
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // ========== 第4步：配置标准VAO的属性 ==========
        //配置vao，告诉gpu属性0（位置）怎么读
        //位置，分量，数据类型，不需要归一化，顶点大小，偏移量
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        //启用属性0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // ========== 第5步：准备翻转Quad的顶点数据 ==========
        // 翻转Quad：专用于 摄像头 → FBO
        // 摄像头数据是Top-Down格式：第一行像素在内存开头
        // 需要将纹理坐标Y轴翻转，让(0,1)对应屏幕底部，(0,0)对应屏幕顶部
        float verticesFlip[] = {
            // 位置          纹理坐标（Y轴翻转）
            -1.0f, -1.0f,   0.0f, 1.0f,  // 左下 → UV(0,1) 对应摄像头数据底部
             1.0f, -1.0f,   1.0f, 1.0f,  // 右下 → UV(1,1)
            -1.0f,  1.0f,   0.0f, 0.0f,  // 左上 → UV(0,0) 对应摄像头数据顶部
             1.0f,  1.0f,   1.0f, 0.0f   // 右上 → UV(1,0)
        };

        // ========== 第6步：创建翻转VAO ==========
        glGenVertexArrays(1, &m_vaoFlip);
        glBindVertexArray(m_vaoFlip);

        // ========== 第7步：创建翻转VBO并上传数据 ==========
        glGenBuffers(1, &m_vboFlip);
        glBindBuffer(GL_ARRAY_BUFFER, m_vboFlip);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticesFlip), verticesFlip, GL_STATIC_DRAW);

        // ========== 第8步：配置翻转VAO的属性 ==========
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // ========== 第9步：解绑VAO ==========
        glBindVertexArray(0);
    }

protected:
    // ========== 成员变量 ==========
    QOpenGLShaderProgram* m_program = nullptr;  // 着色器对象
    QMap<QString, QVariant> m_params;           // 参数存储
    
    // 标准Quad（用于FBO到FBO、FBO到屏幕）
    GLuint m_vao = 0;                           // 标准顶点数组对象
    GLuint m_vbo = 0;                           // 标准顶点缓冲对象
    
    // 翻转Quad（用于摄像头到FBO）
    GLuint m_vaoFlip = 0;                       // 翻转顶点数组对象
    GLuint m_vboFlip = 0;                       // 翻转顶点缓冲对象

};


#endif // FILTERCONTEXT_H
