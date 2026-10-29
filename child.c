#include <unistd.h>
#include <limits.h>

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
        if (result > (LLONG_MAX - digit) / 10) {
            return -2; //переполнение
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

int write_number(long long n) {
    char buf[24];
    int pos = 23;
    buf[pos] = '\n';
    do {
        pos--;
        buf[pos] = '0' + n % 10; // берём последнюю цифру
        n = n / 10;
    } while (n > 0);

    int size = 24 - pos;
    if (write(STDOUT_FILENO, buf + pos, size) != size) {
        return -1;
    }
    return 0;
}

int main(void) {
    long long num;
    int r;
    while ((r = read_number(&num)) == 1) {
        if (num < 0 || is_prime(num)) {
            break;
        }
        if (num < 2) {
            continue;
        }
        if (write_number(num) == -1) {
            return 1;
        }
    }
    if (r == -1) {
        const char msg[] = "Ошибка: неверный ввод\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }
    return 0;
}
