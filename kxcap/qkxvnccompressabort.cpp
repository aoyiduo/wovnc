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

#include "qkxvnccompressabort.h"

#include "qkxzip.h"
#include "qkxh264encoder.h"
#include "qkxutils.h"

#include <QtEndian>
#include <QBuffer>
#include <QDebug>

static inline quint8 colorMapIndex(quint32 rgb) {
    quint32 ri = ((rgb >> 16) & 0xFF) / 0x33;
    quint32 gi = ((rgb >> 8) & 0xFF) / 0x33;
    quint32 bi = (rgb & 0xFF) / 0x33;
    quint32 idx = ri * 36 + gi * 6 + bi;
    return (quint8)idx;
}

static inline quint32 toPixelFormat(quint32 clr, PixelFormat &fmt)
{
    quint32 r = (clr >> 16) & 0xFF;
    quint32 g = (clr >> 8) & 0xFF;
    quint32 b = clr & 0xFF;
    quint32 tmpr = (double)r / (double)0xFF * fmt.redMax;
    quint32 tmpg = (double)g / (double)0xFF * fmt.greenMax;
    quint32 tmpb = (double)b / (double)0xFF * fmt.blueMax;
    quint32 rgb = (tmpr << fmt.redShift) | (tmpg << fmt.greenShift) | (tmpb << fmt.blueShift);
    return rgb;
}

static inline quint32 toCPixelColor(quint32 clr, PixelFormat &fmt)
{
    if(fmt.trueColor) {
        if(fmt.depth == 24) {
            //RGB format is 888.
            return clr;
        }else if(fmt.depth == 16) {
            // RGB format is 565.
            return toPixelFormat(clr, fmt);
        }else if(fmt.depth == 15) {
            //RGB format is 555.
            return toPixelFormat(clr, fmt);
        }else {
            //RGB format is 332.
            return toPixelFormat(clr, fmt);
        }
    }else{
       return colorMapIndex(clr);
    }
}

static inline void writePixelColor(QDataStream &ds, quint32 clr, PixelFormat &fmt)
{
    switch (fmt.depth) {
    case 24:
        ds.writeRawData((char*)&clr, 4);
        break;
    case 15:
    case 16:
        ds.writeRawData((char*)&clr, 2);
        break;
    default:
        ds.writeRawData((char*)&clr, 1);
        break;
    }
}

static inline void writeCPixelColor(QDataStream &ds, quint32 clr, PixelFormat &fmt)
{
    switch (fmt.depth) {
    case 24:
        ds.writeRawData((char*)&clr, 3);
        break;
    case 15:
    case 16:
        ds.writeRawData((char*)&clr, 2);
        break;
    default:
        ds.writeRawData((char*)&clr, 1);
        break;
    }
}

uchar* QKxVNCCompressAbort::convertToPixelFormat(uchar *dst, uchar *src, int srcStep, int width, int height, PixelFormat &fmt)
{
    if(fmt.extendFormat == 1) {
        QKxUtils::rgb32ToNV12(dst, src, srcStep, width, height);
        return dst;
    }else if(fmt.extendFormat == 2) {
        QKxUtils::rgb32ToI420(dst, src, srcStep, width, height);
        return dst;
    }else if(fmt.trueColor) {
        if(fmt.depth == 24) {
            //RGB format is 888.
            return src;
        }else if(fmt.depth == 16) {
            // RGB format is 565.
            // align 4 byte.
            int dstStep = (width * 2 + 3) / 4 * 4;
            QKxUtils::rgb32ToRgb565(dst, dstStep, src, srcStep, width, height);
            return dst;
        }else if(fmt.depth == 15) {
            //RGB format is 555.
            int dstStep = (width * 2 + 3) / 4 * 4;
            QKxUtils::rgb32ToRgb555(dst, dstStep, src, srcStep, width, height);
            return dst;
        }else {
            //RGB format is 332.
            int dstStep = (width + 3) / 4 * 4;
            QKxUtils::rgb32ToRgb332(dst, dstStep, src, srcStep, width, height);
            return dst;
        }
    }else{
        int dstStep = (width + 3) / 4 * 4;
        QKxUtils::rgb32ToRgbMap(dst, dstStep, src, srcStep, width, height);
        return dst;
    }
}

