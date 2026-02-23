#ifndef VIDEORENDERWIDGET_H
#define VIDEORENDERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include <QImage>
#include <QDebug>
#include <QTimer>
#include <QVector>
#include "CameraCapture.h"
#include "Filters.h"  // 新增：包含滤镜类


class VideoRenderWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
     Q_OBJECT
public:
    explicit VideoRenderWidget(QWidget* parent = nullptr) : QOpenGLWidget(parent) {}

    ~VideoRenderWidget(){
        makeCurrent();  // 切换到 OpenGL 上下文
        // 清理资源
        // 删除所有滤镜
        for (auto filter : m_allFilters) {
            delete filter;
        }
        m_allFilters.clear();
        if (m_cameraTexture) delete m_cameraTexture;//删除纹理

        // 清理FBO
        if (m_fboA) delete m_fboA;
        if (m_fboB) delete m_fboB;
        if (m_finalPass) delete m_finalPass;
        doneCurrent();//释放当前opengl上下文
    }

    //实时视频展示，设置定时器，每16毫秒启动一次。
    void setCamera(CameraCapture* camera) {
        m_camera = camera;
        if (m_camera) {
            m_refreshTimer.start(16);  // 60fps
        }
    }

    // 新增：滤镜开关功能，添加到激活列表
    void toggleFilter(const QString& name, bool enable) {
        //没有滤镜就返回
        if (!m_allFilters.contains(name)) {
            qDebug() << "Filter not found:" << name;
            return;
        }
        //提取滤镜对象
        AbstractFilter* filter = m_allFilters[name];
        
        if (enable) {
            // 开启滤镜：添加到激活列表（如果不存在）
            if (!m_activeFilters.contains(filter)) {
                m_activeFilters.append(filter);
                qDebug() << "Filter enabled:" << name;
            }
        } else {
            // 关闭滤镜：从激活列表移除
            m_activeFilters.removeAll(filter);
            qDebug() << "Filter disabled:" << name;
        }
        
        update();  // 触发重绘
    }

    // 新增：更新滤镜参数
    // filterName: 滤镜名称（如 "Blur"），paramName: 参数名称（如 "intensity"），value: 参数值（如 1.5）
    //拖动滑块时调用
    void updateFilterParam(const QString& filterName, const QString& paramName, float value){
        //检查滤镜是否存在
        if (!m_allFilters.contains(filterName)) {
            qDebug() << "Filter not found:" << filterName;
            return;
        }
        
        // 获取滤镜对象
        AbstractFilter* filter = m_allFilters[filterName];
        
        // 设置参数
        filter->setParameter(paramName, value);
        
        qDebug() << "Filter param updated:" << filterName << paramName << "=" << value;
        
        // 触发重绘
        update();
    }

    // 切换LUT预设风格
    void setLutPreset(int presetIndex) {
         // ========== 步骤1：检查LUT滤镜是否存在 ==========
        if (!m_allFilters.contains("Lut")) {
            qDebug() << "Lut filter not found!";
            return;
        }
        
        // 把 AbstractFilter* 转换成 LutFilter*
        auto lutFilter = dynamic_cast<LutFilter*>(m_allFilters["Lut"]);
        if (lutFilter) {
            // 切换到OpenGL上下文
            makeCurrent();
            //生成新纹理
            lutFilter->switchPreset(presetIndex);
            doneCurrent();
            
            qDebug() << "LUT preset changed to:" << presetIndex;
            
            // 触发重绘
            update();
        }
    }


