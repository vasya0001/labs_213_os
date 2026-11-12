#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#define EXCHANGE_SIZE 1048576  

int str_len(const char *s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}
void print_error(const char *s) {
    write(STDERR_FILENO, s, str_len(s));
}

int is_prime(long long n) {
    if (n < 2) {
        return 0;
    }
    for (long long i = 2; i <= n / i; i++) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

int read_number(long long *num) {
    char c;
    int r;
    do {
        r = read(STDIN_FILENO, &c, 1);
        if (r <= 0) {
            return r; 
        }
    } while (c == ' ' || c == '\n' || c == '\t' || c == '\r');

    int negative = 0;
    if (c == '-') {
        negative = 1;
        r = read(STDIN_FILENO, &c, 1);
        if (r <= 0) {
            return -1;
        }
    }

    if (c < '0' || c > '9') {
        return -1;
    }

    long long result = 0;
    while (c >= '0' && c <= '9') {
        int digit = c - '0';
        if (result > (9223372036854775807LL - digit) / 10) {
            return -2; 
        }
        result = result * 10 + digit;

        r = read(STDIN_FILENO, &c, 1);
        if (r == -1) {
            return -1;
        }
        if (r == 0) {
            break;
        }
    }
    *num = negative ? -result : result;
    return 1;
}

int put_number(char *mem, int *pos, long long n) {
    char buf[24];
    int p = 23;
    buf[p] = '\n';
    do {
        p--;
        buf[p] = '0' + n % 10;
        n = n / 10;
    } while (n > 0);

    int size = 24 - p;
    if (*pos + size >= EXCHANGE_SIZE) {
        return 0;
    }
    for (int i = 0; i < size; i++) {
        mem[*pos + i] = buf[p + i];
    }
    *pos += size;
    return 1;
}
int main(int argc, char **argv) {
    if (argc != 2) {
        print_error("Использование: child <файл обмена>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        print_error("Ошибка open: не удалось открыть файл обмена\n");
        return 1;
    }
    char *mem = mmap(0, EXCHANGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        print_error("Ошибка mmap\n");
        close(fd);
        return 1;
    }
    close(fd); 


    int pos = 0;     
    long long num;
    int r;
    int result = 0;
    while ((r = read_number(&num)) == 1) {
        if (num < 0 || is_prime(num)) {
            break;
        }
        if (num < 2) {
            continue;
        }
        if (!put_number(mem, &pos, num)) {
            print_error("Ошибка: не хватило места в общей памяти\n");
            result = 1;
            break;
        }
    }
    if (r == -1) {
        print_error("Ошибка: неверный ввод\n");
        result = 1;
    }
    if (r == -2) {
        print_error("Ошибка: число слишком большое\n");
        result = 1;
    }
    if (munmap(mem, EXCHANGE_SIZE) == -1) {
        print_error("Ошибка munmap\n");
        result = 1;
    }
    return result;
}