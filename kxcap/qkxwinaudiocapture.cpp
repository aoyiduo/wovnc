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

#include "qkxwinaudiocapture.h"

#include <portaudio.h>

QKxWinAudioCapture::QKxWinAudioCapture(QObject *parent)
    : QKxAudioCapture(parent)
{

}

QKxWinAudioCapture::~QKxWinAudioCapture()
{

}

int QKxWinAudioCapture::findDefaultLoopbackDevice()
{
    int idx = Pa_GetDefaultOutputDevice();
    if(idx == paNoDevice) {
        return paNoDevice;
    }
    const PaDeviceInfo *pdi = Pa_GetDeviceInfo(idx);
    QString name = QString::fromUtf8(pdi->name);
    PaDeviceIndex numDevices = Pa_GetDeviceCount();
    for(int i = 0; i < numDevices; i++) {
        const PaDeviceInfo *hit = Pa_GetDeviceInfo(i);
        QString nameHit = QString::fromUtf8(hit->name);
        if(nameHit.contains("[Loopback]") && nameHit.startsWith(name) && hit->maxInputChannels > 0) {
            return i;
        }
    }
    return paNoDevice;
}
