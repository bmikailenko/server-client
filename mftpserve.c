// Ben Mikailenko
// cs360final

#include "mftp.h"

extern int errno;
int datafd, datalistenfd;

void mftpserve();
void client_helper(struct sockaddr_in client, int listenfd, int i);
void connect_success(struct sockaddr_in client, int listenfd, int i);
int main(int argc, char const *argv[]) {

  mftpserve(); // run the server

  return 0;

}

void connect_success(struct sockaddr_in client, int listenfd, int i) {
  char host[NI_MAXHOST];
  char buffer[1024];
  if(getnameinfo((struct sockaddr*) &client, sizeof(client),
  host, sizeof(host),
  NULL, 0, NI_NOFQDN) != 0) {
    printf("couldn't get info from host\n");
    exit(1);
  }

  // tell whos connected to server
  strcpy(buffer, host);
  strcat(buffer, " has connected to the server\n");
  printf("%s", buffer);
}

void client_helper(struct sockaddr_in client, int listenfd, int i) {

  while (1) {
    char buffer[1024], *command, *pathname, *message, request[1024], response[1024];
    int datafd, datalistenfd, error;

    // wait for a client request
    recv(listenfd, request, 1024, 0);

    // client requested "exit" command
    if (request[0] == 'Q') {
      // server logs request and quits
      printf("Q\n");
      break;
    }

    // if command was "rcd"
    if (request[0] == 'C') {

      // server logs request
      printf("%s\n", request);

      // format pathname ( request = C<pathname>, pathname = <pathname> )
      pathname = request;
      pathname++;

      // attempt to change directory
      if ((error = chdir(pathname)) < 0) {

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(listenfd, response, 1024, 0);

      } else {

        // success
        printf("A\n");
        strcpy(response, "A\n");
        send(listenfd, response, 1024, 0);

      }
    }

    // if command was "rls"
    if (request[0] == 'L') {
      pid_t pid;

      // server logs request
      printf("%s\n", request);

      // strcpy(response, "test");
      // send(datalistenfd, response, 1024, 0);


      // fork
      if((pid = fork())) {
        close(datalistenfd);
        wait(NULL);
      } else {
        int fd[2];

        // pipe
        pipe(fd);

        // fork again
        int pid = fork();

        // execute commands
        if (pid == 0) {

          // close read end
          close(fd[0]);

          // replace stdout with data connection
          dup2(fd[1],1);

          // close fd[1]
          close(fd[1]);

          // execute ls
          execlp("ls", "ls", "-la", NULL);

        } else {

          // close write end
          close(fd[1]);

          // wait for ls to be done
          wait(NULL);

          // replace stdin with fd[0]
          dup2(fd[0],0);

          // replace stdout with data connection
          dup2(datalistenfd, 1);

          // close fd[0]
          close(fd[0]);

          // read in from ls and print to data connection
          execlp("more", "more", "-20", NULL);
        }
      }

    }

    // client requested a data connection
    else if (request[0] == 'D') {
      struct sockaddr_in dataAddress, emptyAddress;
      socklen_t dataAddress_len;
      int port;

      // attempt to create a new socket
      if((datafd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(listenfd, response, 1024, 0);
      }

      // data connection structure
      memset(&dataAddress, 0, sizeof(struct sockaddr_in));
      dataAddress.sin_family = AF_INET;
      dataAddress.sin_port = htons(0);
      dataAddress.sin_addr.s_addr = htonl(INADDR_ANY);

      // bind data connection to a random open port
      if(bind(datafd, (struct sockaddr*) &dataAddress, sizeof(struct sockaddr_in)) < 0) {

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(listenfd, response, 1024, 0);
      }

      // make a zeroed out server address structure
      bzero(&emptyAddress,sizeof(emptyAddress));

      // get length of data socket
      dataAddress_len = sizeof(dataAddress);

      // find the random port number that was assigned to the data connection
      if(getsockname(datafd, (struct sockaddr*) &emptyAddress, &dataAddress_len) < 0) {

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(listenfd, response, 1024, 0);
      }

      // listen on port
      if(listen(datafd, 1) < 0){

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(listenfd, response, 1024, 0);
      }

      // format port number into an int
      port = ntohs(emptyAddress.sin_port);

      // success, send port number to client
      printf("A%d\n", port);
      strcpy(response, "A");
      sprintf(response, "%d\n", port);
      send(listenfd, response, 1024, 0);

      // accept new connections
      if((datalistenfd = accept(datafd, (struct sockaddr *) &dataAddress, &dataAddress_len)) < 0) {

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(listenfd, response, 1024, 0);
      }
    }

    else if (request[0] == 'G') {
      struct stat buf;
      int fd, size, sent_bytes;
      char c;

      // log request
      printf("%s\n", request);

      // format pathname
      pathname = request;
      pathname++;
      pathname[strlen(pathname)] = '\0';

      // open file
      if ((fd = open(pathname, O_RDONLY)) < 0) {

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(datalistenfd, response, 1024, 0);
      }

      // get file stats
      if(fstat(fd, &buf) < 0){

        // error
        printf("E%s\n", strerror(errno));
        strcpy(response, "E");
        strcat(response, strerror(errno));
        strcat(response, "\n");
        send(datalistenfd, response, 1024, 0);
      }

      // get file size
      size = buf.st_size;

      // put file size into response
      sprintf(response, "%d", size);

      // tell client file size
      send(datalistenfd, response, 1024, 0);

      // send contents of file
      while (read(fd, &c, 1) > 0 && size != 0) {
        send(datalistenfd, &c, 1, 0);
      }

      // close file
      close(fd);
      close(datalistenfd);

      printf("A\n");
    }

    else if (request[0] == 'P') {
        char c, *filename;
        int fd, size;

        printf("%s\n", request);

        // format pathname
        pathname = request;
        pathname++;
        pathname[strlen(pathname)] = '\0';
        filename = basename(pathname);

        // create a local file with basename of file path
        if ((fd = open(filename, O_WRONLY | O_CREAT, 0666)) < 0) {

          // error
          printf("E%s\n", strerror(errno));
          strcpy(response, "E");
          strcat(response, strerror(errno));
          strcat(response, "\n");
          send(datalistenfd, response, 1024, 0);
        }

        // get number of bytes in file
        recv(datalistenfd, request, 1024, 0);

        // format number of bytes from string to int
        size = atoi(request);

        // recieve file data and write to file
        while (recv(datalistenfd, &c, 1, 0) > 0) {
          write(fd, &c, 1);
        }

        // close file
        close(fd);
        close(datalistenfd);

        printf("A\n");

    }
  }
}

void mftpserve() {

  int listenfd, socketfd;
  struct sockaddr_in serverAddress, clientAddress;

  // make the socket
  if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Couldn't make socketfd\n");
    exit(1);
  }

  // clear the socket
  if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    printf("Couldn't set socket\n");
    exit(1);
  }

  //server structure
  memset(&serverAddress, 0, sizeof(struct sockaddr_in));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(MY_PORT_NUMBER);
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(socketfd, (struct sockaddr*) &serverAddress, sizeof(struct sockaddr_in)) < 0) {
    printf("couldn't bind socket");
    exit(1);
  }

  // listen on server
  if(listen(socketfd, 4) < 0){
    printf("couldn't listen on server");
    exit(1);
  }

  socklen_t len = sizeof(struct sockaddr_in);
  int i = 0;

  while(1) {
    if((listenfd = accept(socketfd, (struct sockaddr *) &clientAddress, &len)) < 0) {
      printf("couldn't accept client\n");
      exit(1);
    }

    // max 4 connections
    if (i != 4) {
      i++;
      int pid;

      pid = fork();
      if(pid == 0) { //child operation

        connect_success(clientAddress, listenfd, i); // prompt connected
        client_helper(clientAddress, listenfd, i); // recieve and execute commands
        close(listenfd); // close connection
        i--;  // decrement number of connections
        exit(0);

      } else if(pid > 0) {
        close(listenfd);
      } else {
        printf("couldn't fork\n");
        exit(1);
      }
    }

    // if 5th connection
    else {
      close(listenfd);
    }
  }
}
