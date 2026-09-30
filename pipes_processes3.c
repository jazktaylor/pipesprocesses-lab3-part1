#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/**
 * Executes the command "cat scores | grep Lakers".  In this quick-and-dirty
 * implementation the parent doesn't wait for the child to finish and
 * so the command prompt may reappear before the child terminates.
 * 
 * Process hierarchy: 
 * 
 *    parent
 *      |-- cat
 *      |
 *      |-- grep
 *            |-- sort
 *
 * pipefd1 connects cat -> grep
 * pipefd2 connects grep -> sort
 */

int main(int argc, char **argv)
{
  if (argc != 2)
    {
      printf("Usage: %s <grep argument>\n", argv[0]);
      return 1;
    }
  int pipefd1[2];
  int pipefd2[2];
  int grep_pid;

  char *cat_args[] = {"cat", "scores", NULL};
  char *grep_args[] = {"grep", argv[1], NULL};

  // create the 2 pipes 

  if (pipe(pipefd1) == -1)
  {
    perror("pipefd1");
    return 1;
  }
  if (pipe(pipefd2) == -1)
  {
    perror("pipefd2");
    close(pipefd1[0]);
    close(pipefd1[1]);
    return 1;
  }

  // create the grep process
  grep_pid = fork();

  if (grep_pid < 0)
    {
      perror("fork grep");
      close(pipefd1[0]);
      close(pipefd1[1]);
      close(pipefd2[0]);
      close(pipefd2[1]);
      return 1;
    }

  if (grep_pid == 0)
    {
      // child process handles grep

      int sort_pid = fork();
      
      if (sort_pid < 0)
        {
          perror("fork sort");
          close(pipefd1[0]);
          close(pipefd1[1]);
          close(pipefd2[0]);
          close(pipefd2[1]);
          return 1;
        }
      if (sort_pid == 0)
        {
          // child's child process handles sort

          if (dup2(pipefd2[0], STDIN_FILENO) == -1)
            {
              perror("dup2 sort stdin");
              return 1;
            }

          close(pipefd1[0]);
          close(pipefd1[1]);
          close(pipefd2[0]);
          close(pipefd2[1]);

          execlp("sort", "sort", NULL);

          perror("exec sort");
          return 1;
        }
      else
      {
        // child process: executes grep

        if (dup2(pipefd1[0], STDIN_FILENO) == -1)
          {
            perror("dup2 grep stdin");
            return 1;
          }

        if (dup2(pipefd2[1], STDOUT_FILENO) == -1)
          {
            perror("dup2 grep stdout");
            return 1;
          }

        close(pipefd1[0]);
        close(pipefd1[1]);
        close(pipefd2[0]);
        close(pipefd2[1]);

        execvp("grep", grep_args);

        perror("exec grep");
        return 1;
      }
    }
  else
    {
      // parent create a process to execute cat

      int cat_pid = fork();

      if (cat_pid < 0)
        {
          perror("fork cat");
          close(pipefd1[0]);
          close(pipefd1[1]);
          close(pipefd2[0]);
          close(pipefd2[1]);

          // wait for grep before exiting
          waitpid(grep_pid, NULL, 0);
          return 1;
        }


      if (cat_pid == 0)
        { 
          // cat process: executes cat scores

          if (dup2(pipefd1[1], STDOUT_FILENO) == -1)
            {
              perror("dup2 cat stdout");
              return 1;
            }

          close(pipefd1[0]);
          close(pipefd1[1]);
          close(pipefd2[0]);
          close(pipefd2[1]);

          execvp("cat", cat_args);

          perror("exec cat");
          return 1;
        }
      else
      {
        // supervising parent closes all unused pipe ends

        close(pipefd1[0]);
        close(pipefd1[1]);
        close(pipefd2[0]);
        close(pipefd2[1]);

        int cat_status;
        int grep_status;

        if (waitpid(cat_pid, &cat_status, 0) == -1)
          {
            perror("waitpid cat");
            return 1;
          }
        if (waitpid(grep_pid, &grep_status, 0) == -1)
          {
            perror("waitpid grep");
            return 1;
          }
        
        // report unsuccessful child termination
        if (!WIFEXITED(cat_status)|| WEXITSTATUS(cat_status) != 0)
          {
            fprintf(stderr, "cat process exited unsuccessfully\n");
            return 1;
          }
        if (!WIFEXITED(grep_status)|| WEXITSTATUS(grep_status) != 0)
          {
            fprintf(stderr, "grep process exited unsuccessfully\n");
            return 1;
          }
      }
    }
    return 0;
}