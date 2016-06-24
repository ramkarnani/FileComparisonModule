#include "helperPthread.cpp"


int main(int argc,const char *argv[]){

      n = atoi(argv[1]);

      if(n==2) threshold=2;
      else threshold = ceil((float)n/2);
      
      pthread_t threads[n];

      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

      pthread_mutex_init(&count_mutex, NULL);
      pthread_mutex_init(&entry_mutex, NULL);
      pthread_mutex_init(&entry_mutex2, NULL);
      pthread_mutex_init(&fileMutex, NULL);
      pthread_key_create(&conn_key, NULL);
      pthread_key_create(&content_key, NULL);
      pthread_barrier_init (&barrier, NULL, n);

      dirList.push("");
      for(int i=0;i<n;i++){
            pthread_create(&threads[i],&attr,compareDir,(void*)argv[i+2]);
      }

      
      for(int i=0;i<n;i++){
            pthread_join(threads[i],NULL);
      }         


      pthread_attr_destroy(&attr);
      pthread_mutex_destroy(&count_mutex);
      pthread_mutex_destroy(&entry_mutex);
      pthread_mutex_destroy(&entry_mutex2);
      pthread_mutex_destroy(&fileMutex);
      pthread_barrier_destroy(&barrier);
      pthread_key_delete(conn_key);
      pthread_key_delete(content_key);

        
      if(finalDir.size()==0){
            divergent =1;    
      } 

      reportResults();

      return 0;

}