
/*
This file contains a framework of classes for an implementation of a
file system.  It is intended as help for the whole class, but
especially the Files teams.  (It is not mandated.)  It follows the
general principle of starting out simple to get something running and
adding complexity and features as necessary.

We need to implement the major options of the major file-related
commands: ls, pwd, cd, mkdir, rmdir, mv, cp, etc.  To do so, we need
to implement (some versions of) the C functions that they call, e.g.,
chdir().  It's very helpful to begin with those that can be of most
use in debugging and/or demonstrating our system.  The objective is
ultimately to give Unix-familiar users the ability to test and debug
their multithreaded programs.

In choosing names for components of the system, e.g., functions, it is
good (but not absolutely necessary) to use the same name as those used
by Unix-like systems, or else use names that should be descriptive for
knowledgeable users --- we can always go back and substitute the
offical Unix name, if that seems needed/helpful.

For simplicity, similarity to Unix systems, and for uniformity of
size for array/vector allocation, I've merged dervived classes into
their base classes.

Also, I've replaced the major and minor number of device driver by
a simple pointer to the driver (instance) itself.  That simplifies
the code and doesn't seem to impair functionality -- we can easily
retrofit the standard Unix mechanism later, if we need/want to.

As a sidenote, each team should keep (in their project notebook) an
annotated list of documents that they used and/or found helpful.
*/



#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>
#include <map>


#include <pthread.h>
#include <semaphore.h>
#include <cassert>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <queue>
#include <iomanip>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <climits>
#include <mutex>
#include <condition_variable>

using namespace std;

template< typename T >
inline string T2a( T x ) { ostringstream s; s<<x; return s.str(); }
template< typename T >
inline string id( T x ) { return T2a( (unsigned long) x ); }

#define cdbg cerr << "\nLn " << __LINE__ << " of " << setw(8) << __FUNCTION__ << " by " << report() 

#define EXCLUSION Sentry exclusion(this); exclusion.touch();
class Thread;
extern string Him(Thread*);
extern string Me();
extern string report( );  

// ====================== priority aueue ============================

// pQueue behaves exactly like STL's queue, except that push has an
// optional integer priority argument that defaults to INT_MAX.
// front() returns a reference to the item with highest priority
// (i.e., smallest priority number) with ties broken on a FIFO basis.
// It may overflow and malfunction after INT_MAX pushes.

template<class T>                // needed for priority comparisions
bool operator<( const pair<pair<int,int>,T>& a, 
                const pair<pair<int,int>,T>& b  
              ) {
  return a.first.first > b.first.first ? true  :
         a.first.first < b.first.first ? false :
               a.first.second > b.first.second ;
} 

template <class T>
class pQueue {
  priority_queue< pair<pair<int,int>,T> > q;  
 int i;  // counts invocations of push to break ties on FIFO basis.
public:
  pQueue() : i(0) {;}
  void push( T t, int n = INT_MAX ) { 
    q.push( pair<pair<int,int>,T>(pair<int,int>(n,++i),t) ); 
  }
  void pop() { q.pop(); }
  T front() { return q.top().second; }
  int size() { return q.size(); }
  bool empty() { return q.empty(); }
};

// =========================== interrupts ======================

class InterruptSystem {
public:       // man sigsetops for details on signal operations.
  static void handler(int sig);
  // exported pseudo constants
  static sigset_t on;                    
  static sigset_t alrmoff;    
  static sigset_t alloff;     
  InterruptSystem() {  
    signal( SIGALRM, InterruptSystem::handler ); 
    sigemptyset( &on );                    // none gets blocked.
    sigfillset( &alloff );                  // all gets blocked.
    sigdelset( &alloff, SIGINT );
    sigemptyset( &alrmoff );      
    sigaddset( &alrmoff, SIGALRM ); //only SIGALRM gets blocked.
    set( alloff );        // the set() service is defined below.
    struct itimerval time;
    time.it_interval.tv_sec  = 0;
    time.it_interval.tv_usec = 400000;
    time.it_value.tv_sec     = 0;
    time.it_value.tv_usec    = 400000;
    cerr << "\nstarting timer\n";
    setitimer(ITIMER_REAL, &time, NULL);
  }
  sigset_t set( sigset_t mask ) {
    sigset_t oldstatus;
    pthread_sigmask( SIG_SETMASK, &mask, &oldstatus );
    return oldstatus;
  } // sets signal/interrupt blockage and returns former status.
  sigset_t block( sigset_t mask ) {
    sigset_t oldstatus;
    pthread_sigmask( SIG_BLOCK, &mask, &oldstatus );
    return oldstatus;
  } //like set but leaves blocked those signals already blocked.
};

