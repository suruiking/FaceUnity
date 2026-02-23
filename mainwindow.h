#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "VideoRenderWidget.h"
#include"cameracapture.h"
#include <QWidget>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QScrollArea>  // 新增：滚动区域
#include <QGroupBox>    // 新增：分组框

class MainWindow : public QWidget
{
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    VideoRenderWidget* m_glWidget;
    CameraCapture* m_camera;

    // 滤镜开关复选框
    QCheckBox* m_chkPassthrough;   // 直通
    QCheckBox* m_chkGrayscale;     // 灰度
    QCheckBox* m_chkInvert;        // 反色
    QCheckBox* m_chkBlur;          // 模糊
    QCheckBox* m_chkEdge;          // 边缘检测
    QCheckBox* m_chkSharpen;       // 锐化
    QCheckBox* m_chkBeauty;        // 美颜（新增）
    QCheckBox* m_chkLut;           // LUT滤镜
    QCheckBox* m_chkWatermark;     // 水印（新增）

    // 滤镜参数控件
    QSlider* m_sliderBlurIntensity;   // 模糊强度滑块
    QLabel* m_labelBlurIntensity;     // 显示当前强度值的标签
    
    // 美颜滤镜控件（新增）
    QSlider* m_sliderBeautyIntensity;
    QLabel* m_labelBeautyIntensity;
    
    // LUT滤镜控件
    QComboBox* m_comboLutPreset;      // LUT风格选择下拉菜单
    QSlider* m_sliderLutIntensity;    // LUT强度滑块
    QLabel* m_labelLutIntensity;      // 显示当前LUT强度值的标签
    
    // 水印滤镜控件（新增）
    QSlider* m_sliderWatermarkOpacity;
    QLabel* m_labelWatermarkOpacity;

};

#endif // MAINWINDOW_H