protected:
    // OpenGL渲染系统初始化
    void initializeGL() override {
        initializeOpenGLFunctions();  //初始化函数
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);  // 设置背景色为深蓝色

        // ========== 创建用户可选滤镜 ==========
        m_allFilters["Passthrough"] = new PassthroughFilter();
        m_allFilters["Grayscale"] = new GrayscaleFilter();
        m_allFilters["Invert"] = new InvertFilter();
        m_allFilters["Blur"] = new BlurFilter();
        m_allFilters["EdgeDetect"] = new EdgeDetectFilter();
        m_allFilters["Sharpen"] = new SharpenFilter();
        m_allFilters["Beauty"] = new BeautySmoothFilter();  // 新增：美颜滤镜
        m_allFilters["Lut"] = new LutFilter();              //lut滤镜
        m_allFilters["Watermark"] = new WatermarkFilter();  // 新增：水印滤镜

        // 初始化滤镜库所有滤镜
        for (auto filter : m_allFilters) {
            filter->init();
        }

        qDebug() << "All filters created:" << m_allFilters.size();

        // ========== 创建上屏的无滤镜 ==========
        m_finalPass = new PassthroughFilter();
        m_finalPass->init();
        qDebug() << "FinalPass filter created";


        // 定时器结束一次就渲染一次
        connect(&m_refreshTimer, &QTimer::timeout, this, [this](){
            this->update();  // 触发 paintGL
        });
    }



    // 窗口大小改变，创建窗口调用
    void resizeGL(int w, int h) override {
        //渲染区域是从(0,0)到(w,h)"
        glViewport(0, 0, w, h);
        // ========== 创建/重建 FBO ==========
        // 1. 删除旧的 FBO（如果存在）
        if (m_fboA) delete m_fboA;
        if (m_fboB) delete m_fboB;

        // 2. 创建新的 FBO（尺寸和窗口一致）
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::NoAttachment);  // 不需要深度/模板缓冲
        format.setInternalTextureFormat(GL_RGBA);                      // RGBA 格式

        m_fboA = new QOpenGLFramebufferObject(w, h, format);
        m_fboB = new QOpenGLFramebufferObject(w, h, format);

        qDebug() << "FBO created: " << w << "x" << h;
    }

    // 渲染
    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT);

           //========== CPU 工作1：从共享缓冲区读取数据 ==========
        if (m_camera) {
            updateCameraTexture();
        }
        // 如果没有摄像头纹理或 FBO，清屏后返回
        if (!m_cameraTexture || !m_fboA) {
            glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            return;
        }
        
        // ========== 步骤2：应用激活的滤镜（乒乓缓冲）==========
        // ========== CPU 工作2：控制流程 ==========
        //从刚刚上传到gpu的纹理数据中拿到id
        GLuint currentInput = m_cameraTexture->textureId();
        
        // 标记当前输入是否是摄像头原始纹理
        // 摄像头纹理需要Y轴翻转，FBO纹理不需要
        bool isRawCameraTexture = true;
        

        // // ========== CPU 工作3：调度滤镜 ==========
        // 如果有激活的滤镜，使用乒乓缓冲处理
        if (!m_activeFilters.isEmpty()) {
            QOpenGLFramebufferObject* currFBO = m_fboA;
            QOpenGLFramebufferObject* nextFBO = m_fboB;
            
            // 循环处理每个激活的滤镜
            for (int i = 0; i < m_activeFilters.size(); ++i) {
                AbstractFilter* filter = m_activeFilters[i];
                
                // 绑定当前 FBO
                currFBO->bind();
                //设定大小
                glViewport(0, 0, currFBO->width(), currFBO->height());
                glClear(GL_COLOR_BUFFER_BIT);
                
                // 应用滤镜时传递yFlip参数
                // 第一个滤镜处理摄像头输入，需要翻转
                // 后续滤镜处理FBO输出，不需要翻转
                filter->process(
                    currentInput,
                    currFBO->width(),
                    currFBO->height(),
                    isRawCameraTexture     // yFlip：第一个滤镜true，后续false
                );
                
                currFBO->release();
                
                // 更新输入为当前 FBO 的纹理id
                currentInput = currFBO->texture();
                
                // 【关键】第一个滤镜处理后，后续输入都是FBO纹理，不需要翻转
                isRawCameraTexture = false;
                
                // 交换 FBO（乒乓）
                std::swap(currFBO, nextFBO);
            }
        }

        // ========== 步骤3：把 FBO 内容渲染到屏幕 ==========
        // 绑定默认帧缓冲（屏幕）
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
        glViewport(0, 0, width(), height());  // 设置视口为窗口尺寸
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // 黑色背景
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用 finalPass 把结果画到屏幕
        if (m_finalPass) {
            m_finalPass->process(
                currentInput,           // 输入是最后处理的纹理
                width(),
                height(),
                isRawCameraTexture      // yFlip：如果是摄像头原始纹理需要翻转
            );
        }

    }

  private:
    //更新摄像头纹理
    void updateCameraTexture() {
        if (!m_camera) return;

        // 1. 获取摄像头视频尺寸
        int camW, camH;
        m_camera->getSize(camW, camH);
        if (camW <= 0 || camH <= 0) return;

        // 2. 如果纹理尺寸变了，重新创建纹理
        if (!m_cameraTexture || m_cameraTexture->width() != camW || m_cameraTexture->height() != camH) {
            if (m_cameraTexture) delete m_cameraTexture;

            //创建纹理对象
           m_cameraTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
            //设置尺寸
           m_cameraTexture->setSize(camW, camH);
           //设置格式
           m_cameraTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
           //分配显存
           m_cameraTexture->allocateStorage();
           //过滤方式
           m_cameraTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
           //边缘处理
           m_cameraTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
        }

        // 3. 获取最新一帧数据
        static QVector<uint8_t> tempBuffer;

        int bufSize = camW * camH * 4;
        if (tempBuffer.size() != bufSize) tempBuffer.resize(bufSize);
        //最新一帧的数据
        m_camera->getLatestFrame(tempBuffer.data(), bufSize);

        // 4. 上传到摄像头数据到gpu，里面还包含id（格式，数据类型，视频帧的纹理数据）
       m_cameraTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, tempBuffer.data());
    }


private:
    QOpenGLTexture* m_cameraTexture = nullptr;    // 纹理对象
    CameraCapture* m_camera = nullptr;
    QTimer m_refreshTimer;

    // ========== 多滤镜系统 ==========
    QMap<QString, AbstractFilter*>                m_allFilters;      // 所有滤镜（名字 → 滤镜对象）
    QVector<AbstractFilter*> m_activeFilters;         // 当前激活的滤镜列表

    // ========== FBO 相关 ==========
    QOpenGLFramebufferObject* m_fboA = nullptr;  // FBO A（乒乓缓冲）
    QOpenGLFramebufferObject* m_fboB = nullptr;  // FBO B（乒乓缓冲）
    PassthroughFilter* m_finalPass = nullptr;    // 直通滤镜，原图

};


#endif // VIDEORENDERWIDGET_H
