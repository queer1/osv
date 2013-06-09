#ifndef LOCKFREE_MUTEX_HH
#define LOCKFREE_MUTEX_HH
// A lock-free mutex implementation, based on the combination of two basic
// techniques:
// 1. Our lock-free multi-producer single-consumer queue technique
//    (see lockfree/queue-mpsc.hh)
// 2. The "responsibility hand-off" (RHO) protocol described in the 2007 paper
//    "Blocking without Locking or LFTHREADS: A lock-free thread library"
//    by Anders Gidenstam and Marina Papatriantafilou.
//
// The operation and correctness of the RHO protocol is discussed in the
// aforementioned G&P 2007 paper, so we will avoid lengthy comments about it
// below, except where we differ from G&P.
//
// One especially important issue that we do need to justify is:
// Our lockfree queue implementation assumes that there cannot be two
// concurrent pop()s. We claim that this is true in the RHO protocol because:
// 1. We have pop() calls at two places:
//    (A) In unlock(), after decrementing count and outside a handoff (=null)
//    (B) in lock(), after picking up a handoff.
// 2. We can't have two threads at (A) at the same time, because one thread
//    at (A) means another thread thread was just in lock() (because count>0),
//    but currently running lock()s cannot complete (and get to unlock and A)
//    until somebody will wake them it (and this is what we're trying to show
//    is impossible), and news lock()s will likewise wait because the waiting
//    lock() is keeping count>0.
// 3. While one lock() is at (B), we cannot have another thread at (A) or (B):
//    This is because in (B) we only pop() after picking a handoff, so other
//    lock()s cannot reach (B) (they did not pick the handoff, we did), and
//    unlock cannot be at (A) because it only reaches (A) before making the
//    handoff of after taking it back - and we know it didn't because we took
//    the handoff.
//
// Another difference from our implementation from G&P is the content of the
// handoff token. G&P use the processor ID, but remark that it is not enough
// because of the ABA problem (it is possible while a CPU running lock() is
// paused, another one finishes unlock(), and then succeeds in another lock()
// and then comes a different unlock() with its unrelated handoff) and suggest
// to add a per-processor sequence number. Instead, we just used a per-mutex
// sequence number. As long as one CPU does not pause for a long enough
// duration for our (currently 32-bit) sequence number to wrap, we won't have
// a problem. A per-mutex sequence number is slower than a per-cpu one, but
// I doubt this will make a practical difference.

#include <atomic>

#include <sched.hh>
#include <lockfree/queue-mpsc.hh>

namespace lockfree {

class mutex {
private:
    std::atomic<int> count;
    // "owner" and "depth" are need for implementing a recursive mutex, but
    // owner is also used to tell a thread being woken that it was made owner.
    unsigned int depth;
    std::atomic<sched::thread *> owner;
    queue_mpsc<sched::thread *> waitqueue;
    std::atomic<unsigned int> handoff;
    unsigned int sequence;
public:
    mutex() : count(0), depth(0), owner(nullptr), waitqueue(), handoff(0), sequence(0) { }
    ~mutex() { assert(count==0); }

    void lock();
    bool try_lock();
    void unlock();

};

}
#endif
