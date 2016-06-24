// Working Code
#include "pthreadHeader.h"



void assignId(){

  pthread_mutex_lock(&count_mutex);
  int* threadid;
  threadid = (int*)malloc(sizeof(int));
  *threadid = idcount;
  idcount++;
  pthread_setspecific(conn_key,(void*) threadid);
  pthread_mutex_unlock(&count_mutex);

}



void* compareDir(void* rootname){

  assignId();

  // Binding a deque to each thread
  deque <string> dirContent;
  pthread_setspecific(content_key,(void*) &dirContent);

  const char* inputName = (const char*) rootname;
  int len = strlen(inputName);
  string root(inputName);
  struct dirent **dirPointer;
  int* id;

 
//*******************************************************BFS START**************************************************************************//
  
   // Level-Order traversal of Filesystems

  while(!dirList.empty()){

    string dirName;
    dirName.append(root);
    dirName.append(dirList.front());
    
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

    //reading the contents of a directory using scandir system call.
    int n1 = scandir(dirName.c_str(),&dirPointer,NULL,alphasort);

    hashwrapper *myWrapper = new md5wrapper();
    
//****************************FOR LOOP START**********************************************************//
    for (int i = 0; i < n1; ++i)
    {

      //excluding this directory and parent directory as they have been already processed.
      if(strcmp(dirPointer[i]->d_name,".")==0 || strcmp(dirPointer[i]->d_name,"..")==0)continue;

      string fileName;
      fileName.append(dirName);
      string t2(dirPointer[i]->d_name);
      fileName.append(t2);

      
      //pushing the level order traversed filenames in the thread specific deque which would help in finding the majority copies of a directory(in terms of filenames and contents).
      
      dirContent.push_back(fileName);


      // stat variable to distinguish a file and a directory and getting size if it is file.
      struct stat sb;
      stat(fileName.c_str(),&sb);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
      // If it is a directory and present in majority in all filesystems(in terms of directory names), push it into the queue.
      if(S_ISDIR(sb.st_mode)){


        pthread_mutex_lock(&entry_mutex);

        //global map to get the majority count and clearing this map at each level of the tree.
        dmap[fileName.c_str()+len] +=1;

        //only one thread would execute this part count would be equal to majority for only one thread.
        if(dmap[fileName.c_str()+len] == threshold){

          //processing the names.
          fileName.append("/");

          //pushing the relative name in the BFS queue, "fileName.c_str()+len" gives the relative name as len is the length of rootName.
          dirList.push(fileName.c_str()+len);

          //pushing the relative name in deque of directories having majority count with respect to directory names.
          outputD.push_back(fileName.c_str()+len);
          
        } 
        pthread_mutex_unlock(&entry_mutex);
 
      }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
      else{
        fileKey fkey;
        fkey.str = fileName.c_str()+len;
        fkey.size = sb.st_size;
        fkey.HashValue = myWrapper->getHashFromFile(fileName.c_str());
        

        pthread_mutex_lock(&entry_mutex2);
        //structure for storing the name, size and MD5 hash value of a file.
        fmap[fkey] +=1;
        if(fmap[fkey] == threshold){
            //pushing the relative name in queue of files having majority count with respect to names,size and MD5 hash value.
            fList.push(fkey);
        }

        pthread_mutex_unlock(&entry_mutex2);
      }
//*************************FOR LOOP END********************************************************//
    }
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    //waiting for all threads to process one item of the BFS queue.
    pthread_barrier_wait(&barrier);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    //only thread with id==0 would perform this section of code, clearing the maps and popping an element from the queue. 
    id = (int*)pthread_getspecific(conn_key); 
    if(*id==0){
      dirList.pop();
      if(!dmap.empty())dmap.clear();
      if(!fmap.empty())fmap.clear();
    }
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

    pthread_barrier_wait(&barrier);

  }

//**************************************WHILE LOOP END***************************************************************************//
//All threads would proceed to File Comparison module.
  pthread_barrier_wait(&barrier);
  compareFiles(rootname);
}





