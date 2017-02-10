
myshell:  myshell.o	main.o  Variable.o
	g++-4.8 -std=c++11 -o myshell  myshell.o main.o Variable.o

myshell.o: myshell.cpp #  myshell.h Tools.h  Type.h  
	g++-4.8 -std=c++11 -c myshell.cpp 

Variable.o:Variable.cpp
	g++-4.8 -std=c++11 -c Variable.cpp

main.o :main.cpp
	g++-4.8 -std=c++11 -c main.cpp

clean:
	rm -f *.o
