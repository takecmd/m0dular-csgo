#include "settings.h"
#include "../sdk/framework/features/aimbot_types.h"
#include "../sdk/framework/utils/stackstring.h"
#include "binds.h"

//We will use atomic flags and atomic ints to implement a interprocess rwlock
#include <atomic>

#ifdef _WIN32
#include <windows.h>
using fileHandle = HANDLE;
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
using fileHandle = int;
#endif


static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);

constexpr unsigned int ALLOC_SIZE = 1 << 25;

std::atomic_int* ipcCounter = nullptr;

uintptr_t Settings::allocBase = 0;
generic_free_list_allocator<Settings::allocBase>* Settings::settingsAlloc = nullptr;

uintptr_t Settings::localAllocBase = (uintptr_t)malloc(ALLOC_SIZE);
generic_free_list_allocator<Settings::localAllocBase> Settings::settingsLocalAlloc(1 << 25, PlacementPolicy::FIND_FIRST, (void**)localAllocBase);

decltype(Settings::globalSettingsPtr) Settings::globalSettingsPtr = nullptr;
decltype(Settings::globalSettings) Settings::globalSettings;
decltype(Settings::bindSettingsPtr) Settings::bindSettingsPtr = nullptr;
decltype(Settings::bindSettings) Settings::bindSettings;
SharedMutex* Settings::ipcLock = nullptr;

AimbotHitbox Settings::aimbotHitboxes[MAX_HITBOXES] = {
	{ HITBOX_HEAD, SCAN_MULTIPOINT, 0.8f },
	{ HITBOX_NECK, SCAN_SIMPLE },
	{ HITBOX_PELVIS, SCAN_SIMPLE },
	{ HITBOX_STOMACH, SCAN_SIMPLE },
	{ HITBOX_LOWER_CHEST, SCAN_SIMPLE },
	{ HITBOX_CHEST, SCAN_MULTIPOINT, 0.8f },
	{ HITBOX_RIGHT_THIGH, SCAN_SIMPLE },
	{ HITBOX_LEFT_THIGH, SCAN_SIMPLE },
	{ HITBOX_RIGHT_CALF, SCAN_SIMPLE },
	{ HITBOX_LEFT_CALF, SCAN_SIMPLE },
	{ HITBOX_RIGHT_FOOT, SCAN_MULTIPOINT, 0.8f },
	{ HITBOX_LEFT_FOOT, SCAN_MULTIPOINT, 0.8f },
	{ HITBOX_RIGHT_HAND, SCAN_MULTIPOINT, 0.8f },
	{ HITBOX_LEFT_HAND, SCAN_MULTIPOINT, 0.8f },
	{ HITBOX_RIGHT_UPPER_ARM, SCAN_SIMPLE },
	{ HITBOX_LEFT_UPPER_ARM, SCAN_SIMPLE },
};

struct IPCInit
{
    uintptr_t* target;
	size_t size;

	template<typename T>
	IPCInit(T*& in) : target((uintptr_t*)&in), size(sizeof(T)) {}
};

static IPCInit initializedPointers[] = {
	IPCInit(ipcCounter),
	IPCInit(Settings::settingsAlloc),
	IPCInit(Settings::globalSettingsPtr),
	IPCInit(Settings::bindSettingsPtr),
	IPCInit(BindManager::sharedInstance),
	IPCInit(Settings::ipcLock),
};

bool MapSharedMemory(fileHandle& fd, void*& addr, size_t msz, const char* name)
{
	bool firstTime = false;

#ifdef _WIN32
	fd = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);

	if (!(void*)fd) {
		fd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, msz, name);
		firstTime = true;
	}

	if (fd) {
		addr = (void*)MapViewOfFile(fd, FILE_MAP_ALL_ACCESS, 0, 0, msz);
	}

	if (!firstTime)
		CloseHandle(fd);

#else
    fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		firstTime = true;
		fd = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	}
	//firstTime = true;

	if (fd != -1 && ftruncate(fd, msz) != -2)
		addr = mmap(nullptr, msz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (addr == (void*)-1)
		addr = nullptr;
#endif

	return firstTime;
}

void UnmapSharedMemory(void* addr, fileHandle& fd, const char* name, size_t msz, bool unlink)
{
#ifdef _WIN32
	UnmapViewOfFile(addr);
	CloseHandle(fd);
#else
	munmap(addr, msz);
	close(fd);
	if (unlink)
		shm_unlink(name);
#endif
}

template<typename T>
static void Destruct(T*& target)
{
	target->~T();
}

template<typename T, typename... Args>
static void Construct(T*& target, Args... args)
{
	target = new(target) T(args...);
}

struct SettingsInstance
{
	void* alloc;
	fileHandle fd;

	SettingsInstance()
	{
	    bool firstTime = MapSharedMemory(fd, alloc, ALLOC_SIZE, "m0d_settings");

		Settings::allocBase = (uintptr_t)alloc;

		uintptr_t finalAddress = Settings::allocBase;
		for (IPCInit& i : initializedPointers) {
			*i.target = finalAddress;
			finalAddress += i.size;
		}

		if (firstTime) {
			Construct(ipcCounter);
			Construct(Settings::settingsAlloc, ALLOC_SIZE - (finalAddress - Settings::allocBase), PlacementPolicy::FIND_FIRST, (void*)finalAddress);
		    Construct(Settings::globalSettingsPtr);
		    Construct(Settings::bindSettingsPtr);
		    Construct(BindManager::sharedInstance);
		    Construct(Settings::ipcLock);
		}

		BindManager::sharedInstance->InitializeLocalData();

		(*ipcCounter)++;
	}

	~SettingsInstance()
	{
		(*ipcCounter)--;

		bool unlink = false;

		if (!ipcCounter->load()) {
			unlink = true;
			Destruct(ipcCounter);
			Destruct(Settings::settingsAlloc);
			Destruct(Settings::globalSettingsPtr);
			Destruct(Settings::bindSettingsPtr);
			Destruct(BindManager::sharedInstance);
			Destruct(Settings::ipcLock);
		}

		UnmapSharedMemory(alloc, fd, "m0d_settings", ALLOC_SIZE, unlink);
	}
};

//TODO: Ensure this class gets constructed before anything else that depends on it. Currently this is done by changing file ordering, but does not work on GCC
SettingsInstance sharedInstance;

namespace Settings
{
#define HANDLE_OPTION(type, defaultVal, name, ...) OPTIONDEF(name)(defaultVal);
#include "option_list.h"
}