// signal mask pseudo constants
sigset_t InterruptSystem::on;   
sigset_t InterruptSystem::alrmoff; 
sigset_t InterruptSystem::alloff;  

InterruptSystem interrupts;               // singleton instance.


// ========= Threads, Semaphores, Sentry's, Monitors ===========

// class Semaphore {           // C++ wrapper for Posix semaphores.
//   sem_t s;                               // the Posix semaphore.
// public:
//   Semaphore( int n = 0 ) { assert( ! sem_init(&s, 0, n) ); }
//   ~Semaphore()           { assert( ! sem_destroy(&s) );    }
//   void release()         { assert( ! sem_post(&s) );       }
//   void acquire() {
//     sigset_t old = interrupts.set( InterruptSystem::alloff );
//     sem_wait( &s );
//     interrupts.set( old );
//   }
// };


class Semaphore {
private:
    mutex mtx;
    condition_variable available;
    int count;

public:
    Semaphore(int count = 0):count(count){;}
    void release() {
        unique_lock<mutex> lck(mtx);
        ++count;
        available.notify_one();
    }
    void acquire() {
        unique_lock<mutex> lck(mtx);
        while(count == 0) available.wait(lck);
        count--;
    }
};


class Lock : public Semaphore {
public:             // A Semaphore initialized to one is a Lock.
  Lock() : Semaphore(1) {} 
};


class Monitor : Lock {
  friend class Sentry;
  friend class Condition;
  sigset_t mask;
public:
  void lock()   { Lock::acquire(); }
  void unlock() { Lock::release(); }
  Monitor( sigset_t mask = InterruptSystem::alrmoff ) 
    : mask(mask) 
  {}
};


class Sentry {            // An autoreleaser for monitor's lock.
  Monitor&  mon;           // Reference to local monitor, *this.
  const sigset_t old;    // Place to store old interrupt status.
public:
  void touch() {}          // To avoid unused-variable warnings.
  Sentry( Monitor* m ) 
    : mon( *m ), 
      old( interrupts.block(mon.mask) )
  { 
    mon.lock(); 
  }
  ~Sentry() {
    mon.unlock();
    interrupts.set( old ); 
  }
};


template< typename T1, typename T2 >
class ThreadSafeMap : Monitor {
  map<T1,T2> m;
public:
  T2& operator [] ( T1 x ) {
    EXCLUSION
    return m[x];
  }
};


class Thread {
  friend class Condition;
  friend class CPUallocator;                      // NOTE: added.
  pthread_t pt;                                    // pthread ID.
  static void* start( Thread* );
  virtual void action() = 0;
  Semaphore go;
  static ThreadSafeMap<pthread_t,Thread*> whoami;  

  virtual int priority() { 
    return INT_MAX;      // place holder for complex CPU policies.
  }   

  void suspend() { 
    cdbg << "Suspending thread \n";
    go.acquire(); 
    cdbg << "Unsuspended thread \n";
  }

  void resume() { 
    cdbg << "Resuming \n";
    go.release(); 
  }

  int self() { return pthread_self(); }

  void join() { assert( pthread_join(pt, NULL) ); }

public:

  string name; 

  static Thread* me();

  virtual ~Thread() { pthread_cancel(pt); }  

  Thread( string name = "" ) 
    : name(name)
  {
    cerr << "\ncreating thread " << Him(this) << endl;
    assert( ! pthread_create(&pt,NULL,(void*(*)(void*))start,this));
  }

};


class Condition : pQueue< Thread* > {
  Monitor&  mon;                     // reference to local monitor
public:
  Condition( Monitor* m ) : mon( *m ) {;}
  int waiting() { return size(); }
  bool awaited() { return waiting() > 0; }
  void wait( int pr = INT_MAX );    // wait() is defined after CPU
  void signal() { 
    if ( awaited() ) {
      Thread* t = front();
      pop();
      cdbg << "Signalling" << endl;
      t->resume();
    }
  }
  void broadcast() { while ( awaited() ) signal(); }
}; 


// ====================== AlarmClock ===========================

