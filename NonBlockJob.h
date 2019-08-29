/*
 * NonBlockJob.h
 *
 *  Created on: Aug 21, 2019
 *      Author: ran-at
 */

#ifndef NONBLOCKJOB_H_
#define NONBLOCKJOB_H_

#include <iostream>
#include <future>
#include <signal.h>
#include <set>


// This class is intended for small functions / jobs involved with IO or SYSCALL.
// // that you want to wait on, only for a specific timeout.
// the class opens a thread and wait for it some time (using pthread_kill() combined with c++11 threads & promise-future).
// if the thread isn't finish after the timeout pass (probably due to sys call blocking),
// it would be signaled, and than join.

class  NonBlockJob {

	using SIGNAL = int;

// NESTED CLASSES

		// Forward deceleration for friendship
		class SignalStackGuard;

		// Nested class safe signals stack
		class SafeSignalsStack {

		public:

			SafeSignalsStack() {
				for (auto iSig = SIGRTMIN; iSig <= SIGRTMAX; ++iSig) {
					signal(iSig, SIG_IGN);
					siginterrupt(iSig, true); //enforce POSIX semantics (NOT retry the SysCall command)
					m_set.insert(iSig);
				}
			}

			static constexpr SIGNAL POP_FAILED = -1;

		private:
			friend class SignalStackGuard;

			void push(const SIGNAL& sig) {
			   std::lock_guard<std::mutex> lock(m_mutex);
			   m_set.insert(sig);
			}

			SIGNAL pop() {
			   std::lock_guard<std::mutex> lock(m_mutex);
			   SIGNAL sig = POP_FAILED;
			   if (! m_set.empty()) {
				   sig = (*m_set.begin());
				   m_set.erase(m_set.begin());
			   }
			   return sig;
			}

			std::mutex m_mutex;
			std::set<SIGNAL> m_set; // I have used std::set for as a unique_stack
		};


		// Nested class guard
		class SignalStackGuard {
		public:

			SignalStackGuard(SafeSignalsStack& lSignalStack)
			: m_SignalStack(lSignalStack)  {
				m_sig = m_SignalStack.pop();
			}

			~SignalStackGuard() {
				m_SignalStack.push(m_sig);
			}

			SIGNAL m_sig;