bool QKxVNCCompressAbort::findDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, PixelFormat &fmt, QRect *prt)
{
    if(fmt.extendFormat == 0) {
        return findRgbDirtyRect(data1, step1, width, height, data2, step2, fmt, prt);
    }
    return findYuvDirtyRect(data1, step1, width, height, data2, step2, prt);
}

int QKxVNCCompressAbort::writeZipData(uchar *src, int cnt, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QKxZip *zip = req->zip;
    QDataStream &ds = req->ds;
    ds << quint32(0);
    QByteArray &buf = req->buf;
    int pos = buf.length();

    int total = zip->encode(QByteArray::fromRawData((char*)src, cnt), buf);
    quint32 *pLenth = reinterpret_cast<quint32*>(buf.data() + pos - 4);
    pLenth[0] = qToBigEndian<quint32>(total);
    total += pos;
    buf.resize(total);
    ds.device()->seek(total);
    return 0;
}

EEncodingType QKxVNCCompressAbort::matchBest(quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    int wanted = width * height * req->fmt.bitsPerPixel / 8;

    if(req->codes.contains(ET_OpenH264) && req->fmt.extendFormat == 2) {
        return ET_OpenH264;
    }else if(req->codes.contains(ET_ZRLE3)) {
        return ET_ZRLE3;
    }else if(req->codes.contains(ET_TRLE3)) {
        return ET_TRLE3;
    }else if(req->codes.contains(ET_ZRLE2)) {
        return ET_ZRLE2;
    }else if(req->codes.contains(ET_ZRLE)) {
        return ET_ZRLE;
    }else if(req->codes.contains(ET_TRLE2)) {
        return ET_TRLE2;
    }else if(req->codes.contains(ET_TRLE)) {
        return ET_TRLE;
    }else if(req->codes.contains(ET_Hextile)) {
        return ET_Hextile;
    }
    return ET_Raw;
}

template<class T>
static int doRaw(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    for(quint16 h = 0; h < height; h++) {
        T *psrc = reinterpret_cast<T*>(src + h * srcPitch);
        T *pdst = reinterpret_cast<T*>(dst + h * dstPitch);
        for(quint16 w = 0; w < width;  w ++) {
            pdst[w] = psrc[w];
            writePixelColor(ds, psrc[w], req->fmt);
        }
    }
    return 0;
}

int QKxVNCCompressAbort::doRaw(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    if(req->fmt.bitsPerPixel == 32) {
        return ::doRaw<quint32>(dst, dstPitch, src, srcPitch, width, height, p);
    }else if(req->fmt.bitsPerPixel == 16) {
        return ::doRaw<quint16>(dst, dstPitch, src, srcPitch, width, height, p);
    }else if(req->fmt.bitsPerPixel == 8) {
        return ::doRaw<quint8>(dst, dstPitch, src, srcPitch, width, height, p);
    }
    return 0;
}

template<class T>
static int doHextile(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    for(quint16 h = 0; h < height; h += 16) {
        quint16 bheight = qMin<quint16>(h + 16, height) - h;
        for(quint16 w = 0; w < width;  w += 16) {
            quint16 bwidth = qMin<quint16>(w + 16, width) - w;
            ds << quint8(0x1);
            for(quint16 bh = 0; bh < bheight; bh++) {
                T *psrc = reinterpret_cast<T*>(src + (h + bh) * srcPitch);
                T *pdst = reinterpret_cast<T*>(dst + (h + bh) * dstPitch);
                psrc += w;
                pdst += w;
                for(quint16 bw = 0;  bw < bwidth; bw++) {
                    pdst[bw] = psrc[bw];
                    writePixelColor(ds, psrc[bw], req->fmt);
                }
            }
        }
    }

    return 0;
}

int QKxVNCCompressAbort::doHextile(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    if(req->fmt.bitsPerPixel == 32) {
        return ::doHextile<quint32>(dst, dstPitch, src, srcPitch, width, height, p);
    }else if(req->fmt.bitsPerPixel == 16) {
        return ::doHextile<quint16>(dst, dstPitch, src, srcPitch, width, height, p);
    }else if(req->fmt.bitsPerPixel == 8) {
        return ::doHextile<quint8>(dst, dstPitch, src, srcPitch, width, height, p);
    }
    return 0;
}

