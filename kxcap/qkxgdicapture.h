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

#ifndef QKXGDICAPTURE_H
#define QKXGDICAPTURE_H

#include "qkxcap_private.h"
#include "qkxabstractcapture.h"

class GDICapturePrivate;
class QKxPrivacyScreen;
class QKxGDICapture : public QKxAbstractCapture
{
public:
    explicit QKxGDICapture(int idx, const QRect& imageRt, const QRect& screenRt, QKxPrivacyScreen* prvScreen, QObject *parent = nullptr);
    virtual ~QKxGDICapture();
    bool reset();
    int getFrame();
    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset);
private:
    GDICapturePrivate *m_prv;
    QRect m_imageRt;
    int m_idx;
};

#endif // QKXGDICAPTURE_H
