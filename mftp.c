// Ben Mikailenko
// cs360final

#include "mftp.h"

extern int errno;

void mftp();
int data_connection(int socketfd, const char * address);

int main(int argc, char const *argv[]) {

  mftp(argv[1]); // run the client

  return 0;

}

void mftp(const char * address) {

  int socketfd, error;
  struct sockaddr_in clientAddress;

  // make a socket
  if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Couldn't make socketfd\n");
    exit(1);
  };

  // get host info ...
  struct addrinfo host, *hostInfo;
  memset(&host, 0, sizeof(host));
  host.ai_socktype = SOCK_STREAM;
  host.ai_family = AF_INET;

  // at address
  if((error = getaddrinfo(address, "49999", &host, &hostInfo)) != 0) {
    fprintf(stderr, "Error: %s\n", gai_strerror(error));
    exit(1);
  }

  // get socket based on host
  if((socketfd = socket(hostInfo->ai_family, hostInfo->ai_socktype, 0)) < 0) {
    printf("couldn't get socket");
    exit(1);
  }

  // allocate memory for client
  memset(&clientAddress, 0, sizeof(struct sockaddr_in));

  // connect to server
  if(connect(socketfd, hostInfo->ai_addr, hostInfo->ai_addrlen) < 0) {
    printf("couldn't connect to server\n");
    exit(1);
  } else {
    printf("Sucessfully conected to server\n");
  }

  char *buffer, *command, *pathname, *message, request[1024], response[1024];
  size_t  n = 1024;
  buffer = malloc(n);

  // request client input into buffer
  while (getline(&buffer, &n, stdin) > 0) {

    if (buffer[0] == 10) continue;
    // get command from buffer (first token) (ex: ls, rls, cd, rcd, get)
    command = strtok(buffer, " \n");

    // if client requested to exit, exit
    if (strcmp(command, "exit") == 0) {
      strcpy(request, "Q");
      send(socketfd, request, 1024, 0);
      printf("Disconnecing from server...\n");
      break;
    }

    // if command was "cd"
    else if (strcmp(command, "cd") == 0) {

      // format client pathname from second token
      pathname = command + 3;
      pathname[strlen(pathname) - 1] = '\0';

      // change directory into pathname
      if((error = chdir(pathname)) < 0) {

        // error
        printf("Couldn't change directory to: %s\n", pathname);
      } else {

        // success
        printf("Changed directory to: %s\n", pathname);
      }
    }

    // if command was "rcd"
    else if (strcmp(command, "rcd") == 0) {

      // format pathname
      pathname = command + 4;
      pathname[strlen(pathname) - 1] = '\0';

      // format server request
      strcpy(request, "C");
      strcat(request, pathname);

      // send server request
      send(socketfd, request, 1024, 0);

      // wait for server response
      recv(socketfd, response, 1024, 0);

      // server encountered an error
      if (response[0] == 'E') {
        message = response;
        message++;
        printf("%s", message);
      }

      // server sucessfully changed directory
      else if (response[0] == 'A') {
        printf("Sucessfully changed directory to %s\n", pathname);
      }

      // client disconnect from server
      else {
        printf("Fatal error, no response from server, exiting...\n");
        break;
      }
    }

    // if command was "ls"
    else if (strcmp(command, "ls") == 0) {
      pid_t pid;

      // fork
      if((pid = fork())) {
        wait(NULL);
      } else {
        int fd[2];

        // pipe
        pipe(fd);

        // fork again
        int pid = fork();

        // execute commands
        if (pid == 0) {
          close(fd[0]);
          dup2(fd[1],1);
          close(fd[1]);
          execlp("ls", "ls", "-la", NULL);
        } else {
          close(fd[1]);
          wait(NULL);
          dup2(fd[0],0);
          close(fd[0]);
          execlp("more", "more", "-20", NULL);
        }
      }
    }

    // if command was "rls"
    else if (strcmp(command, "rls") == 0) {
      int datafd;

      // if sucessfully created a data connection
      if ((datafd = data_connection(socketfd, address)) > 0 ) {

        // format server request
        strcpy(request, "L");

        // send server request
        send(socketfd, request, 1024, 0);

        // wait for server response
        while (recv(datafd, response, 1024, 0) != 0) {
          printf("%s\n", response);
        }

        // close data connection
        close(datafd);
      }
    }

    // if command was "get"
    else if (strcmp(command, "get") == 0) {
      char c, *filename;
      int fd, size;
      int datafd;

      // if sucessfully created a data connection
      if ((datafd = data_connection(socketfd, address)) > 0 ) {
        // format pathname
        pathname = command + 4;
        pathname[strlen(pathname) - 1] = '\0';
        filename = basename(pathname);

        // format request
        strcpy(request, "G");
        strcat(request, pathname);

        // send request to server
        send(socketfd, request, 1024, 0);

        // start recieving data through data connection
        // revieving file size
        recv(datafd, response, 1024, 0);


        // format file size into size from string to int
        size = atoi(response);

        // create a local file with basename of file path
        if ((fd = open(filename, O_WRONLY | O_CREAT, 0666)) < 0) {

          // error
          printf("%s\n", strerror(errno));
        }

        // if opened file
        else {

          // recieve file data and write to file
          while (recv(datafd, &c, 1, MSG_DONTWAIT) > 0) {
            write(fd, &c, 1);
          }

          // close file
          close(fd);
          close(datafd);

          printf("Finished getting file from server\n");
        }
      }
    }

    // if command was "show"
    else if (strcmp(command, "show") == 0) {
      pid_t pid;

      // format pathname
      pathname = command + 5;
      pathname[strlen(pathname) - 1] = '\0';


      // fork
      if((pid = fork())) {
        wait(NULL);
      } else {
        int fd[2];

        // pipe
        pipe(fd);

        // fork again
        int pid = fork();

        // execute commands
        if (pid == 0) {
          close(fd[0]);
          dup2(fd[1],1);
          close(fd[1]);
          execlp("cat", "cat", pathname, NULL);
        } else {
          close(fd[1]);
          wait(NULL);
          dup2(fd[0],0);
          close(fd[0]);
          execlp("more", "more", "-20", NULL);
        }
      }
    }

    // if command was "put"
    else if (strcmp(command, "put") == 0) {
      struct stat buf;
      int datafd, fd, size, sent_bytes;
      char c;

      // open file
      if ((fd = open(pathname, O_RDONLY)) < 0) {

        // error
        printf("%s\n", strerror(errno));
      }

      // if opened the file
      else {

        // if sucessfully created a data connection
        if ((datafd = data_connection(socketfd, address)) > 0 ) {

          // format pathname
          pathname = command + 4;
          pathname[strlen(pathname) - 1] = '\0';

          // format request
          strcpy(request, "P");
          strcat(request, pathname);

          // send request to server
          send(socketfd, request, 1024, 0);

          // get file stats
          if(fstat(fd, &buf) < 0){
            printf("%s\n", strerror(errno));
          }

          // if got file stats
          else {

            // get file size
            size = buf.st_size;

            // put file size into response
            sprintf(request, "%d", size);

            // tell server file size
            send(datafd, request, 1024, 0);


            // send contents of file
            while (read(fd, &c, 1) > 0 && size != 0) {
              send(datafd, &c, 1, 0);
            }

            // close file
            close(fd);
            close(datafd);

            printf("Sucessfully sent file to server\n");

          }
        }
      }
    }

    // if client didn't execute any commands above..
    else {
      printf("invalid command\n");
    }
  }
}

