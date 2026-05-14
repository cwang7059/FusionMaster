#include "rtsp_video_item.h"

#include "rtsp_player_controller.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QQuickWindow>

#include <array>
#include <cstring>

namespace {

constexpr GLfloat kBackgroundRed = 8.0f / 255.0f;
constexpr GLfloat kBackgroundGreen = 23.0f / 255.0f;
constexpr GLfloat kBackgroundBlue = 38.0f / 255.0f;
// The decoder already outputs BGRA-backed ARGB32 frames, so keep that layout to avoid
// an extra per-frame format conversion before uploading to GL.
constexpr QImage::Format kUploadFrameFormat = QImage::Format_ARGB32;
constexpr GLenum kUploadTexturePixelFormat = GL_BGRA;

std::array<GLfloat, 24> buildVertices(const QSize& viewportSize, const QSize& frameSize) {
    float sx = 1.0f;
    float sy = 1.0f;

    if (viewportSize.width() > 0
        && viewportSize.height() > 0
        && frameSize.width() > 0
        && frameSize.height() > 0) {
        const float viewportAspect =
            static_cast<float>(viewportSize.width()) / static_cast<float>(viewportSize.height());
        const float frameAspect =
            static_cast<float>(frameSize.width()) / static_cast<float>(frameSize.height());

        if (viewportAspect > frameAspect) {
            sx = frameAspect / viewportAspect;
        } else {
            sy = viewportAspect / frameAspect;
        }
    }

    return {
        -sx, -sy, 0.0f, 0.0f,
         sx, -sy, 1.0f, 0.0f,
         sx,  sy, 1.0f, 1.0f,
        -sx, -sy, 0.0f, 0.0f,
         sx,  sy, 1.0f, 1.0f,
        -sx,  sy, 0.0f, 1.0f
    };
}

}  // namespace

class RtspVideoItemRenderer final : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    RtspVideoItemRenderer() = default;

    ~RtspVideoItemRenderer() override {
        releaseAcquiredFrame();
        cleanup();
    }

    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
        return new QOpenGLFramebufferObject(size.expandedTo(QSize(1, 1)), format);
    }

    void synchronize(QQuickFramebufferObject* item) override {
        releaseAcquiredFrame();

        videoItem_ = static_cast<RtspVideoItem*>(item);
        frame_ = QImage();
        frameSerial_ = 0;
        window_ = videoItem_ != nullptr ? videoItem_->window() : nullptr;
        if (videoItem_ == nullptr) {
            return;
        }

        videoItem_->acquirePublishedFrame(&frame_, &frameSerial_, &acquiredFrameIndex_);
    }

    void render() override {
        ensureInitialized();

        const QSize viewportSize = framebufferObject() != nullptr
            ? framebufferObject()->size()
            : QSize(1, 1);

        glViewport(0, 0, viewportSize.width(), viewportSize.height());
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glClearColor(kBackgroundRed, kBackgroundGreen, kBackgroundBlue, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (frame_.isNull()) {
            releaseAcquiredFrame();
            resetSceneGraphState();
            return;
        }

        uploadFrameTexture(viewportSize);

        const std::array<GLfloat, 24> vertices = buildVertices(viewportSize, frame_.size());
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_DYNAMIC_DRAW);

        shaderProgram_.bind();
        shaderProgram_.setUniformValue(textureUniformName_, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, frameTexture_);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<const void*>(2 * sizeof(GLfloat)));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        shaderProgram_.release();

        releaseAcquiredFrame();
        resetSceneGraphState();
    }

