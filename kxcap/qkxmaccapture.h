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

#ifndef QKXMACCAPTURE_H
#define QKXMACCAPTURE_H

#include "qkxabstractcapture.h"

class MacCapturePrivate;
class QKxMacCapture : public QKxAbstractCapture
{
public:
    explicit QKxMacCapture(int idx, const QRect& imageRt, const QRect& screenRt, QObject *parent = nullptr);
    virtual ~QKxMacCapture();
    bool reset();
    int getFrame();
    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset);
private:
    MacCapturePrivate *m_prv;
    QRect m_imageRt;
    int m_idx;
};

#endif // QKXMACCAPTURE_H
