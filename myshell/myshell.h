#ifndef _MYSHELL_H_
#define _MYSHELL_H_

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include "Variable.h"

using namespace std;

class MyShell{
private:
	map<string,int> InnerInc;	//to restore the name of the inner instructions
	map<string,string> EnvirVar;//to restore the name and value of the environment variables
	deque<string> BackProcess;	//to restore the name and number of the BackProcess
	bool Ispipe;				//to judge whether the operation data is from stdin or pipe
	bool UseMore;				//to judge whether use "more" operations to show the result			
	
	string Tempfile;			//record the temporary file named used in the program
	string Tempfile2;			//another file like Tempfile
	deque<string> setDeque;		//to restore the value of the positional parameters.
	Variable Var;				//Class Variable: Manage the variable declared in this shell

/*********** Some of Environment Variable **********/
private:
	string HOME;				//record the home path of the user
	string SHELL;				//record the path of this shell program
	string CurPath;				//record the currunt path
	string USER;				//record the name of the user
	string SESSION;				//record the version of this linux system

/*********** Inner Instrctions *************/
private:
	int cd_oper(vector<string>& , int);			//cd 		Change the shell working directory.	
	int pwd_oper(vector<string>& , int);		//pwd 		Print the name of the current working directory.
	int time_oper(vector<string>&, int);		//time 		Print the system date and time.
	int set_oper(vector<string>&, string);		//set 		Set values of shell options and positional parameters.
	int echo_oper(vector<string>&, string,int);	//echo  	Write arguments to the standard output.
	int environ_oper(vector<string>&,int);		//environ   List all the environmental variables.
	int clr_oper(int);							//clr 		Clear the Screen.
	int jobs_oper(int);							//jobs 		Report a snapshot of the current processes.
	int dir_oper(vector<string>&,int);			//dir 		List all the files' name in the directory.	
	int shift_oper(vector<string>&,int);		//shift 	Shift positional parameters.
	int unset_oper(vector<string>&,int);		//unset 	Unset values of shell variables.
	void exit_oper();							//exit 		Exit the shell.					
	int myshell_oper(vector<string>&,int);		//myshell   Interpret and execute a shell script.
	int help_oper(vector<string>&,int);			//help 		Show the help list.

/*********** Handle Functions *************/
private:
	int redirect(vector<string>&, int);					//redirect the output to the object file
	bool operInput(vector<string>& ,string, bool);		//according to the inptut to call different functions
	bool GetVar(string);								//get the name and value of the variable from the input
	bool replaceInput(string&);							//replace the variable to string from the input
	bool replaceVar(string&,int);						//replace the variable from the string
	void lineOut();										//print the currunt path, user name and session at cmd
	int InnerInst_oper(vector<string>& ,string, int);	//function to oper the inner instractions of shell
	int ExterInst_oper(vector<string>& ,string, int);	//function to oper the external and the executable file
	void ShowOutput();									//output the result to the screen
	bool BackHandleProcess(vector<string>&,string,bool);//operate the instruction in the background
	int sorry_oper();									//the inner instruction which hasn't realized

public:
	MyShell();
	~MyShell();
	bool readInput();									//the inferface function of the class MyShell
};

#endif

