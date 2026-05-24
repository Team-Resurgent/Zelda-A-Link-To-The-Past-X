/*==========================================================================
  xbox_audio.cpp  -  DirectSound streaming audio for zelda3 Xbox port

  Mirrors the PC SDL AudioCallback model:
  - SDL calls AudioCallback when it needs N bytes
  - We check how much the play cursor has consumed and fill exactly that
  - ZeldaRenderAudio is called on demand, same as PC
==========================================================================*/

#include "xbox_platform.h"

extern "C" {
    void ZeldaRenderAudio(short* audio_buffer, int samples, int channels);
    void ZeldaDiscardUnusedAudioFrames(void);
}

static LPDIRECTSOUND8       g_pDSound = NULL;
static LPDIRECTSOUNDBUFFER  g_pStreamBuf = NULL;
static DWORD                g_dsWritePos = 0;
static DWORD                g_dsBufSize = 0;

static HANDLE               s_audioThread = NULL;
static HANDLE               s_audioMutex = NULL;
static volatile bool        s_audioRunning = false;
static volatile bool        s_audioPaused  = false;

/* Internal render buffer - same pattern as PC g_audiobuffer */
static short  s_renderBuf[AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS];
static BYTE* s_renderCur = NULL;
static BYTE* s_renderEnd = NULL;

static DWORD WINAPI Xbox_Audio_Thread(LPVOID param)
{
    while (s_audioRunning) {
        if (g_pStreamBuf) {
            /* How much has the play cursor consumed since we last wrote? */
            DWORD playCursor = 0, ignored = 0;
            g_pStreamBuf->GetCurrentPosition(&playCursor, &ignored);

            DWORD available;
            if (g_dsWritePos <= playCursor)
                available = playCursor - g_dsWritePos;
            else
                available = g_dsBufSize - g_dsWritePos + playCursor;

            /* Fill exactly what's been consumed - mirrors SDL AudioCallback */
            while (available > 0) {
                /* Refill render buffer when empty */
                if (s_renderCur >= s_renderEnd) {
                    if (s_audioPaused) {
                        /* Menu open - write silence instead of rendering */
                        ZeroMemory(s_renderBuf, sizeof(s_renderBuf));
                    } else {
                        WaitForSingleObject(s_audioMutex, INFINITE);
                        ZeldaRenderAudio(s_renderBuf, AUDIO_FRAMES_PER_BLOCK, AUDIO_CHANNELS);
                        ZeldaDiscardUnusedAudioFrames();
                        ReleaseMutex(s_audioMutex);
                    }
                    s_renderCur = (BYTE*)s_renderBuf;
                    s_renderEnd = s_renderCur + AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS * sizeof(short);
                }

                DWORD toWrite = s_renderEnd - s_renderCur;
                if (toWrite > available) toWrite = available;

                /* Write to DirectSound buffer */
                void* p1 = NULL, * p2 = NULL;
                DWORD s1 = 0, s2 = 0;
                if (SUCCEEDED(g_pStreamBuf->Lock(g_dsWritePos, toWrite, &p1, &s1, &p2, &s2, 0))) {
                    BYTE* src = s_renderCur;
                    if (p1 && s1) { memcpy(p1, src, s1); src += s1; }
                    if (p2 && s2) { memcpy(p2, src, s2); }
                    g_pStreamBuf->Unlock(p1, s1, p2, s2);
                    g_dsWritePos = (g_dsWritePos + toWrite) % g_dsBufSize;
                    s_renderCur += toWrite;
                    available -= toWrite;
                }
                else {
                    break;
                }
            }
        }

        Sleep(4);
    }
    return 0;
}

HRESULT Xbox_Audio_Init(void)
{
    HRESULT hr = DirectSoundCreate(NULL, &g_pDSound, NULL);
    if (FAILED(hr)) {
        OutputDebugStringA("zelda3: DirectSoundCreate failed\n");
        return hr;
    }

    g_dsBufSize = AUDIO_BUF_BYTES;
    g_dsWritePos = 0;
    s_renderCur = (BYTE*)s_renderBuf;
    s_renderEnd = s_renderCur; /* empty - will fill on first call */

    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(wfx));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = AUDIO_CHANNELS;
    wfx.nSamplesPerSec = AUDIO_SAMPLE_RATE;
    wfx.wBitsPerSample = AUDIO_BITS;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    DSBUFFERDESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_CTRLVOLUME;
    desc.dwBufferBytes = g_dsBufSize;
    desc.lpwfxFormat = &wfx;
    desc.lpMixBins = NULL;
    desc.dwInputMixBin = 0;

    hr = g_pDSound->CreateSoundBuffer(&desc, &g_pStreamBuf, NULL);
    if (FAILED(hr)) {
        OutputDebugStringA("zelda3: CreateSoundBuffer failed\n");
        return hr;
    }

    /* Silence entire buffer */
    void* p1 = NULL, * p2 = NULL;
    DWORD s1 = 0, s2 = 0;
    if (SUCCEEDED(g_pStreamBuf->Lock(0, g_dsBufSize, &p1, &s1, &p2, &s2, 0))) {
        if (p1) ZeroMemory(p1, s1);
        if (p2) ZeroMemory(p2, s2);
        g_pStreamBuf->Unlock(p1, s1, p2, s2);
    }

    g_pStreamBuf->Play(0, 0, DSBPLAY_LOOPING);

    OutputDebugStringA("zelda3: Audio init OK (threaded)\n");
    return S_OK;
}

extern "C" void Xbox_Audio_StartThread(void)
{
    s_audioMutex = CreateMutex(NULL, FALSE, NULL);
    if (!s_audioMutex) {
        OutputDebugStringA("zelda3: CreateMutex failed\n");
        return;
    }

    s_audioRunning = true;
    s_audioThread = CreateThread(NULL, 0, Xbox_Audio_Thread, NULL, 0, NULL);
    if (!s_audioThread) {
        OutputDebugStringA("zelda3: CreateThread failed\n");
        s_audioRunning = false;
    }
    else {
        OutputDebugStringA("zelda3: Audio thread started\n");
    }
}

extern "C" void Xbox_Audio_PushSilence(void)
{
    s_audioPaused = true;
}

extern "C" void Xbox_Audio_Resume(void)
{
    s_audioPaused = false;
}

void Xbox_Audio_Shutdown(void)
{
    s_audioRunning = false;
    if (s_audioThread) {
        WaitForSingleObject(s_audioThread, 2000);
        CloseHandle(s_audioThread);
        s_audioThread = NULL;
    }
    if (s_audioMutex) {
        CloseHandle(s_audioMutex);
        s_audioMutex = NULL;
    }
    if (g_pStreamBuf) { g_pStreamBuf->Stop(); g_pStreamBuf->Release(); g_pStreamBuf = NULL; }
    if (g_pDSound) { g_pDSound->Release(); g_pDSound = NULL; }
}

extern "C" void ZeldaApuLock() { if (s_audioMutex) WaitForSingleObject(s_audioMutex, INFINITE); }
extern "C" void ZeldaApuUnlock() { if (s_audioMutex) ReleaseMutex(s_audioMutex); }