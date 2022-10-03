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

#ifndef QKXH264ENCODER_H
#define QKXH264ENCODER_H

#include <QObject>

class QKxH264EncoderPrivate;
class QKxH264Encoder : public QObject
{
    Q_OBJECT
public:
    explicit QKxH264Encoder(QObject *parent = nullptr);
    ~QKxH264Encoder();
    bool init(int width, int height, bool isCamera);
    void setQualityLevel(int lv);
    int encode(QDataStream &ds, uchar *src);
private:
    QKxH264EncoderPrivate *m_prv;
};

#endif // QKXH264ENCODER_H
