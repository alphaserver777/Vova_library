## Компилятор и флаги (C++17)
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Wno-deprecated-declarations

## Флаги линковки (подключаемые библиотеки)
LDFLAGS = -lpqxx -lpq

## Имена целей и исходников
TARGET = library_app
SRC = main.cpp

all: $(TARGET)

## Стадия компиляции + линковки в один шаг (g++ делает оба этапа)
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

## Отдельные стадии сборки (для демонстрации)
prep: $(SRC)
	# 1) Препроцессинг
	$(CXX) -E $(SRC) -o main.i

asm: prep
	# 2) Компиляция в ассемблер
	$(CXX) -S main.i -o main.s

obj: asm
	# 3) Ассемблирование в объектный файл
	$(CXX) -c main.s -o main.o

link: obj
	# 4) Линковка
	$(CXX) main.o -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o *.i *.s

.PHONY: all run clean prep asm obj link
