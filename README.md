Pthread-Determinant
===================

Parallelization computing the determinant of a matrix

Кроме самой программы (os2.c), приложены файлы с тестовыми матрицами различных размеров
---

__Компиляция программы:__
$ gcc -std=c99 -o os2 os2.c -lpthread –lm
__Запуск программы:__
$ ./os2 <Файл с матрицой> <Количество используемых потоков>
