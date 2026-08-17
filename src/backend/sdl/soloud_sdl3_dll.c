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
#ifdef WITH_SDL3

#include <stdlib.h>
#if defined(_MSC_VER)
#define WINDOWS_VERSION
#endif
#include "SDL3/SDL.h"
#include <math.h>


typedef Uint32            (*SDL3_WasInit_t)(Uint32 flags);
typedef int               (*SDL3_InitSubSystem_t)(Uint32 flags);
typedef SDL_AudioStream * (*SDL3_OpenAudioDeviceStream_t)(
	SDL_AudioDeviceID deviceId,
	SDL_AudioSpec const * spec,
	SDL_AudioStreamCallback callback,
	void * userdata
);

typedef void (*SDL3_CloseAudioDevice_t)(SDL_AudioDeviceID dev);
typedef bool (*SDL3_PauseAudioStreamDevice_t)(SDL_AudioStream* stream);
typedef bool (*SDL3_ResumeAudioStreamDevice_t)(SDL_AudioStream* stream);

typedef bool (*PFN_SDL_GETAUDIODEVICEFORMAT)(SDL_AudioDeviceID devid, SDL_AudioSpec *spec, int *sample_frames);
typedef SDL_AudioDeviceID (*PFN_SDL_GETAUDIOSTREAMDEVICE)(SDL_AudioStream *stream);
typedef bool (*PFN_SDL_PUTAUDIOSTREAMDATA)(SDL_AudioStream *stream, const void *buf, int len);
typedef char* (*PFN_SDL_GETERROR)(void);
typedef bool (*PFN_SDL_SETHINT)(const char *name, const char *value);

static SDL3_WasInit_t SDL3_WasInit = NULL;
static SDL3_InitSubSystem_t SDL3_InitSubSystem = NULL;
static SDL3_OpenAudioDeviceStream_t SDL3_OpenAudioDeviceStream = NULL;
static SDL3_CloseAudioDevice_t SDL3_CloseAudioDevice = NULL;
static SDL3_PauseAudioStreamDevice_t SDL3_PauseAudioStreamDevice = NULL;
static SDL3_ResumeAudioStreamDevice_t SDL3_ResumeAudioStreamDevice = NULL;

static PFN_SDL_GETAUDIODEVICEFORMAT SDL3_GetAudioDeviceFormat = NULL;
static PFN_SDL_GETAUDIOSTREAMDEVICE SDL3_GetAudioStreamDevice = NULL;
static PFN_SDL_PUTAUDIOSTREAMDATA SDL3_PutAudioStreamData = NULL;
static PFN_SDL_GETERROR SDL3_GetError = NULL;
static PFN_SDL_SETHINT SDL3_SetHint = NULL;

#ifdef WINDOWS_VERSION
#include <windows.h>

static HMODULE sdl3_openDll()
{
	HMODULE res = LoadLibraryA("SDL3.dll");
	return res;
}

static void* sdl3_getDllProc(HMODULE aDllHandle, const char *aProcName)
{
	return (void*)GetProcAddress(aDllHandle, (LPCSTR)aProcName);
}

#else
#include <dlfcn.h> // dll functions

static void * sdl3_openDll()
{
	void * res;
	res = dlopen("/Library/Frameworks/SDL3.framework/SDL3", RTLD_LAZY);
	if (!res) res = dlopen("SDL3.so", RTLD_LAZY);
	if (!res) res = dlopen("libSDL3.so", RTLD_LAZY);
	return res;
}

static void* sdl3_getDllProc(void * aLibrary, const char *aProcName)
{
	return dlsym(aLibrary, aProcName);
}

#endif

