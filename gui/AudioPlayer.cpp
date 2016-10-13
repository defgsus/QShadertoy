/***************************************************************************

Copyright (C) 2016  stefan.berke @ modular-audio-graphics.com

This source is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

****************************************************************************/

#include <limits>

#include <QDebug>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QBuffer>
#include <QMap>

#include "AudioPlayer.h"

struct AudioPlayer::Private
{
    Private(AudioPlayer* p)
        : p         (p)
    { }

    struct Sample
    {
        Sample(size_t numSamples, size_t numChannels)
            : data      (numSamples * numChannels * sizeof(float),
                         Qt::Uninitialized)
            , stream    (nullptr)
            , lentDevice(nullptr)
            , audio     (nullptr)
        { }

        ~Sample()
        {
            qDebug() << "destroy " << (void*)this;
            delete stream;
        }

        QByteArray data;
        QBuffer* stream;
        QIODevice* lentDevice;
        QAudioFormat format;
        QAudioOutput* audio;
    };

    QAudioFormat getFormat(size_t numChannels, size_t sampleRate);

    void remove(Sample* s);
    void removeAll();

    AudioPlayer* p;
    QMap<QAudioOutput*, Sample*> sampleMap;
};


AudioPlayer::AudioPlayer(QObject *parent)
    : QObject   (parent)
    , p_        (new Private(this))
{

}

AudioPlayer::~AudioPlayer()
{
    p_->removeAll();
    delete p_;
}

void AudioPlayer::stop() { p_->removeAll(); }



void AudioPlayer::Private::remove(Sample* s)
{
    if (s->audio)
    {
        s->audio->stop();
        s->audio->deleteLater();
    }
    sampleMap.remove(s->audio);
    delete s;
}

void AudioPlayer::Private::removeAll()
{
    while (!sampleMap.isEmpty())
    {
        Sample* s = sampleMap.first();
        remove(s);
    }
}


QAudioFormat AudioPlayer::Private::getFormat(
        size_t numChannels, size_t sampleRate)
{
    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(numChannels);
    format.setSampleSize(32);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::Float);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    format.setByteOrder(QAudioFormat::LittleEndian);
#else
    format.setByteOrder(QAudioFormat::BigEndian);
#endif

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format))
    {
        qWarning() << "Audio format not supported by backend, "
                      "cannot play audio.";
        return QAudioFormat();
    }
    return format;
}

bool AudioPlayer::play(const float* samplesFloat, size_t numSamples,
                        size_t numChannels, size_t sampleRate)
{
    // create sample instance and check format
    auto sample = new Private::Sample(numSamples, numChannels);
    sample->format = p_->getFormat(numChannels, sampleRate);
    if (!sample->format.isValid())
    {
        delete sample;
        return false;
    }

    // copy data
    memcpy(sample->data.data(), samplesFloat, sample->data.size());

    // create iodevice
    sample->stream = new QBuffer(nullptr, nullptr);
    sample->stream->setBuffer(&sample->data);
    sample->stream->open(QIODevice::ReadOnly);

    sample->audio = new QAudioOutput(sample->format, this);
    //sample->audio->setBufferSize(1024);
    connect(sample->audio, &QAudioOutput::stateChanged,
    [=](QAudio::State state)
    {
        switch (state)
        {
            case QAudio::ActiveState: qDebug() << "active"; break;
            case QAudio::SuspendedState: qDebug() << "suspended"; break;
            case QAudio::IdleState:
                qDebug() << "idle";
                p_->remove(sample);
            break;
            case QAudio::StoppedState:
                qDebug() << "stopped";
            break;
        }
    });

    p_->sampleMap.insert(sample->audio, sample);

    sample->audio->start(sample->stream);

    return true;
}

bool AudioPlayer::play(QIODevice* lentDevice,
                        size_t numChannels, size_t sampleRate)
{
    // create sample instance and check format
    auto sample = new Private::Sample(0, numChannels);
    sample->format = p_->getFormat(numChannels, sampleRate);
    if (!sample->format.isValid())
    {
        delete sample;
        return false;
    }

    // copy iodevice
    sample->lentDevice = lentDevice;
    if (!sample->lentDevice->isOpen())
        sample->lentDevice->open(QIODevice::ReadOnly);

    sample->audio = new QAudioOutput(sample->format, this);
    connect(sample->audio, &QAudioOutput::stateChanged,
    [=](QAudio::State state)
    {
        switch (state)
        {
            case QAudio::ActiveState: qDebug() << "active"; break;
            case QAudio::SuspendedState: qDebug() << "suspended"; break;
            case QAudio::IdleState:
                qDebug() << "idle";
                //p_->remove(sample);
                sample->audio->start(sample->lentDevice);
            break;
            case QAudio::StoppedState:
                qDebug() << "stopped";
            break;
        }
    });

    p_->sampleMap.insert(sample->audio, sample);

    sample->audio->start(sample->lentDevice);

    return true;
}

