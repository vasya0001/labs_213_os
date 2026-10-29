#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
int str_len(const char *s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}
void print(const char *s) {
    write(STDOUT_FILENO, s, str_len(s));
}
void print_error(const char *s) {
    write(STDERR_FILENO, s, str_len(s));
}
int main(void) {
    print("Введите имя файла- ");

    char filename[256];
    int len = 0;
    char c;
    while (1) {
        int r = read(STDIN_FILENO, &c, 1); 
        if (r == -1) {
            print_error("Ошибка - не удалось прочитать имя файла\n");
            return 1;
        }
        if (r == 0 || c == '\n') { 
            break;
        }
        if (len >= 255) {
            print_error("Ошибка - имя файла слишком длинное\n");
            return 1; 
        }
        filename[len] = c;
        len++;
    }
    filename[len] = '\0'; 

    if (len == 0) {
        print_error("ошибка - пустое имя файла\n");
        return 1;
    }
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        print_error("ошибка open - не удалось открыть файл\n");
        return 1;
    }
    int pipe1[2];
    if (pipe(pipe1) == -1) {
        print_error("ошибка pipe\n");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        print_error("Ошибка fork\n");
        return 1;
    }

    if (pid == 0) {
        close(pipe1[0]); 

        if (dup2(file_fd, STDIN_FILENO) == -1) {
            print_error("Ошибка dup2\n");
            _exit(1);
        }
        if (dup2(pipe1[1], STDOUT_FILENO) == -1) {
            print_error("Ошибка dup2\n");
            _exit(1);
        }
        close(file_fd);
        close(pipe1[1]);

        execl("./child", "child", (char *)0);
        print_error("Ошибка execl\n");
        _exit(1);
    }

    close(file_fd);  
    close(pipe1[1]); 

    char buffer[256];
    int n;
    while ((n = read(pipe1[0], buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, n) == -1) {
            print_error("Ошибка write\n");
            break;
        }
    }
    if (n == -1) {
        print_error("Ошибка read из pipe\n");
    }
    close(pipe1[0]);

    int status; 
    if (wait(&status) == -1) { 
        print_error("Ошибка wait\n");
        return 1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) { 
        print_error("Дочерний процесс завершился с ошибкой\n");
        return 1;
    }

    return 0;
}
