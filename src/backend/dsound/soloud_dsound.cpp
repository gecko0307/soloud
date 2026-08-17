/*
SoLoud audio engine
Copyright (c) 2013-2018 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/
#include "soloud.h"

#if !defined(WITH_DSOUND)

namespace SoLoud
{
    result dsound_init(
        SoLoud::Soloud * aSoloud,
        unsigned int aFlags,
        unsigned int aSamplerate,
        unsigned int aBufferSize,
        unsigned int aChannels
    )
    {
        return NOT_IMPLEMENTED;
    }
}

#else

#define INITGUID
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <process.h>
#include "soloud_thread.h"

namespace SoLoud
{
    struct DSoundData
    {
        LPDIRECTSOUND8      mDS;
        LPDIRECTSOUNDBUFFER mPrimaryBuffer;
        LPDIRECTSOUNDBUFFER mSecondaryBuffer;
        Thread::ThreadHandle mThread;
        bool                mRunning;
        unsigned int        mBufferSize;
        unsigned int        mChannels;
        SoLoud::Soloud*     mSoloud;
        unsigned int        mLastWritePos;
    };

    static DSoundData gData = { 0 };

    static void dsoundThread(void* aParam)
    {
        DSoundData* data = (DSoundData*)aParam;
        unsigned int bufferSize = data->mBufferSize;

        while (data->mRunning)
        {
            DWORD playPos = 0, writePos = 0;
            data->mSecondaryBuffer->GetCurrentPosition(&playPos, &writePos);

            DWORD bytesToBytes = 0;
            if (playPos >= data->mLastWritePos)
                bytesToBytes = playPos - data->mLastWritePos;
            else
                bytesToBytes = bufferSize - (data->mLastWritePos - playPos);

            if (bytesToBytes >= bufferSize / 2)
            {
                DWORD chunkBytes = bytesToBytes;
                void *ptr1 = NULL, *ptr2 = NULL;
                DWORD size1 = 0, size2 = 0;

                if (SUCCEEDED(data->mSecondaryBuffer->Lock(data->mLastWritePos, chunkBytes, &ptr1, &size1, &ptr2, &size2, 0)))
                {
                    unsigned int samples1 = size1 / (sizeof(short) * data->mChannels);
                    unsigned int samples2 = size2 / (sizeof(short) * data->mChannels);

                    if (samples1 > 0)
                        data->mSoloud->mixSigned16((short*)ptr1, samples1);

                    if (samples2 > 0)
                        data->mSoloud->mixSigned16((short*)ptr2, samples2);

                    data->mSecondaryBuffer->Unlock(ptr1, size1, ptr2, size2);
                    data->mLastWritePos = (data->mLastWritePos + size1 + size2) % bufferSize;
                }
            }

            Thread::sleep(2);
        }
    }

    static void dsoundDeinit(SoLoud::Soloud *aSoloud)
    {
        if (gData.mRunning)
        {
            gData.mRunning = false;
            Thread::wait(gData.mThread);
        }

        if (gData.mSecondaryBuffer) gData.mSecondaryBuffer->Release();
        if (gData.mPrimaryBuffer) gData.mPrimaryBuffer->Release();
        if (gData.mDS) gData.mDS->Release();

        memset(&gData, 0, sizeof(DSoundData));
    }

    result dsound_init(SoLoud::Soloud *aSoloud, unsigned int aFlags, unsigned int aSamplerate, unsigned int aBufferSize, unsigned int aChannels)
    {
        if (FAILED(DirectSoundCreate8(NULL, &gData.mDS, NULL)))
            return UNKNOWN_ERROR;
        
        HWND hwnd = GetForegroundWindow();
        if (!hwnd)
            hwnd = GetDesktopWindow();
        if (FAILED(gData.mDS->SetCooperativeLevel(hwnd, DSSCL_PRIORITY)))
            return UNKNOWN_ERROR;

        DSBUFFERDESC dsbd = { 0 };
        dsbd.dwSize = sizeof(DSBUFFERDESC);
        dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
        if (FAILED(gData.mDS->CreateSoundBuffer(&dsbd, &gData.mPrimaryBuffer, NULL)))
            return UNKNOWN_ERROR;

        WAVEFORMATEX wfx = { 0 };
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = (WORD)aChannels;
        wfx.nSamplesPerSec = aSamplerate;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        if (FAILED(gData.mPrimaryBuffer->SetFormat(&wfx)))
            return UNKNOWN_ERROR;

        // Use a larger hardware buffer to provide enough
        // headroom for the streaming thread.
        gData.mBufferSize = aBufferSize * wfx.nBlockAlign * 2;
        
        DSBUFFERDESC dsbd2 = { 0 };
        dsbd2.dwSize = sizeof(DSBUFFERDESC);
        dsbd2.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
        dsbd2.dwBufferBytes = gData.mBufferSize;
        dsbd2.lpwfxFormat = &wfx;

        if (FAILED(gData.mDS->CreateSoundBuffer(&dsbd2, &gData.mSecondaryBuffer, NULL)))
            return UNKNOWN_ERROR;

        void* ptr = NULL; DWORD size = 0;
        if (SUCCEEDED(gData.mSecondaryBuffer->Lock(0, gData.mBufferSize, &ptr, &size, NULL, NULL, 0)))
        {
            memset(ptr, 0, size);
            gData.mSecondaryBuffer->Unlock(ptr, size, NULL, 0);
        }

        gData.mSoloud = aSoloud;
        gData.mChannels = aChannels;
        gData.mLastWritePos = 0;
        gData.mRunning = true;

        aSoloud->postinit_internal(aSamplerate, aBufferSize, aFlags, aChannels);
        aSoloud->mBackendCleanupFunc = dsoundDeinit;

        gData.mSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
        gData.mThread = Thread::createThread(dsoundThread, &gData);
        
        aSoloud->mBackendData = &gData;
        aSoloud->mBackendString = "DirectSound";

        return 0;
    }
} // namespace SoLoud

#endif
