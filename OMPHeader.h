#include <iostream>
#include <map>
#include <string>
#include <pthread.h>
#include <ftw.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <cmath>
#include <string>
#include <queue>
#include <hashlibpp.h>
#include <deque>
#include <set>
#include <algorithm>




using namespace std;

typedef struct{
      
      string str;
      off_t size;
      string HashValue;

}fileKey;


inline bool operator <(fileKey const& left, fileKey const& right) {
  if (left.str < right.str) { return true; }
  if (left.str== right.str && left.size < right.size) { return true; }
  if (left.str== right.str && left.size == right.size && left.HashValue < right.HashValue) { return true; }
  return false;
}


typedef std::map<fileKey,int> fileMap;
fileMap fmap;


typedef std::map<string,int> Map;
Map dmap;
Map fileCMap;
Map divMap;


queue <string> dirList;
queue <fileKey> fList;

deque <string> output;
deque <string> outputF;

set <string> finalDir;
set <string> FS1;
set <string> FS2;


int divergent=0;
int n;
int loopVar=1;
int threshold;
int fileCounter=0;
int exact=1;


extern Map threadMap;
extern deque <string> dirContent;
extern deque <string> dirContent1;
extern int dirCount;


#pragma omp threadprivate(threadMap)
Map threadMap;

#pragma omp threadprivate(dirContent)
deque <string> dirContent;


void* dirSubtree(void* t);
void* compareFiles(void* rootname);
void* compareDir(void* rootname);
void helperResult();
void reportResults();