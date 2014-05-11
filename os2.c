#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>


// аргументы потока
typedef struct
{
    int **matr;
    int matrSize;
} thread_params;

int nextColumn;
pthread_mutex_t nextColumnMutex;

void showUsage()
{
    printf("Использование: <Файл с матрицой> <Количество потоков>\n");
    printf("    Файл с матрицой так же должен содержать число N - размер квадратной матрицы\n");
    printf("    и саму N*N матрицу. Элементы матрицы разделены пробелами.\n");
    printf("Пример: matrix.txt 4\n");
}


int readMatrix(char *filepath, int ***matr, int *psize)
{
    FILE *fd = fopen(filepath, "r");
    if (fd == NULL)
    {
        printf("Невозможно открыть файл: %s.\n", strerror(errno));
        return -1;
    }

    // чтение размера
    int size;
    fscanf(fd, "%d", &size);
    if (size < 1)
    {
        printf("Некоректный рамзер матрицы.\n");
        return -1;
    }

    *matr = (int**) malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++)
        (*matr)[i] = (int*) malloc(size * sizeof(int));

    // чтение матрицы
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
        {
            if (feof(fd))
            {
                printf("Не хватает элементов матрицы");
                return -1;
            }

            fscanf(fd, "%d", &(*matr)[i][j]);
        }


    fclose(fd);

    *psize = size;

    return 0;
}


void freeMatrix(int ***matr, int size)
{
    for (int i = 0; i < size; i++)
        free((*matr)[i]);
    free(*matr);
}


void printMatrix(int **matr, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
            printf("%3d ", matr[i][j]);
        printf("\n");
    }
}


void getMinor(int **matr, int matrSize, int row, int col, int ***mnr)
{
    *mnr = (int**) malloc((matrSize - 1) * sizeof(int*));
    for (int k = 0; k < matrSize - 1; k++)
        (*mnr)[k] = (int*) malloc((matrSize - 1) * sizeof(int));

    // проход матрицы и занесение нужных элементов
    int mi = 0, mj = 0;
    for (int i = 0; i < matrSize; i++)
    {
        if (i != row) // пропуск строки
        {
            mj = 0;

            for (int j = 0; j < matrSize; j++)
            {
                if (j != col) // пропуск столбца
                {
                    (*mnr)[mi][mj++] = matr[i][j];
                }
            }

            mi++;
        }
    }
}


int determinant(int **matr, int size)
{
	// вычисление 1х1 детерминанта
    if (size == 1)
    {
        return matr[0][0];
    }
	// вычисление 2х2 детерминанта
    else if (size == 2)
    {
        return matr[0][0] * matr[1][1] - matr[1][0] * matr[0][1];
    }
	// рекурсивное вычисление остальных
    else
    {
        int det = 0;

        for (int i = 0; i < size; i++)
        {
            int **mnr;
            getMinor(matr, size, 0, i, &mnr);

            det += pow(-1.0, i) * matr[0][i] * determinant(mnr, size - 1);

            freeMatrix(&mnr, size - 1);
        }

        return det;
    }
}


// функция потоков
void* threadFunc(void *arg)
{
    int det = 0;

    thread_params *param = (thread_params*) arg;

    int col;
    while (1)
    {
        // вход в мьютекс
        pthread_mutex_lock(&nextColumnMutex);

        // получение номера след. столбца и инкрементация
        col = nextColumn;
        nextColumn++;

        // выход из мьютекса
        pthread_mutex_unlock(&nextColumnMutex);

        // завершение когда больше нет столбцов для обработки
        if (col >= param->matrSize)
            break;

        // получение минора
        int **mnr;
        getMinor(param->matr, param->matrSize, 0, col, &mnr);

        // обработка начиная с него и до конца
        det += pow(-1.0, col) * param->matr[0][col] * determinant(mnr, param->matrSize - 1);

        // освобождение памяти минора
        freeMatrix(&mnr, param->matrSize - 1);
    }

    return (void*) det;
}


// вычисление детерминанта многопоточно, методом разложения по строке
int determinantParallel(int **matr, int size, int threadsCount, int *presult)
{
    // если размер меньше 3, просто вычислить одним потоком
    if (size < 3)
    {
        *presult = determinant(matr, size);
        return 0;
    }

    pthread_t *threads = (pthread_t*) malloc(threadsCount * sizeof(pthread_t));

    // инициализация номера след. столбца для потоков, создание мьютекса
    nextColumn = 0;
    pthread_mutex_init(&nextColumnMutex, NULL);

    thread_params param;
    param.matr = matr;
    param.matrSize = size;

    // создание и запуск потоков
    for (int i = 0; i < threadsCount; i++)
    {
        int ret = pthread_create(&threads[i], NULL, threadFunc, &param);
        if (ret != 0)
        {
            printf("Не удалось создать поток. Код ошибки: %d\n", ret);
            return -1;
        }
    }

    // ожидание завершения всех потоков и получение детерминанта (сумма их возвращаемых значений)
    int det = 0;
    for (int i = 0; i < threadsCount; i++)
    {
        int thrDet;

        int ret = pthread_join(threads[i], (void**) &thrDet);
        if (ret != 0)
        {
            printf("Ошибка завершения потока. Код ошибки: %d\n", ret);
            return -1;
        }

        det += thrDet;
    }

    *presult = det;

    return 0;
}


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        showUsage();
        return -1;
    }

    char *filepath = argv[1];
    int threadsCount = atoi(argv[2]);
    if (threadsCount < 1)
    {
        printf("Неверный ввод количества потоков.\n");
        showUsage();
        return -1;
    }

    int size;
    int **matr = NULL;
    if (readMatrix(filepath, &matr, &size) == -1)
        return -1;


    printMatrix(matr, size);

    // получение текущего времени
    struct timeval timeStart;
    gettimeofday(&timeStart, NULL);

    // вычисление детерминанта
    int det;
    if (determinantParallel(matr, size, threadsCount, &det) != 0)
        return -1;

    // получение затраченного времени
    struct timeval timeEnd;
    gettimeofday(&timeEnd, NULL);
    double timeSpentMs = (timeEnd.tv_sec - timeStart.tv_sec) * 1000.0;
    timeSpentMs += (timeEnd.tv_usec - timeStart.tv_usec) / 1000.0;

    printf("Детерминант: %d\n", det);

    printf("Времени затрачено: %.2f сек.\n", timeSpentMs / 1000.0);

    freeMatrix(&matr, size);

    return 0;
}

