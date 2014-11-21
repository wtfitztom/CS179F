// Here is the latest version as of noon Saturday.  It implements the
// following commands: exit mkdir ls rmdir rm cd all of which have
// pass a superfical level of testing.  For diagnostic purposes, their
// outputs are a bit different from Standard Unix-systems output.

// Problems: 
// * mkdir foo/junk works even if foo does not exit but puts
//   junk into current dir.

#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <sstream>
#include <string.h>
#include <utility>
#include <vector>
#include <cassert>
#include <unistd.h>
#include <ctime>
#include <iomanip>

using namespace std;
namespace filesystem {
//==================== here are some utilities =====================

bool correct_pathname(const string& path) {
size_t first_wrong = path.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789./");
return first_wrong == string::npos;
}

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

inline  // joins vector of strings separating via a pattern.
string
join( const vector<string> v, const string pat, int start=0, int end=-1 ) {
  if ( end < 0 ) end += v.size();
  //  assert ( start < v.size() );   // should throw an exception.
  if ( start >= v.size() ) return "";
  string s = v[start++];
  for ( int i = start; i <= end; ++i )  s += pat + v[i];
  return s;
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

typedef vector<string> Args;  // could as well use list<string>
typedef int App(Args);  // apps take a vector of strings and return an int.

// some forward declarations
class Device;
class Directory;


class InodeBase {
public: 
  virtual string type() = 0;
  virtual string show() = 0;
  virtual void ls() = 0; 
  static int count;
  time_t a_time = time(0);
  time_t c_time = time(0);
  time_t m_time = time(0);
  int openCount = 0;
  int linkCount = 1;
  bool readable, writeable;
  InodeBase() { idnum = count ++;}  
  int idnum;
  
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
};
int InodeBase::count = 0;

template< typename T >
class Inode : public InodeBase {
};

template<>
class Inode<App> : public InodeBase {
public:
  App* file;
  Inode ( App* x ) : file(x) {}
  string type() { return "app"; }
  string show() {   // a simple diagnostic aid
    return "This is inode #" + T2a(idnum) + ", which describes a/an " + type() + " at " + ctime(&m_time); 
  } 
  void ls() { cout << show(); }
};


class Directory {
public:
  Inode<Directory>* parent = NULL;
  Inode<Directory>* current = parent;

  // directories point to inodes, which in turn point to files.

  // map<string, Inode*> theMap;  // the data for this directory
  map<string, InodeBase*> theMap;  // the data for this directory

  Directory() { theMap.clear(); }

  void ls() {   
    // for (auto& it : theMap) { 
    for (auto it = theMap.begin(); it != theMap.end(); ++it ) { 
      cout << it->first << " " << it->second->show();
      //if ( it->second ) cout << setw(14) << left << it->first; 
    }
    if ( ! theMap.size() ) cout << "Empty Map"<< endl;  // comment lin out
	//else cout <<setw(1) << endl;
  } 

  int rm( string s ) { theMap.erase(s); }

  template<typename T>                               
  int mk( string s, T* x ) {
    Inode<T>* ind = new Inode<T>(x);
    theMap[s] = ind;
  }
 
};


template<>
class Inode<Directory> : public InodeBase {
public:
  string type() { return "dir"; }
  Directory* file;
  Inode<Directory> ( Directory* x ) : file(x) {}
  string show() {   // a simple diagnostic aid
    return " This is inode #" + T2a(idnum) + ", which describes a/an " + type() + " at " + ctime(&m_time); 
  } 
  void ls() { file->ls(); }
};


class File {
public:
  Inode<Directory>* parent = NULL;
  string bytes;
  File() {};

  template<typename T>                               
  int touch( string s, T* x ) {
    Inode<T>* ind = new Inode<T>(x);
    parent->file->theMap[s] = ind;
  }
};


template<>
class Inode<File> : public InodeBase {
public:
  string type() { return "file"; }
  File* file;
  Inode<File> ( File* x ) : file(x) {}
  string show() {   // a simple diagnostic aid
    return " This is inode #" + T2a(idnum) + ", which describes a/an " + type() + " at " + ctime(&m_time); 
  } 
  void ls() {}
};



//Inode<Directory>* root = new Inode<Directory>;
Inode<Directory> temp( new Directory );  // creates new Directory 
                                                 // and its inode
Inode<Directory>* const root = &temp;  
Inode<Directory>* wdi = root;       // Inode of working directory
Directory* wd() { return wdi->file; }        // Working Directory


// Inode<Directory>* search( string s ) {
//   // To process a path name prefix, which should lead to a directory.
//   // s is all of tok[0]
//   // cerr << "search: s = " << s << endl; // Dignostic
//   assert( s != "" );
//   vector<string> v = split(s,"/");
//   Inode<Directory>* ind = ( v[0] == "" ? root : wdi );
//   stringstream prefix;
//   queue<string> suffix;
//   for ( auto it : v ) suffix.push( it ); // put everything onto a queue
//   for (;;) {  // now iterate through that queue.
//     while ( suffix.size() && suffix.front() == "" ) suffix.pop();
//     if ( suffix.empty() ) return ind;
//     string seg = suffix.front();
//     suffix.pop();
//     if ( prefix.str().size() != 0 ) prefix << "/";
//     prefix << seg; 
//     Directory* dir = dynamic_cast<Directory*>(ind->file);
//     if ( ! dir ) {
//       cerr << ": cannot access "<< prefix << ": Not a directory\n";
//       return 0;
//     }
//     if ( (dir->theMap)[seg] != 0 ) {
//       ind = dynamic_cast<Inode<Directory>*>( (dir->theMap)[seg] );
//       if ( !ind ) {
//         cerr << prefix.str() << ": No such file or directory\n";
//         return 0;
//       }
//     }   
//   }
//   return 0;
// }


string current = "/";


class SetUp {
public:
  string input = "";
  int segCount = 0;
  string firstSeg = "";
  string next2lastSeg = "";
  string lastSeg = "";
  Inode<Directory>* ind = 0;
  InodeBase* b = 0;
  bool error = false;
  string cmd;
  vector<string> args;
  void show() {  // Diagnostic
    cerr << "input = " << input << endl;
    cerr << "segcount = " << segCount << endl;
    cerr << "firstSeg = " << firstSeg << endl;
    cerr << "next2lastSeg = " << next2lastSeg << endl;
    cerr << "lastSeg = " << lastSeg << endl;
    cerr << "ind = " << T2a<int>(long(ind)) << endl;
    cerr << "cmd = " << cmd << endl;
    for ( auto it : args ) cerr << it << " ";
    cerr << endl;
  }
  SetUp ( vector<string> args )   
    : args(args),
      cmd(args[0])
  { input = args.size() > 1 ? args[1] : "";
    if ( input == "" ) return;
    vector< string > v = split( input, "/" );
    segCount = v.size();
    assert( segCount > 0 );
    firstSeg = v.front();  
    lastSeg = v.back();    // might be same as firstSeg
    v.pop_back();
    if ( segCount > 1 ) next2lastSeg = v.back();
    ind = wdi;
    queue<string> suffix;
    stringstream prefix;
    if ( segCount > 1 ) {  
      if ( v[0] == "" ) ind = root;
      for ( auto it : v ) suffix.push( it ); // put everything onto a queue.
      for (;;) {                          // now iterate through that queue.
        // must check each intermediated directory for existence.  FIX
		while ( suffix.size() && suffix.front() == "" ) suffix.pop();
		if ( suffix.empty() ) break;
		string seg = suffix.front();
		suffix.pop();
		if ( prefix.str().size() != 0 ) prefix << "/";
		prefix << seg; 
		Directory* dir = dynamic_cast<Directory*>(ind->file);  
		if ( ! dir ) {
		  cerr << ": cannot access " << prefix.str() << ": Not a directory\n";
		  break;
		}
		if (seg == ".") continue;
		else if (seg == "..") {
          if(ind->file->parent != NULL) ind = ind->file->parent;
		}
		else if ( dir->theMap.find(seg) != dir->theMap.end() ) {    // Added check so only valids can be added to map
		  ind = dynamic_cast<Inode<Directory>*>( (dir->theMap)[seg] );
		  if ( ! ind ) {
			cerr << prefix.str() << ": No such file or directory\n";
			break;
		  }
		}
		else if ( cmd == "mkdir" || cmd == "touch") {
		  cerr << cmd << ": cannot create directory `" << prefix.str() << "/" 
		       << lastSeg << "': No such file or directory\n";
		  error = true;
		}  
      }
    }
    if ( ! ind ) {
      cerr << cmd <<": " << input << " not found\n"; 
      error = true;
      return;
    }
    b = ( lastSeg == "" ? ind : (ind->file->theMap.find(lastSeg) != ind->file->theMap.end() ? ind->file->theMap[lastSeg] : 0)); // Added if-statement so only valids can be added to map
    if ( !b  && cmd != "mkdir" && cmd !="touch") {
      if(lastSeg == "." && ind == root) { // Added checks for . & .. directories
        lastSeg = "/";
        b = root;
      }
      else if(lastSeg == ".." && ind != root) { // Added checks for . & .. directories
        b = wdi->file->parent;
		if(b == root) lastSeg = "/";
		else {
          for (auto it = dynamic_cast<Inode<Directory>*>(b)->file->parent->file->theMap.begin(); it != dynamic_cast<Inode<Directory>*>(b)->file->parent->file->theMap.end(); ++it )
            if (it->second == b) lastSeg = it->first;
		}
      }
      else if(lastSeg == ".." && ind == root) {
        b = root;
		lastSeg = "/";
	  }
      else if(lastSeg == ".") { // Added checks for . & .. directories
        b = wdi->file->current;
        lastSeg = current;
      }
      else {
        cerr << lastSeg << ": No such file or directory\n";
        error = true;
      }
    }

  } // end of constructor
};

void TreeDFS ( Inode<Directory>* ind, string s) {
  int count = 0;
  string old_s = s;
  for(auto it = ind->file->theMap.begin(); it != ind->file->theMap.end(); ++it) {
	++count;
	if(it->second->type() == "dir") {
      if(!dynamic_cast<Inode<Directory>*>(it->second)->file->theMap.empty()) {
	    if(ind->file->theMap.size() != count) {
		  cout << old_s << "├── " << it->first << " " << it->second->show();
		  s = old_s + "│   ";
		}
	    else {
		  cout <<  old_s << "└── " << it->first << " " << it->second->show();
		  s = old_s + "    ";
		}
	  }
	  else {
	    if(ind->file->theMap.size() != count) cout <<  old_s << "├── " << it->first << " " << it->second->show();
	    else cout <<  old_s << "└── " << it->first << " " << it->second->show();
	  }
	  TreeDFS(dynamic_cast<Inode<Directory>*>(it->second), s);
	}
	else if(ind->file->theMap.size() != count) {
	  if(it->second->type() == "file") cout <<  old_s << "├── " << "\033[0;32m" << it->first << " " << it->second->show() << "\033[0;30m";
	  else cout <<  old_s << "├── " << it->first << " " << it->second->show();
	}
	else {
	  if(it->second->type() == "file") cout <<  old_s << "└── " << "\033[0;32m" << it->first << " " << it->second->show() << "\033[0;30m";
	  else cout <<  old_s << "└── " << it->first << " " << it->second->show();
	}
  }
  return;
}


int pwd( Args tok ) {
  vector<string> temp;
  Inode<Directory>* ind = wdi->file->parent;
  Inode<Directory>* indb = wdi;
  while( ind ) {
    for(auto it = ind->file->theMap.begin(); it != ind->file->theMap.end(); ++it ) {
	  if(it->second == indb) {
		temp.push_back(it->first);
		temp.push_back("/");
	    break;
	  }
	}
	indb = ind;
	ind = ind->file->parent;
  }
  if(temp.empty()) temp.push_back("/");
  cout << "Current Directory is: ";
  while (!temp.empty()) {
    cout << temp[temp.size()-1];
	temp.pop_back();
  }
  cout << endl;
  return 0;
}

int tree( Args tok ){
  cout << "." << endl;
  TreeDFS(wdi, "");
  return 0;
}

int touch( Args tok ) {
  if ( tok.size() < 2 ) {
    cerr << "touch: missing operand\n";
    // cerr << "try `touch -- help' for more information";
    return -1;
  }
    SetUp su( tok );
    if ( su.error ) return -1;
    // Need to fix this in the Setup, but shouldn't check for "" here.
    if(su.lastSeg == "") {
      cerr << "touch: missing operand\n";
	  return -1;
    }
    if(!su.b) {
      Inode<Directory>* dir_ptr = dynamic_cast<Inode<Directory>*>(su.ind);
      Directory* d = dir_ptr->file;
      File* sufile = new File();
      sufile->parent = dynamic_cast<Inode<Directory>*>(dir_ptr);
      sufile->touch( su.lastSeg, sufile );
    }
    else {
      su.b->m_time = time(0);
      su.b->a_time = su.b->m_time;
    }
  return 0;
}

int cd( Args tok ) {
  string home = "/";  // root is everybody's home for now.
  if ( tok.size() == 1 ) tok.push_back( home );
  SetUp su( tok );
  if ( su.error ) return -1;
  if ( su.b->type() != "dir" ) {
    cerr << "shell: cd:" << su.lastSeg << ": Not a directory\n";
    return -1;
  }
  wdi = dynamic_cast<Inode<Directory>*>(su.b);
  current = ( su.lastSeg != "" ? su.lastSeg : su.next2lastSeg +"/" );
  return 0;
}


int ls( Args tok ) {
  if ( tok.size() == 1 ) {
    wd()->ls();
    return 0;
  }
  //cerr << "tok size = " << tok.size() << "#" << tok[0] <<"#" << endl;
  SetUp su( tok );
  // cerr << su.cmd << endl;
  assert(su.b);
  if ( su.b->type() != "dir" ) {
    //assert(false);
    cout << "cmd: " << su.lastSeg << ": not a directory\n";
    return -1;
  }
  // assert(false);
  su.b->ls();  
  // assert(false);
  return 0;
}


int mkdir( Args tok ) {
  if ( !correct_pathname(tok[1]) ) {
    cerr << "mkdir: invalid pathname\n";
    return -1;
  }
  else if ( tok.size() < 2 ) {
    cerr << "mkdir: missing operand\n";
    // cerr << "try `mkdir -- help' for more information";
    return -1;
  }
    SetUp su( tok );
    if ( su.error ) return -1;
    // Need to fix this in the Setup, but shouldn't check for "" here.
    if(su.lastSeg == "." || su.lastSeg == ".." || (su.lastSeg).find(".") != std::string::npos || su.lastSeg == "") { // Added checks for . & .. directories
      cerr << "mkdir: invalid directory name\n";
	  return -1;
    }
    Inode<Directory>* dir_ptr = dynamic_cast<Inode<Directory>*>(su.ind);
    // Added Directory Already Exist
    if(dir_ptr->file->theMap.find(su.lastSeg) != dir_ptr->file->theMap.end()) { // Added checks for . & .. directories
      cerr << "mkdir: File exists\n";
    }
	 else {
      Directory* d = dir_ptr->file;
      Directory* sudir = new Directory();
      d->mk( su.lastSeg, sudir );
      sudir->parent = dynamic_cast<Inode<Directory>*>(dir_ptr);
      sudir->current = dynamic_cast<Inode<Directory>*>(d->theMap[su.lastSeg]);
	 }
  return 0;
}   


int rmdir( Args tok ) {
  if ( tok.size() < 2 ) {
    cerr << "rmdir: missing operand\n";
    // cerr << "try `mkdir -- help' for more information";
    return -1;
  }
  SetUp su( tok );
  if ( su.error ) return -1;
  if(su.b->type() != "dir") { // Added checks for directories
      cerr << "rmdir: invalid directory name\n";
	  return -1;
    }
  Inode<Directory>* dir_ptr = dynamic_cast<Inode<Directory>*>(su.ind);
  if ( ! dir_ptr ) { 
    cerr << "rmdir: failed to remove '" << tok[0] 
         << "'; no such file or directory.\n";
  } else if ( dynamic_cast<Inode<Directory>*>(su.b)->file->theMap.size() != 0 ) { 
    cerr << "rmdir: failed to remove '" << tok[0] 
         << "'; directory is not empty.\n";
  } else if (dir_ptr->file->theMap.find(su.lastSeg) == dir_ptr->file->theMap.end() || dynamic_cast<Inode<Directory>*>(dir_ptr->file->theMap[su.lastSeg])->type() != "dir") {  // oops: if not there. added directory check
    cerr << "rmdir: failed to remove '" << su.lastSeg
         << "'; no such file or directory.\n";
  } else {
    dir_ptr->file->rm(su.lastSeg);
  }
}


int rm( Args tok ) {
  SetUp su( tok );
  if ( su.ind->file->theMap[su.lastSeg]->type() == "dir" ) {  
    cout << "rm: cannot remove `"<< su.lastSeg << "': is a  directory\n";
    return 0;
  } 
  cout << "rm: remove regular file `" << su.lastSeg << "'? ";
  string response;
  getline( cin, response );            // read user's response.
  if ( response[0] != 'y' && response[0] != 'Y' )  return 0;
  su.ind->file->rm(su.lastSeg);
  return 0;
}


string pwdStr( Inode<Directory>* indb ) {
	string pwdStr = "";
  vector<string> temp;
  Inode<Directory>* ind = indb->file->parent;
  while( ind ) {
    for(auto it = ind->file->theMap.begin(); it != ind->file->theMap.end(); ++it ) {
	  if(it->second == indb) {
		temp.push_back(it->first);
		temp.push_back("/");
	    break;
	  }
	}
	indb = ind;
	ind = ind->file->parent;
  }
  if(temp.empty()) temp.push_back("/");
  //cout << "Current Directory is: ";
  while (!temp.empty()) {
    pwdStr += temp[temp.size()-1];
	temp.pop_back();
  }
  return pwdStr;
}



void preserveRecursive ( Inode<Directory>* ind, string s, ofstream& store) {
  int count = 0;
  string old_s = s;
  for(auto it = ind->file->theMap.begin(); it != ind->file->theMap.end(); ++it) 
  {
	++count;
	if(it->second == root->file->theMap["bin"]) {
		continue;
	}
	else if(it->second->type() == "dir") {
	  old_s = pwdStr(dynamic_cast<Inode<Directory>*>(it->second));
	  store << it->second->type() << ";" << old_s << ";" << it->second->c_time << ";" << it->second->m_time << ";" << it->second->a_time << endl ;
	  
	  if(dynamic_cast<Inode<Directory>*>(it->second)->file->theMap.size() != 0)
	    preserveRecursive(dynamic_cast<Inode<Directory>*>(it->second), old_s, store);
	}
	else if(it->second->type() == "app") {
		continue;
	}
	else {
	  store << it->second->type() << ";" << s + "/" + it->first << ";" << it->second->c_time << ";" << it->second->m_time << ";" << it->second->a_time<< endl ;
	}
  }
  return;

}



void preserve ( Inode<Directory>* ind, string s) {
  ofstream store ("info.txt");
  if (store.is_open())
  {
    preserveRecursive(ind, s, store);
	store.close();
   }
  else cout << "info.txt does not exist.  No pre-directory to load." << endl;	
  return;
}



int exit( Args tok ) {
   preserve(root, "");
  _exit(0);
}


map<string, App*> apps = {
  pair<const string, App*>("ls", ls),
  pair<const string, App*>("mkdir", mkdir),
  pair<const string, App*>("rmdir", rmdir),
  pair<const string, App*>("exit", exit),
  pair<const string, App*>("rm", rm),
  pair<const string, App*>("cd", cd),
  pair<const string, App*>("touch", touch),
  pair<const string, App*>("pwd", pwd),
  pair<const string, App*>("tree", tree)
};  // app maps mames to their implementations.


void FSInit(string file){
  root->file = new Directory;   // OOPS!!! review this.
  Directory* appdir = new Directory(); //Update to put apps in a directory
  root->file->mk("bin", appdir); //Update to put apps in a directory
  appdir->parent = root; //Update to put apps in a directory
  appdir->current = dynamic_cast<Inode<Directory>*>(root->file->theMap["bin"]); //Update to put apps in a directory
  
  for( auto it : apps ) {
    Inode<App>* temp(new Inode<App>(it.second));
    InodeBase* junk = static_cast<InodeBase*>(temp);
    //    InodeBase* junk = temp;
    dynamic_cast<Inode<Directory>*>(	wd()->theMap["bin"])->file->theMap[it.first] = new Inode<App>(it.second);
  }
  Inode<Directory>* r = root;
  string line = "";
  ifstream myfile (file);
  if (myfile.is_open())
  {
    while ( getline (myfile,line) )
    {
      cout << line << '\n';
      stringstream ss(line);             // split temp at white spaces.
      vector<string> rfile;          // then get its cmd-line arguments.
      string s;
      while( ss >> s ) rfile.push_back(s);
      vector<string> filepaths;
      for(auto it = rfile.begin(); it != rfile.end(); ++it) {
        filepaths = split(*it,";");
	  }
	  if(filepaths[0] == "dir") {
	    filepaths[0] = "mkdir";
	    mkdir( filepaths );
	  }
	  else if(filepaths[0] == "file") {
	    filepaths[0] = "touch";
	    touch( filepaths );
	  }
	  else {
	  }
    }
    myfile.close();
  }
  else cout << "Unable to open file"; 
}

}
