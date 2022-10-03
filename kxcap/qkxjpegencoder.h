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

#ifndef QKXJPEGENCODER_H
#define QKXJPEGENCODER_H

#include <QObject>
#include <QDataStream>

class QKxJpegEncoderPrivate;
class QKxJpegEncoder : public QObject
{
    Q_OBJECT
public:
    explicit QKxJpegEncoder(QObject *parent = nullptr);
    ~QKxJpegEncoder();
    void setQualityLevel(int lv);
    bool encode(QDataStream &ds, uchar *src, int bytesPerLine, int width, int height, int quality);
private:
    QKxJpegEncoderPrivate *m_prv;
};

#endif // QKXJPEGENCODER_H
