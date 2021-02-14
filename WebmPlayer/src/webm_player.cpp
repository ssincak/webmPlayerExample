/*
*  Copyright (c) 2016 Stefan Sincak. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree.
*/

#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include "webm_player.hpp"

#include <SDL2/SDL_syswm.h>
#include <iostream>

namespace wp
{
    // called by libWebmPlayer when new PCM data is available
    void OnAudioData(void *userPtr, float *pcm, size_t count)
    {
        WebmPlayer *player = static_cast<WebmPlayer*>(userPtr);

        player->insertAudioData(pcm, count);
    }

    // called by SDL when audio device wants PCM data
    void sdlAudioCallback(void *userPtr, Uint8 *stream, int len)
    {
        WebmPlayer *player = static_cast<WebmPlayer*>(userPtr);

        player->readAudioData((float*)stream, len / sizeof(float));
    }

    WebmPlayer::WebmPlayer()
    {
    }

    WebmPlayer::~WebmPlayer()
    {
    }

    bool  WebmPlayer::load(const char *fileName)
    {
        if (!loadVideo(fileName))
            return false;

        if (!initSDL(fileName, m_video->info().width, m_video->info().height))
            return false;

        if (!initAudio())
            return false;

        // print some video stats
        printInfo();

        return true;
    }

    bool WebmPlayer::loadVideo(const char *fileName)
    {
        m_video = new uvpx::Player(uvpx::Player::defaultConfig());

        uvpx::Player::LoadResult res = m_video->load(fileName, 0, false);

        switch (res)
        {
        case uvpx::Player::LoadResult::FileNotExists:
            log("Failed to open video file '%s'", fileName);
            return false;

        case uvpx::Player::LoadResult::UnsupportedVideoCodec:
            log("Unsupported video codec");
            return false;

        case uvpx::Player::LoadResult::NotInitialized:
            log("Video player not initialized");
            return false;

        case uvpx::Player::LoadResult::Success:
            log("Video loaded successfully");
            break;

        default:
            log("Failed to load video (%d)", (int)res);
            return false;
        }

        m_video->setOnAudioData(OnAudioData, this);

        return true;
    }

    bool WebmPlayer::initSDL(const char *windowTitle, int width, int height)
    {
        if (SDL_Init(
            SDL_INIT_TIMER |
            SDL_INIT_AUDIO |
            SDL_INIT_VIDEO |
            SDL_INIT_EVENTS) != 0)
        {
            log("Failed to initialize SDL (%s)", SDL_GetError());
            return false;
        }

        // create window
        m_window = SDL_CreateWindow(
            windowTitle,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width/2,
            height/2,
			SDL_WINDOW_SHOWN | SDL_WINDOW_MINIMIZED);
        if (!m_window)
        {
            log("Could not create SDL window (%s)", SDL_GetError());
            return false;
        }

        // create renderer
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_renderer)
        {
            log("Could not create SDL renderer (%s)", SDL_GetError());
            return false;
        }

        // create video texture
        m_texture = SDL_CreateTexture(
            m_renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
            );
        if (!m_texture)
        {
            log("Could not create SDL video texture (%s)", SDL_GetError());
            return false;
        }