class AlarmClock : Monitor {
  unsigned long now;
  unsigned long alarm;
  Condition wakeup;
public:
  AlarmClock() 
    : now(0),
      alarm(INT_MAX),
      wakeup(this)
  {;}
  int gettime() { EXCLUSION return now; }
  void wakeme_at(int myTime) {
    EXCLUSION
    if ( now >= myTime ) return;  // don't wait
    if ( myTime < alarm ) alarm = myTime;      // alarm min= myTime
    while ( now < myTime ) {
      cdbg << " ->wakeup wait " << endl;
      wakeup.wait(myTime);
      cdbg << " wakeup-> " << endl;
      if ( alarm < myTime ) alarm = myTime;
    }
    alarm = INT_MAX;
    wakeup.signal();
  }
  void wakeme_in(int time) { wakeme_at(now+time); }
  void tick() { 
    EXCLUSION
    cdbg << "now=" << now << " alarm=" << alarm
         << " sleepers=" << wakeup.waiting() << endl;
    if ( now++ >= alarm ) wakeup.signal();
  }
};

extern AlarmClock dispatcher;                  // singleton instance.


class Idler : Thread {                       // awakens periodically.
  // Idlers wake up periodically and then go back to sleep.
  string name;
  int period;
  int priority () { return 0; }                  // highest priority.
  void action() { 
    cdbg << "Idler running\n";
    int n = 0;
    for(;;) { 
      cdbg << "calling wakeme_at\n";
      dispatcher.wakeme_at( ++n * period ); 
    } 
  }
public:
  Idler( string name, int period = 5 )    // period defaults to five.
    : Thread(name), period(period)
  {}
};


//==================== CPU-related stuff ========================== 


// The following comment is left over from an old assignment, but is
// has some useful comments embedded in it.  (10/7/2014)

// /*
// class CPUallocator : Monitor {                            
//   // a scheduler for the pool of CPUs or anything else.  (In reality
//   // we are imlementing a prioritized semaphore.)

//   friend string report();
//   pQueue<Thread*> ready;       // queue of Threads waiting for a CPU.
//   int cpu_count;           // count of unallocated (i.e., free) CPUs.

// public:

//   CPUallocator( int n ) : cpu_count(n) {;}

//   void release()  {
//     EXCLUSION
//     // Return your host to the pool by incrementing cpu_count.
//     // Awaken the highest priority sleeper on the ready queue.

//     // Fill in your code.

//   }

//   void acquire( int pr = Thread::me()->priority() ) {
//     EXCLUSION

//     // While the cpu_count is zero, repeatedly, put yourself to 
//     // sleep on the ready queue at priority pr.  When you finally 
//     // find the cpu_count greater than zero, decrement it and 
//     // proceed.
//     // 
//     // Always unlock the monitor the surrounding Monitor before 
//     // going to sleep and relock it upon awakening.  There's no
//     // need to restore the preemption mask upon going to sleep; 
//     // this host's preemption mask will automatically get reset to 
//     // that of its next guest.

//     // Fill in your code here.

//   }

//   void defer( int pr = Thread::me()->priority() ) {
//     release(); 
//     acquire(pr); 
//     // This code works but is a prototype for testing purposes only:
//     //  * It unlocks the monitor and restores preemptive services
//     //    at the end of release, only to undo that at the beginning 
//     //    of acquire.
//     //  * It gives up the CPU even if this thread has the higher 
//     //    priority than any of the threads waiting on ready.
//   }

//   void defer( int pr = Thread::me()->priority() ) {
//     EXCLUSION
//     if ( ready.empty() ) return;
//     //  Put yourself onto the ready queue at priority pr.  Then,
//     //  extract the highest priority thread from the ready queue.  If
//     //  that thread is you, return.  Otherwise, put yourself to sleep.
//     //  Then proceed as in acquire(): While the cpu_count is zero,
//     //  repeatedly, put yourself to sleep on the ready queue at
//     //  priority pr.  When you finally find the cpu_count greater than
//     //  zero, decrement it and proceed.

//     // Fill in your code.

//   }

// };
// */


// class CPUallocator {
//   // a scheduler for the pool of CPUs.
//   Semaphore sem;         // a semaphore appearing as a pool of CPUs.
// public:
//   int cpu_count;  // not used here and set to visibly invalid value.
//   pQueue<Thread*> ready;
//   CPUallocator( int count ) : sem(count), cpu_count(-1) {}
//   void defer() { sem.release(); sem.acquire(); }
//   void acquire( int pr = INT_MAX ) { sem.acquire(); }
//   void release() { sem.release(); }
// };


