/*
 * NonBlockJob.h
 *
 *  Created on: Aug 21, 2019
 *      Author: ran-at
 */

#ifndef NONBLOCKJOB_H_
#define NONBLOCKJOB_H_

#include <future>
#include <set>
#include <signal.h>

// This class is intended for small functions / jobs involved with IO or SYSCALL.
// that you want to wait on, only for a specific timeout.
// the class opens a thread and wait for it some time.
// if the thread isn't finish after the timeout pass (probably due to sys-call blocking)
// it would be signaled, and than join.

// TIPS : If you wan't to use this class from different threads on the same time it's OK, under the following :
// 1) don't invoke the same IO or SYSCALL function from different jobs. If one of those jobs will be killed,
// the second one could not start at all, or get stuck. it's undefined behavior (could be mutual exclusion).
// 2) please avoid using secronized tools (mutexes, etc.) as much as possible inside the jobs.
// if a job will be killed, it could not release those resources (actually there is still a possibility that those
// resources would be released. As long as the sig_handler does nothing, the jobs continue just from when it was blocked).

class  NonBlockJob {
	using SIGNAL = int;

// NESTED CLASSES
		class SignalStackGuard; // Forward deceleration for friendship

		// Nested class Safe Signals Stack
		class SafeSignalsStack {
			friend class SignalStackGuard;

		public:
			SafeSignalsStack();
			static constexpr SIGNAL POP_FAILED = -1;

		private:
			void push(const SIGNAL& sig);
			SIGNAL pop();
			std::mutex m_mutex;
			std::set<SIGNAL> m_set; // I have used std::set for as a unique_stack
		};

		// Nested class Signal Stack Guard
		class SignalStackGuard {
		public:
			SignalStackGuard(SafeSignalsStack& lSignalStack);
			~SignalStackGuard();
			const SIGNAL& get() const;

		private:
			SIGNAL m_signal;
			SafeSignalsStack& m_SignalStack;
		};


// SINGELTON class NonBlockJob
public:

static NonBlockJob& getInstance();

// delete copy and move constructors and assign operators
NonBlockJob(NonBlockJob const&) = delete;             // Copy construct
NonBlockJob(NonBlockJob&&) = delete;                  // Move construct
NonBlockJob& operator=(NonBlockJob const&) = delete;  // Copy assign
NonBlockJob& operator=(NonBlockJob &&) = delete;      // Move assign


//////////////////////////////////////		PUBLIC FUNCTIONS	///////////////////////////////////////////


// HOW_TO_USE : this function THROWS exceptions, and should be used only with try-catch as the following example :
// 	try {
// 		auto future =
// 			NonBlockJob::getInstance().try_run_for(2000,
// 					[this, &data]() {
// 						return m_Wrapper->d_read(data);
// 					});
// 		std::cout << "future: " << fut.get() << std::endl;
// 		// if the thread didn't set yet the promise value, main thread will wait here
// 	}
// 	catch (const std::exception& e) {
//		when timeout pass we will get here!
// 		std::cout << "catch : " << e.what() << std::endl;
// 	}
// NOTE : If you don't have anything to get back from this non blocking job, you don't have to use a future as a func result.

template<typename FUNCTION>
auto try_run_for(unsigned int aTimeOutInMilliSec, FUNCTION&& aFunc) -> std::future<decltype(aFunc())> {

	SignalStackGuard lSignalStackGuard(m_signalsStack); // THIS IS NOT A MUTEX
	SIGNAL lSignal = lSignalStackGuard.get();
	if (lSignal == SafeSignalsStack::POP_FAILED) {
		printf("Failed to get a free signal from stack. Maybe stack is empty!\n");
		throw std::runtime_error("Failed to get a free signal from stack. Maybe stack is empty!");
	}

	try {
		std::thread lthread;
		auto lFuture = asyncJob(lthread, lSignal, aFunc);
		printf("waiting...\n");

		std::future_status status = lFuture.wait_for(std::chrono::milliseconds(aTimeOutInMilliSec));
		if (status != std::future_status::ready) {
			printf("try_run_for timeout [%d] has passed!\n", aTimeOutInMilliSec);
			Kill(lthread, lSignal);
			lthread.join();
		    throw std::runtime_error("try_run_for timeout passed!");
		}

		lthread.join();
		return lFuture;

	} catch (const std::exception& ex) {
		//printf("Abnormal sequence : std::exception [%s]\n", ex.what());
		throw;
	} catch (...) {
		printf("Abnormal sequence : Unknown exception caught\n");
		throw;
	}
}

//////////////////////////////////////////		PRIVATE		   ////////////////////////////////////////////////
private:

NonBlockJob() = default;
virtual ~NonBlockJob() = default;
// void SetTraceSubject(){SetSubjectName("NonBlockJob");}

template<typename FUNCTION>
void promSetter(FUNCTION&& aFunc, std::shared_ptr<std::promise<void> > aProm) {
    aFunc();
    aProm->set_value();
}

template<typename FUNCTION, typename T>
void promSetter(FUNCTION&& aFunc, std::shared_ptr<std::promise<T> > aProm) {
    auto res = aFunc();
    aProm->set_value(res);
}

template<typename FUNCTION>
auto asyncJob(std::thread& aThread, SIGNAL signum, FUNCTION&& aFunc) -> std::future<decltype(aFunc())> {

	using ret_type = decltype(aFunc()); // not a real call to aFunc, don't worry
	auto prom = std::make_shared<std::promise<ret_type> >(); // in order to keep alive future object outside of this function

	aThread = std::thread([=]() { // ASYNC RUNTIME, pass by VALUE. pass shared pointers always by value
		try {
			signal(signum, sig_handler);
			promSetter(aFunc, prom);
		} catch (...) {
			prom->set_exception(std::current_exception());
		}

		// we will get here if :
		// 1. function finish gracefully without blocking.
		// 2. after timeout has passed & after the signal handler (as long as sig_handler empty).

		printf("about finish asyncJob\n");
		signal(signum, SIG_IGN);

		});
	return prom->get_future();
}


void Kill(std::thread& thread, SIGNAL signum);
static void sig_handler(int signum);

// PRIVATE MEMBERS

SafeSignalsStack m_signalsStack;

};


#endif /* NONBLOCKJOB_H_ */