template<class T>
static int doRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, quint16 bsize, bool rleSkip, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    for(quint16 h = 0; h < height; h += bsize) {
        quint16 bheight = qMin<quint16>(h + bsize, height) - h;
        for (quint16 w = 0; w < width; w += bsize) {
            quint16 bwidth = qMin<quint16>(w + bsize, width) - w;
            qint32 runLength = 0;
            quint32 rgbval = 0;
            quint8 subcode = 100; // 17 to 126: Unused. (Packed palettes of these sizes would offer no
                                // advantage over palette RLE).
            qint64 pos = ds.device()->pos();
            ds << quint8(128);
            for(quint16 bh = 0; bh < bheight; bh++) {
                T *psrc = reinterpret_cast<T*>(src + (h + bh) * srcPitch);
                T *pdst = reinterpret_cast<T*>(dst + (h + bh) * dstPitch);
                psrc += w;
                pdst += w;
                /* example, length 1 is represented as [0], 255 as [254], 256 as
                 [255,0], 257 as [255,1], 510 as [255,254], 511 as [255,255,0], and
                 so on. */
                for(quint16 bw = 0; bw < bwidth; bw++) {
                    quint32 clrSrc = psrc[bw];
                    quint32 clrDst = pdst[bw];
                    pdst[bw] = psrc[bw];
                    if(subcode != 128){
                        if( clrSrc != clrDst){
                            subcode = 128;
                        }
                    }
                    if(runLength == 0) {
                        writeCPixelColor(ds, clrSrc, req->fmt);
                        rgbval = clrSrc;
                        runLength = 1;
                    }else if(clrSrc == rgbval) {
                        runLength++;
                    }else{
                        while(runLength > 0xFF) {
                            ds << quint8(0xFF);
                            runLength -= 0xFF;
                        }
                        runLength--;
                        ds << quint8(runLength);
                        writeCPixelColor(ds, clrSrc, req->fmt);
                        rgbval = clrSrc;
                        runLength = 1;
                    }
                }
            }
            while(runLength > 0xFF) {
                ds << quint8(0xFF);
                runLength -= 0xFF;
            }
            runLength--;
            ds << quint8(runLength);
            if(subcode != 128 && rleSkip) {
                req->buf.resize(pos);
                ds.device()->seek(pos);
                ds << subcode;
            }
        }
    }
    return 0;
}

int QKxVNCCompressAbort::doTRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    if(req->fmt.bitsPerPixel == 32) {
        return doRLE<quint32>(dst, dstPitch, src, srcPitch, width, height, 16, false, p);
    }else if(req->fmt.bitsPerPixel == 16) {
        return doRLE<quint16>(dst, dstPitch, src, srcPitch, width, height, 16, false, p);
    }else if(req->fmt.bitsPerPixel == 8) {
        return doRLE<quint8>(dst, dstPitch, src, srcPitch, width, height, 16, false, p);
    }
    return 0;
}

int QKxVNCCompressAbort::doZRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);


    UpdateRequest tmp;
    tmp.buf.reserve(100 * 1024);
    QBuffer dev(&tmp.buf);
    dev.open(QIODevice::WriteOnly);
    tmp.ds.setDevice(&dev);
    tmp.fmt = req->fmt;
    tmp.match = req->match;
    tmp.codes = req->codes;
    if(req->fmt.bitsPerPixel == 32) {
        doRLE<quint32>(dst, dstPitch, src, srcPitch, width, height, 64, false, &tmp);
    }else if(req->fmt.bitsPerPixel == 16) {
        doRLE<quint16>(dst, dstPitch, src, srcPitch, width, height, 64, false, &tmp);
    }else if(req->fmt.bitsPerPixel == 8) {
        doRLE<quint8>(dst, dstPitch, src, srcPitch, width, height, 64, false, &tmp);
    }
    return writeZipData((uchar*)tmp.buf.data(), tmp.buf.length(), p);
}

int QKxVNCCompressAbort::doTRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    if(req->fmt.bitsPerPixel == 32) {
        return doRLE<quint32>(dst, dstPitch, src, srcPitch, width, height, 16, true, p);
    }else if(req->fmt.bitsPerPixel == 16) {
        return doRLE<quint16>(dst, dstPitch, src, srcPitch, width, height, 16, true, p);
    }else if(req->fmt.bitsPerPixel == 8) {
        return doRLE<quint8>(dst, dstPitch, src, srcPitch, width, height, 16, true, p);
    }
    return 0;
}

