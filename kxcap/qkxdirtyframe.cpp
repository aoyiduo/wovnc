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

#include "qkxdirtyframe.h"

#include <QDateTime>
#include <QDebug>

QKxDirtyFrame::QKxDirtyFrame(int width, int height, QObject *parent)
    : QObject(parent)
    , m_width(width)
    , m_height(height)
{
    m_frames.reserve(3);
    m_timeLast = QDateTime::currentMSecsSinceEpoch();
    m_jpegThreshold = 70;
    m_nrow = (m_height + 15) / 16;
    m_ncol = (m_width + 15) / 16;
}

void QKxDirtyFrame::setJpegDetectThreshold(int scoreLess)
{
    m_jpegThreshold = scoreLess;
}

bool QKxDirtyFrame::findDirtyRect(uchar *dataLastFrame, int stepLastFrame, uchar *dataFrame, int stepFrame)
{
    int left = m_ncol + 1;
    int right = 0;
    int top = m_nrow + 1;
    int bottom = 0;
    int nbyte = 4;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if(now - m_timeLast > 1000) {
        m_timeLast = now;
        m_frames.clear();
    }

    DirtyFrame df;
    df.block.fill(false, m_nrow * m_ncol);
    df.time = now;
    df.roughness = -1;
    int dirtyTotal = 0;
    for(int r = 0; r < m_nrow; r++) {
        int bheight = ((r+1) == m_nrow) ? (m_height % 16) : 16;
        if(bheight == 0) {
            bheight = 16;
        }
        for(int c = 0; c < m_ncol; c++) {
            int bwidth = ((c+1) == m_ncol) ? (m_width % 16 ) : 16;
            if(bwidth == 0) {
                bwidth = 16;
            }
            quint8 *p1 = reinterpret_cast<quint8*>(dataLastFrame + r * 16 * stepLastFrame + c * 16 * nbyte);
            quint8 *p2 = reinterpret_cast<quint8*>(dataFrame + r * 16 * stepFrame + c * 16 * nbyte);
            for(int bh = 0; bh < bheight; bh++) {
                if(memcmp(p1, p2, bwidth * nbyte) != 0) {
                    df.block[r * m_ncol + c] = true;
                    dirtyTotal++;
                    if(c < left) {
                        left = c;
                    }
                    if(r < top) {
                        top = r;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                    mergeRect(df.dirtyRt, QRect(c * 16, r * 16, bwidth, bheight));
                    break;
                }
                p1 += stepLastFrame;
                p2 += stepFrame;
            }
        }
    }
    if(dirtyTotal == 0) {
        return false;
    }

    df.blockRt.setCoords(left, top, right, bottom);
    df.boundRt.setRect(df.blockRt.left() * 16, df.blockRt.top() * 16,
                       df.blockRt.width() * 16, df.blockRt.height() * 16);
    df.boundRt = df.boundRt.intersected(QRect(0, 0, m_width, m_height));

    int total = df.blockRt.width() * df.blockRt.height();
    df.score = dirtyTotal * 100 / total;    
    for(auto it = df.dirtyRt.begin(); it != df.dirtyRt.end(); it++) {
        QRect &rt = *it;
        rt.adjust(16, 16, -16, -16);
    }
#if 0
    QRect boundRt;
    for(auto it = df.dirtyRt.begin(); it != df.dirtyRt.end(); it++) {
        QRect &rt = *it;
        boundRt = boundRt.united(rt);
    }
    if(df.boundRt != boundRt) {
        qDebug() << "maybe error" << df.boundRt << boundRt;
    }
    qDebug() << "dirtyRect" << df.dirtyRt;
#endif
    m_frames.append(df);
    if(m_frames.length() > 5) {
        m_frames.pop_front();
    }
    return dirtyTotal > 0;
}

QList<QRect> QKxDirtyFrame::dirtyRects()
{
    if(m_frames.isEmpty()) {
        return QList<QRect>();
    }
    return m_frames.last().dirtyRt;
}

bool QKxDirtyFrame::jpegPreffer(uchar *pbits, int step, int idx)
{
    const DirtyFrame& df = m_frames.last();
    QRect rt = df.dirtyRt[idx];
    if(rt.width() * rt.height() < 200 * 200) {
        return false;
    }
    int left = rt.left() / 16;
    int top = rt.top() / 16;
    int right = left + (rt.width() + 15) / 16;
    int bottom = top + (rt.height() + 15) / 16;
    int total = 0;
    int diff = 0;
    for(int r = top; r < bottom; r++) {
        for(int c = left; c < right; c++) {
            total++;
            if(df.block[r * m_ncol + c]) {
                diff++;
            }
        }
    }
    if(diff * 100 / total > 70) {
        return false;
    }
    QVector<int> pts;
    detectImageSmooth(pbits, step, 8, rt.left(), rt.top(), rt.width(), rt.height(), pts);
    int v = checkRoughness(pts, rt.width() * rt.height());
    if(v == 0) {
        return false;
    }
    return true;
}

bool QKxDirtyFrame::h264Preffer(uchar *pbits, int step, int idx)
{

    return m_jpegPrefferCount > 10;
}

void QKxDirtyFrame::mergeRect(QList<QRect>& dirtyRt, const QRect &tmp)
{
    QRect rt = tmp.adjusted(-16, -16, 16, 16);
    for(auto it = dirtyRt.begin(); it != dirtyRt.end(); ) {
        const QRect& tmp = *it;
        if(rt.intersects(tmp)) {
            rt = rt.united(tmp);
            it = dirtyRt.erase(it);
        }else{
            it++;
        }
    }
    dirtyRt.append(rt);
}

int QKxDirtyFrame::checkRoughness(QVector<int> &pts, int pixelCount)
{
    // 250 * 250
    if(pixelCount < (250 * 250)) {
        return 0;
    }
    int val = pts[0] * 33 / pixelCount ;
    if(val >= 80) {
        // 80% area is same color.
        return 0;
    }
    if(pts[0] < pts[1]) {
        return 1;
    }
    return 1;
}

void QKxDirtyFrame::detectImageSmooth(uchar *pbits, int bytesPerline, int bsize, int x, int y, int width, int height, QVector<int> &pts)
{
    int left[3];
    pts.resize(256);
    pts.fill(0);
    int *diff = pts.data();
    for(int h = 0; h < height; h+=bsize) {
        int bheight = qMin<int>(h + bsize, y + height) - h;
        uchar *pLine = pbits + h * bytesPerline;
        for(int w = 0; w < width; w += bsize) {
            int bwidth = qMin<int>(w + bsize, x + width) - w;
            uchar *pBlock = pLine + w * 4;
            for(int bh = 0; bh < bheight; bh++) {
                uchar *rgb = pBlock + bh * bytesPerline;
                left[0] = rgb[0] & 0xFF;
                left[1] = rgb[1] & 0xFF;
                left[2] = rgb[2] & 0xFF;
                rgb += 4;
                for(int bw = 1; bw < bwidth; bw++) {
                    quint8 v0 = qAbs(rgb[0] - left[0]);
                    diff[v0]++;
                    left[0] = rgb[0];
                    quint8 v1 = qAbs(rgb[1] - left[1]);
                    diff[v1]++;
                    left[1] = rgb[1];
                    quint8 v2 = qAbs(rgb[2] - left[2]);
                    diff[v2]++;
                    left[2] = rgb[2];
                    rgb += 4;
                }
            }
        }
    }
}

