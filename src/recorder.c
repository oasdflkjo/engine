#include "recorder.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <glad/glad.h>

static bool parse_bool_env(const char* key, bool fallback) {
    const char* raw = getenv(key);
    if (!raw || !raw[0]) {
        return fallback;
    }
    if (strcmp(raw, "1") == 0 || strcmp(raw, "true") == 0 || strcmp(raw, "yes") == 0 || strcmp(raw, "on") == 0) {
        return true;
    }
    if (strcmp(raw, "0") == 0 || strcmp(raw, "false") == 0 || strcmp(raw, "no") == 0 || strcmp(raw, "off") == 0) {
        return false;
    }
    return fallback;
}

#ifdef ENABLE_RECORDING
#include <pthread.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

typedef struct RecorderImpl {
    AVFormatContext* formatCtx;
    AVCodecContext* codecCtx;
    AVStream* stream;
    AVBufferRef* hwDeviceCtx;
    AVBufferRef* hwFramesCtx;
    struct SwsContext* swsCtx;
    AVFrame* swFrame;
    AVFrame* hwFrame;
    int rgbaStride;
    size_t rgbaBufferSize;
    GLuint pbos[3];
    int pboCount;
    int frameIndex;
    int framesQueued;
    int64_t nextPts;
    int droppedFrames;
    int warmupCaptured;
    bool workerStop;
    bool workerRunning;
    pthread_t workerThread;
    pthread_mutex_t queueMutex;
    pthread_cond_t queueCond;
    struct {
        uint8_t* data;
    } queue[8];
    int queueHead;
    int queueTail;
    int queueCount;
    char outputPath[512];
} RecorderImpl;

static int parse_int_env(const char* key, int fallback) {
    const char* raw = getenv(key);
    if (!raw || !raw[0]) {
        return fallback;
    }
    char* end = NULL;
    long value = strtol(raw, &end, 10);
    if (end == raw || *end != '\0') {
        return fallback;
    }
    return (int)value;
}

static void copy_env_or_default(char* dst, size_t dstSize, const char* key, const char* fallback) {
    const char* value = getenv(key);
    if (!value || !value[0]) {
        value = fallback;
    }
    snprintf(dst, dstSize, "%s", value);
}

static bool equals_ignore_case(const char* lhs, const char* rhs) {
    return lhs && rhs && strcasecmp(lhs, rhs) == 0;
}

static void print_av_error(const char* step, int errCode) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errCode, errbuf, sizeof(errbuf));
    fprintf(stderr, "[recorder] %s failed: %s\n", step, errbuf);
}

static int encode_and_write(RecorderImpl* impl, AVFrame* frame) {
    int ret = avcodec_send_frame(impl->codecCtx, frame);
    if (ret < 0) {
        return ret;
    }

    while (1) {
        AVPacket* packet = av_packet_alloc();
        if (!packet) {
            return AVERROR(ENOMEM);
        }

        ret = avcodec_receive_packet(impl->codecCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&packet);
            return 0;
        }
        if (ret < 0) {
            av_packet_free(&packet);
            return ret;
        }

        av_packet_rescale_ts(packet, impl->codecCtx->time_base, impl->stream->time_base);
        packet->stream_index = impl->stream->index;

        ret = av_interleaved_write_frame(impl->formatCtx, packet);
        av_packet_free(&packet);
        if (ret < 0) {
            return ret;
        }
    }
}

static int encode_mapped_rgba_frame(RecorderImpl* impl, const uint8_t* rgbaData, int width, int height) {
    if (!rgbaData) {
        return AVERROR(EINVAL);
    }

    if (av_frame_make_writable(impl->swFrame) < 0) {
        return AVERROR(EINVAL);
    }

    const uint8_t* srcData[4] = {0};
    int srcLinesize[4] = {0};
    srcData[0] = rgbaData + (size_t)(height - 1) * (size_t)impl->rgbaStride;
    srcLinesize[0] = -impl->rgbaStride;

    sws_scale(
        impl->swsCtx,
        srcData,
        srcLinesize,
        0,
        height,
        impl->swFrame->data,
        impl->swFrame->linesize);

    av_frame_unref(impl->hwFrame);
    if (av_hwframe_get_buffer(impl->codecCtx->hw_frames_ctx, impl->hwFrame, 0) < 0) {
        return AVERROR_EXTERNAL;
    }

    if (av_hwframe_transfer_data(impl->hwFrame, impl->swFrame, 0) < 0) {
        return AVERROR_EXTERNAL;
    }

    impl->hwFrame->pts = impl->nextPts++;
    return encode_and_write(impl, impl->hwFrame);
}

