#include "Variable.h"
using namespace std;

bool Variable::InsertVar(string key,string value) 
{
	//if don't find, then add to the VarArry
	if(VarArry.find(key) == VarArry.end())
		VarArry.insert({key,value});
	//if find, then change the value
	else 
		VarArry[key] = value;
	return true;
}

bool Variable::DeleteVar(string key)
{
	//if find, then delete
	auto it=VarArry.find(key);
	if(it != VarArry.end())
	{
		VarArry.erase(it);
		return true;
	}
	else return false;
}

//if get, then return true
bool Variable::SearchVar(string key)
{
	return (VarArry.find(key) != VarArry.end());
}