int QKxVNCCompressAbort::doZRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);


    UpdateRequest tmp;
    tmp.buf.reserve(100 * 1024);
    QBuffer dev(&tmp.buf);
    dev.open(QIODevice::WriteOnly);
    tmp.ds.setDevice(&dev);
    tmp.fmt = req->fmt;
    tmp.match = req->match;
    tmp.codes = req->codes;
    if(req->fmt.bitsPerPixel == 32) {
        doRLE<quint32>(dst, dstPitch, src, srcPitch, width, height, 64, true, &tmp);
    }else if(req->fmt.bitsPerPixel == 16) {
        doRLE<quint16>(dst, dstPitch, src, srcPitch, width, height, 64, true, &tmp);
    }else if(req->fmt.bitsPerPixel == 8) {
        doRLE<quint8>(dst, dstPitch, src, srcPitch, width, height, 64, true, &tmp);
    }
    return writeZipData((uchar*)tmp.buf.data(), tmp.buf.length(), p);
}

template<class T>
int doRgbBlock(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, quint16 bsize, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    qint32 runLength = 0;
    quint32 rgbval = 0;
    bool isPFrame = false;
    QVector<quint32> cache;
    cache.reserve(100);

    int totalCount = 0;
    auto frameOutput = [&](quint8 type, qint32 length) {
        //qDebug() << "doRgbFrameType:" << type << length;
        totalCount += length;
    };

    auto savePFrameBlockData = [&](int cnt) {
        ds << quint8(255);
        frameOutput(255, cnt);
    };

    auto savePFrameCurrentData = [&]() {
        if(runLength <= 0) {
            return;
        }
        if(runLength <= (253-168)) {
            ds << quint8(168+runLength-1);
            frameOutput(168, runLength);
        }else if(runLength <= 0xFFFF) {
            ds << quint8(254) << quint16(runLength);
            frameOutput(254, runLength);
        }else {
            ds << quint8(253) << quint32(runLength);
            frameOutput(253, runLength);
        }
        runLength = 0;
    };

    auto saveIFrameCacheData = [&]() {
        if(cache.length() > 0) {
            quint32 cnt = cache.length();
            if(cnt <= (166-85)) {
                ds << quint8(85+cnt-1);
                frameOutput(85, cnt);
            }else if(cnt <= 0xFFFF) {
                ds << quint8(167) << quint16(cnt);
                frameOutput(167, cnt);
            }else{
                ds << quint8(166) << quint32(cnt);
                frameOutput(166, cnt);
            }

            for(int i = 0; i < cache.length(); i++) {
                writeCPixelColor(ds, cache.at(i), req->fmt);
            }
            cache.clear();
        }
    };

    auto saveIFrameCurrentData = [&]() {
        if(runLength <= 0) {
            return;
        }
        //pixel tile in frame:
        // format:type | length | color ;
        //   1:           8bit.
        //   2:           16bit.
        //   3:           24bit.
        //   4:           32bit.
        if(runLength <= 83) {
            ds << quint8(runLength-1);
            frameOutput(0, runLength);
        }else if(runLength <= 0xFFFF) {
            ds << quint8(84) << quint16(runLength);
            frameOutput(84, runLength);
        }else {
            ds << quint8(83) << quint32(runLength);
            frameOutput(83, runLength);
        }
        writeCPixelColor(ds, rgbval, req->fmt);
        runLength = 0;
    };

    for(quint16 h = 0; h < height; h += bsize) {
        quint16 bheight = qMin<quint16>(h + bsize, height) - h;
        quint16 blockSizeLast = 0;
        for (quint16 w = 0; w < width; w += bsize) {
            quint16 bwidth = qMin<quint16>(w + bsize, width) - w;
            if(isPFrame && blockSizeLast > 0 && runLength >= blockSizeLast) {
                runLength -= blockSizeLast;
                savePFrameCurrentData();
                savePFrameBlockData(blockSizeLast);
                runLength = 0;
            }
            blockSizeLast = bwidth * bheight;
            for(quint16 bh = 0; bh < bheight; bh++) {
                T *psrc = reinterpret_cast<T*>(src + (h + bh) * srcPitch);
                T *pdst = reinterpret_cast<T*>(dst + (h + bh) * dstPitch);
                psrc += w;
                pdst += w;
                for(quint16 bw = 0; bw < bwidth; bw++) {
                    quint32 clrSrc = psrc[bw];
                    quint32 clrDst = pdst[bw];
                    pdst[bw] = psrc[bw];

                    if(clrSrc == clrDst) {
                        if(!isPFrame && runLength > 0) {
                            // store iframe content.
                            saveIFrameCacheData();
                            saveIFrameCurrentData();
                        }
                        isPFrame = true;
                        runLength++;
                    }else{
                        if(isPFrame && runLength > 0) {
                            // store pframe content.
                            savePFrameCurrentData();
                        }
                        isPFrame = false;
                        if(runLength == 0) {
                            rgbval = clrSrc;
                            runLength = 1;
                        }else if(clrSrc == rgbval) {
                            runLength++;
                        }else{
                            if(runLength == 1) {
                                cache.append(rgbval); //store to cache first.
                            }else{
                                // write cache first.
                                saveIFrameCacheData();
                                saveIFrameCurrentData();
                            }
                            rgbval = clrSrc;
                            runLength = 1;
                        }
                    }
                }
            }
        }
    }
    if(isPFrame) {
        savePFrameCurrentData();
    }else{
        saveIFrameCacheData();
        saveIFrameCurrentData();
    }
    return 0;
}

