// Example client from lecture 2.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
// For checking file size.
#include <sys/stat.h>


// Error function. Calls and error and exits.
void error (const char *msg)
{
  perror(msg);
  exit(0);
}


int main(int argc, char *argv[])
{

  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;
  char buffer[256];
  // Define that it is the right client type to connect to otp_enc_d.
  // 1 = encrypt, 2 = decrypt;
  int client_type = 1;
  // This will be determined later when the server responds with its type.
  int server_type;



  // Error if not the correct 4 args.
  if (argc != 4)
  {
    fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
    exit(0);
  }

  // Before setting up everything, we need to check if the input is valid.
  // Make sure key is not too small and there are no badchars in the files, per assignment page.
  // source: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
  // Make two stat structs for both the files, then call stat on them with the command line input.
  // That will get the size of them so we can compare the two.
  struct stat plain_text_stat, key_stat;
  stat(argv[1], &plain_text_stat);
  int plain_text_size = plain_text_stat.st_size;

  // Do the same for the key.
  stat(argv[2], &key_stat);
  int key_size = key_stat.st_size;

  //printf("plaintext size: %d, key size: %d\n", plain_text_size, key_size);

  // If key too small, terminate, error and exit value to 1.
  if (key_size < plain_text_size)
  {
    fprintf(stderr, "Key too small\n");
    exit(1);
  }

  // Check for bad chars by reading file into string.
  // source: https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
  char * plain_text_buffer = 0;
  long length;
  FILE * f = fopen(argv[1], "rb");
  
  if (f)
  {
    // Use fseek to start from the beginning.
    fseek(f, 0, SEEK_END);
    // Get current file position of the stream.
    // source: https://www.tutorialspoint.com/c_standard_library/c_function_ftell.htm
    length = ftell (f);
    fseek(f, 0, SEEK_SET);
    plain_text_buffer = malloc (length);
    // If there is text, copy it into the buffer we made earlier.
    if (plain_text_buffer)
    {
      fread(plain_text_buffer, 1, length, f);
    }
    fclose(f);
  }

  // Remove trailing newline. From smallsh example.
  plain_text_buffer[strcspn(plain_text_buffer, "\n")] = '\0';

  //printf("file: %s\n", plain_text_buffer);

  // Iterate through string and check for chars that are not letters or spaces.
  // source: https://stackoverflow.com/questions/22283840/compare-a-char-in-char-with-a-loop
  int i;
  for (i = 0; i < length - 2; i++)
  {
    // If it is NOT an all-caps char or a space, then error and exit.
    // Check with ascii values.
    // source: https://simple.wikipedia.org/wiki/ASCII#/media/File:ASCII-Table-wide.svg
    if (plain_text_buffer[i] > 90 || ( plain_text_buffer[i] < 65 && plain_text_buffer[i] != 32) )
    {
      //printf("Found a non-alpha/space: %c, %d at position %d\n", plain_text_buffer[i], plain_text_buffer[i], i);
      fprintf(stderr, "Bad characters found in file %s\n", argv[1]);
      exit(1);
    }
    //printf("char: %c, ascii: %d, position: %d\n", plain_text_buffer[i], plain_text_buffer[i], i);
  }

  // Now do the same for the key.
  //printf("No bad chars in file\n");

  char * key_buffer = 0;
  long key_buffer_length;
  FILE * f_key = fopen(argv[2], "rb");
  
  if (f_key)
  {
    // Use fseek to start from the beginning.
    fseek(f_key, 0, SEEK_END);
    // Get current file position of the stream.
    // source: https://www.tutorialspoint.com/c_standard_library/c_function_ftell.htm
    key_buffer_length = ftell (f_key);
    fseek(f_key, 0, SEEK_SET);
    key_buffer = malloc (key_buffer_length);
    // If there is text, copy it into the buffer we made earlier.
    if (key_buffer)
    {
      fread(key_buffer, 1, key_buffer_length, f_key);
    }
    fclose(f_key);
  }

  // Remove trailing newline. From smallsh example.
  key_buffer[strcspn(key_buffer, "\n")] = '\0';

  //printf("key: %s\n", key_buffer);

  // Iterate through string and check for chars that are not letters or spaces.
  // source: https://stackoverflow.com/questions/22283840/compare-a-char-in-char-with-a-loop
  for (i = 0; i < key_buffer_length - 1; i++)
  {
    // If it is NOT an all-caps char or a space, then error and exit.
    // Check with ascii values.
    // source: https://simple.wikipedia.org/wiki/ASCII#/media/File:ASCII-Table-wide.svg
    if (key_buffer[i] > 90 || ( key_buffer[i] < 65 && key_buffer[i] != 32) )
    {
      //printf("Found a non-alpha/space: %c, %d at position %d\n", key_buffer[i], key_buffer[i], i);
      fprintf(stderr, "Bad characters found in key\n");
      exit(1);
    }
    // printf("char: %c, ascii: %d, position: %d\n", key_buffer[i], key_buffer[i], i);
  }

  //printf("No bad chars in key\n");

  // Truncate the key if need be.
  // source: https://stackoverflow.com/questions/6480440/how-to-truncate-c-char
  //printf("File length: %d\n", length);
  //printf("key length: %d\n", key_buffer_length);
  
  // Allocate space for the new truncated key.
  char* key_truncate = malloc(length);
  // Copy only the number of key chars necessary for the plain text.
  strncpy(key_truncate, key_buffer, length - 1);
  key_buffer_length = (length - 1);
  key_size = key_buffer_length;


  // Verify side-by-side that the two are the same length.
  //printf("Truncating key. Side-byside view:\n");

  /*
  for (i = 0; i < key_buffer_length; i++)
  {
    printf("%c  %c\n", key_truncate[i], plain_text_buffer[i]);
  }
  */

  //printf("new key size: %d\n", key_size);

  // Set up the server address struct.
  // First, clear out the address struct.
  memset((char*)&serverAddress, '\0', sizeof(serverAddress));
  // Get the port number, convert from string to int.
  portNumber = atoi(argv[3]);
  // Create a network-capable socket.
  serverAddress.sin_family = AF_INET;
  // Store the port number.
  serverAddress.sin_port = htons(portNumber);
  // Convert the machine name into a special form of address.
  serverHostInfo = gethostbyname("localhost");

  //printf("port num: %d\n", portNumber);
  
  // If cannot find the host, exit.
  if (serverHostInfo == NULL)
  {
    fprintf(stderr, "CLIENT: ERROR, no such host\n");
    exit(1);
  }
  
  // Copy the address.
  memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

  // Set up and create the socket.
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  
  // Error if socket creation failed.
  if (socketFD < 0)
  {
    error("CLIENT: ERROR opening socket\n");
  }

  // Connect socket to the server.
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
  {
    error("CLIENT: ERROR in connect()\n");
  }

  // Check if the server is otp_enc_d. If it is not, terminate the connection.
  int msg_sent = send(socketFD, &client_type, sizeof(client_type), 0);

  // If not successful, error.
  if (msg_sent < 0)
  {
    fprintf(stderr, "CLIENT: ERROR in sending msg to server\n");
    exit(1);
  }

  //printf("Just sent to server\n");

  // Now, get server type from server.
  int msg_received = recv(socketFD, &server_type, sizeof(server_type), 0);
  
  if (msg_received < 0)
  {
    fprintf(stderr, "CLIENT: ERROR in sending msg to server\n");
    exit(1);
  }

  //printf("CLIENT: server type: %d\n", server_type);

  if (client_type != server_type)
  {
    fprintf(stderr, "CLIENT: not the same type, connection not allowed\n");
    close(socketFD);
    exit(2);
  }
  // Otherwise, they are allowed to connect and proceed as normally.
  else
  {
    //printf("Client and server are the same. Proceed\n");
  }

  // Send the plain text size and key size to the server for verification.
  msg_sent = send(socketFD, &plain_text_size, sizeof(plain_text_size), 0);
  if (msg_sent < 0)
  {
    fprintf(stderr, "CLIENT: ERROR in sending plain text size to server\n");
    exit(1);
  }

  // Do the same for the key.
  msg_sent = send(socketFD, &key_size, sizeof(key_size), 0);
  if (msg_sent < 0)
  {
    fprintf(stderr, "CLIENT: ERROR in sending key size to server\n");
    exit(1);
  }

  // Copy the plain text into a char[] so it will be sent properly.
  char plain_text_send[plain_text_size];
  strncpy(plain_text_send, plain_text_buffer, plain_text_size);

  //printf("Plain text send: %s\n", plain_text_send);

  // Now, send the plaintext to the server. 
  msg_sent = send(socketFD, plain_text_send, sizeof(plain_text_send), 0);
  if (msg_sent < 0)
  {
    fprintf(stderr, "CLIENT: ERROR in sending plain text to server\n");
    exit(1);
  }
  //printf("CLIENT: Sent plaintext to server successfully\n");

  // Now send the key to the server.
  // Copy key into char[] to send.
  char key_send[key_size];
  strncpy(key_send, key_truncate, key_size);

  msg_sent = send(socketFD, key_send, sizeof(key_send), 0);
  if (msg_sent < 0)
  {
    fprintf(stderr, "CLIENT: ERROR in sending key to server\n");
    exit(1);
  }

  // Get the cipher text from the server now.
  // Make a char[] to hold the incoming cipher text.
  char cipher_text[plain_text_size];
  // Memset it just in case.
  memset(cipher_text, '\0', plain_text_size);
  // Get the message.
  msg_received = recv(socketFD, cipher_text, sizeof(cipher_text), MSG_WAITALL);
  if (msg_received < 0)
  {
    fprintf(stderr, "CLIENT: ERROR in getting cipher text from server\n");
    exit(1);
  }

  //printf("CLIENT: cipher text received: %s\n", cipher_text);
  fprintf(stdout, "%s\n", cipher_text);

  // Output the cipher text to stdout, per assignment instructions.
  // source: https://stackoverflow.com/questions/16430108/what-does-it-mean-to-write-to-stdout-in-c
  //fprintf(stdout, "%s\n", cipher_text);



  // Close the socket.
  close(socketFD);

  return 0;
}













