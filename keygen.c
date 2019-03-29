/*******************************************************
 * John Williams
 * Creates a random key of length argv[1].
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
// For random function.
#include <time.h>

int main(int argc, char* argv[])
{
  // Get size of key from char* argv[1].
  int key_size = atoi(argv[1]);
  // For random char generation.
  int random_char;

  // If wrong number of args, let the user know.
  if (argc != 2)
  {
    printf("Need keygen followed by a number: <keygen num>\n");
    exit(0);
  }

  // Print out values for debugging.
  //printf("Num args: %d, size of key: %d\n", argc, key_size);


  // Generate key_size num random capital letters to put in the key.
  // Use srand for random.
  srand(time(NULL));

  // Determine the available chars to choose from: All caps including a space.
  // source: https://stackoverflow.com/questions/19724346/generate-random-characters-in-c
  char available_chars[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  // Loop for each char to be generated.
  int i;
  for (i = 0; i < key_size; i++)
  {
    // Generate random char and print it.
    int rando = rand() % 27;

    printf("%c", available_chars[rando]);

  }

  // Print newline then exit to stdout.
  // source: https://stackoverflow.com/questions/16430108/what-does-it-mean-to-write-to-stdout-in-c
  printf("\n");
  return 0;
}








