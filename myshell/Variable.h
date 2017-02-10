#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include <map>
#include <string>

using namespace std;

class Variable{
public:
	map<string,string> VarArry;

public:
	bool InsertVar(string key, string value);	//insert one variable to the VarArry
	bool DeleteVar(string);						//delete one variable in the VarArry
	bool SearchVar(string);						//search one variable in the VarArry
};

#endif