static int doYuvBlock(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, quint16 bsize, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    qint32 runLength = 0;
    quint64 rgbval = 0;
    bool isPFrame = false;
    QVector<quint64> cache;
    cache.reserve(100);

    int ystride = (req->imageSize.width() + 3) / 4 * 4;
    int uvstride = ystride / 2;

    int x = req->updateRect.left();
    int y = req->updateRect.top();
    uchar *ydst = req->dstHeader + ystride * y + x;
    uchar *uvdst = req->dstHeader + ystride * req->imageSize.height() + uvstride * (y / 2) + x;
    uchar *ysrc = req->srcHeader + ystride * y + x;
    uchar *uvsrc = req->srcHeader + ystride * (req->imageSize.height() + 1) + uvstride * (y / 2) + x;

    if(req->updateRect.width() != width || req->updateRect.height() != height) {
        //qDebug() << "width x height" << width << height;
    }

    int totalCount = 0;
    auto frameOutput = [&](quint8 type, qint32 length) {
        //qDebug() << "doYuvFrameType:" << type << length;
        totalCount += length;
    };

    auto savePFrameBlockData = [&](int cnt) {
        ds << quint8(255);
        frameOutput(255, cnt);
    };

    auto savePFrameCurrentData = [&]() {
        if(runLength <= 0) {
            return;
        }
        if(runLength <= (253-168)) {
            ds << quint8(168+runLength-1);
            frameOutput(168, runLength);
        }else if(runLength <= 0xFFFF) {
            ds << quint8(254) << quint16(runLength);
            frameOutput(254, runLength);
        }else {
            ds << quint8(253) << quint32(runLength);
            frameOutput(253, runLength);
        }
        runLength = 0;
    };

    auto saveIFrameCacheData = [&]() {
        if(cache.length() > 0) {
            quint32 cnt = cache.length();
            if(cnt <= (166-85)) {
                ds << quint8(85+cnt-1);
                frameOutput(85, cnt);
            }else if(cnt <= 0xFFFF) {
                ds << quint8(167) << quint16(cnt);
                frameOutput(167, cnt);
            }else{
                ds << quint8(166) << quint32(cnt);
                frameOutput(166, cnt);
            }
            for(int i = 0; i < cache.length(); i++) {
                quint64 val = cache.at(i);
                quint8 *pval = (quint8*)&val;
                ds.writeRawData((char*)pval, 6);
            }
            cache.clear();
        }
    };

    auto saveIFrameCurrentData = [&]() {
        if(runLength <= 0) {
            return;
        }
        quint8 *pval = (quint8*)&rgbval;
        if(runLength <= 83) {
            ds << quint8(runLength-1);
            frameOutput(0, runLength);
        }else if(runLength <= 0xFFFF) {
            ds << quint8(84) << quint16(runLength);
            frameOutput(84, runLength);
        }else {
            ds << quint8(83) << quint32(runLength);
            frameOutput(83, runLength);
        }
        ds.writeRawData((char*)pval, 6);
        runLength = 0;
    };

    for(quint16 h = 0; h < height; h += bsize) {
        quint16 bheight = qMin<quint16>(h + bsize, height) - h;
        quint16 blockSizeLast = 0;
        for (quint16 w = 0; w < width; w += bsize) {
            quint16 bwidth = qMin<quint16>(w + bsize, width) - w;
            if(isPFrame && blockSizeLast > 0 && runLength >= blockSizeLast) {
                runLength -= blockSizeLast;
                savePFrameCurrentData();
                savePFrameBlockData(blockSizeLast);
                runLength = 0;
            }
            int swidth = (bwidth + 1) / 2;
            int sheight = (bheight + 1) / 2;
            blockSizeLast = swidth * sheight;
            for(quint16 bh = 0; bh < sheight; bh++) {
                quint16 *pydst1 = reinterpret_cast<quint16*>(ydst + (h + bh * 2) * ystride + w);
                quint16 *pydst2 = reinterpret_cast<quint16*>(ydst + (h + bh * 2 + 1) * ystride + w);
                quint16 *puvdst = reinterpret_cast<quint16*>(uvdst + (h/2 + bh) * uvstride + w);
                quint16 *pysrc1 = reinterpret_cast<quint16*>(ysrc + (h + bh * 2) * ystride + w);
                quint16 *pysrc2 = reinterpret_cast<quint16*>(ysrc + (h + bh * 2 + 1) * ystride + w);
                quint16 *puvsrc = reinterpret_cast<quint16*>(uvsrc + (h/2 + bh) * uvstride + w);
                for(quint16 bw = 0; bw < swidth; bw++) {
                    quint64 clrDst = 0;
                    quint16 *pclrdst = (quint16*)&clrDst;
                    pclrdst[0] = pydst1[bw];
                    pclrdst[1] = pydst2[bw];
                    pclrdst[2] = puvdst[bw];
                    quint64 clrSrc = 0;
                    quint16 *pclrsrc = (quint16*)&clrSrc;
                    pclrsrc[0] = pysrc1[bw];
                    pclrsrc[1] = pysrc2[bw];
                    pclrsrc[2] = puvsrc[bw];

                    pydst1[bw]   = pysrc1[bw];
                    pydst2[bw]   = pysrc2[bw];
                    puvdst[bw]   = puvsrc[bw];

                    if(clrSrc == clrDst) {
                        if(!isPFrame && runLength > 0) {
                            // store iframe content.
                            saveIFrameCacheData();
                            saveIFrameCurrentData();
                        }
                        isPFrame = true;
                        runLength++;
                    }else{
                        if(isPFrame && runLength > 0) {
                            // store pframe content.
                            savePFrameCurrentData();
                        }
                        isPFrame = false;
                        if(runLength == 0) {
                            rgbval = clrSrc;
                            runLength = 1;
                        }else if(clrSrc == rgbval) {
                            runLength++;
                        }else{
                            if(runLength == 1) {
                                cache.append(rgbval); //store to cache first.
                            }else{
                                // write cache first.
                                saveIFrameCacheData();
                                saveIFrameCurrentData();
                            }
                            rgbval = clrSrc;
                            runLength = 1;
                        }
                    }
                }
            }
        }
    }
    if(isPFrame) {
        savePFrameCurrentData();
    }else{
        saveIFrameCacheData();
        saveIFrameCurrentData();
    }
    return 0;
}