// This is my current CPUallocator (thp 10/8/2014)
class CPUallocator : Monitor {                            

  // a prioritized scheduler for the pool of CPUs or anything else.
  friend string report();
  pQueue<Thread*> ready;       // queue of Threads waiting for a CPU.
  int cpu_count;           // count of unallocated (i.e., free) CPUs.

public:

  CPUallocator( int n ) : cpu_count(n) {;}

  void release() {
    // called from Conditon::wait()
    EXCLUSION
    ++ cpu_count;                     // return this CPU to the pool.
    assert( cpu_count > 0 );
    if ( ready.empty() ) return;          // caller will now suspend.
    Thread* t = ready.front();
    ready.pop();
    t->resume();
  }

  void acquire( int pr = Thread::me()->priority() ) {
    EXCLUSION
    while ( cpu_count == 0 ) { // sleep, waiting for a nonempty pool.
      ready.push( Thread::me(), pr );
      unlock();            // release the lock before going to sleep.
      // The host's next guest will reset you host's preemption mask.
      Thread::me()->suspend();
      lock();                 // reacquire lock after being awakened.
    }
    assert( cpu_count > 0 );
    -- cpu_count;         // decrement the CPU pool (i.e., take one).
  }

  void defer( int pr = Thread::me()->priority() ) {
    EXCLUSION
    if ( ready.empty() ) return;
    assert ( cpu_count == 0 );   
    ready.push( Thread::me(), pr );
    Thread* t = ready.front();            // now ready is not empty.
    ready.pop();
    if ( t == Thread::me() ) return;
    ++cpu_count;  // leaving a CPU for *t or whoever beat him to it.
    assert( cpu_count == 1 );
    t->resume();    
    unlock();
    Thread::me()->suspend();
    lock();                 // reacquire lock after getting resumed.
    while ( cpu_count == 0 ) { 
      ready.push( Thread::me(), pr );
      unlock();
      Thread::me()->suspend();
      lock();
    }
    assert( cpu_count > 0 );
    --cpu_count;                          // taking a CPU for *me().
  }

};

extern CPUallocator CPU;  // single instance, init declaration here.


void InterruptSystem::handler(int sig) {                  // static.
  static int tickcount = 0;
  cdbg << "TICK " << tickcount << endl; 
  dispatcher.tick(); 
  if ( ! ( (tickcount++) % 3 ) ) {
    cdbg << "DEFERRING \n"; 
    CPU.defer();                              // timeslice: 3 ticks.
  }
  assert( tickcount < 35 );
} 


void* Thread::start(Thread* myself) {                     // static.
  interrupts.set(InterruptSystem::alloff);
  cerr << "\nStarting thread " << Him(myself)  // cdbg crashes here.
       << " pt=" << id(pthread_self()) << endl; 
  assert( myself );
  whoami[ pthread_self() ] = myself;
  assert ( Thread::me() == myself );
  interrupts.set(InterruptSystem::on);
  cdbg << "waiting for my first CPU ...\n";
  CPU.acquire();  
  cdbg << "got my first CPU\n";
  myself->action();
  cdbg << "exiting and releasing cpu.\n";
  CPU.release();
  pthread_exit(NULL);   
}


void Condition::wait( int pr ) {
  push( Thread::me(), pr );
  cdbg << "releasing CPU just prior to wait.\n";
  mon.unlock();
  CPU.release();  
  cdbg << "WAITING\n";
  Thread::me()->suspend();
  CPU.acquire();  
  mon.lock(); 
}


// ================ application stuff  ==========================

class InterruptCatcher : Thread {
public:
  // A thread to catch interrupts, when no other thread will.
  int priority () { return 1; }       // second highest priority.
  void action() { 
   // cdbg << "now running\n";
    for(;;) {
      CPU.release();         
     // pause(); 
    //  cdbg << " requesting CPU.\n";
      CPU.acquire();
    }
  }
public:
  InterruptCatcher( string name ) : Thread(name) {;}
};

/*
class Pauser {                            // none created so far.
public:
  Pauser() { pause(); }
};

*/

// ================== threads.cc ===================================

// Single-instance globals

ThreadSafeMap<pthread_t,Thread*> Thread::whoami;         // static
Idler idler(" Idler ");                        // single instance.
InterruptCatcher theInterruptCatcher("IntCatcher");  // singleton.
AlarmClock dispatcher;                         // single instance.
CPUallocator CPU(1);                 // single instance, set here.
string Him( Thread* t ) { 
  string s = t->name;
  return s == "" ? id(t) : s ; 
}
Thread* Thread::me() { return whoami[pthread_self()]; }  // static
string Me() { return Him(Thread::me()); }

               // NOTE: Thread::start() is defined after class CPU