static int process_pbo_frame(RecorderImpl* impl, int pboIndex, int width, int height) {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, impl->pbos[pboIndex]);
    const uint8_t* mapped = (const uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, (GLsizeiptr)impl->rgbaBufferSize, GL_MAP_READ_BIT);
    if (!mapped) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        return AVERROR_EXTERNAL;
    }

    int ret = encode_mapped_rgba_frame(impl, mapped, width, height);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return ret;
}

static bool queue_push_frame(RecorderImpl* impl, const uint8_t* src) {
    pthread_mutex_lock(&impl->queueMutex);
    if (impl->queueCount >= (int)(sizeof(impl->queue) / sizeof(impl->queue[0]))) {
        impl->droppedFrames++;
        pthread_mutex_unlock(&impl->queueMutex);
        return false;
    }

    uint8_t* dst = (uint8_t*)malloc(impl->rgbaBufferSize);
    if (!dst) {
        impl->droppedFrames++;
        pthread_mutex_unlock(&impl->queueMutex);
        return false;
    }

    memcpy(dst, src, impl->rgbaBufferSize);
    impl->queue[impl->queueTail].data = dst;
    impl->queueTail = (impl->queueTail + 1) % (int)(sizeof(impl->queue) / sizeof(impl->queue[0]));
    impl->queueCount++;
    pthread_cond_signal(&impl->queueCond);
    pthread_mutex_unlock(&impl->queueMutex);
    return true;
}

static bool queue_pop_frame(RecorderImpl* impl, uint8_t** outData) {
    pthread_mutex_lock(&impl->queueMutex);
    while (impl->queueCount == 0 && !impl->workerStop) {
        pthread_cond_wait(&impl->queueCond, &impl->queueMutex);
    }

    if (impl->queueCount == 0 && impl->workerStop) {
        pthread_mutex_unlock(&impl->queueMutex);
        return false;
    }

    *outData = impl->queue[impl->queueHead].data;
    impl->queue[impl->queueHead].data = NULL;
    impl->queueHead = (impl->queueHead + 1) % (int)(sizeof(impl->queue) / sizeof(impl->queue[0]));
    impl->queueCount--;
    pthread_mutex_unlock(&impl->queueMutex);
    return true;
}

static void* recorder_worker_main(void* userData) {
    RecorderImpl* impl = (RecorderImpl*)userData;
    while (1) {
        uint8_t* frameData = NULL;
        if (!queue_pop_frame(impl, &frameData)) {
            break;
        }

        int ret = encode_mapped_rgba_frame(impl, frameData, impl->codecCtx->width, impl->codecCtx->height);
        free(frameData);
        if (ret < 0) {
            print_av_error("encode_mapped_rgba_frame(worker)", ret);
        }
    }
    return NULL;
}

