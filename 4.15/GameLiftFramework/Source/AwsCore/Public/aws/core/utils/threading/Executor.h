/*
  * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License").
  * You may not use this file except in compliance with the License.
  * A copy of the License is located at
  *
  *  http://aws.amazon.com/apache2.0
  *
  * or in the "license" file accompanying this file. This file is distributed
  * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
  * express or implied. See the License for the specific language governing
  * permissions and limitations under the License.
  */

#pragma once

#include <aws/core/Core_EXPORTS.h>
#include <aws/core/utils/memory/stl/AWSFunction.h>
#include <aws/core/utils/memory/stl/AWSQueue.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <functional>
#include <future>
#include <mutex>
#include <aws/core/utils/threading/Semaphore.h>

namespace Aws
{
    namespace Utils
    {
        namespace Threading
        {
            class ThreadTask;

            /**
            * Interface for implementing an Executor, to implement a custom thread execution strategy, inherit from this class
            * and override SubmitToThread().
            */
            class AWS_CORE_API Executor
            {
            public:                
                virtual ~Executor() = default;

                /**
                 * Send function and its arguments to the SubmitToThread function.
                 */
                template<class Fn, class ... Args>
                bool Submit(Fn&& fn, Args&& ... args)
                {
                    return SubmitToThread(AWS_BUILD_TYPED_FUNCTION(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...), void()));
                }

            protected:
                /**
                * To implement your own executor implementation, then simply subclass Executor and implement this method.
                */
                virtual bool SubmitToThread(std::function<void()>&&) = 0;
            };


            /**
            * Default Executor implementation. Simply spawns a thread and detaches it.
            */
            class AWS_CORE_API DefaultExecutor : public Executor
            {
            public:
                DefaultExecutor() {}
            protected:
                bool SubmitToThread(std::function<void()>&&) override;
            };

            enum class OverflowPolicy
            {
                QUEUE_TASKS_EVENLY_ACCROSS_THREADS,
                REJECT_IMMEDIATELY
            };

            /**
            * Thread Pool Executor implementation.
            */
            class AWS_CORE_API PooledThreadExecutor : public Executor
            {
            public:
                PooledThreadExecutor(size_t poolSize, OverflowPolicy overflowPolicy = OverflowPolicy::QUEUE_TASKS_EVENLY_ACCROSS_THREADS);
                ~PooledThreadExecutor();

                /**
                * Rule of 5 stuff.
                * Don't copy or move
                */
                PooledThreadExecutor(const PooledThreadExecutor&) = delete;
                PooledThreadExecutor& operator =(const PooledThreadExecutor&) = delete;
                PooledThreadExecutor(PooledThreadExecutor&&) = delete;
                PooledThreadExecutor& operator =(PooledThreadExecutor&&) = delete;

            protected:
                bool SubmitToThread(std::function<void()>&&) override;

            private:
                Aws::Queue<std::function<void()>*> m_tasks;
                std::mutex m_queueLock;
                Aws::Utils::Threading::Semaphore m_sync;
                Aws::Vector<ThreadTask*> m_threadTaskHandles;
                size_t m_poolSize;
                OverflowPolicy m_overflowPolicy;

                /**
                 * Once you call this, you are responsible for freeing the memory pointed to by task.
                 */
                std::function<void()>* PopTask();
                bool HasTasks();

                friend class ThreadTask;
            };


        } // namespace Threading
    } // namespace Utils
} // namespace Aws
