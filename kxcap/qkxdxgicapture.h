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

#ifndef QKXDXGICAPTURE_H
#define QKXDXGICAPTURE_H

#include "qkxcap_private.h"

#include <QRect>
#include <QDataStream>
#include "qkxabstractcapture.h"

class DXGICapturePrivate;
class QKxDXGICapture : public QKxAbstractCapture
{
    Q_OBJECT
public:
    explicit QKxDXGICapture(int idx, const QRect& imageRt, const QRect& screenRt, QObject *parent=nullptr);
    virtual ~QKxDXGICapture();
    bool init();
    int getFrame();
    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset);
    void releaseFrame();
private:
    DXGICapturePrivate *m_prv;
    QRect m_imageRt;
    int m_idx;
};

#endif // QKXDXGICAPTURE_H