private:
    void ensureInitialized() {
        if (initialized_) {
            return;
        }

        initializeOpenGLFunctions();
        glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

        const char* vertexShaderSource =
            "attribute vec2 aPos;\n"
            "attribute vec2 aUv;\n"
            "varying vec2 vUv;\n"
            "void main() {\n"
            "    vUv = aUv;\n"
            "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
            "}\n";

        const char* fragmentShaderSource =
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            "varying vec2 vUv;\n"
            "uniform sampler2D uTex;\n"
            "void main() {\n"
            "    gl_FragColor = texture2D(uTex, vUv);\n"
            "}\n";

        shaderProgram_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
        shaderProgram_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
        shaderProgram_.bindAttributeLocation("aPos", 0);
        shaderProgram_.bindAttributeLocation("aUv", 1);
        shaderProgram_.link();

        glGenTextures(1, &frameTexture_);
        glBindTexture(GL_TEXTURE_2D, frameTexture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenBuffers(1, &vertexBuffer_);

        initialized_ = true;
    }

    void uploadFrameTexture(const QSize& viewportSize) {
        if (frame_.isNull()) {
            return;
        }

        const bool useMipmaps =
            viewportSize.width() > 0
            && viewportSize.height() > 0
            && (frame_.width() > viewportSize.width() || frame_.height() > viewportSize.height());

        glBindTexture(GL_TEXTURE_2D, frameTexture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        updateTextureFiltering(useMipmaps);

        const bool textureSizeChanged = textureSize_ != frame_.size();
        const bool frameChanged = uploadedFrameSerial_ != frameSerial_;
        if (textureSizeChanged) {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                frame_.width(),
                frame_.height(),
                0,
                kUploadTexturePixelFormat,
                GL_UNSIGNED_BYTE,
                frame_.constBits());
            textureSize_ = frame_.size();
        } else if (frameChanged) {
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                0,
                frame_.width(),
                frame_.height(),
                kUploadTexturePixelFormat,
                GL_UNSIGNED_BYTE,
                frame_.constBits());
        }

        if (useMipmaps && (textureSizeChanged || frameChanged)) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        uploadedFrameSerial_ = frameSerial_;
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void updateTextureFiltering(bool useMipmaps) {
        const GLint minFilter = useMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        if (currentMinFilter_ != minFilter) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            currentMinFilter_ = minFilter;
        }
        if (currentMagFilter_ != GL_LINEAR) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            currentMagFilter_ = GL_LINEAR;
        }
    }

    void cleanup() {
        if (!initialized_ || QOpenGLContext::currentContext() == nullptr) {
            return;
        }

        if (frameTexture_ != 0) {
            glDeleteTextures(1, &frameTexture_);
            frameTexture_ = 0;
        }
        if (vertexBuffer_ != 0) {
            glDeleteBuffers(1, &vertexBuffer_);
            vertexBuffer_ = 0;
        }

        initialized_ = false;
    }

    void resetSceneGraphState() {
        if (window_ != nullptr) {
            window_->resetOpenGLState();
        }
    }

    void releaseAcquiredFrame() {
        if (videoItem_ != nullptr && acquiredFrameIndex_ >= 0) {
            videoItem_->releaseFrame(acquiredFrameIndex_);
        }
        acquiredFrameIndex_ = -1;
    }

private:
    QImage frame_;
    QOpenGLShaderProgram shaderProgram_;
    GLuint frameTexture_ = 0;
    GLuint vertexBuffer_ = 0;
    QSize textureSize_;
    RtspVideoItem* videoItem_ = nullptr;
    QQuickWindow* window_ = nullptr;
    quint64 frameSerial_ = 0;
    quint64 uploadedFrameSerial_ = 0;
    int acquiredFrameIndex_ = -1;
    const char* textureUniformName_ = "uTex";
    GLint currentMinFilter_ = GL_LINEAR;
    GLint currentMagFilter_ = GL_LINEAR;
    bool initialized_ = false;
};

RtspVideoItem::RtspVideoItem(QQuickItem* parent)
    : QQuickFramebufferObject(parent) {
    for (int index = 0; index < kFrameBufferCount; ++index) {
        frameReaders_[index].store(0, std::memory_order_release);
        frameSerials_[index].store(0, std::memory_order_release);
    }
}

RtspVideoItem::~RtspVideoItem() {
    disconnectController();
}

QObject* RtspVideoItem::controller() const {
    return controller_.data();
}

void RtspVideoItem::setController(QObject* controller) {
    auto* typedController = qobject_cast<RtspPlayerController*>(controller);
    if (controller_ == typedController) {
        return;
    }

    disconnectController();
    controller_ = typedController;
    connectController();
    emit controllerChanged();
}

QString RtspVideoItem::emptyText() const {
    return emptyText_;
}

void RtspVideoItem::setEmptyText(const QString& text) {
    if (emptyText_ == text) {
        return;
    }

    emptyText_ = text;
    emit emptyTextChanged();
}

bool RtspVideoItem::hasFrame() const {
    return hasFrame_.load(std::memory_order_acquire);
}

QQuickFramebufferObject::Renderer* RtspVideoItem::createRenderer() const {
    return new RtspVideoItemRenderer();
}

void RtspVideoItem::connectController() {
    if (controller_ == nullptr) {
        return;
    }

    frameConnection_ = connect(
        controller_,
        &RtspPlayerController::framePresented,
        this,
        [this](const QImage& image, const QString& streamId) {
            handleFramePresented(image, streamId);
        });

    clearConnection_ = connect(
        controller_,
        &RtspPlayerController::frameCleared,
        this,
        [this]() {
            handleFrameCleared();
        });
}

void RtspVideoItem::disconnectController() {
    if (frameConnection_) {
        disconnect(frameConnection_);
        frameConnection_ = {};
    }
    if (clearConnection_) {
        disconnect(clearConnection_);
        clearConnection_ = {};
    }
    handleFrameCleared();
}

