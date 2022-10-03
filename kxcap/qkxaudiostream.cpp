/*******************************************************************************************
*
* Copyright (C) 2022 Guangzhou AoYiDuo Network Technology Co.,Ltd. All Rights Reserved.
*
* Contact: http://www.aoyiduo.com
*
*   this file is used under the terms of the GPLv3[GNU GENERAL PUBLIC LICENSE v3]
* more information follow the website: https://www.gnu.org/licenses/gpl-3.0.en.html
*
*******************************************************************************************/

#include "qkxaudiostream.h"

#include "qkxresample.h"

#include <portaudio.h>
#include <QTimer>
#include <QDebug>

class QKxAudioStreamPrivate
{
private:
    QKxAudioStream *m_p;
    PaDeviceIndex m_idx;
    int m_sampleRateDefault; // inner support.
    int m_sampleRateNeed;
    int m_channel;
    int m_framesPerBuffer;
    std::function<int(char*, int)> m_cb;
    PaStream* m_stream;
    QKxResample m_resample;
public:
    QKxAudioStreamPrivate(QKxAudioStream *p)
        : m_p(p) {
        Q_UNUSED(m_p)
        m_idx = paNoDevice;
        m_sampleRateNeed = 0;
        m_channel = 0;
        m_framesPerBuffer = 0;
        m_cb = nullptr;
        m_stream = nullptr;
        m_sampleRateDefault = 0;
    }

    ~QKxAudioStreamPrivate() {
        close();
    }

    static int recordCallback( const void *inputBuffer, void *outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData )
    {
        QKxAudioStreamPrivate *prv = reinterpret_cast<QKxAudioStreamPrivate*>(userData);
        return prv->onCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
    }

    int onCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags) {
        int sampleCount = framesPerBuffer * m_channel;
        int bytesTotal = sampleCount * sizeof(short);
        m_resample.process((char*)inputBuffer, bytesTotal);

        sampleCount = m_framesPerBuffer * m_channel;
        bytesTotal = sampleCount * sizeof(short);
        while(m_resample.popSamples((char*)inputBuffer, sampleCount) > 0) {
            m_cb((char*)inputBuffer, bytesTotal);
        }
        m_cb(nullptr, 0);
        return paContinue;
    }

    bool init(int idx, int sampleRate, int channel, int framesPerBuffer, std::function<int(char*, int)> cb)
    {
        PaStreamParameters inputParameters = {0};
        inputParameters.device = idx;
        inputParameters.channelCount = channel;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(idx)->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;
        inputParameters.sampleFormat = paInt16;

        PaStream* stream;
        const PaDeviceInfo *di = Pa_GetDeviceInfo(idx);
        if(di == nullptr) {
            return false;
        }

        int err = Pa_OpenStream(&stream, &inputParameters, nullptr, di->defaultSampleRate, 4096, paClipOff, recordCallback, this);
        if(err != paNoError) {
            return false;
        }

        m_idx = idx;
        m_sampleRateDefault = (int)di->defaultSampleRate;
        m_sampleRateNeed = sampleRate;
        m_channel = channel;
        m_framesPerBuffer = framesPerBuffer;
        m_cb = cb;
        m_stream = stream;

        m_resample.init(channel, m_sampleRateDefault, 16, channel, m_sampleRateNeed, 16);
        return true;
    }

    bool start() {
        return Pa_StartStream(m_stream) == paNoError;
    }

    void close() {
        if(m_stream != nullptr) {
            Pa_StopStream(m_stream);
            Pa_CloseStream(m_stream);
            m_stream = nullptr;
        }
    }
};

QKxAudioStream::QKxAudioStream(QObject *parent) : QObject(parent)
{
    m_prv = new QKxAudioStreamPrivate(this);
}

QKxAudioStream::~QKxAudioStream()
{
    delete m_prv;
}

void QKxAudioStream::initialize()
{
    Pa_Initialize();
}

void QKxAudioStream::terminate()
{
    Pa_Terminate();
}

bool QKxAudioStream::init(int idx, int sampleRate, int channel, int framesPerBuffer, std::function<int(char*, int)> cb)
{
    return m_prv->init(idx, sampleRate, channel, framesPerBuffer, cb);
}

bool QKxAudioStream::start()
{
    return m_prv->start();
}

void QKxAudioStream::close()
{
    m_prv->close();
}
