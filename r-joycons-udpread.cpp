// Client side implementation of UDP client-server model 

#include <Rcpp.h>
using namespace Rcpp;

#include <string>
using namespace std;

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT     8080 
#define MAXLINE 1024 

int sockfd = -99; 

char buffer[MAXLINE]; 

#include  <time.h>
#include <sys/time.h>

struct timeval  tv1, tv2;

// [[Rcpp::export]]
double timer_start() {
  gettimeofday(&tv1, NULL);
}

// [[Rcpp::export]]
double timer_elapsed() {
   gettimeofday(&tv2, NULL);
   
   double time =          (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
     (double) (tv2.tv_sec - tv1.tv_sec);

   return time;
}


// [[Rcpp::export]]
std::string udp_message() { 

  char *hello = (char*)"Hello from server"; 
  struct sockaddr_in servaddr, cliaddr; 
  
  if(sockfd == -99) {
    printf("Init\n");
    
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
      perror("socket creation failed"); 
      exit(EXIT_FAILURE); 
    } 
    
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
    
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
    
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
              sizeof(servaddr)) < 0 ) 
    { 
      perror("bind failed"); 
      exit(EXIT_FAILURE); 
    } 
    
  }
  
  // Creating socket file descriptor 
  
  int n; 
  socklen_t len;
  
  len = sizeof(cliaddr);  //len is value/resuslt 
  
  n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
               MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
               &len); 
  
  if(n<0) {
      printf("Error %d\n",n);
    std::string s = "ERROR";
    
    return s;
  }
  
  buffer[n] = '\0'; 
  //printf("Client : %s\n", buffer); 
  sendto(sockfd, (const char *)hello, strlen(hello),  
         MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
         len); 
  //printf("Hello message sent.\n");  
  
  std::string s(buffer);
  
  return s;
  
} 

