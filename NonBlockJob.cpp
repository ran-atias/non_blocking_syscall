/*
 * NonBlockJob.cpp
 *
 *  Created on: Aug 21, 2019
 *      Author: ran-at
 */

#include "NonBlockJob.h"
#include <stdio.h>
#include <iostream>

using SIGNAL = int;

NonBlockJob::SafeSignalsStack::SafeSignalsStack() {
	for (auto iSig = SIGRTMIN; iSig <= SIGRTMAX; ++iSig) {
		signal(iSig, SIG_IGN);
		siginterrupt(iSig, true); //enforce POSIX semantics (NOT retry the SysCall command)
		m_set.insert(iSig);
	}
}

void NonBlockJob::SafeSignalsStack::push(const SIGNAL& sig) {
   std::lock_guard<std::mutex> lock(m_mutex);
   m_set.insert(sig);
}

SIGNAL NonBlockJob::SafeSignalsStack::pop() {
   std::lock_guard<std::mutex> lock(m_mutex);
   SIGNAL sig = POP_FAILED;
   if (! m_set.empty()) {
	   sig = (*m_set.begin());
	   m_set.erase(m_set.begin());
   }
   return sig;
}


NonBlockJob::SignalStackGuard::SignalStackGuard(SafeSignalsStack& lSignalStack)
: m_SignalStack(lSignalStack)  {
	m_signal = m_SignalStack.pop();
}

NonBlockJob::SignalStackGuard::~SignalStackGuard() {
	m_SignalStack.push(m_signal);
}

const SIGNAL& NonBlockJob::SignalStackGuard::get() const {
    return m_signal;
}


NonBlockJob& NonBlockJob::getInstance() {
	// Since it's a static variable, if the class has already been created, it won't be created again.
	// And it **is** thread-safe in C++11.
	// will call constructor once, and be sets on data-segment instead on the heap.

	static NonBlockJob myInstance;
	return myInstance;
}


void NonBlockJob::Kill(std::thread& thread, SIGNAL signum) {
	printf("Going to kill thread with signal [%d]\n", signum);
	pthread_kill(thread.native_handle(), signum);
}

void NonBlockJob::sig_handler(int signum) {
    //QTMTrace(Application::WARN, QTM_LOC, "thread signaled with [%d] signal", signum);
    signal(signum, SIG_IGN);
}
