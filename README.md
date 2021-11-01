# TEST_Signature
Создание сигнатуры файла по блоками заданной величены. Хэш MD5.

## Project VS2019
### Release x86
Компиляцию можно произвести в конфигурации Release x86.
```bash
git clone git://github.com/SOTODiLah/TEST_Signature.git
````
### Other config
Для компиляции иной конфигурации необходимо скачать и скопилировать библеотеку [CryptoPP](https://github.com/weidai11/cryptopp) в нужной вам конфигурации.
```bash
git clone git://github.com/weidai11/cryptopp
````

## Download program

Google Drive [ZIP](https://drive.google.com/file/d/1_awqe0CbxfvD5BDJobVKWrq-Ot9Cajm5/view?usp=sharing) - только программа.

Google Drive [ZIP](https://drive.google.com/file/d/1vOXv8lHla6tN9cDsl4ukCkBggUd0F65t/view?usp=sharing) - программа и тестовый файл 1gb.

## Launch

Запуск программы осуществляет через командную строку.<br>
```bash
Options:
  -h,--help                   Вывод этого справочного сообщения.
  -i,--input TEXT             Путь до входного файла.
  -o,--output TEXT            Путь до выходного файла.
  -s,--sizeBuffer UINT        Размер буфера (блока файла).
  -t,--timeWork BOOLEAN       Вывод времени работы программы.
````
## Struct project

### Class Signature

Содержит в себе 3 метода создания сигнатуры файла. В последней сборке проекта используется основной метод (Signature::signatureFileSingleReader). 
Два других (Signature::signatureFileAllReader и Signature::signatureFileHalfReader) реализованы в другой архитектуре многопоточного программирования,
и методах работы с файлами, а также имеют меньшую скорость работы.

### Class SmartQueue

Класс основан на атомарных операциях, имеет в себе основные методы очереди SmartQueue::push и SmartQueue::pop, очередь в памяти представляет контейнер вектор фиксированной
длины, позиции вставки (push) и получения (pop) элементов циклично перемещаются по индексам вектора.

### Class FileBlock

Вспомогательный класс, используется как элемент SmartQueue. Хранит в себе блок байтов файла и позицию блока в файле. Позволяет получить MD5 хэш файла,
и необходимое место в выходном файле

### Struct TimeCode

Структура упрощающая вычисление времени работы определенного участка кода. Часто использовалась при тестировании разных сборок программы.

