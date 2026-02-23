#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include <QObject>
#include <QCamera>                  // Qt 摄像头类
#include <QMediaCaptureSession>     // 媒体捕获会话（连接摄像头和接收器）
#include <QVideoSink>               // 视频接收器（接收摄像头的每一帧）
#include <QVideoFrame>              // 视频帧（单帧图像数据）
#include <QImage>                   // Qt 图像类（用于格式转换）
#include <QMutex>                      // 线程锁（保护共享数据）
#include <QMutexLocker>             //自动锁
#include <QByteArray>                   // 字节数组



class CameraCapture : public QObject
{
    Q_OBJECT
public:
    explicit CameraCapture(QObject *parent = nullptr);
      ~CameraCapture();
    // ========== 公共接口 ==========

    // 启动摄像头
    // 返回值：true = 成功，false = 失败
    bool start();

    // 停止摄像头
    void stop();

    // 获取视频尺寸
    void getSize(int& w, int& h);

    // 获取最新一帧数据（RGBA 格式）
    // 参数：outBuf = 输出缓冲区，bufSize = 缓冲区大小
    // 注意：bufSize 必须等于 width * height * 4
    void getLatestFrame(uint8_t* outBuf, int bufSize);

private slots:
    // ========== 私有槽函数 ==========

    // 处理新的视频帧（当摄像头有新帧时被调用）
    // 参数：frame = 视频帧对象
    // 注意：这个函数在摄像头线程中被调用，需要线程安全
    void onVideoFrameChanged(const QVideoFrame& frame);

private:






private:

    // ========== Qt 多媒体对象 ==========

    QCamera* m_camera = nullptr;                    // 摄像头对象
    QMediaCaptureSession* m_captureSession = nullptr;  // 捕获会话（连接摄像头和接收器）
    QVideoSink* m_videoSink = nullptr;              // 视频接收器（接收视频帧）

    // ========== 数据存储 ==========

    QMutex m_mutex;              // Qt 线程锁（保护共享数据）
    QByteArray m_pixelBuffer;    // Qt 字节数组（存储 RGBA 像素数据）
    int m_width = 0;                     // 视频宽度
    int m_height = 0;                    // 视频高度


};

#endif // CAMERACAPTURE_H
