// Server program demo from lecture.
// Modified slightly to decrypt rather than encrypt, other than that
// it is mostly the same as otp_enc_d in order to save time.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function for reporting issues.
void error(const char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
  socklen_t sizeOfClientInfo;
  char buffer[256];
  struct sockaddr_in serverAddress, clientAddress;
  // For determining what kind of client it can connect to.
  // 1 = encrypt, 0 = decrypt.
  int server_type = 2;
  int client_type;
  int plain_text_size, key_size;

  // Error if wrong num of command line args.
  if (argc < 2)
  {
    fprintf(stderr, "USAGE: %s port\n", argv[0]);
    exit(1);
  }

  // Set up address struct for the server.
  // Clear out the address struct.
  memset((char*)&serverAddress, '\0', sizeof(serverAddress));
  // Get the port number, convert from string to int.
  portNumber = atoi(argv[1]);
  // Create a network-capable socket.
  serverAddress.sin_family = AF_INET;
  // Store the port number.
  serverAddress.sin_port = htons(portNumber);
  // Any address is allowed for connection to this process.
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Set up the socket.
  // Create the socket.
  listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
  
  // Error if socket creation failed.
  if (listenSocketFD < 0)
  {
    error("ERROR opening socket\n");
  }

  int val = 1;
  // Set to have multiple connections.
  // source: https://stackoverflow.com/questions/4233598/about-setsockopt-and-getsockopt-function
  setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));


  // Enable socket to begin listening. If failed, then error.
  if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    error("ERROR on binding\n");
  } 
  // Flip the socket on - it can now receive up to 5 connections.
  if (listen(listenSocketFD, 5) < 0)
  {
    fprintf(stderr, "SERVER: ERROR in listen\n");
  }


  // Accept a connection, blocking if one is not available until one connects.
  // Get the size of the address for the client that will connect.
  // But loop while waiting for clients to connect to it.
  while(1)
  {
    
    sizeOfClientInfo = sizeof(clientAddress);
    // Accept the connection.
    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
  
    // If accept fails, error.
    if (establishedConnectionFD < 0)
    {
      error("ERROR on accept\n");
    }

    // Spawn a child process to handle operations like in smallsh.
    pid_t spawn_pid = -5;
    spawn_pid = fork();
    int childExitMethod = -5;

    //printf("Just forked\n");

    // If failure, close the socket, set to bad value and error out.
    if (spawn_pid < 0)
    {
      close(listenSocketFD);
      listenSocketFD = -1;
      fprintf(stderr, "Failure to fork child\n");
    }
    // Else, if we are in the child process..
    if (spawn_pid == 0)
    {
      //printf("In child proc\n");



      // Get the message the client saying whether it is encrypt(1) or decrypt(2).
      int msg_received = recv(establishedConnectionFD, &client_type, sizeof(client_type), 0);

      if (msg_received < 0)
      {
        fprintf(stderr, "SERVER: ERROR in receiving client type\n");
      }

      //printf("SERVER: client type: %d\n", client_type);

      // Now we send back to the client the type of the server.
      int msg_sent = send(establishedConnectionFD, &server_type, sizeof(server_type), 0);

      if (msg_sent < 0)
      {
        fprintf(stderr, "SERVER: ERROR failed to send type to client\n");
      }

      // If client and server are both not the same type, then do nothing.
      if (client_type != server_type)
      {
        // Client will disconnect.
        //fprintf(stderr, "SERVER: client and server not same type. Not allowed to connect.\n");
      }

      // Receive the plain text and the key files for verification as well.
      msg_received = recv(establishedConnectionFD, &plain_text_size, sizeof(plain_text_size), 0);
      if (msg_received < 0)
      {
        fprintf(stderr, "SERVER: ERROR in receiving plain text size\n");
      }
      
      // Do the same thing for the key.
      msg_received = recv(establishedConnectionFD, &key_size, sizeof(key_size), 0);
      if (msg_received < 0)
      {
        fprintf(stderr, "SERVER: ERROR in receiving key size\n");
      }

      printf("Plain text size: %d, key size: %d\n", plain_text_size, key_size);
   
      // Check if key is too small. If so, client will take care of it.
      if (key_size < plain_text_size)
      {
        printf("key smaller, client will handle it\n");
      }

      // Now we need to receive the plaintext file from the client.
      char plain_text_buffer[plain_text_size];
      // Fill with memset just in case.
      // source: https://www.tutorialspoint.com/c_standard_library/c_function_memset.htm
      memset(plain_text_buffer, '\0', plain_text_size);

      // Get the file from the client. Since it is huge, we need to use MSG_WAITALL.
      // source: https://stackoverflow.com/questions/8470403/socket-recv-hang-on-large-message-with-msg-waitall
      msg_received = recv(establishedConnectionFD, &plain_text_buffer, sizeof(plain_text_buffer), MSG_WAITALL);
      if (msg_received < 0)
      {
        fprintf(stderr, "SERVER: ERROR in receiving plaintext file\n");
      }
      //printf("SERVER: Plain text received: %s\n", plain_text_buffer);
   
      // Now get the key.
      char key[key_size];
      // Fill with memset just in case.
      memset(key, '\0', key_size);

      // Get the key file from the client.
      msg_received = recv(establishedConnectionFD, &key, sizeof(key), MSG_WAITALL);
      if (msg_received < 0)
      {
        fprintf(stderr, "SERVER: ERROR in receiving key file\n");
      }
      //printf("SERVER: key received: %s\n", key);


      // Now that we have plaintext and key, perform encryption process.
      // Make char[] for holding cipher text.
      char cipher_text[plain_text_size];
      // Memset it just in case.
      memset(cipher_text, '\0', plain_text_size);

      // Encrypt the plaintext into cipher text.
      // Loop through the entire plaintext char by char.
      int i, plain_char, cipher_char, result;
      // Make a list of available chars to choose from and determine index just like in keygen.
      char character_list[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    
      //printf("SERVER: about to decrypt\n");

      for (i = 0; i < plain_text_size; i++)
      {
        // Convert each char to an ascii value and do math to figure out what it should be changed to.
        // First, convert plaintext char to the correct index in character list.
        // If the char is a space, it is last in the position.
        // source: https://stackoverflow.com/questions/37231300/encryption-decryption-errorone-time-pad-encryption
        // Using ascii rather than chars in example.
	if (plain_text_buffer[i] == ' ')
        {
          plain_char = 26;
        }
        // Else, get its position by subtracting 65 from its ascii value.
        // source: https://simple.wikipedia.org/wiki/ASCII#/media/File:ASCII-Table-wide.svg
        else
        {
          plain_char = plain_text_buffer[i] - 65;
        }

        // Convert the key to the correct char.
        // If a space, the cipher char is last in the list.
        if (key[i] == ' ')
        {
          cipher_char = 26;
        }
        // Otherwise, do the same thing as before.
        else 
        {
          cipher_char = key[i] - 65;
        }

        // Add the plain text and the key together, as demonstrated in the assignment description.
        // Then, add to the ciphertext we will send back to the client.
        result = plain_char - cipher_char;
        // Check if it goes below 0, since we are doing subtraction.
        if (result < 0)
        {
          // Add 27 to get back into the proper range.
          result = result + 27;
        }
        // Modulo it to get the wrapping around effect.
        result = result % 27;
        // Insert the proper char from the character list based on its adjusted position.
        cipher_text[i] = character_list[result];
      }
    
      //printf("SERVER: decoded message sent: %s\n", cipher_text);

      // After encryption, send ciphertext back to client.
      msg_sent = send(establishedConnectionFD, cipher_text, sizeof(cipher_text), 0);
      if (msg_sent < 0)
      {
        fprintf(stderr, "SERVER: failed to send decoded text to client\n");
        exit(1);
      } 

      // Exit the child and go to parent.
      exit(0);
    }
    // Otherwise, close and we are in the parent. Close socket and set to bad value.
    else
    {
      // Wait for the child to terminate before looping again.
      waitpid(spawn_pid, &childExitMethod, 0);
    }
    // Close per multiserver.c
    close(establishedConnectionFD);

  } // End while.

  // Close the listening socket.
  close(listenSocketFD);

  return 0;
}
