static bool init_ffmpeg_recorder(Recorder* recorder) {
    RecorderImpl* impl = (RecorderImpl*)recorder->impl;
    const int width = recorder->width;
    const int height = recorder->height;
    const int fps = recorder->fps;
    int ret = 0;

    char codecName[128];
    char vaapiDevice[256];
    copy_env_or_default(codecName, sizeof(codecName), "ENGINE_RECORD_CODEC", "hevc_vaapi");
    copy_env_or_default(vaapiDevice, sizeof(vaapiDevice), "ENGINE_RECORD_DEVICE", "/dev/dri/renderD128");
    copy_env_or_default(impl->outputPath, sizeof(impl->outputPath), "ENGINE_RECORD_OUTPUT", "capture.mkv");

    char rcMode[32];
    copy_env_or_default(rcMode, sizeof(rcMode), "ENGINE_RECORD_RATE_CONTROL", "quality");
    const int bitrateKbps = parse_int_env("ENGINE_RECORD_BITRATE_KBPS", 50000);
    const int qp = parse_int_env("ENGINE_RECORD_QP", 18);
    const int keyint = parse_int_env("ENGINE_RECORD_KEYINT", fps * 2);
    const AVCodec* codec = avcodec_find_encoder_by_name(codecName);
    if (!codec) {
        fprintf(stderr, "[recorder] Encoder '%s' was not found.\n", codecName);
        return false;
    }

    ret = av_hwdevice_ctx_create(&impl->hwDeviceCtx, AV_HWDEVICE_TYPE_VAAPI, vaapiDevice, NULL, 0);
    if (ret < 0) {
        print_av_error("av_hwdevice_ctx_create(VAAPI)", ret);
        return false;
    }

    ret = avformat_alloc_output_context2(&impl->formatCtx, NULL, NULL, impl->outputPath);
    if (ret < 0 || !impl->formatCtx) {
        print_av_error("avformat_alloc_output_context2", ret < 0 ? ret : AVERROR_UNKNOWN);
        return false;
    }

    impl->codecCtx = avcodec_alloc_context3(codec);
    if (!impl->codecCtx) {
        fprintf(stderr, "[recorder] avcodec_alloc_context3 failed.\n");
        return false;
    }

    impl->codecCtx->codec_id = codec->id;
    impl->codecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    impl->codecCtx->pix_fmt = AV_PIX_FMT_VAAPI;
    impl->codecCtx->width = width;
    impl->codecCtx->height = height;
    impl->codecCtx->time_base = (AVRational){1, fps};
    impl->codecCtx->framerate = (AVRational){fps, 1};
    if (equals_ignore_case(rcMode, "cbr")) {
        impl->codecCtx->bit_rate = (int64_t)bitrateKbps * 1000LL;
    } else {
        impl->codecCtx->bit_rate = 0;
    }
    impl->codecCtx->gop_size = keyint > 0 ? keyint : (fps * 2);
    impl->codecCtx->max_b_frames = 0;

    impl->hwFramesCtx = av_hwframe_ctx_alloc(impl->hwDeviceCtx);
    if (!impl->hwFramesCtx) {
        fprintf(stderr, "[recorder] av_hwframe_ctx_alloc failed.\n");
        return false;
    }

    AVHWFramesContext* framesCtx = (AVHWFramesContext*)impl->hwFramesCtx->data;
    framesCtx->format = AV_PIX_FMT_VAAPI;
    framesCtx->sw_format = AV_PIX_FMT_NV12;
    framesCtx->width = width;
    framesCtx->height = height;
    framesCtx->initial_pool_size = 32;

    ret = av_hwframe_ctx_init(impl->hwFramesCtx);
    if (ret < 0) {
        print_av_error("av_hwframe_ctx_init", ret);
        return false;
    }

    impl->codecCtx->hw_frames_ctx = av_buffer_ref(impl->hwFramesCtx);
    if (!impl->codecCtx->hw_frames_ctx) {
        fprintf(stderr, "[recorder] av_buffer_ref(hwFramesCtx) failed.\n");
        return false;
    }

    if (impl->formatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        impl->codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary* codecOpts = NULL;
    if (equals_ignore_case(rcMode, "cbr")) {
        av_dict_set(&codecOpts, "rc_mode", "CBR", 0);
    } else {
        char qpStr[16];
        snprintf(qpStr, sizeof(qpStr), "%d", qp);
        av_dict_set(&codecOpts, "rc_mode", "CQP", 0);
        av_dict_set(&codecOpts, "qp", qpStr, 0);
    }
    ret = avcodec_open2(impl->codecCtx, codec, &codecOpts);
    av_dict_free(&codecOpts);
    if (ret < 0) {
        print_av_error("avcodec_open2", ret);
        return false;
    }

    impl->stream = avformat_new_stream(impl->formatCtx, NULL);
    if (!impl->stream) {
        fprintf(stderr, "[recorder] avformat_new_stream failed.\n");
        return false;
    }
    impl->stream->time_base = impl->codecCtx->time_base;

    ret = avcodec_parameters_from_context(impl->stream->codecpar, impl->codecCtx);
    if (ret < 0) {
        print_av_error("avcodec_parameters_from_context", ret);
        return false;
    }

    if (!(impl->formatCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&impl->formatCtx->pb, impl->outputPath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            print_av_error("avio_open", ret);
            return false;
        }
    }

    ret = avformat_write_header(impl->formatCtx, NULL);
    if (ret < 0) {
        print_av_error("avformat_write_header", ret);
        return false;
    }

    impl->swsCtx = sws_getContext(
        width,
        height,
        AV_PIX_FMT_RGBA,
        width,
        height,
        AV_PIX_FMT_NV12,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL);
    if (!impl->swsCtx) {
        fprintf(stderr, "[recorder] sws_getContext failed.\n");
        return false;
    }

    impl->swFrame = av_frame_alloc();
    impl->hwFrame = av_frame_alloc();
    if (!impl->swFrame || !impl->hwFrame) {
        fprintf(stderr, "[recorder] av_frame_alloc failed.\n");
        return false;
    }

    impl->swFrame->format = AV_PIX_FMT_NV12;
    impl->swFrame->width = width;
    impl->swFrame->height = height;
    ret = av_frame_get_buffer(impl->swFrame, 32);
    if (ret < 0) {
        print_av_error("av_frame_get_buffer", ret);
        return false;
    }

    impl->rgbaStride = width * 4;
    impl->rgbaBufferSize = (size_t)impl->rgbaStride * (size_t)height;
    impl->pboCount = 3;
    impl->frameIndex = 0;
    impl->framesQueued = 0;
    impl->warmupCaptured = 0;
    impl->droppedFrames = 0;
    impl->workerStop = false;
    impl->workerRunning = false;
    impl->queueHead = 0;
    impl->queueTail = 0;
    impl->queueCount = 0;

    glGenBuffers(impl->pboCount, impl->pbos);
    for (int i = 0; i < impl->pboCount; i++) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, impl->pbos[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizeiptr)impl->rgbaBufferSize, NULL, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    if (pthread_mutex_init(&impl->queueMutex, NULL) != 0 || pthread_cond_init(&impl->queueCond, NULL) != 0) {
        fprintf(stderr, "[recorder] Failed to initialize queue synchronization.\n");
        return false;
    }

    if (pthread_create(&impl->workerThread, NULL, recorder_worker_main, impl) != 0) {
        fprintf(stderr, "[recorder] Failed to create recorder worker thread.\n");
        return false;
    }
    impl->workerRunning = true;

    impl->nextPts = 0;
    if (equals_ignore_case(rcMode, "cbr")) {
        fprintf(stderr, "[recorder] Recording started: %s (%dx%d @ %d fps, %s, CBR %d kbps)\n", impl->outputPath, width, height, fps, codecName, bitrateKbps);
    } else {
        fprintf(stderr, "[recorder] Recording started: %s (%dx%d @ %d fps, %s, QUALITY/CQP qp=%d)\n", impl->outputPath, width, height, fps, codecName, qp);
    }
    return true;
}
#endif

int recorder_target_fps_from_env(int fallbackFps) {
    if (fallbackFps <= 0) {
        fallbackFps = 60;
    }

    const char* raw = getenv("ENGINE_RECORD_FPS");
    if (!raw || !raw[0]) {
        return fallbackFps;
    }

    char* end = NULL;
    long value = strtol(raw, &end, 10);
    if (end == raw || *end != '\0' || value <= 0) {
        return fallbackFps;
    }
    return (int)value;
}

bool recorder_init_from_env(Recorder* recorder, int width, int height, int fps) {
    if (!recorder) {
        return false;
    }

    recorder->enabled = false;
    recorder->width = width;
    recorder->height = height;
    recorder->fps = fps > 0 ? fps : 60;
    recorder->impl = NULL;

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "[recorder] Invalid dimensions.\n");
        return false;
    }

    if (width % 2 != 0 || height % 2 != 0) {
        recorder->width = width - (width % 2);
        recorder->height = height - (height % 2);
        fprintf(stderr, "[recorder] Adjusted capture size to even dimensions: %dx%d\n", recorder->width, recorder->height);
    }

    if (!parse_bool_env("ENGINE_RECORD", false)) {
        return false;
    }

