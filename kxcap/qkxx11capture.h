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

#ifndef QKXX11CAPTURE_H
#define QKXX11CAPTURE_H

#include "qkxabstractcapture.h"

class QKxH264Encoder;
class X11CapturePrivate;
class QKxX11Capture : public QKxAbstractCapture
{
public:
    explicit QKxX11Capture(int idx, const QRect& imageRt, const QRect& screenRt, QObject *parent = nullptr);
    virtual ~QKxX11Capture();
    bool reset();
    int getFrame();
    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset);
private:
    X11CapturePrivate *m_prv;
    QRect m_imageRt;
    int m_idx;
};

#endif // QKXX11CAPTURE_H
