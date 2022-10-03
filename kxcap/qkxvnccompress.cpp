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

#include "qkxvnccompress.h"

#include "qkxzip.h"
#include "qkxh264encoder.h"
#include "qkxjpegencoder.h"
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

static inline quint32 toPixelFormat(uchar *clr, PixelFormat &fmt)
{
#if 1
    if(fmt.depth == 16) {
        quint32 b = clr[0] >> 3;
        quint32 g = clr[1] >> 2;
        quint32 r = clr[2] >> 3;
        quint32 rgb = (r << fmt.redShift) | (g << fmt.greenShift) | (b << fmt.blueShift);
        return rgb;
    }else if(fmt.depth == 15) {
        quint32 b = clr[0] >> 3;
        quint32 g = clr[1] >> 3;
        quint32 r = clr[2] >> 3;
        quint32 rgb = (r << fmt.redShift) | (g << fmt.greenShift) | (b << fmt.blueShift);
        return rgb;

    }else if(fmt.depth == 8) {
        quint32 b = clr[0] >> 6;
        quint32 g = clr[1] >> 5;
        quint32 r = clr[2] >> 5;
        quint32 rgb = (r << fmt.redShift) | (g << fmt.greenShift) | (b << fmt.blueShift);
        return rgb;
    }
    return 0;
#else
    quint32 tmpr = (double)r / (double)0xFF * fmt.redMax;
    quint32 tmpg = (double)g / (double)0xFF * fmt.greenMax;
    quint32 tmpb = (double)b / (double)0xFF * fmt.blueMax;

    quint32 rgb = (tmpr << fmt.redShift) | (tmpg << fmt.greenShift) | (tmpb << fmt.blueShift);
    return rgb;
#endif
}

