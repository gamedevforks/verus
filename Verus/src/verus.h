// Copyright (C) 2021, Dmitry Maluev (dmaluev@gmail.com). All rights reserved.
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

// System:
#ifdef _WIN32
#	pragma comment(lib, "ws2_32.lib")
#	include <WS2tcpip.h>
#	include <Shlwapi.h>
#	pragma comment(lib, "Shlwapi.lib")
#	include <shellapi.h>
#	include <ShlObj.h>
#	include <ShellScalingAPI.h>
#	pragma comment(lib, "Shcore.lib")
//#	pragma comment(lib, "shell32.lib")
//#	define _WIN32_DCOM
//#	include <comdef.h>
//#	include <wbemidl.h>
//#	pragma comment(lib, "wbemuuid.lib")
#	include <tchar.h>
#	include <mmsystem.h>
#else
#	include <dlfcn.h>
#endif

// SDL:
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

// Vulkan SDK:
#ifdef VERUS_INCLUDE_VULKAN
#	include <vulkan/vulkan.h>
#	pragma comment(lib, "vulkan-1.lib")
#	ifdef _DEBUG
#		define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#		define VMA_DEBUG_MARGIN 16
#		define VMA_DEBUG_DETECT_CORRUPTION 1
#	endif
#	include "ThirdParty/vk_mem_alloc.h" // See: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
#endif

// Direct3D 12:
#ifdef VERUS_INCLUDE_D3D12
#	include <dxgi1_6.h>
#	include <d3d12.h>
#	include <d3dcompiler.h>
#	include <wrl.h>
using namespace Microsoft::WRL;
#	pragma comment(lib, "d3d12.lib")
#	pragma comment(lib, "d3dcompiler.lib")
#	pragma comment(lib, "dxgi.lib")
#	pragma comment(lib, "dxguid.lib")
#	include "ThirdParty/d3dx12.h" // See: https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12
#	include "ThirdParty/D3D12MemAlloc.h" // See: https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
#endif

// Bullet Physics:
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <Serialize/BulletWorldImporter/btBulletWorldImporter.h>
#ifdef _DEBUG
#	pragma comment(lib, "BulletCollision_vs2010_x64_debug.lib")
#	pragma comment(lib, "BulletDynamics_vs2010_x64_debug.lib")
#	pragma comment(lib, "BulletFileLoader_vs2010_x64_debug.lib")
#	pragma comment(lib, "BulletWorldImporter_vs2010_x64_debug.lib")
#	pragma comment(lib, "LinearMath_vs2010_x64_debug.lib")
#else
#	pragma comment(lib, "BulletCollision_vs2010_x64_release.lib")
#	pragma comment(lib, "BulletDynamics_vs2010_x64_release.lib")
#	pragma comment(lib, "BulletFileLoader_vs2010_x64_release.lib")
#	pragma comment(lib, "BulletWorldImporter_vs2010_x64_release.lib")
#	pragma comment(lib, "LinearMath_vs2010_x64_release.lib")
#endif

// OpenAL:
#include <AL/al.h>
#include <AL/alc.h>
#pragma comment(lib, "OpenAL32.lib")

// OGG:
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#ifdef _DEBUG
#	pragma comment(lib, "liboggD.lib")
#	pragma comment(lib, "libvorbis_staticD.lib")
#	pragma comment(lib, "libvorbisfileD.lib")
#else
#	pragma comment(lib, "libogg.lib")
#	pragma comment(lib, "libvorbis_static.lib")
#	pragma comment(lib, "libvorbisfile.lib")
#endif

// AMD Compressonator:
#ifdef VERUS_INCLUDE_COMPRESSONATOR
#	include <CMP_Framework.h>
#	pragma comment(lib, "CMP_Framework_MD_DLL.lib")
#endif

// AMD Tootle:
#ifdef VERUS_INCLUDE_TOOTLE
#	include <tootlelib.h>
#	pragma comment(lib, "TootleDLL_2k8_64.lib")
#endif

// DevIL:
#include <IL/il.h>
#pragma comment(lib, "DevIL.lib")

// C/C++:
#include <atomic>
#include <cassert>
#include <ctime>
#include <future>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define VERUS_SDK_VERSION 0x01000000

#include "ThirdParty/ThirdParty.h" // Third-party software components
#include "Global/Basic.h"          // Essentials
#include "D/D.h"                   // Debugging, diagnostics, development
#include "Global/Global.h"
#include "Math/Math.h"             // Mathematics, vectors, matrices
#include "IO/IO.h"                 // Input/output, file system
#include "AI/AI.h"                 // Artificial intelligence
#include "Anim/Anim.h"             // Animation
#include "Physics/Physics.h"
#include "App/App.h"
#include "Audio/Audio.h"           // Audio, sounds
#include "CGI/CGI.h"               // Computer-generated imagery
#include "Input/Input.h"
#include "Effects/Effects.h"
#include "Scene/Scene.h"
#include "Game/Game.h"
#include "GUI/GUI.h"               // Graphical user interface
#include "Net/Net.h"               // Networking
#include "Security/Security.h"     // Cryptography
#include "Extra/Extra.h"