#ifndef ENABLE_RECORDING
    fprintf(stderr, "[recorder] ENGINE_RECORD=1 ignored because build was done without ENABLE_RECORDING.\n");
    return false;
#else
    RecorderImpl* impl = (RecorderImpl*)calloc(1, sizeof(RecorderImpl));
    if (!impl) {
        fprintf(stderr, "[recorder] Recorder allocation failed.\n");
        return false;
    }

    recorder->impl = impl;
    if (!init_ffmpeg_recorder(recorder)) {
        recorder_cleanup(recorder);
        return false;
    }

    recorder->enabled = true;
    return true;
#endif
}

void recorder_capture_frame(Recorder* recorder) {
    if (!recorder || !recorder->enabled || !recorder->impl) {
        return;
    }

#ifndef ENABLE_RECORDING
    (void)recorder;
#else
    RecorderImpl* impl = (RecorderImpl*)recorder->impl;
    const int width = recorder->width;
    const int height = recorder->height;

    const int writeIndex = impl->frameIndex % impl->pboCount;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, impl->pbos[writeIndex]);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    if (impl->warmupCaptured >= 1) {
        const int readIndex = (impl->frameIndex + impl->pboCount - 1) % impl->pboCount;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, impl->pbos[readIndex]);
        const uint8_t* mapped = (const uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, (GLsizeiptr)impl->rgbaBufferSize, GL_MAP_READ_BIT);
        if (mapped) {
            queue_push_frame(impl, mapped);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        } else {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            print_av_error("glMapBufferRange", AVERROR_EXTERNAL);
        }
    } else {
        impl->warmupCaptured++;
    }

    impl->frameIndex++;