void* compareFiles(void* rootname){


  const char* inputName = (const char*) rootname;
  int len = strlen(inputName);
  string root(inputName);
    
//************************************************BFS START************************************************************************//

  //BFS for the fileList.

  while(!fList.empty()){

    string fileName;
    fileName.append(root);
    string s1(fList.front().str);
    fileName.append(s1);    

    char readBuf[8192];
    int *id;

    struct stat sb;
    FILE* fp ;
    fp = fopen(fileName.c_str(),"r");


//*******************************************************************************************//
    //Files are geeting compared in this section of code
    //fileCmap --> Map is used to get file content in char array and mapping them. Map value would indicate the no.of files where it is present
    //which can be then compared with the threshold value required for majority. 
    
   int lFlag=1;
   int lenL;
   string fname;

   while(loopVar){

    pthread_barrier_wait(&barrier);
    loopVar=0;

    pthread_barrier_wait(&barrier);


    if(fp!= NULL){

      if((lenL=fread(readBuf,sizeof(char),8191,fp))>0){
        fname = readBuf;
        fname = fname.substr(0,lenL);
      }
      if(feof(fp))
      {
        lFlag=0;
      }
      if(lFlag){
        pthread_mutex_lock(&count_mutex);
        {
        fileCMap[fname]+=1;
        if(fileCMap[fname]== threshold){
          loopVar=1;
          if(!fileCMap.empty()) fileCMap.clear();
          }    
        }
        pthread_mutex_unlock(&count_mutex);
      }
      
    }
    pthread_barrier_wait(&barrier);

  }

    pthread_barrier_wait(&barrier);

    if(lFlag==0){
      pthread_mutex_lock(&count_mutex);
      fileCounter++;
      pthread_mutex_unlock(&count_mutex);
    }

    pthread_barrier_wait(&barrier);

    id = (int*)pthread_getspecific(conn_key); 
    if(*id==0)
    {
      if(fileCounter >=threshold){
        outputF.push_back(fList.front().str);
      }
      fList.pop();
      fileCMap.clear();
      fileCounter=0;
      loopVar=1;
    }


//*********************************************************************************************//

    if(fp!=NULL) fclose(fp);
    pthread_barrier_wait(&barrier);
  }

//****************************************************BFS END***************************************************************************//

  //All threads would now procede to find the majority directory in the fileystems, in terms of pathnames and contents.

  pthread_barrier_wait(&barrier);
  dirSubtree(rootname);
  
}





