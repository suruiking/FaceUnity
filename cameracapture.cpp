#include "cameracapture.h"
#include<QDebug>

CameraCapture::CameraCapture(QObject *parent)
    : QObject{parent}
{
    //创建摄像头对象
    m_camera = new QCamera(this);

    //创建视频接收器
    m_videoSink = new QVideoSink(this);

    //创建捕获会话（连接摄像头和接收器）
    m_captureSession = new QMediaCaptureSession(this);
    m_captureSession->setCamera(m_camera);        // 设置摄像头
    m_captureSession->setVideoSink(m_videoSink);  // 设置接收器

    // 连接信号槽：当有新视频帧时，调用 onVideoFrameChanged
    // 工作原理：
    //   - 摄像头捕获新帧 → QVideoSink 发出 videoFrameChanged 信号
    //   - 信号触发 → onVideoFrameChanged 槽函数被调用
    //   - 槽函数处理 → 转换格式并存储到 m_pixelBuffer
    connect(m_videoSink, &QVideoSink::videoFrameChanged,
            this, &CameraCapture::onVideoFrameChanged);
}

// ========== 析构函数 ==========
CameraCapture::~CameraCapture()
{
    stop();  // 停止摄像头
}

//开启摄像头
bool CameraCapture::start()
{
    // 检查摄像头是否可用
    if (!m_camera) {
        qDebug() << "Camera not available";
        return false;
    }

    // 启动摄像头
    // 启动后，摄像头开始捕获视频帧，并通过 QVideoSink 发送信号
    m_camera->start();
      qDebug() << "Camera started";
    return true;
}

// 停止摄像头
void CameraCapture::stop()
{
    if (m_camera) {
        m_camera->stop();  // 停止捕获
        qDebug() << "Camera stopped";
    }
}

// 获取视频尺寸
void CameraCapture::getSize(int& w, int& h)
{
    // 使用 QMutexLocker 自动加锁和解锁
    // 原因：m_width 和 m_height 可能在 onVideoFrameChanged 中被修改
    // 作用：构造时自动加锁，析构时自动解锁（离开作用域时）
    QMutexLocker locker(&m_mutex);
    w = m_width;
    h = m_height;
}

// 拿最新一帧的数据,渲染线程
void CameraCapture::getLatestFrame(uint8_t *outBuf, int bufSize)
{
    // 使用 QMutexLocker 保护共享数据
    QMutexLocker locker(&m_mutex);//加锁

    // 检查缓冲区大小是否匹配
    // QByteArray::size() 返回字节数
    if (m_pixelBuffer.size() == bufSize) {
        // 把摄像头的像素数据从一个缓冲区复制到另一个缓冲区
        // 注意：QByteArray::data() 返回 char*，需要转换成 uint8_t*
       // memcpy()内存复制函数，outbuf目标缓冲区，data是源数据数据地址，size是字节数
        memcpy(outBuf, reinterpret_cast<const uint8_t*>(m_pixelBuffer.data()), bufSize);
    }
}//解锁

// ========== 处理新的视频帧（核心函数）==========
void CameraCapture::onVideoFrameChanged(const QVideoFrame &frame)
{
    // ========== 步骤1：检查视频帧是否有效 ==========
    if (!frame.isValid()) return;

    // ========== 步骤2：复制视频帧 ==========
    // 原因：frame 是引用，可能在其他线程被释放，需要复制一份
    QVideoFrame clonedFrame = frame;

    // ========== 步骤3：映射视频帧到内存 ==========
    // map() 的作用：让我们可以访问视频帧的像素数据
    // 用map映射视频帧才能访问到
    if (!clonedFrame.map(QVideoFrame::ReadOnly)) {
         qDebug() << "Failed to map video frame";
        return;
    }

    // ========== 步骤4：转换为 QImage ==========
    // toImage() 的作用：把视频帧转换成 Qt 图像对象
    // 好处：可以方便地进行格式转换和像素访问
    QImage img = clonedFrame.toImage();

    // 解除映射（释放视频帧的内存锁定）
    clonedFrame.unmap();

    // 检查转换是否成功
    if (img.isNull()) {
        qDebug() << "Failed to convert frame to image";
        return;
    }

    // ========== 步骤5：转换为 RGBA 格式 ==========
    // 原因：OpenGL 需要 RGBA 格式的纹理数据
    // Format_RGBA8888：每个像素 4 字节（R、G、B、A 各 1 字节）
    img = img.convertToFormat(QImage::Format_RGBA8888);

    // ========== 步骤6：更新缓冲区 ==========
    {
        // 使用 QMutexLocker 保护共享数据
        // 原因：m_pixelBuffer 可能在 getLatestFrame 中被读取
        QMutexLocker locker(&m_mutex);

        // 更新视频尺寸
        m_width = img.width();
        m_height = img.height();

        // 计算缓冲区大小
        // RGBA = 4 字节/像素
        int bufSize = m_width * m_height * 4;

        // 调整 QByteArray 大小
        m_pixelBuffer.resize(bufSize);

        // 复制像素数据到 QByteArray
       //从 img.bits() 这个地址开始，向后数 bufSize 这么多字节，全部搬运到 m_pixelBuffer 里去。
        memcpy(m_pixelBuffer.data(), img.bits(), bufSize);//拷贝到cpu中
    }
    // ========== 完成 ==========
    // 现在 m_pixelBuffer 中存储了最新一帧的 RGBA 数据
    // VideoRenderWidget 可以通过 getLatestFrame() 获取这些数据
}


