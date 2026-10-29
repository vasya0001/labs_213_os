#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#define MAX_N 1500            
#define MAX_THREADS 64        
#define INPUT_SIZE 33554432   
#define EPS 1e-9              

double a[MAX_N][MAX_N + 1];   
int n = 0;                    
int num_threads;              
int demo_delay = 0;           
char input[INPUT_SIZE + 1];   


typedef struct {
    int k;      
    int first;  
    int last;   
} ThreadArgs;



int str_len(char *s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

void print(char *s) {
    long r = write(1, s, str_len(s));
}

void printerr(char *s) {
    long r = write(2, s, str_len(s));
}

void print_int(long long v) {
    char buf[24];
    int pos = 23;
    buf[pos] = '\0';
    int negative = v < 0;
    if (negative) {
        v = -v;
    }
    do {
        pos--;
        buf[pos] = '0' + v % 10;
        v = v / 10;
    } while (v > 0);
    if (negative) {
        pos--;
        buf[pos] = '-';
    }
    print(buf + pos);
}


void print_double(double x) {
    int negative = x < 0;
    if (negative) {
        x = -x;
    }
    if (x > 1e12) {
        print("(слишком большое число)");
        return;
    }
    long long scaled = (long long)(x * 1000000.0 + 0.5);
    if (negative && scaled != 0) {
        print("-");
    }
    print_int(scaled / 1000000);
    print(".");
    long long frac = scaled % 1000000;
    char buf[7];
    for (int i = 5; i >= 0; i--) {
        buf[i] = '0' + frac % 10;
        frac = frac / 10;
    }
    buf[6] = '\0';
    print(buf);
}




int parse_uint(char *s, int *result) {
    if (s[0] == '\0') {
        return 0;
    }
    int value = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9' || value > 100000) {
            return 0;
        }
        value = value * 10 + (s[i] - '0');
    }
    *result = value;
    return 1;
}


int parse_number(char **p, long long *out) {
    char *s = *p;
    int negative = 0;
    if (*s == '-') {
        negative = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    if (*s < '0' || *s > '9') {
        return 0;
    }
    long long v = 0;
    while (*s >= '0' && *s <= '9') {
        if (v > LLONG_MAX) {
            return 0; 
        }
        v = v * 10 + (*s - '0');
        s++;
    }
    *out = negative ? -v : v;
    *p = s;
    return 1;
}

int parse_matrix(void) {
    char *p = input;
    int cols = 0; 
    int col = 0;  

    while (1) {
        while (*p == ' ' || *p == '\t' || *p == '\r') {
            p++;
        }

        if (*p == '\n' || *p == '\0') {
            if (col > 0) { 
                if (n == 0) {
                    cols = col;
                } else if (col != cols) {
                    return 0;
                }
                n++;
            }
            col = 0;
            if (*p == '\0') {
                break;
            }
            p++;
        } else {
            long long value;
            if (n >= MAX_N || col > MAX_N || !parse_number(&p, &value)) {
                return 0;
            }
            a[n][col] = value;
            col++;
        }
    }

    return n > 0 && cols == n + 1;
}

double my_abs(double x) {
    return x < 0 ? -x : x;
}



void *work(void *arg) {
    ThreadArgs *t = (ThreadArgs *)arg;

    if (demo_delay > 0 && t->k == 0) {
        sleep(demo_delay); 
    }

    for (int i = t->first; i <= t->last; i++) {
        double с = a[i][t->k] / a[t->k][t->k];
        for (int j = t->k; j <= n; j++) {
            a[i][j] = a[i][j] - с * a[t->k][j];
        }
    }
    return 0;
}
int forward_elimination(void) {
    for (int k = 0; k < n; k++) {
        int pivot = k;
        for (int i = k + 1; i < n; i++) {
            if (my_abs(a[i][k]) > my_abs(a[pivot][k])) {
                pivot = i;
            }
        }
        if (my_abs(a[pivot][k]) < EPS) {
            print("Нет единственного решения (система несовместна или имеет бесконечно много решений)\n");
            return 0;
        }
        if (pivot != k) {
            for (int j = 0; j <= n; j++) {
                double tmp = a[k][j];
                a[k][j] = a[pivot][j];
                a[pivot][j] = tmp;
            }
        }

        int rows = n - 1 - k; 
        if (rows == 0) {
            continue;
        }
        int count = num_threads < rows ? num_threads : rows; 
        int base = rows / count;    
        int extra = rows % count;   

        pthread_t threads[MAX_THREADS];
        ThreadArgs args[MAX_THREADS];
        int next = k + 1;
        for (int t = 0; t < count; t++) {
            int size = base + (t < extra ? 1 : 0);
            args[t].k = k;
            args[t].first = next;
            args[t].last = next + size - 1;
            next += size;
            if (pthread_create(&threads[t], 0, work, &args[t]) != 0) {
                printerr("Ошибка pthread_create\n");
                return 0;
            }
        }
        for (int t = 0; t < count; t++) {
            if (pthread_join(threads[t], 0) != 0) {
                printerr("Ошибка pthread_join\n");
                return 0;
            }
        }
    }
    return 1;
}

void back_substitution(double *x) {
    for (int i = n - 1; i >= 0; i--) {
        double sum = a[i][n];
        for (int j = i + 1; j < n; j++) {
            sum = sum - a[i][j] * x[j];
        }
        x[i] = sum / a[i][i];
    }
}


int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        printerr("Ошибка подачи количества потоков!\n");
        return 1;
    }
    if (!parse_uint(argv[1], &num_threads) || num_threads < 1 || num_threads > MAX_THREADS) {
        printerr("Ошибка - число потоков должно быть целым от 1 до 64\n");
        return 1;
    }
    if (argc == 3 && !parse_uint(argv[2], &demo_delay)) {
        printerr("Ошибка - пауза должна быть целым числом секунд\n");
        return 1;
    }

    int total = 0;
    while (1) {
        int r = read(0, input + total, INPUT_SIZE - total);
        if (r == -1) {
            printerr("Ошибка read\n");
            return 1;
        }
        if (r == 0) {
            break;
        }
        total += r;
        if (total == INPUT_SIZE) {
            printerr("Ошибка: слишком много входных данных\n");
            return 1;
        }
    }
    input[total] = '\0';

    if (!parse_matrix()) {
        printerr("Ошибка: неверная матрица\n");
        return 1;
    }

    if (!forward_elimination()) {
        return 1;
    }

    static double x[MAX_N];
    back_substitution(x);

        print("Решение:\n");
    for (int i = 0; i < n; i++) {
        print("x");
        print_int(i + 1);
        print(" = ");
        print_double(x[i]);
        print("\n");
    }
    return 0;
}