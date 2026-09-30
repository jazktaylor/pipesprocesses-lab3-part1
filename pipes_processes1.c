// C program to demonstrate use of fork() and pipe() 
#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 
  
int main() 
{ 
    // We use two pipes 
    // First pipe to send input string from parent 
    // Second pipe to send concatenated string from child 
  
    int fd1[2];  // Used to store two ends of first pipe 
    int fd2[2];  // Used to store two ends of second pipe 
  
    char fixed_str[] = "howard.edu";  
    pid_t p; 
  
    if (pipe(fd1)== -1) 
    { 
        fprintf(stderr, "Pipe Failed" ); 
        return 1; 
    } 
    if (pipe(fd2)==-1) 
    { 
        fprintf(stderr, "Pipe Failed" ); 
        return 1; 
    } 
  
    printf("Other string is : %s\n\n", fixed_str);
    
    p = fork(); 
  
    if (p < 0) 
    { 
        fprintf(stderr, "fork Failed" ); 
        return 1; 
    } 
  
    // Parent process 
    else if (p > 0) 
    { 
        char input_str[100];
        char final_str[100];

        close(fd1[0]);  // Close reading end of pipes 
        close(fd2[1]);

        printf("Input : ");
        fgets(input_str, sizeof(input_str), stdin);
        input_str[strcspn(input_str, "\n")] = '\0';
  
        // Write input string and close writing end of first 
        // pipe. 
        if (write(fd1[1], input_str, strlen(input_str)+1) == -1)
        {
          perror("write");
          return 1;
        }
  
        close(fd1[1]); // Close writing end of pipes 

        // Receive the updated string from P2
        if (read(fd2[0], final_str, sizeof(final_str)) == -1)
        {
          perror("read");
          return 1;
        }

        // Add "gobison.org"
        strcat(final_str, "gobison.org");

        // Print the final result
        printf("Output : %s\n", final_str);

        // Close the reading end of fd2
        close(fd2[0]); 

        // Wait for child to print the concatenated string 
        wait(NULL); 

    } 
  
    // child process 
    else
    { 
        char concat_str[100];
        char second_input[100];

        close(fd1[1]);  // Close writing end of first pipes 
        close(fd2[0]); 
      
        // Read a string using first pipe  
        if (read(fd1[0], concat_str, sizeof(concat_str)) == -1)
        {
          perror("read");
          exit(1);
        } 
  
        // P2 is finished reading from fd1
        close(fd1[0]); 

        // Append howard.edu
        strcat(concat_str, fixed_str);
  
        // Print first Output
        printf("Output : %s\n\n", concat_str);

        // Get second input
        printf("Input : ");
        fgets(second_input, sizeof(second_input), stdin);
        second_input[strcspn(second_input, "\n")] = '\0';

        // Append second input to the string 
        strcat(concat_str, second_input);

        // Send the updated string back to P1 through fd2
        if (write(fd2[1], concat_str, strlen(concat_str) + 1) == -1)
        {
          perror("write");
          exit(1);
        }

        // Close both reading ends 
        close(fd2[1]); 

  
        exit(0); 
    } 
    return 0;
} 