int QKxVNCCompressAbort::doTRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    if(req->fmt.extendFormat == 1) {
        // yuv420
        return doYuvBlock(dst, dstPitch, src, srcPitch, width, height, 16, p);
    }
    if(req->fmt.bitsPerPixel == 32) {
        return doRgbBlock<quint32>(dst, dstPitch, src, srcPitch, width, height, 16, p);
    }else if(req->fmt.bitsPerPixel == 16) {
        return doRgbBlock<quint16>(dst, dstPitch, src, srcPitch, width, height, 16, p);
    }else if(req->fmt.bitsPerPixel == 8) {
        return doRgbBlock<quint8>(dst, dstPitch, src, srcPitch, width, height, 16, p);
    }
    return 0;
}

int QKxVNCCompressAbort::doZRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);

    UpdateRequest tmp;
    tmp.buf.reserve(100 * 1024);
    QBuffer dev(&tmp.buf);
    dev.open(QIODevice::WriteOnly);
    tmp.ds.setDevice(&dev);
    tmp.fmt = req->fmt;
    tmp.match = req->match;
    tmp.codes = req->codes;
    tmp.srcHeader = req->srcHeader;
    tmp.dstHeader = req->dstHeader;
    tmp.imageSize = req->imageSize;
    tmp.updateRect = req->updateRect;
    if(req->fmt.extendFormat == 1) {
        // yuv420
        doYuvBlock(dst, dstPitch, src, srcPitch, width, height, 64, &tmp);
    }
    if(req->fmt.bitsPerPixel == 32) {
        doRgbBlock<quint32>(dst, dstPitch, src, srcPitch, width, height, 64, &tmp);
    }else if(req->fmt.bitsPerPixel == 16) {
        doRgbBlock<quint16>(dst, dstPitch, src, srcPitch, width, height, 64, &tmp);
    }else if(req->fmt.bitsPerPixel == 8) {
        doRgbBlock<quint8>(dst, dstPitch, src, srcPitch, width, height, 64, &tmp);
    }
    return writeZipData((uchar*)tmp.buf.data(), tmp.buf.length(), p);
}

