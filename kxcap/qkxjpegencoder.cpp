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

#include "qkxjpegencoder.h"

#include <turbojpeg.h>
#include <QDataStream>

class QKxJpegEncoderPrivate
{
public:
    QKxJpegEncoderPrivate(QKxJpegEncoder *p) {

    }

    ~QKxJpegEncoderPrivate() {

    }

    void setQualityLevel(int lv) {

    }

    bool encode(QDataStream &ds, uchar *src, int bytesPerLine, int width, int height, int quality) {
        tjhandle compressor = tjInitCompress();
        if (nullptr == compressor) {
            return false;
        }
        unsigned char *jpegBuf = nullptr;
        unsigned long jpegSize = 0;
        quality = qBound(70, quality, 90);
        int status = tjCompress2(compressor, src, width, bytesPerLine, height, TJPF::TJPF_BGRX,
                                 &jpegBuf, &jpegSize, TJSAMP_444, quality, TJFLAG_FASTDCT);
        if (status != 0) {
            tjDestroy(compressor);
            return false;
        }
        ds << qint32(jpegSize);
        ds.writeRawData((char*)jpegBuf, jpegSize);
        tjDestroy(compressor);
        return true;
    }

    bool compress(unsigned char *data_uncompressed, unsigned char *&out_compressed, unsigned long *out_size, int width, int height, int pixel_format)
    {
        if (nullptr == data_uncompressed) {
            return false;
        }

        tjhandle compressor = tjInitCompress();
        if (nullptr == compressor) {
            return false;
        }

        //pixel_format : TJPF::TJPF_BGR or other
        const int JPEG_QUALITY = 75;
        int pitch = tjPixelSize[pixel_format] * width;
        int status = tjCompress2(compressor, data_uncompressed, width, pitch, height, pixel_format,
                                 &out_compressed, out_size, TJSAMP_444, JPEG_QUALITY, TJFLAG_FASTDCT);
        if (status != 0) {
            tjDestroy(compressor);
            return false;
        }

        tjDestroy(compressor);
        return true;
    }
};

QKxJpegEncoder::QKxJpegEncoder(QObject *parent)
    : QObject(parent)
{
    m_prv = new QKxJpegEncoderPrivate(this);
}

QKxJpegEncoder::~QKxJpegEncoder()
{
    delete m_prv;
}

void QKxJpegEncoder::setQualityLevel(int lv)
{
    m_prv->setQualityLevel(lv);
}

bool QKxJpegEncoder::encode(QDataStream &ds, uchar *src, int bytesPerLine, int width, int height, int quality)
{
    return m_prv->encode(ds, src, bytesPerLine, width, height, quality);
}