static int sdl3_load_dll()
{
#ifdef WINDOWS_VERSION
	HMODULE dll = NULL;
#else
	void * dll = NULL;
#endif

	if (SDL3_OpenAudioDeviceStream != NULL)
	{
		return 1;
	}

	dll = sdl3_openDll();

	if (dll)
	{
		SDL3_WasInit = (SDL3_WasInit_t)sdl3_getDllProc(dll, "SDL_WasInit");
		SDL3_InitSubSystem = (SDL3_InitSubSystem_t)sdl3_getDllProc(dll, "SDL_InitSubSystem");
		SDL3_OpenAudioDeviceStream = (SDL3_OpenAudioDeviceStream_t)sdl3_getDllProc(dll, "SDL_OpenAudioDeviceStream");
		SDL3_CloseAudioDevice = (SDL3_CloseAudioDevice_t)sdl3_getDllProc(dll, "SDL_CloseAudioDevice");
		SDL3_PauseAudioStreamDevice = (SDL3_PauseAudioStreamDevice_t)sdl3_getDllProc(dll, "SDL_PauseAudioStreamDevice");
		SDL3_ResumeAudioStreamDevice = (SDL3_ResumeAudioStreamDevice_t)sdl3_getDllProc(dll, "SDL_ResumeAudioStreamDevice");
		
		SDL3_GetAudioDeviceFormat = (PFN_SDL_GETAUDIODEVICEFORMAT)sdl3_getDllProc(dll, "SDL_GetAudioDeviceFormat");
		SDL3_GetAudioStreamDevice = (PFN_SDL_GETAUDIOSTREAMDEVICE)sdl3_getDllProc(dll, "SDL_GetAudioStreamDevice");
		SDL3_PutAudioStreamData = (PFN_SDL_PUTAUDIOSTREAMDATA)sdl3_getDllProc(dll, "SDL_PutAudioStreamData");
		SDL3_GetError = (PFN_SDL_GETERROR)sdl3_getDllProc(dll, "SDL_GetError");
		SDL3_SetHint = (PFN_SDL_SETHINT)sdl3_getDllProc(dll, "SDL_SetHint");

		if (SDL3_WasInit &&
			SDL3_InitSubSystem &&
			SDL3_OpenAudioDeviceStream &&
			SDL3_CloseAudioDevice &&
			SDL3_PauseAudioStreamDevice)
		{
			return 1;
		}
	}
	SDL3_OpenAudioDeviceStream = NULL;
	return 0;
}

int dll_SDL3_found()
{
	return sdl3_load_dll();
}

Uint32 dll_SDL3_WasInit(Uint32 flags)
{
	if (SDL3_WasInit)
		return SDL3_WasInit(flags);
	return 0;
}

int dll_SDL3_InitSubSystem(Uint32 flags)
{
	if (SDL3_InitSubSystem)
		return SDL3_InitSubSystem(flags);
	return -1;
}

SDL_AudioStream * dll_SDL3_OpenAudioDeviceStream(
	SDL_AudioDeviceID deviceId,
	SDL_AudioSpec const * spec,
	SDL_AudioStreamCallback callback,
	void * userdata
)
{
	if (SDL3_OpenAudioDeviceStream)
	{
		return SDL3_OpenAudioDeviceStream(
			deviceId,
			spec,
			callback,
			userdata
		);
	}
	return 0;
}

void dll_SDL3_CloseAudioDevice (SDL_AudioDeviceID dev)
{
	if (SDL3_CloseAudioDevice)
		SDL3_CloseAudioDevice(dev);
}

bool dll_SDL3_PauseAudioStreamDevice(SDL_AudioStream* stream)
{
	if (SDL3_PauseAudioStreamDevice)
		return SDL3_PauseAudioStreamDevice(stream);
	return false;
}

bool dll_SDL3_ResumeAudioStreamDevice(SDL_AudioStream* stream)
{
	if (SDL3_ResumeAudioStreamDevice)
		return SDL3_ResumeAudioStreamDevice(stream);
	return false;
}

bool dll_SDL3_GetAudioDeviceFormat(SDL_AudioDeviceID devid, SDL_AudioSpec *spec, int *sample_frames)
{
	if (SDL3_GetAudioDeviceFormat)
		return SDL3_GetAudioDeviceFormat(devid, spec, sample_frames);
	return false;
}

SDL_AudioDeviceID dll_SDL3_GetAudioStreamDevice(SDL_AudioStream *stream)
{
	if (SDL3_GetAudioStreamDevice)
		return SDL3_GetAudioStreamDevice(stream);
	return 0;
}

bool dll_SDL3_PutAudioStreamData(SDL_AudioStream *stream, const void *buf, int len)
{
	if (SDL3_PutAudioStreamData)
		return SDL3_PutAudioStreamData(stream, buf, len);
	return false;
}

char* dll_SDL3_GetError(void)
{
	if (SDL3_GetError)
		return SDL3_GetError();
	return NULL;
}

bool dll_SDL3_SetHint(const char *name, const char *value)
{
	if (SDL3_SetHint)
		return SDL3_SetHint(name, value);
	return NULL;
}

#endif
