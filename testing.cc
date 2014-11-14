#include<iostream>
#include<vector>
#include<queue>
#include<map>
#include<string>
#include<sstream>
#include<string.h>
#include<utility>
#include<vector>
#include<cassert>
#include<unistd.h>

using namespace std;

//==================== here are some utilities =====================

// a simple utility for splitting strings at a find-pattern.
inline vector<string>
split( const string s, const string pat ) {
  // Returns the vector of fragments (substrings) obtained by
  // splitting s at occurrences of pat.  Use C++'s string::find. If s
  // begins with an occurrence of pat, it has an empty-string
  // fragment.  Similarly for trailing occurrences of pat.  After each
  // occurrence of pat, scanning resumes at the following character,
  // if any.
  vector<string> v;
  int i = 0;
  for (;;) {
    int j = s.find(pat,i);
    if ( j == (int) string::npos ) {          // pat not found.
      v.push_back( s.substr(i) );      // append final segment.
      return v;
    }
    v.push_back( s.substr(i,j-i) ); // append nonfinal segment.
    i = j + pat.size();                     // next valus of i.
  }
}

template< typename T > string T2a( T x ) {
  // A simple utility to turn things of type T into strings
  stringstream ss;
  ss << x;
  return ss.str();
}

template< typename T > T a2T( string x ) {
  // A simple utility to turn strings into things of type T
  // Must be tagged with the desired type, e.g., a2T<float>(35)
  stringstream ss(x);
  T t;
  ss >> t;
  return t;
}

//===================== end of utilities =========================

typedef vector<string> Args;
typedef int App(Args);  // apps take a vector of strings and return an int.

// some forward declarations
class Device;
class Directory;


class InodeBase {
public: 
  string type;
  virtual string show() = 0;
};

template< typename T >
class Inode : public InodeBase {
  // put a static counter here.
};

template<>
class Inode<App> : public InodeBase {
public:
  string type = "app";
  App* file;
  Inode ( App* x ) 
    : file(x)
  {}
  string show() {   // a simple diagnostic aid
    return " This is an inode that describes a/an " + type + ".\n"; 
  } 
};

template<>
class Inode<Directory> : public InodeBase {
public:
  string type = "dir";
  Directory* file;
  Inode<Directory> ( Directory* x ) 
    : file(x)
  {}
  string show() {   // a simple diagnostic aid
    return " This is an inode that describes a/an " + type + ".\n"; 
  } 
};


class Directory {
public:

  // directories point to inodes, which in turn point to files.
  //map<string, Inode*> theMap;  // the data for this directory
  map<string, InodeBase*> theMap;  // the data for this directory

  void ls() {                             // print out this directory
    for (auto it : theMap) cout << it.second << " " << it.first << it.second->show(); 
  } 

  int rm( string s ) { theMap.erase(s); }

  void cd( string s, Inode<Directory>* &wdi, Inode<Directory>* root ) {
	if ( theMap.find(s) !=  theMap.end()) {
		InodeBase* ind = this->theMap[s];
		Inode<Directory>* dir_ptr = dynamic_cast<Inode<Directory>*>(ind);
		wdi = dynamic_cast<Inode<Directory>*>(theMap[s]	);
	}
	else if(s == ".." ) wdi = root;
	else {
		cerr << "cd: failed. No such file or directory named " << s 
             << ".\n";
	}
  }

  template<typename T>                               
  int mk( string s, T* x ) {
    Inode<T>* ind = new Inode<T>(x);
    theMap[s] = ind;
  }
 
};


//Inode<Directory>* root = new Inode<Directory>;
Inode<Directory> temp( new Directory );  // creates new Directory 
                                                 // and its inode
Inode<Directory>* const root = &temp;  
Inode<Directory>* wdi = root;       // Inode of working directory
Inode<Directory>* rdi = root;     	  // Inode of working directory
Directory* wd() { return wdi->file; }        // Working Directory
Directory* rootdir() { return rdi->file; }        // Root Directory

