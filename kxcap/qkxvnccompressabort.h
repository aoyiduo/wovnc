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

#ifndef QKXVNCCOMPRESSABORT_H
#define QKXVNCCOMPRESSABORT_H

#include "qkxcap_private.h"

/***
 *
 *   When using this class, first convert the full image format to the target format, and then encode it.
 * Because of the use of hard acceleration such as SSE, the system consumption increases, and the frame rate does not increase.
 * Therefore, abandon this scheme and return to the previous old scheme, at least reducing the consumption.
 *
 ***/
class QKxVNCCompressAbort
{
public:
    static uchar* convertToPixelFormat(uchar *dst, uchar *src, int srcStep, int width, int height, PixelFormat& fmt);
    static bool findDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, PixelFormat& fmt, QRect *prt);
    static int writeZipData(uchar *src, int cnt, void *p);
    static EEncodingType matchBest(quint16 width, quint16 height, void *p);
    static int doRaw(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doHextile(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);    
    static int doTRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doZRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);

    // private
    static int doTRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doZRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);

    static int doTRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
    static int doZRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);

    static int doOpenH264(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
private:
    static bool findRgbDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, PixelFormat& fmt, QRect *prt);
    static bool findYuvDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, QRect *prt);
};

#endif // QKXVNCCOMPRESSABORT_H