#endif
}

void recorder_cleanup(Recorder* recorder) {
    if (!recorder) {
        return;
    }

#ifdef ENABLE_RECORDING
    if (recorder->enabled && recorder->impl) {
        RecorderImpl* impl = (RecorderImpl*)recorder->impl;

        const int pendingDrain = impl->warmupCaptured > 0 ? 1 : 0;
        for (int i = 0; i < pendingDrain; i++) {
            const int readIndex = (impl->frameIndex + impl->pboCount - 1 - i + impl->pboCount) % impl->pboCount;
            glBindBuffer(GL_PIXEL_PACK_BUFFER, impl->pbos[readIndex]);
            const uint8_t* mapped = (const uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, (GLsizeiptr)impl->rgbaBufferSize, GL_MAP_READ_BIT);
            if (mapped) {
                queue_push_frame(impl, mapped);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            }
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }

        pthread_mutex_lock(&impl->queueMutex);
        impl->workerStop = true;
        pthread_cond_signal(&impl->queueCond);
        pthread_mutex_unlock(&impl->queueMutex);

        if (impl->workerRunning) {
            pthread_join(impl->workerThread, NULL);
            impl->workerRunning = false;
        }

        int ret = encode_and_write(impl, NULL);
        if (ret < 0) {
            print_av_error("encode_and_write(flush)", ret);
        }
        av_write_trailer(impl->formatCtx);
    }

    if (recorder->impl) {
        RecorderImpl* impl = (RecorderImpl*)recorder->impl;
        if (impl->formatCtx && !(impl->formatCtx->oformat->flags & AVFMT_NOFILE) && impl->formatCtx->pb) {
            avio_closep(&impl->formatCtx->pb);
        }
        av_frame_free(&impl->swFrame);
        av_frame_free(&impl->hwFrame);
        sws_freeContext(impl->swsCtx);
        avcodec_free_context(&impl->codecCtx);
        avformat_free_context(impl->formatCtx);
        av_buffer_unref(&impl->hwFramesCtx);
        av_buffer_unref(&impl->hwDeviceCtx);
        if (impl->pboCount > 0) {
            glDeleteBuffers(impl->pboCount, impl->pbos);
        }
        for (int i = 0; i < (int)(sizeof(impl->queue) / sizeof(impl->queue[0])); i++) {
            free(impl->queue[i].data);
            impl->queue[i].data = NULL;
        }
        pthread_cond_destroy(&impl->queueCond);
        pthread_mutex_destroy(&impl->queueMutex);
        if (impl->droppedFrames > 0) {
            fprintf(stderr, "[recorder] Dropped %d frames due to encoder backpressure.\n", impl->droppedFrames);
        }
        free(impl);
    }
#endif

    recorder->enabled = false;
    recorder->impl = NULL;
}