		private:
			SafeSignalsStack& m_SignalStack;

		};


// NonBlockJob START :: SINGELTON

public:

static NonBlockJob& getInstance() {
	// Since it's a static variable, if the class has already been created, it won't be created again.
	// And it **is** thread-safe in C++11.
	// will call constructor once, and be sets on data-segment instead on the heap.
	static NonBlockJob myInstance;

	// Return a reference to our instance.
	return myInstance;
}

// delete copy and move constructors and assign operators
NonBlockJob(NonBlockJob const&) = delete;             // Copy construct
NonBlockJob(NonBlockJob&&) = delete;                  // Move construct
NonBlockJob& operator=(NonBlockJob const&) = delete;  // Copy assign
NonBlockJob& operator=(NonBlockJob &&) = delete;      // Move assign


/////////////////////////////////		PUBLIC FUNCTIONS			////////////////////////////////////////////////


// HOW_TO_USE : this function ISN'T throw an exception.
// it should be use when your SYSCALL or IO job returns void,
// and you don't care about any return value from your job.
// should be used as the following example :
// if (	NonBlockJob::getInstance.try_non_blocking_void_job_for(std::chrono::milliseconds(2000), [this](){
// 				std::this_thread::sleep_for(std::chrono::seconds(10));
				// returning nothing
// 				}))
// {
//   printf("return true\n");
// }
// else {
//   printf("return false\n");
// }

template<typename FUNCTION>
bool try_non_blocking_void_job_for(std::chrono::milliseconds blockingTimeOut, FUNCTION f) {

	SignalStackGuard lSignalStackGuard(m_signalsStack); // THIS IS NOT A MUTEX
	if (lSignalStackGuard.m_sig == SafeSignalsStack::POP_FAILED) {
		std::cout << "Failed to get a free signal from stack. Maybe stack is empty!/n";
		return false;
	}

	try {

		 std::unique_ptr<std::thread> lthread(nullptr);
		 auto fut = asyncVoidJob(lthread, lSignalStackGuard.m_sig, f);
		 std::cout << "waiting...\n";

		 std::future_status status = fut.wait_for(blockingTimeOut);

		 if (status != std::future_status::ready) {
		    std::cout << "timeout pass!\n";
		    Quit(lthread, lSignalStackGuard.m_sig);
		    Join(lthread);
		    return false;
		} else {
			Join(lthread);
			return true;
		}
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
	catch (...) {
		std::cout << "Unknown exception caught" << std::endl;
	}
	return false;
}


// HOW_TO_USE : this function THROWS exceptions, and should be used only with try-catch as the following example :
// 	try {
// 		auto future =
// 			NonBlockJob::try_non_blocking_job_for(std::chrono::milliseconds(2000),
// 					[this, &data]() {
// 						return m_Wrapper->d_read(data);
// 					});
// 		std::cout << "future: " << future.get() << std::endl;
// 		if the thread didn't set yet the promise value, main thread will wait here
// 	}
// 	catch (const std::exception& e) {
//		if timeout pass we will get here!
// 		std::cout << "catch : " << e.what() << std::endl;
// 	}

template<typename FUNCTION>
auto try_non_blocking_job_for(std::chrono::milliseconds blockingTimeOut, FUNCTION f) -> std::future<decltype(f())> {

	SignalStackGuard lSignalStackGuard(m_signalsStack); // THIS IS NOT A MUTEX
	if (lSignalStackGuard.m_sig == SafeSignalsStack::POP_FAILED) {
		std::cout << "Failed to get a free signal from stack. Maybe stack is empty!/n";
		throw std::runtime_error("Failed to get a free signal from stack. Maybe stack is empty!");
	}

	try {

		std::unique_ptr<std::thread> lthread(nullptr);
		auto fut = asyncJob(lthread, lSignalStackGuard.m_sig, f);
		std::cout << "waiting...\n";
		std::future_status status = fut.wait_for(blockingTimeOut);

		if (status != std::future_status::ready) {
		    //std::cout << "timeout pass!\n";
		    Quit(lthread, lSignalStackGuard.m_sig);
		    Join(lthread);
		    throw std::runtime_error("timeout pass!");
		} else {
			Join(lthread);
			return fut;
		}
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		throw;
	}
	catch (...) {
		std::cout << "Unknown exception caught" << std::endl;
		throw;
	}
}


//////////////////////////////////////////		PRIVATE			////////////////////////////////////////////////
private:

NonBlockJob() {
}

virtual ~NonBlockJob() {
}

template<typename FUNCTION>
static std::future<bool> asyncVoidJob(std::unique_ptr<std::thread>& lthread, SIGNAL signum, FUNCTION f) {

	auto prom = std::make_shared<std::promise<bool> >(); // in order to future obj keep alive outside of this function

	lthread = std::unique_ptr<std::thread>(new std::thread([=]() { // ASYNC RUNTIME, pass by VALUE, shared ptr always by value
		try {

			signal(signum, sig_handler);

			f();
			prom->set_value(true);
		}
		catch (...) {
			prom->set_exception(std::current_exception());
		}

		// we will get here :
		// if function finish gracefully without blocking
		// after timeout, after signal handler (as long as sig_handler empty).
		std::cout << "hi void :)\n";

		signal(signum, SIG_IGN);

		})); // when using promise-futue use detach, if you will wait on future.get()

	return prom->get_future();
}


template<typename FUNCTION>
static auto asyncJob(std::unique_ptr<std::thread>& lthread, SIGNAL signum, FUNCTION f) -> std::future<decltype(f())> {

	using ret_type = decltype(f()); // not a real call to f, don't worry
	auto prom = std::make_shared<std::promise<ret_type> >(); // in order to future obj keep alive outside of this function

	lthread = std::unique_ptr<std::thread>(new std::thread([=]() { // ASYNC RUNTIME, pass by VALUE, shared ptr always by value
		try {

			signal(signum, sig_handler);
			auto res = f();
			prom->set_value(res);
		}
		catch (...) {
			prom->set_exception(std::current_exception());
		}

		// we will get here :
		// if function finish gracefully without blocking
		// after timeout, after signal handler (as long as sig_handler empty).
		std::cout << "hi :)\n";

		signal(signum, SIG_IGN);

		})); // when using promise-future use detach, if you will wait on future.get()

	return prom->get_future();
}


static void sig_handler(int signum) {
    std::cout << "thread signaled with " << signum << "signal" << std::endl;
    signal(signum, SIG_IGN);
}


static void Join(std::unique_ptr<std::thread>& thread) {
    if (thread != nullptr) {
	    if (thread->joinable()) {
		    std::cout << "going to join thread" << std::endl;
		    thread->join();
		    std::cout << "after join" << std::endl;
	    }
    }
    else {
      std::cout << "thread in NULL, maybe wasn't initilized yet" << std::endl;
    }
}

static void Quit(std::unique_ptr<std::thread>& thread, SIGNAL signum) {
    if (thread != nullptr) {
	    std::cout << "going to kill thread" << std::endl;
	    pthread_kill(thread->native_handle(), signum);
    }
    else {
      std::cout << "thread in NULL, maybe wasn't initilized yet" << std::endl;

    }
}

// PRIVATE MEMBERS

SafeSignalsStack m_signalsStack;

};


#endif /* NONBLOCKJOB_H_ */
