#include "Cafe/OS/common/OSCommon.h"
#include <atomic>
#include <array>
#include <cstring>
#include <mutex>
#include "coreinit_Atomic.h"

namespace coreinit
{
	namespace
	{
		static constexpr size_t ATOMIC64_LOCK_COUNT = 256;

		// Guest memory doesn't guarantee host-native 64-bit atomic alignment, which can fault on ARM64.
		std::array<std::mutex, ATOMIC64_LOCK_COUNT>& GetAtomic64Locks()
		{
			static std::array<std::mutex, ATOMIC64_LOCK_COUNT> s_atomic64Locks{};
			return s_atomic64Locks;
		}

		std::mutex& GetAtomic64Lock(const void* mem)
		{
			uintptr_t ptrValue = reinterpret_cast<uintptr_t>(mem);
			return GetAtomic64Locks()[(ptrValue >> 3) & (ATOMIC64_LOCK_COUNT - 1)];
		}

		uint64be LoadAtomic64Value(const std::atomic<uint64be>* mem)
		{
			uint64be value;
			std::memcpy(&value, mem, sizeof(value));
			return value;
		}

		void StoreAtomic64Value(std::atomic<uint64be>* mem, uint64be value)
		{
			std::memcpy(mem, &value, sizeof(value));
		}
	}

	/* 32bit atomic operations */

	uint32 OSSwapAtomic(std::atomic<uint32be>* mem, uint32 newValue)
	{
		uint32be _newValue = newValue;
		uint32be previousValue = mem->exchange(_newValue);
		return previousValue;
	}

	bool OSCompareAndSwapAtomic(std::atomic<uint32be>* mem, uint32 compareValue, uint32 swapValue)
	{
		// seen in GTA3 homebrew port
		uint32be _compareValue = compareValue;
		uint32be _swapValue = swapValue;
		return mem->compare_exchange_strong(_compareValue, _swapValue);
	}

	bool OSCompareAndSwapAtomicEx(std::atomic<uint32be>* mem, uint32 compareValue, uint32 swapValue, uint32be* previousValue)
	{
		// seen in GTA3 homebrew port
		uint32be _compareValue = compareValue;
		uint32be _swapValue = swapValue;
		bool r = mem->compare_exchange_strong(_compareValue, _swapValue);
		*previousValue = _compareValue;
		return r;
	}

	uint32 OSAddAtomic(std::atomic<uint32be>* mem, uint32 adder)
	{
        // used by SDL Wii U port
		uint32be knownValue;
		while (true)
		{
			knownValue = mem->load();
			uint32be newValue = knownValue + adder;
			if (mem->compare_exchange_strong(knownValue, newValue))
				break;
		}
		return knownValue;
	}

	/* 64bit atomic operations */

	uint64 OSSwapAtomic64(std::atomic<uint64be>* mem, uint64 newValue)
	{
		uint64be _newValue = newValue;
		std::lock_guard lock(GetAtomic64Lock(mem));
		uint64be previousValue = LoadAtomic64Value(mem);
		StoreAtomic64Value(mem, _newValue);
		return previousValue;
	}

	uint64 OSSetAtomic64(std::atomic<uint64be>* mem, uint64 newValue)
	{
		return OSSwapAtomic64(mem, newValue);
	}

	uint64 OSGetAtomic64(std::atomic<uint64be>* mem)
	{
		std::lock_guard lock(GetAtomic64Lock(mem));
		return LoadAtomic64Value(mem);
	}

	uint64 OSAddAtomic64(std::atomic<uint64be>* mem, uint64 adder)
	{
		std::lock_guard lock(GetAtomic64Lock(mem));
		uint64be knownValue = LoadAtomic64Value(mem);
		uint64be newValue = knownValue + adder;
		StoreAtomic64Value(mem, newValue);
		return knownValue;
	}

	uint64 OSAndAtomic64(std::atomic<uint64be>* mem, uint64 val)
	{
		std::lock_guard lock(GetAtomic64Lock(mem));
		uint64be knownValue = LoadAtomic64Value(mem);
		uint64be newValue = knownValue & val;
		StoreAtomic64Value(mem, newValue);
		return knownValue;
	}

	uint64 OSOrAtomic64(std::atomic<uint64be>* mem, uint64 val)
	{
		std::lock_guard lock(GetAtomic64Lock(mem));
		uint64be knownValue = LoadAtomic64Value(mem);
		uint64be newValue = knownValue | val;
		StoreAtomic64Value(mem, newValue);
		return knownValue;
	}

	bool OSCompareAndSwapAtomic64(std::atomic<uint64be>* mem, uint64 compareValue, uint64 swapValue)
	{
		uint64be _compareValue = compareValue;
		uint64be _swapValue = swapValue;
		std::lock_guard lock(GetAtomic64Lock(mem));
		uint64be currentValue = LoadAtomic64Value(mem);
		if (currentValue != _compareValue)
			return false;
		StoreAtomic64Value(mem, _swapValue);
		return true;
	}

	bool OSCompareAndSwapAtomicEx64(std::atomic<uint64be>* mem, uint64 compareValue, uint64 swapValue, uint64be* previousValue)
	{
		uint64be _compareValue = compareValue;
		uint64be _swapValue = swapValue;
		std::lock_guard lock(GetAtomic64Lock(mem));
		uint64be currentValue = LoadAtomic64Value(mem);
		bool r = currentValue == _compareValue;
		if (r)
			StoreAtomic64Value(mem, _swapValue);
		*previousValue = currentValue;
		return r;
	}

	void InitializeAtomic()
	{
		// 32bit atomic operations
		cafeExportRegister("coreinit", OSSwapAtomic, LogType::Placeholder);
		cafeExportRegister("coreinit", OSCompareAndSwapAtomic, LogType::Placeholder);
		cafeExportRegister("coreinit", OSCompareAndSwapAtomicEx, LogType::Placeholder);
		cafeExportRegister("coreinit", OSAddAtomic, LogType::Placeholder);
		
		// 64bit atomic operations
		cafeExportRegister("coreinit", OSSetAtomic64, LogType::Placeholder);
		cafeExportRegister("coreinit", OSGetAtomic64, LogType::Placeholder);
		cafeExportRegister("coreinit", OSSwapAtomic64, LogType::Placeholder);
		cafeExportRegister("coreinit", OSAddAtomic64, LogType::Placeholder);
		cafeExportRegister("coreinit", OSAndAtomic64, LogType::Placeholder);
		cafeExportRegister("coreinit", OSOrAtomic64, LogType::Placeholder);
		cafeExportRegister("coreinit", OSCompareAndSwapAtomic64, LogType::Placeholder);
		cafeExportRegister("coreinit", OSCompareAndSwapAtomicEx64, LogType::Placeholder);
	}
}