static inline quint32 toCPixelColor(quint32 clr, PixelFormat &fmt)
{
    if(fmt.trueColor) {
        if(fmt.depth == 24) {
            //RGB format is 888.
            return clr;
        }else if(fmt.depth == 16) {
            // RGB format is 565.
            return toPixelFormat((uchar*)&clr, fmt);
        }else if(fmt.depth == 15) {
            //RGB format is 555.
            return toPixelFormat((uchar*)&clr, fmt);
        }else {
            //RGB format is 332.
            return toPixelFormat((uchar*)&clr, fmt);
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

bool QKxVNCCompress::findDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, QRect *prt)
{
    return findRgbDirtyRect(data1, step1, width, height, data2, step2, prt);
}

int QKxVNCCompress::writeZipData(uchar *src, int cnt, void *p)
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

EEncodingType QKxVNCCompress::matchBest(quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    int wanted = width * height * req->fmt.bitsPerPixel / 8;

    if(req->codes.contains(ET_OpenH264) && req->fmt.extendFormat == 2) {
        return ET_OpenH264;
    }else if(req->codes.contains(ET_JPEG) && req->fmt.extendFormat == 3) {
        return ET_JPEG;
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

int QKxVNCCompress::doRaw(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    for(quint16 h = 0; h < height; h++) {
        quint32 *psrc = reinterpret_cast<quint32*>(src + h * srcPitch);
        quint32 *pdst = reinterpret_cast<quint32*>(dst + h * dstPitch);
        for(quint16 w = 0; w < width;  w ++) {
            quint32 clr = toCPixelColor(psrc[w], req->fmt);
            pdst[w] = psrc[w];
            writePixelColor(ds, clr, req->fmt);
        }
    }
    return 0;
}

int QKxVNCCompress::doHextile(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    for(quint16 h = 0; h < height; h += 16) {
        quint16 bheight = qMin<quint16>(h + 16, height) - h;
        for(quint16 w = 0; w < width;  w += 16) {
            quint16 bwidth = qMin<quint16>(w + 16, width) - w;
            ds << quint8(0x1);
            for(quint16 bh = 0; bh < bheight; bh++) {
                quint32 *psrc = reinterpret_cast<quint32*>(src + (h + bh) * srcPitch);
                quint32 *pdst = reinterpret_cast<quint32*>(dst + (h + bh) * dstPitch);
                psrc += w;
                pdst += w;
                for(quint16 bw = 0;  bw < bwidth; bw++) {
                    quint32 clr = toCPixelColor(psrc[bw], req->fmt);
                    pdst[bw] = psrc[bw];
                    writePixelColor(ds, clr, req->fmt);
                }
            }
        }
    }
    return 0;
}

int QKxVNCCompress::doRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, quint16 bsize, bool rleSkip, void *p)
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
                quint32 *psrc = reinterpret_cast<quint32*>(src + (h + bh) * srcPitch);
                quint32 *pdst = reinterpret_cast<quint32*>(dst + (h + bh) * dstPitch);
                psrc += w;
                pdst += w;
                /* example, length 1 is represented as [0], 255 as [254], 256 as
                 [255,0], 257 as [255,1], 510 as [255,254], 511 as [255,255,0], and
                 so on. */
                for(quint16 bw = 0; bw < bwidth; bw++) {
                    quint32 clrSrc = toCPixelColor(psrc[bw], req->fmt);
                    quint32 clrDst = toCPixelColor(pdst[bw], req->fmt);
                    pdst[bw] = psrc[bw];
                    if(subcode != 128){
                        if(clrSrc != clrDst){
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

int QKxVNCCompress::doTRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    return doRLE(dst, dstPitch, src, srcPitch, width, height, 16, false, p);
}

int QKxVNCCompress::doZRLE(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
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
    doRLE(dst, dstPitch, src, srcPitch, width, height, 64, false, &tmp);
    return writeZipData((uchar*)tmp.buf.data(), tmp.buf.length(), p);
}

int QKxVNCCompress::doTRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    return doRLE(dst, dstPitch, src, srcPitch, width, height, 16, true, p);
}

int QKxVNCCompress::doZRLE2(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
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
    doRLE(dst, dstPitch, src, srcPitch, width, height, 64, true, &tmp);
    return writeZipData((uchar*)tmp.buf.data(), tmp.buf.length(), p);
}

int QKxVNCCompress::doRgbBlock(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, quint16 bsize, void *p)
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
                quint32 *psrc = reinterpret_cast<quint32*>(src + (h + bh) * srcPitch);
                quint32 *pdst = reinterpret_cast<quint32*>(dst + (h + bh) * dstPitch);
                psrc += w;
                pdst += w;
                for(quint16 bw = 0; bw < bwidth; bw++) {
                    quint32 clrSrc = toCPixelColor(psrc[bw], req->fmt);
                    quint32 clrDst = toCPixelColor(pdst[bw], req->fmt);
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

int QKxVNCCompress::doTRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    return doRgbBlock(dst, dstPitch, src, srcPitch, width, height, 16, p);
}

int QKxVNCCompress::doZRLE3(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
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
    doRgbBlock(dst, dstPitch, src, srcPitch, width, height, 64, &tmp);
    return writeZipData((uchar*)tmp.buf.data(), tmp.buf.length(), p);
}

int QKxVNCCompress::doOpenH264(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    QKxH264Encoder *h264 = req->h264;
    width = req->imageSize.width();
    height = req->imageSize.height();
    int ystride = (width + 3) / 4 * 4;
    int ustride = ystride / 2;
    int vstride = ustride;
    uchar *dstY = dst;
    uchar *dstU = dst + ystride * height;
    uchar *dstV = dstU + ustride * height / 2;
    QKxUtils::rgb32ToI420(dst, src, srcPitch, width, height);
    qint64 pos = ds.device()->pos();
    quint32 *pLength = reinterpret_cast<quint32*>(req->buf.data() + pos);
    ds << quint32(0) << quint32(3);
    int etype = h264->encode(ds, dst);
    int total = ds.device()->pos() - pos - 8;
    if(etype >= 0) {
        pLength[0] = qToBigEndian<quint32>(total);
        pLength[1] = qToBigEndian<quint32>(etype == 1 ? 0 : 3);
    }
    return 0;
}

int QKxVNCCompress::doOpenJpeg(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p)
{
    UpdateRequest *req = reinterpret_cast<UpdateRequest*>(p);
    QDataStream &ds = req->ds;
    QKxJpegEncoder enc;
    if(!enc.encode(ds, src, srcPitch, width, height, 80)) {
        return 0;
    }
    for(int h = 0; h < height; h++) {
        memcpy(dst, src, width * 4);
        dst += dstPitch;
        src += srcPitch;
    }
    return 0;
}

bool QKxVNCCompress::findRgbDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, QRect *prt)
{
    int nrow = (height + 15) / 16;
    int ncol = (width + 15) / 16;
    int left = ncol + 1;
    int right = 0;
    int top = nrow + 1;
    int bottom = 0;
    int nbyte = 4;

    // find top left.
    bool isSame = true;
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
                    isSame = false;
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    break;
                }
                p1 += step1;
                p2 += step2;
            }
        }
    }
    if(isSame) {
        return false;
    }
    right = left;
    bottom = top;
    // find bottom right.
    for(int r = nrow - 1; r >= top && (right+1) != ncol; r--) {
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
    QRect rt(left * 16, top * 16, (right - left + 1) * 16, (bottom - top + 1) * 16);
    QRect dirty = imgRt.intersected(rt);
    dirty.setRect(0, 0, width, height);
   // qDebug() << "bound rect" << left << top << right << bottom << ncol << nrow << dirty;
    if(prt != nullptr) {
        *prt = dirty;
    }
    return dirty.isValid();
}
