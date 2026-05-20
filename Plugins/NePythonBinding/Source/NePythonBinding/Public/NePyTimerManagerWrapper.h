#pragma once
#include "NePyBase.h"
#include "NePyHouseKeeper.h"
#include "TimerManager.h"
#include "Templates/SharedPointer.h"

// 为TimerManager提供引用类型的访问
// NOTE: 同一个TimerManager一定会返回同一个TimerManagerWrapper
struct NEPYTHONBINDING_API FNePyTimerManagerWrapper : FNePyPropObject
{
	inline FTimerManager* GetValue()
	{
		return (FTimerManager*)Value;
	}

	// 初始化PyType类型
	static void InitPyType();

	// 为UObject的TimerManager生成Python对象
	static FNePyTimerManagerWrapper* New(UObject* InObject, FTimerManager* InTimerManager);

private:
	// tp_dealloc
	static void Dealloc(FNePyTimerManagerWrapper* InSelf);

	// tp_init
	static int Init(FNePyTimerManagerWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

private:
	// 检测底层UObject是否已被销毁
	static PyObject* IsValid(FNePyTimerManagerWrapper* InSelf);
	
	/**
	 * Sets a timer to call the given function at a set interval.
	 *
	 * @param InTimerMethod			Method to call when timer fires.
	 * @param InRate				The amount of time (in seconds) between set and firing.
	 * @param InbLoop				true to keep firing at Rate intervals, false to fire only once.
	 * @param InFirstDelay			The time (in seconds) for the first iteration of a looping timer. If < 0.f InRate will be used.
	 * @return TimerHandle
	 */
	static PyObject* SetTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArgs);

	/**
	 * Sets a timer to call the given native function on the next tick.
	 *
	 * @param InTimerMethod			Method to call when timer fires.
	 * @return TimerHandle
	 */
	static PyObject* SetTimerForNextTick(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	* Clears a previously set timer.
	* Invalidates the timer handle as it should no longer be used.
	*
	* @param InHandle The handle of the timer to clear.
	*/
	static PyObject* ClearTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	 * Pauses a previously set timer.
	 *
	 * @param InHandle The handle of the timer to pause.
	 */
	static PyObject* PauseTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	 * Unpauses a previously set timer
	 *
	 * @param InHandle The handle of the timer to unpause.
	 */
	static PyObject* UnPauseTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	 * Gets the current rate (time between activations) for the specified timer.
	 *
	 * @param InHandle The handle of the timer to return the rate of.
	 * @return The current rate or -1.f if timer does not exist.
	 */
	static PyObject* GetTimerRate(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	 * Returns true if the specified timer exists and is not paused
	 *
	 * @param InHandle The handle of the timer to check for being active.
	 * @return true if the timer exists and is active, false otherwise.
	 */
	static PyObject* IsTimerActive(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	* Returns true if the specified timer exists and is paused
	*
	* @param InHandle The handle of the timer to check for being paused.
	* @return true if the timer exists and is paused, false otherwise.
	*/
	static PyObject* IsTimerPaused(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	* Returns true if the specified timer exists and is pending
	*
	* @param InHandle The handle of the timer to check for being pending.
	* @return true if the timer exists and is pending, false otherwise.
	*/
	static PyObject* IsTimerPending(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	* Returns true if the specified timer exists
	*
	* @param InHandle The handle of the timer to check for existence.
	* @return true if the timer exists, false otherwise.
	*/
	static PyObject* TimerExists(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	 * Gets the current elapsed time for the specified timer.
	 *
	 * @param InHandle The handle of the timer to check the elapsed time of.
	 * @return The current time elapsed or -1.f if the timer does not exist.
	 */
	static PyObject* GetTimerElapsed(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

	/**
	 * Gets the time remaining before the specified timer is called
	 *
	 * @param InHandle The handle of the timer to check the remaining time of.
	 * @return	 The current time remaining, or -1.f if timer does not exist
	 */
	static PyObject* GetTimerRemaining(FNePyTimerManagerWrapper* InSelf, PyObject* InArg);

private:
	static bool ToCppHandle(PyObject* InPyObj, FTimerHandle& OutHandle);

	static PyObject* ToPyHandle(FTimerHandle InHandle);
};

// 为非UObject类型对象的TimerManager提供引用类型的访问
// NOTE: 同一个TimerManager一定会返回同一个TimerManagerWrapper
// WARNING: FNePySharedTimerManagerWrapper一旦创建了就不会销毁，并且会阻止智能指针销毁！
//    考虑到FNePySharedTimerManagerWrapper目前只会配合GEditor使用，这应该不成问题。
struct NEPYTHONBINDING_API FNePySharedTimerManagerWrapper : FNePyTimerManagerWrapper
{
	// 为UObject的TimerManager生成Python对象
	static FNePySharedTimerManagerWrapper* New(const TSharedRef<FTimerManager>& InTimerManager);

private:
	// TimerManager与对应的PythonWrapper
	static TMap<TSharedRef<FTimerManager>, FNePySharedTimerManagerWrapper*> PythonWrappers;
};