int QKxVNCCompressAbort::doOpenH264(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    QKxH264Encoder *h264 = req->h264;
    width = req->imageSize.width();
    height = req->imageSize.height();
    int ystride = (width + 1 + 3) / 4 * 4;
    int ustride = (width + 1 + 3) / 4 * 2;
    int vstride = (width + 1 + 3) / 4 * 2;
    uchar *dstY = dst;
    uchar *dstU = dst + ystride * height;
    uchar *dstV = dstU + ustride * height / 2;
    QKxUtils::rgb32ToI420(dst, src, srcPitch, width, height);
    qint64 pos = ds.device()->pos();
    quint32 *pLength = reinterpret_cast<quint32*>(req->buf.data() + pos);
    ds << quint32(0) << quint32(3);
    int total = h264->encode(ds, dst);
    if(total >= 0) {
        pLength[0] = qToBigEndian<quint32>(total);
    }
    return 0;
}

bool QKxVNCCompressAbort::findRgbDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, PixelFormat &fmt, QRect *prt)
{
    int nrow = (height + 15) / 16;
    int ncol = (width + 15) / 16;
    int left = ncol + 1;
    int right = 0;
    int top = nrow + 1;
    int bottom = 0;
    int nbyte = fmt.bitsPerPixel / 8;

    // find top left.
    for(int r = 0; r < nrow && left > 0; r++) {
        int bheight = ((r+1) == nrow) ? (height % 16) : 16;
        if(bheight == 0) {
            bheight = 16;
        }
        for(int c = 0; c < ncol && c < left; c++) {
            int bwidth = ((c+1) == ncol) ? (width % 16 ) : 16;
            if(bwidth == 0) {
                bwidth = 16;
            }
            quint8 *p1 = reinterpret_cast<quint8*>(data1 + r * 16 * step1 + c * 16 * nbyte);
            quint8 *p2 = reinterpret_cast<quint8*>(data2 + r * 16 * step2 + c * 16 * nbyte);
            for(int bh = 0; bh < bheight; bh++) {
                if(memcmp(p1, p2, bwidth * nbyte) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                    break;
                }
                p1 += step1;
                p2 += step2;
            }
        }
    }

    // find bottom right.
    for(int r = nrow - 1; r > top && (right+1) != ncol; r--) {
        int bheight = ((r+1) == nrow) ? (height % 16) : 16;
        if(bheight == 0) {
            bheight = 16;
        }
        for(int c = ncol - 1; c > right; c--) {
            int bwidth = ((c+1) == ncol) ? (width % 16 ) : 16;
            if(bwidth == 0) {
                bwidth = 16;
            }
            quint8 *p1 = reinterpret_cast<quint8*>(data1 + r * 16 * step1 + c * 16 * nbyte);
            quint8 *p2 = reinterpret_cast<quint8*>(data2 + r * 16 * step2 + c * 16 * nbyte);
            for(int bh = 0; bh < bheight; bh++) {
                if(memcmp(p1, p2, bwidth * nbyte) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                    break;
                }
                p1 += step1;
                p2 += step2;
            }
        }
    }

    if(left > right || top > bottom) {
        return false;
    }

    QRect imgRt(0, 0, width, height);
    QRect rt(left * 16, top * 16, (right + 1 - left) * 16, (bottom + 1 - top) * 16);
    QRect dirty = imgRt.intersected(rt);
    //qDebug() << "bound rect" << left << top << right << bottom << ncol << nrow << dirty;
    if(prt != nullptr) {
        *prt = dirty;
    }
    return dirty.isValid();
}

