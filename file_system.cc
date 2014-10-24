
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
using namespace std;

// In this version of the code, I've left out the parameter lists
// (a.k.a. calling sequences) in most places.  You'll need to look
// them up (say via man pages) and fill them in.

class Monitor {
  // ...
};

class Condition {
public:
  Condition( Monitor* m ) {}
  // ...
};


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
  DeviceDriver* driver;  // null unless kind == device.  set by open
  string* bytes;         // null unless kind == regular.  set by open
                         // Not using major and minor numbers.
};  // Inode


list<Inode> ilist;  // Change this to a vector with free/release
// manual garbage collection, where new inodes are allocated to the
// first free entry.  That's what Unix does.

// A file's index within the ilist is called its "inumber."  The name
// manager (not described here) translates pathnames to inumbers, via
// the directory tree.


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


list<OpenFile> systemwideOpenFileTable;


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

int main() {}  // just to avoid compile-time diagnostics.