// ================== diagnostic functions =======================

string report() {               
  // diagnostic report on number of unassigned cpus, number of 
  // threads waiting for cpus, and which thread is first in line.
  ostringstream s;
  s << Me() << "/" << CPU.cpu_count << "/" << CPU.ready.size() << "/" 
    << (CPU.ready.empty() ? "none" : Him(CPU.ready.front())) << ": ";
  return s.str(); 
}

// ===================== mulitor.cc ================================

// #include "mulitortest.H"


// =================== main.cc ======================================

//
// main.cc 
//

// applied monitors and thread classes for testing

class SharableInteger: public Monitor {
public:
  int data;
  SharableInteger() : data(0) {;}
  void increment() {
    EXCLUSION
    ++data;
  }
  string show() {
    EXCLUSION
    return T2a(data);
  }  
} counter;                                        // single instance




// ======= control order of construction of initial threads  ========


// In this version of the code, I've left out the parameter lists
// (a.k.a. calling sequences) in most places.  You'll need to look
// them up (say via man pages) and fill them in.


/*
class Monitor {
  // ...
};

class Condition {
public:
  Condition( Monitor* m ) {}
  // ...
};*/


// Note that the methods of these Monitors need EXCLUSION.  Including
// constructors and destructors?  Open question: what about
// initialization lists; what protects them?  I suspect that, ilist
// and systemwideOpenFileTable will need to be encapsulated within
// monitors that protect the creation and destruction of their
// entries.


class DeviceDriver : Monitor {

  Condition ok2read;   // nonempty
  Condition ok2write;  // nonfull

public: 

  DeviceDriver() 
    : ok2read(this), ok2write(this)
  {
    // initialize this device
    // install completion handler
  }

  ~DeviceDriver() {
    // finalize this device: whatever's opposite of initialize
  }

  int registerDevice( string s ) {
    // create an inode for this device, install it into the 
    // ilist, and return it inumbner, i.e., index within the
    // ilist.
  }

  void online() {
    // make this device accessible
  }

  void offline() {
    // make this device inaccessible
  }

  void fireup()  { // begin processing a stream of data.
    // device-specific implementation
  }

  void suspend() { // end processing of astream of data.
    // device-specific implementation
  }
  
  int read(...) {
    // device-specific implementation
  }
  
  int write(...) {
    // device-specific implementation
  }

  int seek(...) {
    // device-specific implementation
  }

  int rewind(...) {
    // device-specific implementation
  }

  int ioctl(...) {
    // device-specific implementation
  }

};

class Inode : Monitor {
public:
  int linkCount; // =0 // incremented/decremented by ln/rm
  int openCount; // =0 // incremented/decremented by OpenFile()/~OPenFile()
  bool readable; // = false;  // set by create. 
  bool writeable; // = false; // set by create.
  enum Kind { regular, directory, symlink, pipe, socket, device } kind;
  // kind is set by open.
  time_t ctime, mtime, atime;


  // ******* SET THE ctime, mtime, ...etc ********s
  Inode(Kind kind)
  : kind(this->kind), linkCount(0), openCount(0), readable(0),
     writeable(0), ctime(0), mtime(0), atime(0)
  {}
  
  ~Inode() {
  }

  int unlink() { 
    assert( linkCount > 0 );
    -- linkCount;
    cleanup();
  }
  void cleanup() {
    if ( ! openCount && ! linkCount ) {
      // throwstuff away
    }
  }
  DeviceDriver* driver;       // null unless kind == device.  set by open
  string* bytes;              // null unless kind == regular.  set by open
  map<string, Inode*> dir;    // Directories
  void* func;                 // Functions
                              // Not using major and minor numbers.
};  // Inode



// GLOBAL
// Shell team will have the iNumber for the Current Working Directory
vector<Inode> ilist;  // Change this to a vector with free/release
// manual garbage collection, where new inodes are allocated to the
// first free entry.  That's what Unix does.

// A file's index within the ilist is called its "inumber."  The name
// manager (not described here) translates p	athnames to inumbers, via
// the directory tree.



class Directory {
public:

  // directories point to inodes, which in turn point to files.
  map<string, Inode*> theMap;  // the data for this directory

