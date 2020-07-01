/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Provides an generic implementation of ThreadStackManager features
 *          for use on Zephyr platforms.
 */

#ifndef GENERIC_THREAD_STACK_MANAGER_IMPL_ZEPHYR_H
#define GENERIC_THREAD_STACK_MANAGER_IMPL_ZEPHYR_H

#include <net/openthread.h>
#include <zephyr.h>

namespace chip {
namespace DeviceLayer {

class ThreadStackManagerImpl;

namespace Internal {

/**
 * Provides a generic implementation of ThreadStackManager features that works on Zephyr platforms.
 *
 * This template contains implementations of select features from the ThreadStackManager abstract
 * interface that are suitable for use on Zephyr-based platforms.  It is intended to be
 * inherited, directly or indirectly, by the ThreadStackManagerImpl class, which also appears as
 * the template's ImplClass parameter.
 *
 * Any task using this class must have cooperative scheduling priority.
 */
template <class ImplClass>
class GenericThreadStackManagerImpl_Zephyr : public GenericThreadStackManagerImpl_OpenThread<ImplClass>
{
public:
    // ===== Methods that implement the ThreadStackManager abstract interface.
    CHIP_ERROR _InitThreadStack(void);

protected:
    // ===== Methods that implement the ThreadStackManager abstract interface.

    CHIP_ERROR _StartThreadTask(void);
    void _LockThreadStack(void);
    bool _TryLockThreadStack(void);
    void _UnlockThreadStack(void);

    // ===== Methods that override the GenericThreadStackManagerImpl_OpenThread abstract interface.

    void _ProcessThreadActivity(void);
    void _OnCHIPoBLEAdvertisingStart(void);
    void _OnCHIPoBLEAdvertisingStop(void);

private:
    // ===== Private members for use by this class only.

    inline ImplClass * Impl() { return static_cast<ImplClass *>(this); }

    k_mutex mThreadStackLock;
};

// Instruct the compiler to instantiate the template only when explicitly told to do so.
extern template class GenericThreadStackManagerImpl_Zephyr<ThreadStackManagerImpl>;

template <class ImplClass>
CHIP_ERROR GenericThreadStackManagerImpl_Zephyr<ImplClass>::_InitThreadStack(void)
{
    k_mutex_init(&mThreadStackLock);
    return GenericThreadStackManagerImpl_OpenThread<ImplClass>::DoInit(openthread_get_default_instance());
}

template <class ImplClass>
CHIP_ERROR GenericThreadStackManagerImpl_Zephyr<ImplClass>::_StartThreadTask(void)
{
    // Intentionally empty.
    return CHIP_NO_ERROR;
}

template <class ImplClass>
void GenericThreadStackManagerImpl_Zephyr<ImplClass>::_LockThreadStack(void)
{
    k_mutex_lock(&mThreadStackLock, K_FOREVER);
}

template <class ImplClass>
bool GenericThreadStackManagerImpl_Zephyr<ImplClass>::_TryLockThreadStack(void)
{
    return k_mutex_lock(&mThreadStackLock, K_NO_WAIT) == 0;
}

template <class ImplClass>
void GenericThreadStackManagerImpl_Zephyr<ImplClass>::_UnlockThreadStack(void)
{
    k_mutex_unlock(&mThreadStackLock);
}

template <class ImplClass>
void GenericThreadStackManagerImpl_Zephyr<ImplClass>::_ProcessThreadActivity(void)
{
    // Intentionally empty.
}

template <class ImplClass>
void GenericThreadStackManagerImpl_Zephyr<ImplClass>::_OnCHIPoBLEAdvertisingStart(void)
{
    ChipLogError(DeviceLayer, "%s: NOT IMPLEMENTED", __PRETTY_FUNCTION__);
}

template <class ImplClass>
void GenericThreadStackManagerImpl_Zephyr<ImplClass>::_OnCHIPoBLEAdvertisingStop(void)
{
    ChipLogError(DeviceLayer, "%s: NOT IMPLEMENTED", __PRETTY_FUNCTION__);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

#endif // GENERIC_THREAD_STACK_MANAGER_IMPL_ZEPHYR_H