/*
InodeBase* search( Inode<Directory>* n, vector<string> v ) {
   // to process a path name
   Inode<Directory>* ind = wdi;
   if ( v[0] == "" ) ind = root;
   stringstream prefix;
   queue<string> suffix;
   for ( auto it : v ) suffix.push( it ); 
   for (;;) {
     InodeBase* x = ind;
     if ( suffix.empty() ) return ind;
     string s = suffix.front();
     suffix.pop();
     if ( prefix.str().size() != 0 ) prefix << "/";
     prefix << s; 
     if ( s == "" ) continue;
     if ( ind->type == "dir" && (ind->dir->theMap)[s] != 0 ) {
       ind = (ind->dir->theMap)[s];
       continue;
     }   
     cerr << prefix.str() << ": No such file or directory\n";
     return ind;
   }
 }
*/

int ls( Args tok ) {
  cout << wdi << " /" <<endl;
  wd()->ls();
}

int mkdir( Args tok ) {
  wd()->mk( tok[0], new Directory );
}   

int cddir( Args tok ) {
   if ( tok[0] != "mkdir" && tok[0] != "ls" && tok[0] !=  "cd" && tok[0] != "rmdir" && tok[0] != "exit")
       wd()->cd(tok[0], wdi, root);
   else {
	   cerr << "cd: failed. No such file or directory named " << tok[0]
            << ".\n";
   }   
}

int rmdir( Args tok ) {
  //Inode<Directory>* ind = wd()->theMap[tok[0]];
  InodeBase* ind = wd()->theMap[tok[0]];
  Inode<Directory>* dir_ptr = dynamic_cast<Inode<Directory>*>(ind);
  if ( ! dir_ptr ) { 
    cerr << "rmdir: failed to remove `" << tok[0] 
         << "'; no such file or directory.\n";
  } else if ( ! dir_ptr->file->theMap.empty() ) {
    cerr << "rmdir: failed to remove `" << tok[0] 
         << ": Directory not empty";  
  } else {
    wd()->rm(tok[0]);
  }
}


int exit( Args tok ) {
  _exit(0);
}


map<string, App*> apps = {
  pair<const string, App*>("ls", ls),
  pair<const string, App*>("mkdir", mkdir),
  pair<const string, App*>("rmdir", rmdir),
  pair<const string, App*>("exit", exit),
  pair<const string, App*>("cd", cddir)
};  // app maps mames to their implementations.


int main( int argc, char* argv[] ) {
  root->file = new Directory;   // OOPS!!! review this.
  for( auto it : apps ) {
    Inode<App>* temp(new Inode<App>(rmdir));
    InodeBase* junk = static_cast<InodeBase*>(temp);
    //    InodeBase* junk = temp;
    wd()->theMap[it.first] = new Inode<App>(it.second);
  }
  // a toy shell routine.
  while ( ! cin.eof() ) {
    cout << "? "  ;                                        // prompt.
    string temp; 
    getline( cin, temp );                    // read user's response.
    stringstream ss(temp);             // split temp at white spaces.
    string cmd;
    ss >> cmd;                              // first get the command.
    if ( cmd == "" ) continue;
    vector<string> args;          // then get its cmd-line arguments.
    string s;
    while( ss >> s ) args.push_back(s);

    //App* thisApp = static_cast<App*>((wd()->theMap)[cmd]->file);
    //if ( cmd == "mkdir" || cmd == "ls" || cmd ==  "cd" || cmd == "rmdir" || cmd == "exit") {
    if ( (rootdir()->theMap).find(cmd) !=  (rootdir()->theMap).end()) {
      Inode<App>* junk = static_cast<Inode<App>*>( (rootdir()->theMap)[cmd] );
      App* thisApp = static_cast<App*>(junk->file);
      thisApp(args);          // if possible, apply cmd to its args.
    } else { 
      cerr << "Instruction " << cmd << " not implemented.\n";
    }
  }

  cout << "exit" << endl;
  return 0;                                                  // exit.

}
