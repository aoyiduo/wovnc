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

#ifndef QKXDIRTYFRAME_H
#define QKXDIRTYFRAME_H

#include <QObject>
#include <QVector>
#include <QList>
#include <QRect>

class QKxDirtyFrame : public QObject
{
public:
    explicit QKxDirtyFrame(int width, int height, QObject *parent = nullptr);
    void setJpegDetectThreshold(int scoreLess);
    bool findDirtyRect(uchar *dataLastFrame, int stepLastFrame, uchar *dataFrame, int stepFrame);
    QList<QRect> dirtyRects();
    bool jpegPreffer(uchar *pbits, int step, int idxRt);
    bool h264Preffer(uchar *pbits, int step, int idxRt);
private:
    void mergeRect(QList<QRect>& dirtyRt, const QRect &rt);
    int checkRoughness(QVector<int> &pts, int pixelCount);
    void detectImageSmooth(uchar *pbits, int bytesPerline, int bsize, int x, int y, int width, int height, QVector<int> &pts);
private:
    struct DirtyFrame {
        QVector<bool> block; // dirty block
        QRect blockRt; // block dirty bound rect

        QList<QRect> dirtyRt; // dirty Rect.
        QRect boundRt; // image dirty bound rect

        int score; // dirty level score, [0,100]
        int roughness; // dirty area roughness level.
        qint64 time; //dirty timestamp, [0, ~]
    };
    QList<DirtyFrame> m_frames;
    int m_width, m_height;
    int m_ncol, m_nrow;
    int m_timeLast;
    int m_jpegThreshold;
    int m_jpegPrefferCount;
};

#endif // QKXDIRTYFRAME_H
