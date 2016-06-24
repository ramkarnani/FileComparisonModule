#include "helperOMP.cpp"

int main(int argc,const char *argv[]){

  int iter;
  n = atoi(argv[1]);
  if(n==2) threshold=2;
  else threshold = ceil((float)n/2);

  dirList.push("");
  #pragma omp parallel for num_threads(n) private(iter) shared(argc,argv,dirList,dmap,output,fmap,fList) 
    for(iter=0;iter<n;iter++){
    
      compareDir((void*)argv[iter+2]);


   }

  #pragma omp barrier

  if(finalDir.size()==0){
           divergent =1;    
     } 
  reportResults();
}