// function establishes a data connection with server with command "D"
// returns NULL on failure
// returns data connection fd on success
int data_connection(int socketfd, const char * address) {
  char request[1024], response[1024], *message;
  int datafd, error;
  struct sockaddr_in dataAddress;

  // request a data connection
  strcpy(request, "D");

  // send server request
  send(socketfd, request, 1024, 0);

  // wait for server response
  recv(socketfd, response, 1024, 0);

  // server encountered an error, abort
  if (response[0] == 'E') {
    message = response;
    message++;
    printf("%s", message);
    return -1;
  }

  // server sucessfully created a data connection
  else {

    // format response
    response[strlen(response) -1] = '\0';

    // create a socket for the data connection
    if((datafd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("%s\n", strerror(errno));
      return -1;
    }

    // get host info ...
    struct addrinfo data, *dataInfo;
    memset(&data, 0, sizeof(data));
    data.ai_socktype = SOCK_STREAM;
    data.ai_family = AF_INET;

    // at address
    if((error = getaddrinfo(address, response, &data, &dataInfo)) != 0) {
      printf("%s\n", strerror(errno));
      return -1;
    }

    else {

      // get socket based on data connection
      if((datafd = socket(dataInfo->ai_family, dataInfo->ai_socktype, 0)) < 0) {
        printf("%s\n", strerror(errno));
        return -1;
      }

      else {
        // allocate memory for client
        memset(&dataAddress, 0, sizeof(struct sockaddr_in));

        // connect to server
        if(connect(datafd, dataInfo->ai_addr, dataInfo->ai_addrlen) < 0) {

          // error
          printf("%s\n", strerror(errno));
          return -1;
        } else {

          // success
          return datafd;
        }
      }
    }
  }
}
