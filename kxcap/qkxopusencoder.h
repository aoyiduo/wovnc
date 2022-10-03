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

#ifndef QKXOPUSENCODER_H
#define QKXOPUSENCODER_H

#include <QObject>
#include <QDataStream>

class QKxOpusEncoderPrivate;
class QKxOpusEncoder : public QObject
{
    Q_OBJECT
public:
    explicit QKxOpusEncoder(QObject *parent = nullptr);
    ~QKxOpusEncoder();
    bool init(int sampleRate, int channelCount);
    int process(char *pcm, int bytesTotal);
    int result(QDataStream& ds);
private:
    QKxOpusEncoderPrivate *m_prv;
};

#endif // QKXOPUSENCODER_H
