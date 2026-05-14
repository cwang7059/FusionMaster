#pragma once

#include <array>
#include <atomic>

#include <QImage>
#include <QMetaObject>
#include <QPointer>
#include <QQuickFramebufferObject>

class RtspPlayerController;
class RtspVideoItemRenderer;

class RtspVideoItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(QString emptyText READ emptyText WRITE setEmptyText NOTIFY emptyTextChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)

public:
    explicit RtspVideoItem(QQuickItem* parent = nullptr);
    ~RtspVideoItem() override;

    QObject* controller() const;
    void setController(QObject* controller);

    QString emptyText() const;
    void setEmptyText(const QString& text);
    bool hasFrame() const;

    Renderer* createRenderer() const override;

signals:
    void controllerChanged();
    void emptyTextChanged();
    void hasFrameChanged();

private:
    static constexpr int kFrameBufferCount = 3;

    void connectController();
    void disconnectController();
    void handleFramePresented(const QImage& image, const QString& streamId);
    void handleFrameCleared();
    bool acquirePublishedFrame(QImage* frame, quint64* serial, int* frameIndex) const;
    void releaseFrame(int frameIndex) const;
    int reserveWriteBuffer(const QSize& size);
    bool copyFrameIntoBuffer(int bufferIndex, const QImage& image);

    QPointer<RtspPlayerController> controller_;
    QMetaObject::Connection frameConnection_;
    QMetaObject::Connection clearConnection_;
    std::array<QImage, kFrameBufferCount> frameBuffers_;
    mutable std::array<std::atomic_int, kFrameBufferCount> frameReaders_{};
    std::array<std::atomic<quint64>, kFrameBufferCount> frameSerials_{};
    std::atomic<quint64> nextFrameSerial_{1};
    std::atomic_int publishedFrameIndex_{-1};
    std::atomic_bool frameBuffersReady_{false};
    QString emptyText_;
    std::atomic_bool hasFrame_{false};

    friend class RtspVideoItemRenderer;
};
