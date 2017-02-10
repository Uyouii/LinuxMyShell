//A simple shell program by Dong Taiyou 
//ZheJinag University

//This Program is writed in the syntax of C++11. So something will go wrong in the syntax of C++98

#include "myshell.h"

using namespace std;

int main(void){

	//class MyShell: the main class of this program 
	MyShell Shell;
	
	//to get the input and return a bool after each operation
	//each time to get one line
	while (Shell.readInput()) {
		//cout << "myshell> ";
		
		//After reading user input, the steps are: 
		//内部命令：
		//…..
		//外部命令：
		// (1) fork a child process using fork()
		// (2) the child process will invoke execvp()
		// (3) if command included &, parent will invoke wait()
		//…..
	}
	return 0;
}
