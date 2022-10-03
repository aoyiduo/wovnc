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

#include "qkxopusencoder.h"

#include <opus/opus.h>
#include <QByteArray>
#include <QDebug>
#include <QDateTime>
#include <QMutex>
#include <QDataStream>

#define MAX_PACKET  (1500)

class QKxOpusEncoderPrivate
{
    QKxOpusEncoder *m_pub;
    OpusEncoder *m_enc = nullptr;
    int m_sampleRate;
    int m_channelCount;
    int m_bytesPerSecond;
    qint64 m_timeLast;
    QVector<QByteArray> m_opusData;
    QMutex m_mtx;
public:
    QKxOpusEncoderPrivate(QKxOpusEncoder *p)
        : m_pub(p){
        m_enc = nullptr;
        m_bytesPerSecond = 0;
        m_timeLast = 0;
    }

    ~QKxOpusEncoderPrivate() {
        if(m_enc) {
            opus_encoder_destroy(m_enc);
        }
    }

    bool init(int sampleRate, int channelCount) {
        int err = 0;
        OpusEncoder *enc = opus_encoder_create(sampleRate, channelCount, OPUS_APPLICATION_AUDIO, &err);
        if(err != OPUS_OK) {
            return false;
        }
        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
        opus_encoder_ctl(enc, OPUS_SET_VBR(1));
        opus_encoder_ctl(enc, OPUS_SET_BITRATE(24000));
#ifdef USE_FEC
        // no usefull for kcp feather.
        opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(1));
        opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(20));
#else
        opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(0));
#endif
        m_enc = enc;
        m_sampleRate = sampleRate;
        m_channelCount = channelCount;
        return true;
    }

    int process(char *pcm, int bytesTotal) {
        if(pcm == nullptr) {
            return -1;
        }
        int sampleCount = bytesTotal / sizeof(short);
        int frameCount = sampleCount / m_channelCount;
        QByteArray packet;
        packet.resize(bytesTotal);
        int len = opus_encode(m_enc, (opus_int16*)pcm, frameCount, (uchar*)packet.data(), bytesTotal);
        if(len < 0 || len > bytesTotal) {
            return -1;
        }
        packet.resize(len);
        pushResult(packet);
#if 0
        int nb_encoded = opus_packet_get_samples_per_frame((uchar*)m_packet.data(), m_sampleRate)
                * opus_packet_get_nb_frames((uchar*)m_packet.data(), len);
        int remaining = frameCount - nb_encoded;
        if(remaining > 0) {
            qDebug() << "test" << remaining;
        }
#endif
        m_bytesPerSecond += len;
        qint64 now = QDateTime::currentSecsSinceEpoch() / 60;
        if(now != m_timeLast) {
            m_timeLast = now;
            qDebug() << "opus 1 minute byte total:" << m_bytesPerSecond;
            m_bytesPerSecond = 0;
        }

        return 0;
    }

    int result(QDataStream& ds) {
        QMutexLocker lk(&m_mtx);
        int cnt = m_opusData.length();
        ds << qint16(cnt);
        for(int i = 0; i < m_opusData.length(); i++) {
            ds << m_opusData.at(i);
        }
        m_opusData.clear();
        return cnt;
    }

    /**
     * just keep about one second data.
     * 48000 / 960 = 50;
     * */
    void pushResult(const QByteArray& data) {
        QMutexLocker lk(&m_mtx);
        m_opusData.append(data);
        if(m_opusData.length() > 100) {
            m_opusData.pop_front();
        }
    }
};

QKxOpusEncoder::QKxOpusEncoder(QObject *parent)
    : QObject(parent)
{
    m_prv = new QKxOpusEncoderPrivate(this);
}

QKxOpusEncoder::~QKxOpusEncoder()
{
    delete m_prv;
}

bool QKxOpusEncoder::init(int sampleRate, int channelCount)
{
    return m_prv->init(sampleRate, channelCount);
}

int QKxOpusEncoder::process(char *pcm, int bytesTotal)
{
    return m_prv->process(pcm, bytesTotal);
}

int QKxOpusEncoder::result(QDataStream& ds)
{
    return m_prv->result(ds);
}
