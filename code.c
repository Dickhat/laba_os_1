#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

// Поиск файлов по каталогу
void search_dir(char* dir, char massive[1024][256], int* current_number_path, char* root)
{
    char current_path [256];    // Текущий путь

    // Проверка для корневого пути
    if(strcmp(dir, root) == 0)
        strcpy(current_path, "");  // Копирование текущего пути
    else
        strcpy(current_path, dir);  // Копирование текущего пути


    DIR * dp;               // Создание потока каталога
    struct dirent * entry;  // Стуктура директории
    struct stat statbuf;    // Структура с информацией о файле

    dp = opendir(dir);

    //// Проверка, что директорию можно открыть
    //if (dp == NULL)
    //    printf("Can't open directory:%s\n", dir);
    //    return;

    chdir(dir); //Смена текущей директории на dir
    
    // Пока можно прочитать директорию
    while((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name, &statbuf); //Считывание информации в буфер

        // Проверка, что файл - директория
        if(S_ISDIR(statbuf.st_mode))
        {
            // Пропуск текущего каталога и родительского
            if(strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;

            puts(entry->d_name);
            // Сохранение пути
            strcpy(massive[*current_number_path], current_path);
            strcat(massive[*current_number_path], entry->d_name);
            (*current_number_path)++;

            strcat(current_path, entry->d_name);                            // Новый путь к директории
            strcat(current_path, "/");                                  // Новый путь к директории
            search_dir(current_path, massive, current_number_path, root);   // Рекурсивный поиск по директории

            if(strcmp(dir, root) != 0)
                strcpy(current_path, dir); // Возвращение пути директории
            else
                strcpy(current_path, ""); // Возвращение пути директории
        }
        else
        {
            puts(entry->d_name);
            
            // Сохранение пути
            strcpy(massive[*current_number_path], current_path);
            strcat(massive[*current_number_path], entry->d_name);
            (*current_number_path)++;

            if(*current_number_path == 3)
                puts(entry->d_name);
        }
    }

    chdir("..");
    closedir(dp);
}

// Архивирование файлов
void archivate(char* in, char* out)
{
    int* current_path = (int*)malloc(1);          // Номер текущего пути
    *current_path = 0;

    char massive_paths[1024][256];                // Массив 1024 путей длинной 256 символов
    int massive_sizes[1024];                      // Массив размеров для файлов

    // Поиск всех файлов и поддиректорий
    search_dir(in, massive_paths, current_path, in);

    // Число файлов > 0
    if(*current_path > 0)
    {
        chdir(in); //Смена текущей директории на dir

        FILE* out_file = fopen(out, "wb");              // Создани потока для записи в байтах в выходной файл
        fprintf(out_file, "%d\n", *current_path);        // Запись числа файлов, хранящихся в папке для архивирования

        // Запись размеров файлов для создания 'шапки' архива
        for(int i = 0; i < *current_path; ++i)
        {
            char * file_path = massive_paths[i];
            FILE * read_file = fopen(file_path, "r");   //Создание потока для чтения файла

            struct stat fileStatbuff;                   // Характеристика файла
            fstat(read_file->_fileno, &fileStatbuff);   // Получение характеристики

            int file_size;

            // Если директория, то обозначить ее размер -1
            if(S_ISDIR(fileStatbuff.st_mode))
                file_size = -1;                         // Получение размера файла
            else
                file_size = fileStatbuff.st_size;       // Получение размера файла


            massive_sizes[i] = file_size;               // Запоминание размера файла
            fprintf(out_file, "%s %d\n", massive_paths[i], file_size); // Запись информации о файле в архив
    
            fclose(read_file);
        }

        // Копирование данных из файла исходника в файл в архива
        for (int i = 0; i < *current_path; ++i)
        {
            FILE* read_file = fopen(massive_paths[i], "rb"); //открытие файла

            if (massive_sizes[i] != -1)
            {
                // цикл копирования каждого байта данных из текущего файла в архив
                for (int j = 0; j < massive_sizes[i]; ++j)
                    fputc(fgetc(read_file), out_file);           // Побайтовая запись в файл
            }

            fclose(read_file);
        }
        
        fclose(out_file);
    }

    free(current_path);
}

// Разархивирование файлов
void dearchivate(char* in, char* out)
{
    int *number_files = (int*)malloc(sizeof(int));  // Число файлов
    *number_files = 0;

    FILE* read_file = fopen(in, "rb");          // Открытие файла архива
    fscanf(read_file, " %d\n", number_files);   // Определение числа файлов

    char massive_paths[1024][256];              // Массив 1024 путей длинной 256 символов
    int massive_sizes[1024];                    // Массив размеров для файлов

    for(int k = 0; k < 1024; ++k)
        massive_sizes[k] = 0;

    // Формирование путей для распаковки и получение размеров файлов
    for(int i = 0; i < *number_files; ++i)
    {
        char path[256];
        int * size = (int*)malloc(sizeof(int));

        // Создание пути файла
        strcpy(massive_paths[i], out);              // Куда распаковывается архив
        strcat(massive_paths[i], "/");
        fscanf(read_file, "%s %d\n", path, size);   // Получение пути и размера файла
        massive_sizes[i] = *size;                   // Присвоение размера файла
        strcat(massive_paths[i], path);             // unpackage_path/path
    }

    umask(0);
    mkdir(out, S_IRWXU | S_IRWXG | S_IRWXO);

    //цикл создания файлов из архива 
    for(int i = 0; i < *number_files; ++i)
    {
        char *tem = (char*)malloc(sizeof(char)*256);
        strcpy(tem, massive_paths[i]);

        // Если сейчас директория
        if(massive_sizes[i] == -1)
        {           
            mkdir(tem, S_IRWXU | S_IRWXG | S_IRWXO);
            continue;
        }

        FILE * write_file = fopen(tem, "wb");
        //fprintf(stderr, "Error opening file: %s\n", strerror(errno));   

        char* buffer = (char*)malloc(massive_sizes[i]*sizeof(char));            // Буфер данных
        fread(buffer, massive_sizes[i], 1, read_file);                          // Чтение
        
        size_t write_count = fwrite(buffer, massive_sizes[i], 1, write_file);   // Запись
    
        free (buffer);      // Освобождение буфера

        fclose(write_file); // Закрытие файла
    }

    free(number_files);
    fclose(read_file);
}

//  Основная программа
int main(int argc, char* argv[])
{
    struct stat st; // Характеристика файлов

    //// Печать аргументов
    //for(int i = 0; i < argc; ++i)
    //    puts(argv[i]);
 
    char* input_file = NULL;
    char* output_file_archive = (char*)malloc(256);
    strcpy(output_file_archive, "/home/jenkism/Desktop/arhive/temp");    // Папка по умолчанию для архивирования  

    char* output_file_dearchive = (char*)malloc(256);
    strcpy(output_file_dearchive, "/home/jenkism/Desktop/dearhive");    // Папка по умолчанию для разархивирования

    // Проверка, что директория существует
    if (stat("/home/jenkism/Desktop/dearhive", &st) != 0)
    {
        umask(0);
        mkdir("/home/jenkism/Desktop/dearhive", S_IRWXU | S_IRWXG | S_IRWXO);
    }

   // Проверка, что директория существует
    if (stat("/home/jenkism/Desktop/arhive/", &st) != 0)
    {        
        umask(0);
        mkdir("/home/jenkism/Desktop/arhive/", S_IRWXU | S_IRWXG | S_IRWXO);
    }

    //if Если не указана папка для разархивиривания или архивирования
    if (argc == 3)
    {
        input_file = argv[2];   // Входной поток
    }
    else if (argc == 4)
    {
        input_file = argv[2];

        // Смена папок по умолчанию для выходного потока
        if(strcmp(argv[1], "-a") == 0)
            strcpy(output_file_archive, argv[3]);
        else
            strcpy(output_file_dearchive, argv[3]);
    }
    else
    {
        printf(" Указано некорректное число аргументов в программе\n");
        return 1;
    }

    // Указан ключ архивирования
    if(strcmp(argv[1], "-a") == 0)
        archivate(input_file, output_file_archive);
    // Указан ключ разархивирования
    else if (strcmp(argv[1], "-d") == 0)
        dearchivate(input_file, output_file_dearchive);
    // Указан некорректный ключ
    else
        printf("Keys incorrect\n");

    free(output_file_archive);
    free(output_file_dearchive);

    return 0;
}