void RtspVideoItem::handleFramePresented(const QImage& image, const QString& streamId) {
    Q_UNUSED(streamId)

    if (image.isNull()) {
        handleFrameCleared();
        return;
    }

    QImage uploadFrame = image;
    if (uploadFrame.format() != kUploadFrameFormat) {
        uploadFrame = uploadFrame.convertToFormat(kUploadFrameFormat);
    }
    if (uploadFrame.isNull()) {
        return;
    }

    const int writeBufferIndex = reserveWriteBuffer(uploadFrame.size());
    if (writeBufferIndex < 0 || !copyFrameIntoBuffer(writeBufferIndex, uploadFrame)) {
        return;
    }

    const quint64 frameSerial = nextFrameSerial_.fetch_add(1, std::memory_order_acq_rel);
    frameSerials_[writeBufferIndex].store(frameSerial, std::memory_order_release);
    publishedFrameIndex_.store(writeBufferIndex, std::memory_order_release);
    frameBuffersReady_.store(true, std::memory_order_release);

    const bool hadFrame = hasFrame_.exchange(true, std::memory_order_acq_rel);
    if (!hadFrame) {
        emit hasFrameChanged();
    }
    update();
}

void RtspVideoItem::handleFrameCleared() {
    frameBuffersReady_.store(false, std::memory_order_release);
    publishedFrameIndex_.store(-1, std::memory_order_release);

    const bool hadFrame = hasFrame_.exchange(false, std::memory_order_acq_rel);
    if (hadFrame) {
        emit hasFrameChanged();
    }
    update();
}

bool RtspVideoItem::acquirePublishedFrame(QImage* frame, quint64* serial, int* frameIndex) const {
    if (frame != nullptr) {
        *frame = QImage();
    }
    if (serial != nullptr) {
        *serial = 0;
    }
    if (frameIndex != nullptr) {
        *frameIndex = -1;
    }
    if (!frameBuffersReady_.load(std::memory_order_acquire)) {
        return false;
    }

    const int publishedFrameIndex = publishedFrameIndex_.load(std::memory_order_acquire);
    if (publishedFrameIndex < 0 || publishedFrameIndex >= kFrameBufferCount) {
        return false;
    }

    frameReaders_[publishedFrameIndex].fetch_add(1, std::memory_order_acq_rel);
    if (!frameBuffersReady_.load(std::memory_order_acquire)
        || publishedFrameIndex_.load(std::memory_order_acquire) != publishedFrameIndex
        || frameBuffers_[publishedFrameIndex].isNull()) {
        frameReaders_[publishedFrameIndex].fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }

    if (frame != nullptr) {
        *frame = frameBuffers_[publishedFrameIndex];
    }
    if (serial != nullptr) {
        *serial = frameSerials_[publishedFrameIndex].load(std::memory_order_acquire);
    }
    if (frameIndex != nullptr) {
        *frameIndex = publishedFrameIndex;
    }
    return true;
}

void RtspVideoItem::releaseFrame(int frameIndex) const {
    if (frameIndex < 0 || frameIndex >= kFrameBufferCount) {
        return;
    }
    frameReaders_[frameIndex].fetch_sub(1, std::memory_order_acq_rel);
}

int RtspVideoItem::reserveWriteBuffer(const QSize& size) {
    if (!size.isValid()) {
        return -1;
    }

    const int publishedFrameIndex = publishedFrameIndex_.load(std::memory_order_acquire);
    for (int index = 0; index < kFrameBufferCount; ++index) {
        if (index == publishedFrameIndex) {
            continue;
        }
        if (frameReaders_[index].load(std::memory_order_acquire) != 0) {
            continue;
        }

        QImage& buffer = frameBuffers_[index];
        if (buffer.size() != size || buffer.format() != kUploadFrameFormat) {
            buffer = QImage(size, kUploadFrameFormat);
        }
        if (!buffer.isNull()) {
            return index;
        }
    }

    return -1;
}

bool RtspVideoItem::copyFrameIntoBuffer(int bufferIndex, const QImage& image) {
    if (bufferIndex < 0 || bufferIndex >= kFrameBufferCount || image.isNull()) {
        return false;
    }

    QImage& buffer = frameBuffers_[bufferIndex];
    if (buffer.isNull() || buffer.size() != image.size() || buffer.format() != kUploadFrameFormat) {
        return false;
    }

    const int rowBytes = qMin(buffer.bytesPerLine(), image.bytesPerLine());
    if (rowBytes <= 0) {
        return false;
    }

    const uchar* source = image.constBits();
    uchar* destination = buffer.bits();
    for (int row = 0; row < image.height(); ++row) {
        std::memcpy(
            destination + row * buffer.bytesPerLine(),
            source + row * image.bytesPerLine(),
            rowBytes);
    }

    return true;
}
