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

#ifndef QKXAUDIOSTREAM_H
#define QKXAUDIOSTREAM_H

#include <functional>
#include <QObject>

class QKxAudioStreamPrivate;
class QKxAudioStream : public QObject
{
    Q_OBJECT
public:
    explicit QKxAudioStream(QObject *parent = nullptr);
    ~QKxAudioStream();
    static void initialize();
    static void terminate();
    bool init(int idx, int sampleRate, int channel, int framesPerBuffer,  std::function<int(char*, int)> cb);
    bool start();
    void close();
private:
    QKxAudioStreamPrivate *m_prv;
};

#endif // QKXAUDIOSTREAM_H