        return true;
    }

    void  WebmPlayer::run()
    {
        if (m_video == nullptr)
            return;

        // start playing immediatelly
        playback(Play);

        bool running = true;

        Uint32 startTime = SDL_GetTicks();
        float dt = 1.0f / 60.0f;

        uvpx::Frame *yuv = nullptr;

        while (running)
        {
            // handle SDL events
            SDL_Event evt;
            while (SDL_PollEvent(&evt))
            {
                switch (evt.type)
                {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                {
                    switch (evt.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                        running = false;
                        break;

                    case SDLK_SPACE:
                        if (m_video->isPlaying())
                            playback(Pause);
                        else if (m_video->isPaused())
                            playback(Resume);
                        else if (m_video->isStopped())
                            playback(Play);

                        break;

                    case SDLK_s:
                        playback(Stop);
                        break;
                    }

                    break;
                }

                default:
                    break;
                }
            }

            // update video
            m_video->update(dt);

            // check if we have new YUV frame
            if ((yuv = m_video->lockRead()) != nullptr)
            {
                SDL_UpdateYUVTexture(
                    m_texture,
                    NULL,
                    yuv->y(),
                    yuv->yPitch(),
                    yuv->u(),
                    yuv->uvPitch(),
                    yuv->v(),
                    yuv->uvPitch()
                    );

                m_video->unlockRead();

                SDL_RenderClear(m_renderer);
                SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
                SDL_RenderPresent(m_renderer);
            }

            // calc elapsed time
            Uint32 endTime = SDL_GetTicks();
            dt = (endTime - startTime) / 1000.0f;
            startTime = endTime;
        }
    }

    void  WebmPlayer::destroy()
    {
        if (m_video)
            delete m_video;

        if(m_audioDevice != 0)
        {
            SDL_CloseAudioDevice(m_audioDevice);
            m_audioDevice = 0;
        }

        if (m_texture)
        {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }

        if (m_renderer)
        {
            SDL_DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }

        if (m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_Quit();
    }

    void  WebmPlayer::log(const char *format, ...)
    {
        using namespace std;

        char buffer[512];

        va_list arglist;
        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        cout << buffer << endl;
    }

    void  WebmPlayer::printInfo()
    {
        uvpx::Player::VideoInfo info = m_video->info();

        log("Clip info:\n"
            "- width = %d\n"
            "- height = %d\n"
            "- duration = %f (sec.)\n"
            "- has audio = %s\n"
            "- audio channels = %d\n"
            "- audio frequency = %d\n"
            "- audio samples = %d\n"
            "- decode threads count = %d\n"
            "- frame rate = %f\n",
            info.width,
            info.height,
            info.duration,
            info.hasAudio ? "yes" : "no",
            info.audioChannels,
            info.audioFrequency,
            info.audioSamples,
            info.decodeThreadsCount,
            info.frameRate);
    }

    bool WebmPlayer::initAudio()
    {
        uvpx::Player::VideoInfo info = m_video->info();

        if (!info.hasAudio)
            return true;

        SDL_AudioSpec wanted_spec, obtained_spec;

        wanted_spec.freq = info.audioFrequency;
        wanted_spec.format = AUDIO_F32;
        wanted_spec.channels = info.audioChannels;
        wanted_spec.silence = 0;
        wanted_spec.samples = 4096;
        wanted_spec.callback = sdlAudioCallback;
        wanted_spec.userdata = this;

        m_audioDevice = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &obtained_spec, 0);
        if (m_audioDevice == 0)
        {
            log("Could not create SDL audio (%s).", SDL_GetError());
            return false;
        }
        else if (wanted_spec.format != obtained_spec.format)
        {
            log("Wanted and obtained SDL audio formats are different! (%d : %d)", wanted_spec.format, obtained_spec.format);
            return false;
        }

        SDL_PauseAudioDevice(m_audioDevice, 0);

        return true;
    }

    void  WebmPlayer::insertAudioData(float *src, size_t count)
    {
        std::lock_guard<std::mutex> lock(m_audioMutex);

        m_audioBuffer.resize(m_audioBuffer.size() + count);
        memcpy(&m_audioBuffer[m_audioBuffer.size() - count], src, count * sizeof(float));
    }

    bool  WebmPlayer::readAudioData(float *dst, size_t count)
    {
        std::lock_guard<std::mutex> lock(m_audioMutex);

        if (m_audioBuffer.size() < count)
            return false;

        memcpy(dst, &m_audioBuffer[0], count * sizeof(float));

        m_audioBuffer.erase(m_audioBuffer.begin(), m_audioBuffer.begin() + count);

        return true;
    }

    void  WebmPlayer::clearAudioData()
    {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        m_audioBuffer.clear();
    }

    void  WebmPlayer::playback(PlaybackCommand cmd)
    {
        switch (cmd)
        {
        case PlaybackCommand::Play:
            m_video->play();
            SDL_PauseAudioDevice(m_audioDevice, 0);
            break;

        case PlaybackCommand::Pause:
            m_video->pause();
            SDL_PauseAudioDevice(m_audioDevice, 1);
            break;

        case PlaybackCommand::Resume:
            m_video->play();
            SDL_PauseAudioDevice(m_audioDevice, 0);
            break;

        case PlaybackCommand::Stop:
            m_video->stop();
            SDL_PauseAudioDevice(m_audioDevice, 1);
            SDL_ClearQueuedAudio(m_audioDevice);
            clearAudioData();
            break;
        }
    }
}
