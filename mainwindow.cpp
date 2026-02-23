#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QScrollArea>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    this->setWindowTitle("视频处理学习项目");
    this->resize(1200, 700);  // 增加宽度以适应左右布局

    // ========== 主布局：左右分割 ==========
    auto mainLayout = new QHBoxLayout(this);
    //布局边距为0，紧贴窗口
    mainLayout->setContentsMargins(0, 0, 0, 0);
    //控件之间间距为0
    mainLayout->setSpacing(0);

    // ========== 左侧：OpenGL 渲染区 ==========
    m_glWidget = new VideoRenderWidget(this);
    m_glWidget->setMinimumSize(640, 480);  // 设置最小尺寸
    mainLayout->addWidget(m_glWidget, 1);  // 拉伸因子为1，占据剩余空间

    // ========== 右侧：控制面板（固定宽度300px）==========
    QWidget* controlPanel = new QWidget();
    controlPanel->setStyleSheet("QWidget { background-color: #2b2b2b; }");  // 深色背景
    
    //垂直布局
    auto panelLayout = new QVBoxLayout(controlPanel);
    panelLayout->setContentsMargins(10, 10, 10, 10);
    panelLayout->setSpacing(15);
    
    // 创建滚动区域（如果控件太多可以滚动）
    QScrollArea* scrollArea = new QScrollArea(this);
     // 设置滚动区域的内容为控制面板
    scrollArea->setWidget(controlPanel);
     // 自动调整内容大小
    scrollArea->setWidgetResizable(true);
    //固定宽度（300 + 滚动条宽度）
    scrollArea->setFixedWidth(320);
    // 水平滚动条：始终隐藏
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
     // 垂直滚动条：需要时显示
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
     // 设置样式（无边框，深色背景）
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #2b2b2b; }");
    // 拉伸因子为0，固定宽度
    mainLayout->addWidget(scrollArea, 0);

    // ========== 标题 ==========
    QLabel* titleLabel = new QLabel("滤镜控制面板", controlPanel);
    // 设置样式：白色文字，16px，粗体
    titleLabel->setStyleSheet("QLabel { color: white; font-size: 16px; font-weight: bold; }");
    // 文字居中对齐
    titleLabel->setAlignment(Qt::AlignCenter);
    //添加到面板布局
    panelLayout->addWidget(titleLabel);



    // ========== 分组1：基础滤镜 ==========
     // 创建分组框（QGroupBox）
    QGroupBox* basicGroup = new QGroupBox("基础滤镜", controlPanel);
    basicGroup->setStyleSheet("QGroupBox { "
                              "color: white; " // 标题文字颜色
                              "font-weight: bold;" // 标题粗体
                              " border: 1px solid #555; " // 边框
                              "border-radius: 5px;"// 圆角
                              " margin-top: 10px; " // 上边距
                              "padding-top: 10px; }");// 内边距

    // 创建垂直布局（复选框从上到下排列）
    auto basicLayout = new QVBoxLayout(basicGroup);
    basicLayout->setSpacing(8);

    // 创建复选框（6个基础滤镜的复选框垂直排列）
    m_chkPassthrough = new QCheckBox("直通", basicGroup);
    m_chkPassthrough->setStyleSheet("QCheckBox { color: white; }");
    basicLayout->addWidget(m_chkPassthrough);

    m_chkGrayscale = new QCheckBox("灰度", basicGroup);
    m_chkGrayscale->setStyleSheet("QCheckBox { color: white; }");
    basicLayout->addWidget(m_chkGrayscale);

    m_chkInvert = new QCheckBox("反色", basicGroup);
    m_chkInvert->setStyleSheet("QCheckBox { color: white; }");
    basicLayout->addWidget(m_chkInvert);

    m_chkBlur = new QCheckBox("模糊", basicGroup);
    m_chkBlur->setStyleSheet("QCheckBox { color: white; }");
    basicLayout->addWidget(m_chkBlur);

    m_chkEdge = new QCheckBox("边缘检测", basicGroup);
    m_chkEdge->setStyleSheet("QCheckBox { color: white; }");
    basicLayout->addWidget(m_chkEdge);

    m_chkSharpen = new QCheckBox("锐化", basicGroup);
    m_chkSharpen->setStyleSheet("QCheckBox { color: white; }");
    basicLayout->addWidget(m_chkSharpen);
    
    panelLayout->addWidget(basicGroup);
    
    // ========== 分组2：高级滤镜（依旧按钮组加三个高极滤镜） ==========
    QGroupBox* advancedGroup = new QGroupBox("高级滤镜", controlPanel);
    advancedGroup->setStyleSheet("QGroupBox { "
                                 "color: white;"
                                 " font-weight: bold;"
                                 " border: 1px solid #555; "
                                 "border-radius: 5px; "
                                 "margin-top: 10px; "
                                 "padding-top: 10px; }");

    //垂直布局
    auto advancedLayout = new QVBoxLayout(advancedGroup);
    advancedLayout->setSpacing(8);
    
    m_chkBeauty = new QCheckBox("美颜磨皮", advancedGroup);
    m_chkBeauty->setStyleSheet("QCheckBox { color: white; }");
    advancedLayout->addWidget(m_chkBeauty);
    
    m_chkLut = new QCheckBox("色彩风格(LUT)", advancedGroup);
    m_chkLut->setStyleSheet("QCheckBox { color: white; }");
    advancedLayout->addWidget(m_chkLut);
    
    m_chkWatermark = new QCheckBox("水印叠加", advancedGroup);
    m_chkWatermark->setStyleSheet("QCheckBox { color: white; }");
    advancedLayout->addWidget(m_chkWatermark);
    //添加到控制面板
    panelLayout->addWidget(advancedGroup);

    // ========== 分组3：参数调节（按钮组加4个进度条和一个下拉框） ==========
    QGroupBox* paramGroup = new QGroupBox("参数调节", controlPanel);
    paramGroup->setStyleSheet("QGroupBox { color: white;"
                              " font-weight: bold;"
                              " border: 1px solid #555; "
                              "border-radius: 5px; "
                              "margin-top: 10px;"
                              " padding-top: 10px; }");
    //垂直布局
    auto paramLayout = new QVBoxLayout(paramGroup);
    paramLayout->setSpacing(12);

    // 模糊强度
    QLabel* labelBlurTitle = new QLabel("模糊强度:", paramGroup);
    //浅灰色
    labelBlurTitle->setStyleSheet("QLabel { color: #aaa; }");
    paramLayout->addWidget(labelBlurTitle);
    
    //水平布局（滑块+数值标签）
    auto blurSliderLayout = new QHBoxLayout();
    //创建滑块
    m_sliderBlurIntensity = new QSlider(Qt::Horizontal, paramGroup);
    m_sliderBlurIntensity->setMinimum(0);// 最小值0
    m_sliderBlurIntensity->setMaximum(30);// 最大值30
    m_sliderBlurIntensity->setValue(10); // 初始值10（对应1.0）
    blurSliderLayout->addWidget(m_sliderBlurIntensity);

    // 创建数值标签（显示当前值）
    //[━━━━━●━━━━━━━━━━━━━━] 1.0
    m_labelBlurIntensity = new QLabel("1.0", paramGroup);
    m_labelBlurIntensity->setStyleSheet("QLabel { color: white; }");
    // 固定宽度40px
    m_labelBlurIntensity->setFixedWidth(40);
    //添加到水平布局
    blurSliderLayout->addWidget(m_labelBlurIntensity);
    //水平布局添加到竖直布局中
    paramLayout->addLayout(blurSliderLayout);

    // 美颜强度
    QLabel* labelBeautyTitle = new QLabel("美颜强度:", paramGroup);
    labelBeautyTitle->setStyleSheet("QLabel { color: #aaa; }");
    paramLayout->addWidget(labelBeautyTitle);
    
    auto beautySliderLayout = new QHBoxLayout();
    m_sliderBeautyIntensity = new QSlider(Qt::Horizontal, paramGroup);
    m_sliderBeautyIntensity->setMinimum(0);
    m_sliderBeautyIntensity->setMaximum(100);
    m_sliderBeautyIntensity->setValue(50);
    beautySliderLayout->addWidget(m_sliderBeautyIntensity);

    m_labelBeautyIntensity = new QLabel("0.50", paramGroup);
    m_labelBeautyIntensity->setStyleSheet("QLabel { color: white; }");
    m_labelBeautyIntensity->setFixedWidth(40);
    beautySliderLayout->addWidget(m_labelBeautyIntensity);
    paramLayout->addLayout(beautySliderLayout);

    // LUT风格选择
    QLabel* labelLutPresetTitle = new QLabel("LUT风格:", paramGroup);
    labelLutPresetTitle->setStyleSheet("QLabel { color: #aaa; }");
    paramLayout->addWidget(labelLutPresetTitle);
    
    //下拉框
    m_comboLutPreset = new QComboBox(paramGroup);
    m_comboLutPreset->setStyleSheet("QComboBox { color: white;"
                                    " background-color: #3a3a3a; "
                                    "border: 1px solid #555;"
                                    " padding: 5px; }"
                                    "QComboBox::drop-down { border: none; }"
                                    "QComboBox QAbstractItemView { color: white; background-color: #3a3a3a; selection-background-color: #555; }");
    m_comboLutPreset->addItem("原图", 0);
    m_comboLutPreset->addItem("清凉", 1);
    m_comboLutPreset->addItem("粉嫩", 2);
    m_comboLutPreset->addItem("怀旧", 3);
    m_comboLutPreset->addItem("黑白", 4);
    m_comboLutPreset->addItem("赛博朋克", 5);
    paramLayout->addWidget(m_comboLutPreset);
    
    // LUT强度
    QLabel* labelLutIntensityTitle = new QLabel("LUT强度:", paramGroup);
    labelLutIntensityTitle->setStyleSheet("QLabel { color: #aaa; }");
    paramLayout->addWidget(labelLutIntensityTitle);
    
    auto lutSliderLayout = new QHBoxLayout();
    m_sliderLutIntensity = new QSlider(Qt::Horizontal, paramGroup);
    m_sliderLutIntensity->setMinimum(0);
    m_sliderLutIntensity->setMaximum(100);
    m_sliderLutIntensity->setValue(100);
    lutSliderLayout->addWidget(m_sliderLutIntensity);
    
    m_labelLutIntensity = new QLabel("1.0", paramGroup);
    m_labelLutIntensity->setStyleSheet("QLabel { color: white; }");
    m_labelLutIntensity->setFixedWidth(40);
    lutSliderLayout->addWidget(m_labelLutIntensity);
    paramLayout->addLayout(lutSliderLayout);

    // 水印透明度
    QLabel* labelWatermarkTitle = new QLabel("水印透明度:", paramGroup);
    labelWatermarkTitle->setStyleSheet("QLabel { color: #aaa; }");
    paramLayout->addWidget(labelWatermarkTitle);
    
    auto watermarkSliderLayout = new QHBoxLayout();
    m_sliderWatermarkOpacity = new QSlider(Qt::Horizontal, paramGroup);
    m_sliderWatermarkOpacity->setMinimum(0);
    m_sliderWatermarkOpacity->setMaximum(100);
    m_sliderWatermarkOpacity->setValue(80);
    watermarkSliderLayout->addWidget(m_sliderWatermarkOpacity);

    m_labelWatermarkOpacity = new QLabel("0.80", paramGroup);
    m_labelWatermarkOpacity->setStyleSheet("QLabel { color: white; }");
    m_labelWatermarkOpacity->setFixedWidth(40);
    watermarkSliderLayout->addWidget(m_labelWatermarkOpacity);
    paramLayout->addLayout(watermarkSliderLayout);
    
    panelLayout->addWidget(paramGroup);
    
    // 添加弹簧，把控件推到顶部
    panelLayout->addStretch();

    // ========== 连接信号槽 ==========
    //直通滤镜（当复选框激活的的时候触发）
    connect(m_chkPassthrough, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Passthrough", checked);
    });
    //灰度滤镜
    connect(m_chkGrayscale, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Grayscale", checked);
    });
    //其他同理
    connect(m_chkInvert, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Invert", checked);
    });

    connect(m_chkBlur, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Blur", checked);
    });

    connect(m_chkEdge, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("EdgeDetect", checked);
    });

    connect(m_chkSharpen, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Sharpen", checked);
    });
    
    connect(m_chkBeauty, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Beauty", checked);
    });
    
    connect(m_chkLut, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Lut", checked);
    });
    
    connect(m_chkWatermark, &QCheckBox::toggled, this, [this](bool checked){
        m_glWidget->toggleFilter("Watermark", checked);
    });

    connect(m_sliderBlurIntensity, &QSlider::valueChanged, this, [this](int value){
        float intensity = value / 10.0f;
        m_labelBlurIntensity->setText(QString::number(intensity, 'f', 1));
        m_glWidget->updateFilterParam("Blur", "intensity", intensity);
    });

    connect(m_sliderBeautyIntensity, &QSlider::valueChanged, this, [this](int value){
        float intensity = value / 100.0f;
        m_labelBeautyIntensity->setText(QString::number(intensity, 'f', 2));
        m_glWidget->updateFilterParam("Beauty", "intensity", intensity);
    });
    
    connect(m_comboLutPreset, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        int presetIndex = m_comboLutPreset->itemData(index).toInt();
        m_glWidget->setLutPreset(presetIndex);
    });
    
    connect(m_sliderLutIntensity, &QSlider::valueChanged, this, [this](int value){
        float intensity = value / 100.0f;
        m_labelLutIntensity->setText(QString::number(intensity, 'f', 2));
        m_glWidget->updateFilterParam("Lut", "intensity", intensity);
    });

    connect(m_sliderWatermarkOpacity, &QSlider::valueChanged, this, [this](int value){
        float opacity = value / 100.0f;
        m_labelWatermarkOpacity->setText(QString::number(opacity, 'f', 2));
        m_glWidget->updateFilterParam("Watermark", "opacity", opacity);
    });

    // 创建摄像头对象
    m_camera = new CameraCapture(this);
    
    // 直接启动摄像头
    m_glWidget->setCamera(m_camera);
    m_camera->start();
}

MainWindow::~MainWindow() {}