  void ls() {                             // print out this directory
    for (auto it : theMap) cout << it.first << it.second->show(); 
  } 

  int rm( string s ) { theMap.erase(s); }

  template<typename T>                               
  int mk( string s, T* x ) {
    Inode<T>* ind = new Inode<T>(x);
    theMap[s] = ind;
  }
 
};


class OpenFile : Monitor {  
public:

  int accessCount;  // = 0;        // incremented by open()
  // the following are filled in by open()
  Inode* inode;        
  bool open4read;      
  bool open4write;     
  int byteOffset; // = 0; 
  string* bytes;
  DeviceDriver* driver; 
  Inode::Kind kind;    

  OpenFile( Inode* inode )        // use of RAII
    : inode(inode), 
      // chche some info from inode into loacal variables:
      kind(inode->kind),
      bytes(inode->bytes),
      driver(inode->driver)
  {}

  ~OpenFile() {                    // use of RAII
    --inode->openCount;
    inode->cleanup();
  }

  int close() {
    // pass to inode and invoke
  }

  // These operations must update access times in inode
  int read(...)  {
    if ( kind == Inode::device ) return driver->read(); 
    // implement here read(...) on regular files here
  }
  int write(...) {
    if ( kind == Inode::device ) return driver->write(); 
    // implement here write(...) to regular files here
  }
  int seek(...)  {
    if ( kind == Inode::device ) return driver->seek(); 
    // implement here read(...) on regular files here
  }
  int rewind( int pos ) {
    if ( kind == Inode::device ) return driver->rewind(); 
    // implement rewind(...) from regular files here
  } 
  int ioctl( ... ) {
    if ( kind == Inode::device ) return inode->driver->ioctl(); 
    // report error; I don't think that regular files support ioctl()
    // Please check this.
  }

};

// Will rid This
list<OpenFile> systemwideOpenFileTable; // By the Threads Team {Per Process}


class ThreadLocalOpenFile {        // don't need to be Monitors
  OpenFile* file;
  ThreadLocalOpenFile( OpenFile* file )     // use of RAII
    : file(file)
  {
    ++ file->accessCount;
  }
  ~ThreadLocalOpenFile() {                  // use of RAII
    -- file->accessCount;
  }
};


// =============================================================


/*
#include<stdio.h>
int main() {  // to create a file
  FILE *fp;
  char ch;
  fp = fopen("file.txt","w");
  printf("\nEnter data to be stored in to the file:");
  while( (ch=getchar()) != EOF )
    putc(ch,fp);
  fclose(fp);
    
  return 0;
}


#include <stdio.h>
int main () { // to rename a file
   int ret;
   char oldname[] = "file.txt";
   char newname[] = "newfile.txt";
   ret = rename(oldname, newname);
   if(ret == 0) {
      printf("File renamed successfully");
   } else {
      printf("Error: unable to rename the file");
   }
   return(0);
}


To create a file of a given name read this: 
http://en.wikibooks.org/wiki/C_Programming/C_Reference/stdio.h/fopen.
This creates an inode and installs it into a directory.


*/
/*
void mkdir (string path, map<string, Inode*> &dirname) {
	//Does it exist?
	
	//If it Doesn't
		Inode* newDir = new Inode(directory);
		dirname[path] = newDir;
}*/

int main() {
  	// Creating Inode & Map for Root
  	Inode* root = new Inode(Inode::Kind(1));
  	Inode* files = new Inode(Inode::Kind(1));
  	Inode* devices = new Inode(Inode::Kind(1));
  	Inode* functions = new Inode(Inode::Kind(1));
  	Inode* file1 = new Inode(Inode::Kind(0));
  	map<string, Inode*> rootDir;
  	rootDir["/root"] = root;
  	rootDir["/root"]->dir["/devices"] = devices;
  	rootDir["/root"]->dir["/files"] = files;
  	rootDir["/root"]->dir["/functions"] = functions;
	rootDir["/root/file.txt"] = file1;
  	string path;
  	
  	cout << "Enter path:";
  	cin >> path;
  	
  	// mkdir XXXXX
  	// breakup into two tokens {cmd & pathname}
  	// break-up pathname into tokens & returns the Inode
  	// 3 If Statements
		// 1 - If it is in current directory
		// 2 - If at any token directory doesn't exists, if not make them all
		// 3 - If directory does exist, make it in the right directory
  	
  	
  	
}  // just to avoid compile-time diagnostics.
