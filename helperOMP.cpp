// Working Code
#include "OMPHeader.h"


void* compareDir(void* rootname){

  const char* inputName = (const char*) rootname;
  int len = strlen(inputName);
  string root(inputName);
  struct dirent **dirPointer;

//*******************************************************BFS START**************************************************************************//
  
   // Level-Order traversal of Filesystems

  while(!dirList.empty()){

    string dirName;
    dirName.append(root);
    dirName.append(dirList.front());

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

      /*
      pushing the level order traversed filenames in the thread specific deque which would help in finding the 
      majority copies of a directory(in terms of filenames and contents).
      */
      dirContent.push_back(fileName);


      // stat variable to distinguish a file and a directory and getting size if it is file.
      struct stat sb;
      stat(fileName.c_str(),&sb);

//*********************************************************************//
      // If it is a directory and present in majority in all filesystems(in terms of directory names), push it into the queue.
  
      if(S_ISDIR(sb.st_mode)){

        //cout << sb.st_mode << endl; 

        #pragma omp critical (lock1)
        {
         //global map to get the majority count and clearing this map at each level of the tree.
        dmap[fileName.c_str()+len] +=1;

        //only one thread would execute this part count would be equal to majority for only one thread.
        if(dmap[fileName.c_str()+len] == threshold){

          //processing the names.
          fileName.append("/");

          //pushing the relative name in the BFS queue, "fileName.c_str()+len" gives the relative name as len is the length of rootName.
          dirList.push(fileName.c_str()+len);

          //pushing the relative name in deque of directories having majority count with respect to directory names.
          output.push_back(fileName.c_str()+len);
          
          } 
        }      
 
      }

//*******************************************************************//
      else{
        //structure for storing the name, size and MD5 hash value of a file.
        fileKey fkey;
        fkey.str = fileName.c_str()+len;
        fkey.size = sb.st_size;
        // hashwrapper *myWrapper = new md5wrapper();
        fkey.HashValue = myWrapper->getHashFromFile(fileName.c_str());

        #pragma omp critical (lock2)            //lock name changed.
        {
   
        fmap[fkey] +=1;
        if(fmap[fkey] == threshold){
            //pushing the relative name in queue of files having majority count with respect to names,size and MD5 hash value.
            fList.push(fkey);

          }

        }
      }
//*************************FOR LOOP END********************************************************//
    }
//*********************************************************************************************//    
    //waiting for all threads to process one item of the BFS queue.
  
    #pragma omp barrier

//*********************************************************************************************//
    //only Master thread would perform this section of code, clearing the maps and popping an element from the queue. 
 
    #pragma omp master
    {
      dirList.pop();
      if(!dmap.empty())dmap.clear();
      if(!fmap.empty())fmap.clear();
    }
      
    #pragma omp barrier
  }

//All threads would proceed to File Comparison module.
  #pragma omp barrier
  compareFiles(rootname);

}




void* compareFiles(void* rootname){

  const char* inputName = (const char*) rootname;
  int len = strlen(inputName);
  string root(inputName);
    
//************************************************BFS START************************************************************************//


  //BFS for the fileList.
  while(!fList.empty()){

    //cout << "216" << endl;
    string fileName;
    fileName.append(root);
    string s1(fList.front().str);
    fileName.append(s1);    

    char readBuf[8192];

    struct stat sb;
    FILE* fp ;

    //Files are geeting compared in this section of code
    //fileCmap --> Map is used to get file content in char array and mapping them. Map value would indicate the no.of files where it is present
    //which can be then compared with the threshold value required for majority. 
    
    fp = fopen(fileName.c_str(),"r");

   int lFlag=1;
   int lenL;
   string fname;

   while(loopVar){

    #pragma omp barrier
    loopVar=0;

    #pragma omp barrier

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
        #pragma omp critical (fileCheck)
        {
          fileCMap[fname]+=1;
          if(fileCMap[fname]== threshold){
            loopVar=1;
            if(!fileCMap.empty()) fileCMap.clear();
          }
        }
      }
      
    }
    #pragma omp barrier

  }

    //#pragma omp barrier

    if(lFlag==0){
      #pragma omp critical (incre)
      fileCounter++;
    }

    #pragma omp barrier

    #pragma omp master
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
    #pragma omp barrier
  }

//****************************************************BFS END***************************************************************************//

  //All threads would now procede to find the majority directory in the fileystems, in terms of pathnames and contents.

  #pragma omp barrier
  
  dirSubtree(rootname);

}





void* dirSubtree(void* t){

  const char* inputName = (const char*) t;
  int len = strlen(inputName);

  Map threadMap;
  deque <string> ::iterator it;

//******************************************************************************************************//
    
    //creating a local threadMap for candidates of majority directories and setting count to 0.

  for(it = output.begin(); it!= output.end();it++){
          string root(inputName);
          root.append(*it);
          int rlen = root.length();
          root = root.substr(0,rlen-1);
          //cout << root << endl;
          threadMap[root]=0;
          //cout << root << endl;
      }
    
    //creating a local threadMap for majority files list and setting count to 1.

  for(it = outputF.begin(); it!= outputF.end();it++){
          string root(inputName);
          threadMap[root.append(*it)]=1;
      }
    
//******************************************************************************************************************//

  //traversing the File System in backward direction from a deque which have file system in level-order direction.

    set <string> fullList;

    int exactCheck = dirContent.size();
    
    while (!dirContent.empty())
      {

        string fpath = dirContent.back();
        struct stat sb;
        stat(fpath.c_str(),&sb);

//****************************************************************//
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
        if(n==2) fullList.insert(dirContent.back().c_str()+len); 
        dirContent.pop_back();
      }

//****************************************************************************************//

  #pragma omp critical (lockthread)
    {
        for ( Map::iterator it=threadMap.begin(); it!=threadMap.end(); ++it){
          if(it->second==1){
              string s1(it->first);
              finalDir.insert(s1.c_str()+len);
          }
        }
    }
  threadMap.clear();

  #pragma omp barrier
//***************************************************************************************//
  //processing for 2 FileSystems

  if(n==2 && exactCheck != finalDir.size()){

    // create sections
    #pragma omp sections
    {

      #pragma omp section
      {
        set_difference(fullList.begin(),fullList.end(),finalDir.begin(),finalDir.end(),inserter(FS1,FS1.begin()));
      }

      #pragma omp section
      {
        set_difference(fullList.begin(),fullList.end(),finalDir.begin(),finalDir.end(),inserter(FS2,FS2.begin()));
      }
    }

  }

//*************************************************************************************//
  fullList.clear();

  if(exactCheck != finalDir.size()){
    #pragma omp critical
      exact=0;
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

      if(divergent){
        cout << "File Systems are completely divergent" << endl;
        return;
      }

      if(exact){
        cout << "File Systems are exact copies." << endl;
      }
      else{

        if(n==2){    
            helperResult();   
        }
        else
        {
            for (std::set<string>::iterator it=finalDir.begin(); it!=finalDir.end(); ++it)
                std::cout << *it << endl;  
            finalDir.clear();
        }
      }
}