bool QKxVNCCompressAbort::findYuvDirtyRect(uchar *data1, int step1, int _width, int _height, uchar *data2, int step2, QRect *prt)
{
    int width = _width;
    int height = _height;
    //int width = _width & ~0x1;
    //int height = _height & ~0x1;
    int nrow = (height + 15) / 16;
    int ncol = (width + 15) / 16;
    int left = ncol + 1;
    int right = 0;
    int top = nrow + 1;
    int bottom = 0;

    int ystride = (_width + 1 + 3) / 4 * 4;
    int uvstride = (_width + 1 + 3) / 4 * 4;

    uchar *y1 = data1;
    uchar *uv1 = data1 + ystride * (_height+1);
    uchar *y2 = data2;
    uchar *uv2 = data2 + ystride * (_height+1);

    // find top left.
    for(int r = 0; r < nrow && left > 0; r++) {
        int bheight = ((r+1) == nrow) ? (height % 16) : 16;
        if(bheight == 0) {
            bheight = 16;
        }
        for(int c = 0; c < ncol && c < left; c++) {
            int bwidth = ((c+1) == ncol) ? (width % 16 ) : 16;
            if(bwidth == 0) {
                bwidth = 16;
            }
            quint8 *py1 = reinterpret_cast<quint8*>(y1 + r * 16 * ystride + c * 16);
            quint8 *puv1 = reinterpret_cast<quint8*>(uv1 + r * 16 * uvstride + c * 16);
            quint8 *py2 = reinterpret_cast<quint8*>(y2 + r * 16 * ystride + c * 16);
            quint8 *puv2 = reinterpret_cast<quint8*>(uv2 + r * 16 * uvstride + c * 16);
            bool isSame = true;
            for(int bh = 0; bh < (bheight-1) / 2; bh++) {
                if(memcmp(py1, py2, bwidth) != 0
                        || memcmp(py1+ystride, py2+ystride, bwidth) != 0
                        || memcmp(puv1, puv2, (bwidth+1)/2*2) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                    isSame = false;
                    break;
                }
                py1 += ystride * 2;
                puv1 += uvstride;
                py2 += ystride * 2;
                puv2 += uvstride;
            }
            if(isSame && (bheight & 0x1)) {
                if(memcmp(py1, py2, bwidth) != 0
                        || memcmp(puv1, puv2, (bwidth+1)/2*2) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                }
            }
        }
    }

    // find bottom right.
    for(int r = nrow - 1; r > top && (right+1) != ncol; r--) {
        int bheight = ((r+1) == nrow) ? (height % 16) : 16;
        if(bheight == 0) {
            bheight = 16;
        }
        for(int c = ncol - 1; c > right; c--) {
            int bwidth = ((c+1) == ncol) ? (width % 16 ) : 16;
            if(bwidth == 0) {
                bwidth = 16;
            }
            quint8 *py1 = reinterpret_cast<quint8*>(y1 + r * 16 * ystride + c * 16);
            quint8 *puv1 = reinterpret_cast<quint8*>(uv1 + r * 16 * uvstride + c * 16);
            quint8 *py2 = reinterpret_cast<quint8*>(y2 + r * 16 * ystride + c * 16);
            quint8 *puv2 = reinterpret_cast<quint8*>(uv2 + r * 16 * uvstride + c * 16);
            bool isSame = true;
            for(int bh = 0; bh < (bheight-1)/2; bh++) {
                if(memcmp(py1, py2, bwidth) != 0
                        || memcmp(py1+ystride, py2+ystride, bwidth) != 0
                        || memcmp(puv1, puv2, (bwidth+1)/2*2) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                    isSame = false;
                    break;
                }
                py1 += ystride * 2;
                puv1 += uvstride;
                py2 += ystride * 2;
                puv2 += uvstride;
            }
            if(isSame && (bheight & 0x1)) {
                if(memcmp(py1, py2, bwidth) != 0
                        || memcmp(puv1, puv2, (bwidth+1)/2*2) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                }
            }
        }
    }

    if(left > right || top > bottom) {
        return false;
    }

    QRect imgRt(0, 0, width, height);
    QRect rt(left * 16, top * 16, (right + 1 - left) * 16, (bottom + 1 - top) * 16);
    QRect dirty = imgRt.intersected(rt);
    //qDebug() << "bound rect" << left << top << right << bottom << ncol << nrow << dirty;
    if(prt != nullptr) {
        *prt = dirty;
    }
    return dirty.isValid();
}
