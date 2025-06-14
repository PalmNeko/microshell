#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

int     microshell(char *argv[]);
int     list(char *argv[]);
int     pipeline(char *argv[]);
int     multi_process_pipeline(char *argv[]);
pid_t   run_multi_process_pipeline(char *argv[]);
pid_t   run_with_fork(char *argv[], bool use_pipe);
int     wait_last_pid(pid_t pid);
int     run_command(char *argv[]);
int     cd_command(char *argv[]);
void    ft_putstr_fd(int fd, const char *str);

#define fatal(x, ret) if (x == -1) { \
    ft_putstr_fd(2, "syscall Errorをここで出力\n"); \
    ret ; \
}

#define pass_fatal(x, ret) if (x == -1) { ret ; }

int stdin_fd;
char **env; // environ や errnoなどの組み込みグローバル変数？を使うと機械採点弾かれる。

int main(int argc, char *argv[], char *envp[])
{
    if (argc == 1)
        return (0);
    env = envp;
    return microshell(argv + 1);
}

int microshell(char *argv[])
{
    int status;

    stdin_fd = dup(0);
    fatal(stdin_fd, return 1);
    status = list(argv);
    fatal(close(stdin_fd), return 1);
    return status;
}

int list(char *argv[])
{
    int status;
    char **head;

    status = 0;
    head = argv;
    while (*argv != NULL)
    {
        if (strcmp(*argv, ";") == 0)
        {
            *argv = NULL;
            status = pipeline(head);
            pass_fatal(status, return 1);
            fatal(dup2(stdin_fd, 0), return 1);
            head = argv + 1;
        }
        argv++;
    }
    if (head != argv)
    {
        status = pipeline(head);
        pass_fatal(status, return 1);
    }
    return (status);
}

int pipeline(char *argv[])
{
    char **head;
    bool has_pipe;

    head = argv;
    has_pipe = false;
    while (*argv != NULL)
    {
        if (strcmp(*argv, "|") == 0)
        {
            has_pipe = true;
            break ;
        }
        argv++;
    }
    argv = head;
    if (has_pipe == false && strcmp("cd", argv[0]) == 0)
        return run_command(argv);
    else
        return multi_process_pipeline(argv);
}

int multi_process_pipeline(char *argv[])
{
    pid_t   last_pid;
    int     status;

    last_pid = run_multi_process_pipeline(argv);
    // last_pid が -1の時は許容する。
    // wait_last_pid関数で途中まで実行したコマンドも全てをwaitする。
    status = wait_last_pid(last_pid);
    pass_fatal(status, return 1);
    return (status);
}

pid_t run_multi_process_pipeline(char *argv[])
{
    pid_t last_pid;
    char **head;

    head = argv;
    while (*argv != NULL)
    {
        if (strcmp(*argv, "|") == 0)
        {
            *argv = NULL;
            last_pid = run_with_fork(head, true);
            pass_fatal(last_pid, return -1);
            head = argv + 1;
        }
        argv++;
    }
    if (head != argv)
        last_pid = run_with_fork(head, false);
    return last_pid;
}

pid_t run_with_fork(char *argv[], bool use_pipe)
{
    pid_t   pid;
    int     status;
    int     pipefds[2];

    if (use_pipe)
        fatal(pipe(pipefds), return -1);
    pid = fork();
    fatal(pid, return -1);
    if (pid == 0)
    {
        if (use_pipe)
        {
            fatal(dup2(pipefds[1], 1), exit(1));
            fatal(close(pipefds[1]), exit(1));
            fatal(close(pipefds[0]), exit(1));
        }
        fatal(close(stdin_fd), exit(1));
        status = run_command(argv);
        exit(status);
    }
    if (use_pipe)
    {
        fatal(dup2(pipefds[0], 0), return -1);
        fatal(close(pipefds[0]), return -1);
        fatal(close(pipefds[1]), return -1);
    }
    return (pid);
}

int wait_last_pid(pid_t pid)
{
    int     wstatus;
    int     status;
    pid_t   endpid;

    status = 0;
    endpid = waitpid(-1, &wstatus, 0);
    while (endpid != -1)
    {
        if (pid != -1 && pid == endpid)
        {
            if (WIFEXITED(wstatus))
                status = WEXITSTATUS(wstatus);
            else if (WIFSIGNALED(wstatus))
                status = WTERMSIG(wstatus) + 128;
        }
        endpid = waitpid(-1, &wstatus, 0);
    }
    if (pid == -1)
        return (1);
    return (status);
}

int run_command(char *argv[])
{
    if (argv[0] == NULL)
        return (0);
    if (strcmp(argv[0], "cd") == 0)
        return cd_command(argv + 1);
    fatal(execve(argv[0], argv, env), {
        ft_putstr_fd(2, "execve失敗しました: ");
        ft_putstr_fd(2, argv[0]);
        ft_putstr_fd(2, "\n");
    });
    return (1);
}

int cd_command(char *argv[])
{
    if (argv[0] == NULL || argv[1] != NULL) // 少ない(argv[0] == NULL) 多い(argv[1] != NULL)
    {
        ft_putstr_fd(2, "引数がおかしいです\n");
        return (1);
    }
    fatal(chdir(argv[0]), {
        ft_putstr_fd(2, "chdirに失敗しました。: ");
        ft_putstr_fd(2, argv[0]);
        ft_putstr_fd(2, "\n");
        return (1);
    });
    return (0);
}

void ft_putstr_fd(int fd, const char *str)
{
    size_t len;

    len = 0;
    while (str[len] != '\0')
        len++;
    fatal(write(fd, str, len), ;);
    return ;
}