void* dirSubtree(void* t){

  const char* inputName = (const char*) t;
  int len = strlen(inputName);

  Map threadMap;
  deque <string> ::iterator it;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    
    //creating a local threadMap for candidates of majority directories and setting count to 0.

  for(it = outputD.begin(); it!= outputD.end();it++){
          string root(inputName);
          root.append(*it);
          int rlen = root.length();
          root = root.substr(0,rlen-1);
          threadMap[root]=0;
      }
    
    //creating a local threadMap for majority files list and setting count to 1.

  for(it = outputF.begin(); it!= outputF.end();it++){
          string root(inputName);
          threadMap[root.append(*it)]=1;
      }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

  //traversing the File System in backward direction from a deque which have file system in level-order direction.
    deque <string>* dirContent;
    dirContent = (deque <string>*)pthread_getspecific(content_key);

    set <string> fullList;


    int exactCheck = dirContent->size();
//****************************************************WHILE LOOP START*************************************************//

    while (!dirContent->empty())
      {

        string fpath = dirContent->back();
        struct stat sb;
        stat(fpath.c_str(),&sb);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
       if(S_ISDIR(sb.st_mode) && threadMap.find(fpath) != threadMap.end()){

          int flag=1;
          struct dirent** dirPointer;
          int n1 = scandir(fpath.c_str(),&dirPointer,NULL,alphasort);
    
          for (int i = 0; i < n1; ++i)
              {

              if(strcmp(dirPointer[i]->d_name,".")==0 || strcmp(dirPointer[i]->d_name,"..")==0)continue;
              string s1 = fpath;
              s1.append("/");
              s1.append(dirPointer[i]->d_name);

              if(threadMap.find(s1) == threadMap.end() || threadMap[s1]==0){
                flag=0;
                break;
                }
              }
          if(flag==1){
            threadMap[fpath]=1;
            } 
        }
        if(n==2) fullList.insert(dirContent->back().c_str()+len);                     //to output the differences.
        dirContent->pop_back();
      }

//******************************************WHILE LOOP END**********************************************************//



  pthread_mutex_lock(&entry_mutex);
  for ( Map::iterator it=threadMap.begin(); it!=threadMap.end(); ++it){
    if(it->second==1){
      string s1(it->first);
      finalDir.insert(s1.c_str()+len);             //set for storing the majority copies.

    }
  }
  pthread_mutex_unlock(&entry_mutex);
  threadMap.clear();

  pthread_barrier_wait(&barrier);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
  //processing for 2 FileSystems

  if(n==2 && exactCheck != finalDir.size()){

    int* id = (int*)pthread_getspecific(conn_key);

    if(*id==0){
        set_difference(fullList.begin(),fullList.end(),finalDir.begin(),finalDir.end(),inserter(FS1,FS1.begin()));
    }
    else{
        set_difference(fullList.begin(),fullList.end(),finalDir.begin(),finalDir.end(),inserter(FS2,FS2.begin()));
    }
  }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//


  fullList.clear();


  if(exactCheck != finalDir.size()){
    pthread_mutex_lock(&count_mutex);
    exact=0;
    pthread_mutex_unlock(&count_mutex);
  }
}


void helperResult(){


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    set <string> contentChange;
    set <string> addition1;
    set <string> addition2;

    set_intersection(FS1.begin(),FS1.end(),FS2.begin(),FS2.end(),inserter(contentChange,contentChange.begin()));            
    set_difference(FS1.begin(),FS1.end(),contentChange.begin(),contentChange.end(),inserter(addition1,addition1.begin()));            
    set_difference(FS2.begin(),FS2.end(),contentChange.begin(),contentChange.end(),inserter(addition2,addition2.begin()));            

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//


    cout << "Content Change list is -->" << endl;
    for (std::set<string>::iterator it=contentChange.begin(); it!=contentChange.end(); ++it)
          std::cout << *it << endl;  
          
    for(int i=0;i<5;i++) cout << endl;                 //Spacing
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//


    if(addition1.size()==0){
        cout << "No addition in Filesystem1" << endl;
        }
    else{
        cout << "Additions in FileSystem1 is -->" << endl;
        for (std::set<string>::iterator it= addition1.begin(); it!=addition1.end(); ++it)
          std::cout << *it << endl;  
        for(int i=0;i<5;i++) cout << endl;             //Spacing
      }
            
    if(addition2.size()==0){
        cout << "No addition in Filesystem2" << endl;
        }
    else{
        cout << "Additions in FileSystem2 is -->" << endl;
        for (std::set<string>::iterator it= addition2.begin(); it!=addition2.end(); ++it)
          std::cout << *it << endl;  
      }
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
    contentChange.clear();
    addition1.clear();
    addition2.clear();
    FS1.clear();
    FS2.clear();
}


void reportResults(){

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
      if(divergent){
        cout << "File Systems are completely divergent" << endl;
        return;
      }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
      if(exact){
        cout << "File Systems are exact copies." << endl;
      }
      else{

        if(n==2){    
            helperResult();
         
        }
        else
        {
            //cout << finalDir.size() << endl;
            for (std::set<string>::iterator it=finalDir.begin(); it!=finalDir.end(); ++it)
                std::cout << *it << endl;  
            finalDir.clear();
        }
      }
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
}


