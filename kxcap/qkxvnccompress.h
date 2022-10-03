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

#ifndef QKXVNCCOMPRESS_H
#define QKXVNCCOMPRESS_H

#include "qkxcap_private.h"

class QKxVNCCompress
{
public:
    static bool findDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, QRect *prt);
    static int writeZipData(uchar *src, int cnt, void *p);
    static EEncodingType matchBest(quint16 width, quint16 height, void *p);
    static int doRaw(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doHextile(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);    
    static int doRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, quint16 bsize, bool rleSkip, void *p);
    static int doTRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doZRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);

    // private
    static int doTRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doZRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);

    static int doRgbBlock(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, quint16 bsize, void *p);
    static int doTRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doZRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);

    static int doOpenH264(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doOpenJpeg(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
private:
    static bool findRgbDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, QRect *prt);
};

#endif // QKXVNCCOMPRESS_H
