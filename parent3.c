#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define EXCHANGE_NAME "exchange.bin"  
#define EXCHANGE_SIZE 1048576        

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
    print("Введите имя файла: ");

    char filename[256];
    int len = 0;
    char c;
    while (1) {
        int r = read(STDIN_FILENO, &c, 1);
        if (r == -1) {
            print_error("Ошибка read: не удалось прочитать имя файла\n");
            return 1;
        }
        if (r == 0 || c == '\n') {
            break;
        }
        if (len >= 255) {
            print_error("Ошибка: имя файла слишком длинное\n");
            return 1;
        }
        filename[len] = c;
        len++;
    }
    filename[len] = '\0';

    if (len == 0) {
        print_error("Ошибка: пустое имя файла\n");
        return 1;
    }
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        print_error("Ошибка open: не удалось открыть файл\n");
        return 1;
    }
    int ex_fd = open(EXCHANGE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (ex_fd == -1) {
        print_error("Ошибка open: не удалось создать файл обмена\n");
        close(file_fd);
        return 1;
    }
    if (ftruncate(ex_fd, EXCHANGE_SIZE) == -1) {
        print_error("Ошибка ftruncate\n");
        close(file_fd);
        close(ex_fd);
        unlink(EXCHANGE_NAME);
        return 1;
    }
    char *mem = mmap(0, EXCHANGE_SIZE, PROT_READ, MAP_SHARED, ex_fd, 0);
    if (mem == MAP_FAILED) {
        print_error("Ошибка mmap\n");
        close(file_fd);
        close(ex_fd);
        unlink(EXCHANGE_NAME);
        return 1;
    }
    close(ex_fd); 

    pid_t pid = fork();
    if (pid == -1) {
        print_error("Ошибка fork\n");
        close(file_fd);
        munmap(mem, EXCHANGE_SIZE);
        unlink(EXCHANGE_NAME);
        return 1;
    }

    if (pid == 0) {
        if (dup2(file_fd, STDIN_FILENO) == -1) {
            print_error("Ошибка dup2\n");
            _exit(1);
        }
        close(file_fd);
        execl("./child", "child", EXCHANGE_NAME, (char *)0);
        print_error("Ошибка execl: не удалось запустить ./child\n");
        _exit(1);
    }

    close(file_fd); 

    int status;
    if (wait(&status) == -1) {
        print_error("Ошибка wait\n");
        munmap(mem, EXCHANGE_SIZE);
        unlink(EXCHANGE_NAME);
        return 1;
    }

    int result = 0;
    int size = str_len(mem);
    if (size > 0 && write(STDOUT_FILENO, mem, size) == -1) {
        print_error("Ошибка write\n");
        result = 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        print_error("Дочерний процесс завершился с ошибкой\n");
        result = 1;
    }

    if (munmap(mem, EXCHANGE_SIZE) == -1) {
        print_error("Ошибка munmap\n");
        result = 1;
    }
    if (unlink(EXCHANGE_NAME) == -1) {
        print_error("Ошибка unlink\n");
        result = 1;
    }
    return result;
}
