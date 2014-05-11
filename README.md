Pthread-Determinant
===================

Parallelization computing the determinant of a matrix

Кроме самой программы (os2.c), приложены файлы с тестовыми матрицами различных размеров

---

__Компиляция программы:__<br>
$ gcc -std=c99 -o os2 os2.c -lpthread –lm<br>
__Запуск программы:__<br>
$ ./os2 <Файл с матрицой> <Количество используемых потоков><br>
