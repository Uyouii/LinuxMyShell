#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <vector>
#include "Type.h"
#include "myshell.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <set>
#include "Tools.h"
#include "Variable.h"

using namespace std;

/*
/* The construct function of class MyShell.
/* The environment variables is assigned in this functions.
/* Some private variables is initialized.
/*/
MyShell::MyShell()
{
	//use map to bound the inner instruction with the number
	InnerInc={
		{"bg",1} , {"cd",2} , {"continue",3} , {"echo",4} , {"exec",5},
		{"exit",6} , {"fg",7} , {"clr",8} , {"pwd",9} , {"set",10},
		{"shift",11} , {"environ",12} , {"time",13} , {"umask",14} , {"unset",15},
		{"jobs",16} , {"dir",17}, {"myshell",18}, {"help",19}
	};

	Ispipe=false;
	UseMore = false;
	char buf[MAX_PATH_LENGTH];
	//get the currunt path
	getcwd(buf, sizeof(buf));

	SHELL = buf;
	//initialize the temporary file name which used in the following operations
	Tempfile = SHELL + "/" + TEMP_FILE;
	Tempfile2 = SHELL + "/" + TEMP_FILE2;
	//SHELL = the path of the program + "/myshell"
	SHELL = SHELL + "/myshell";

	//use getenv() to get the value of the environment variable value
	HOME = getenv("HOME");
	USER = getenv("USER");
	string PATH,TERM;
	SESSION = getenv("SESSION");
	PATH = getenv("PATH");
	TERM = getenv("TERM");
	
	//get into the HOME directory of user
	chdir((char*)HOME.c_str());
	CurPath=HOME;

	//initialize the value of the environment variable
	EnvirVar={
		{"HOME",HOME},{"USER",USER},{"SESSION",SESSION},{"SHELL",SHELL},{"PWD",CurPath},
		{"PATH",PATH},{"TERM",TERM}
	};
}

MyShell::~MyShell(){

}

/***********************************************************************************/
/**************************       Input Operations        **************************/
/***********************************************************************************/

/* print the currunt path, user name and session at cmd
/*/
void MyShell::lineOut()
{
	cout << "myshell> ";
	cout << USER << "@" << SESSION << ":";
	//if the $HOME is contained in the CurPath, change the $HOME to "~"
	if(CurPath.find(HOME) != string::npos)
	{
		string restpath = CurPath.substr(HOME.size());
		cout << "~" << restpath << "$ ";
	}
	else cout << CurPath << "$ ";
}

/* replace the variable in the input string to its value.
/* if has echo insruction operation in the input string,
/* then the variable in the echo shouldn't be replaced now 
/* because the value of the variable will be treated as instruction
/*/
bool MyShell::replaceInput(string& input)
{
	int where=0,begin;

	//if there is "echo" in the input string, then get to the end of the echo instruction
	if(input.find("echo") != string::npos)
	{
		where=input.find("echo");
		while( (input[where] != '|' ) && (input[where] != '>' ) && (where < input.size()) )
			where++;
	}
	//if don't get to the end of the input string, then replace the rest input
	if(where < input.size())
	{
		//call the replaceVar() to realize the replace operation
		return replaceVar(input,where);
	}
	return true;
}


/* if there is a '(' after the '$', then esacpe the '('
/* if there is a digit after the '$', then replace the variable with the value of positional variable
/* if this positional has no value, then replace the variable with ""
/* if there is @ after the '$', then replace the variable with the whole value of the positional variable
/* if there is # after the '$', then replace the variable with number of the positional variable
/* else found the variable name from the variable arry and environment variable arry,
/* if not find, then throw a error and return
/*/
bool MyShell::replaceVar(string& input,int where)
{
	int begin;
	while(input.find("$",where) != string::npos)
	{
		begin=input.find("$",where);
		//if is '(', then esacpe
		if(input[begin+1] == '(')
			where=begin+1;
		// if is digit, use the positonal variable
		// the positonal is restored in the deque of SetDeque
		else if(isdigit(input[begin+1]))
		{
			int n=input[begin+1]-'0';
			if(n != 0 && n <= setDeque.size())
				input.replace(begin,2,setDeque[n-1]);
			else
				input.replace(begin,2,"");
			where=begin;
		}
		//is is "@", then replace with all the positional varibale
		else if(input[begin+1] == '@')
		{
			string str="";
			for(int i=0;i<setDeque.size();i++)
				str=str + setDeque[i] + " ";
			if(str.size() > 0)
				str.pop_back();
			input.replace(begin,2,str);
			where=begin;
		}
		//if is "#", replace with the number of positional variable
		else if(input[begin+1] == '#')
		{
			int n=setDeque.size();
			stringstream ss;
			string str;
			ss << n;
			ss >> str;
			input.replace(begin,2,str);
			where=begin;
		}
		else 
		{
			int end = begin+1;
			while(isalnum(input[end]))
				end++;
			if(end == begin +1)
				return false;
			//first find the all the probable length of the variable
			int length=end-begin-1;
			bool yes=false;
			//for each length, to find the variable name in the arry of varibale
			for(int i=1;i<=length;i++)
			{
				string str=input.substr(begin+1,i);
				//in the environment variable arry
				if(EnvirVar.find(str) != EnvirVar.end())
				{
					yes=true;
					input.replace(begin,1+i,EnvirVar[str]);
					break;
				}
				//in the variable arry
				else if(Var.SearchVar(str))
				{
					yes = true;
					input.replace(begin,1+i,Var.VarArry[str]);
					break;
				}
			}
			//if don't find, then return false
			if(!yes)
				return false;
			where=begin;
		}
	}
	// this function don't handle the difference between the " and the ',
	// so replace the ' with the " 
	while(input.find("\'") != string::npos)
		input.replace(input.find("\'"),1,"\"");
	return true;
}

/* read each line from the screen
/* then split the input string to each word stored in the Str_arry
/*/
bool MyShell::readInput()
{
	lineOut();
	string input;
	getline(cin,input);

	if(input == "quit")
		exit(0);
	//if the input is empty, then read again
	else if(input.empty())
		return true;
	//replace the variable in the input
	if(!replaceInput(input))
	{
		//if input a wrong name of the variable, throw an error
		cout << "error: something wrong about $." << endl;
		return true;
	}

#ifdef DEBUG
	cout << "input: " << input << endl;
#endif

	vector<string> Str_arry;
	char *p;
	const char* spilit = " \t";
	char tem[MAX_INPUT_LENGTH];
	strcpy(tem,input.c_str());
	//use strtok() to spilit the input string and stored in the Str_arry
	p=strtok(tem,spilit);
	while(p != NULL)
	{
		Str_arry.push_back(p);
		p=strtok(NULL,spilit);
	}
	return operInput(Str_arry,input,false);
}

/**************************       Main Handle Function        **************************/
/* the output of each instruction will be stored in a temporary file .
/* then if has pipe instruction, the input will get from the temporary file.
/* use the '|' and '>' to spilit the Str_arry again.
/* then is get to the end of the input string, print the temporary file to the screen
/*/

bool MyShell::operInput(vector<string>& Str_arry,string input,bool inner)
{
	//if there is '&' at the end of the input, then call for the BackHandleProcess() to operate the instruction
	if(Str_arry[Str_arry.size()-1] == "&")
	{
		return BackHandleProcess(Str_arry,input,inner);
	}
	else
	{
		//if there is "=" in the input, this instruction is an assignment operation
		//call for the GetVar() to operate the instruction
		if(input.find("=",0) != string::npos)
		{
			if( !GetVar(input) )
				cout << "erro: fail to get the variable." << endl;
		}
		else
		{
			int i=0,N=Str_arry.size();
			string inst=Str_arry[0];
			Ispipe=false;
			while(i < N)
			{
				//the operation for the redirect
				if(Str_arry[i] == ">" || Str_arry[i] == ">>")
				{
					i = redirect(Str_arry,i);
				}
				//set the Ispipe=true, the next instruction's come from the temporary file
				else if(Str_arry[i] == "|")
				{
					Ispipe=true;
					i++;
					inst=Str_arry[i];
				}
				//inner instructions
				else if(InnerInc.find(inst) != InnerInc.end())
				{
					i=InnerInst_oper(Str_arry,input,i);
					if(i < 0) break;
				}
				//external instructions or the executable files
				else 
				{
					i = ExterInst_oper(Str_arry,input,i);
					if(i < 0) break;
				}

				if(i == N && !inner)
				{
					//wait for the child process to finish
					wait(NULL);
					if(BackProcess.size() > 0)
						BackProcess.pop_front();
					//print the output to the screen
					ShowOutput();
				}
				UseMore = false;
			}
		}
	}
	return (true);
}

/**************************       Background Handle Function        **************************/
/* if operate the instruction in the background, then call the operInput() function in the child process
/* when get a new child process, then put the process name and ID to the BackProcess
/* when the chlid process is done, then free the process
/*/
bool MyShell::BackHandleProcess(vector<string>& Str_arry,string input,bool inner)
{
	Str_arry.pop_back();
	while(input[input.size()-1] != '&')
		input.pop_back();
	input.pop_back();
	//get a child process
	int pid = fork();
	BackProcess.push_back(input);
	if(pid == 0)
	{
		//if at child process, then use the operInput to handle the input
		int i = 0;
		operInput(Str_arry,input,false);
		//get and print the child process ID
		while(i< BackProcess.size() && BackProcess[i] != input)
			i++;
		cout << "[" << i+1 << "]  " << "done" << "                    "  << input << endl;
		exit(0);
		return false;
	}
	else
	{
		int i = 0;
		while(i< BackProcess.size() && BackProcess[i] != input)
			i++;
		cout << "[" << (i+1) << "]  " << pid << endl;
		//the parent return to handle a new input
		return true;
	}
}

/**************************       Output Function       **************************/
/* print the output from the temporary file to the screen
/* if chooe the "more" operation, then use the exec to use "more" instruction open the temporary
/* else cout the temporary to the screen directly
/*/
void MyShell::ShowOutput()
{
	//if could find the temporary tile Tempfile
	if(access(Tempfile.c_str(),F_OK) != -1)
	{
		//don't use more instructions
		if(!UseMore)
		{
			string buf;
			ifstream in(Tempfile);
			if (! in.is_open())  
   			{ 
   				cout << "Error opening file" << endl; 
   				return;
   			}  
   			buf = string(( std::istreambuf_iterator<char>(in)),  std::istreambuf_iterator<char>() );  

   			if(buf[buf.size()-1] == '\n')
   				buf.pop_back();
   			//output the result
			cout << buf << endl;
			in.close();
		}
		//use more instructions
		else
		{
			int result = fork();
			if(result == 0)
			{
				execlp("more","more","-12",Tempfile.c_str(),NULL);
				exit(0);
			}
			else wait(NULL);
		}
		//delete the temporary file 
		remove(Tempfile.c_str());
	}
#ifdef DEBUG
	else 
		cout << "error: Cant't find output file!" << endl;
#endif
}

/**************************      Inner Instruction Operations       **************************/
/*allocate the input string to specific inner instruction function*/

int MyShell::InnerInst_oper(vector<string>& Str_arry,string input, int num)
{
	switch(InnerInc[Str_arry[num]])
	{
		case 1: 	num = sorry_oper();					break;	//bg    	Move jobs to the background.	
		case 2: 	num = cd_oper(Str_arry , num); 		break;	//cd 		Change the shell working directory.
		case 3: 	num = sorry_oper();					break;	//continue  Resume for, while, or until loops.
		case 4: 	num = echo_oper(Str_arry,input,num);break;	//echo 	 	Write arguments to the standard output.	
		case 5: 	num = sorry_oper();					break;	//exec 		Replace the shell with the given command.
		case 6: 	exit_oper();						break;	//exit 		Exit the shell.
		case 7: 	num = sorry_oper();					break;	//fg 		Move job to the foreground.	
		case 8: 	num = clr_oper(num);				break;	//clr 	 	Clear the Screen.	
		case 9:  	num = pwd_oper(Str_arry , num);		break;	//pwd 	 	Print the name of the current working directory.	
		case 10: 	num = set_oper(Str_arry,input); 	break;	//set 		Set values of shell options and positional parameters.	
		case 11: 	num = shift_oper(Str_arry,num);		break;	//shift 	Shift positional parameters.
		case 12: 	num = environ_oper(Str_arry,num);	break;	//environ 	List all the environmental variables.
		case 13: 	num = time_oper(Str_arry,num);		break;	//time 		Print the system date and time.
		case 14: 	num = sorry_oper();					break;	//umask 	Display or set file mode mask.
		case 15: 	num = unset_oper(Str_arry,num);		break;	//unset 	Unset values of shell variables.
		case 16:	num = jobs_oper(num);				break;	//jobs 		Report a snapshot of the current processes.
		case 17:	num = dir_oper(Str_arry,num);		break;	//dir 		List all the files' name in the directory.
		case 18: 	num = myshell_oper(Str_arry,num);	break;	//myshell  	Interpret and execute a shell script.
		case 19:	num = help_oper(Str_arry,num);		break;	//help 		Show the help list.	
	}
	return num;
}

/**************************      External Instruction Operations       **************************/
/* handle the external instructions and the execuable file
/* use the fork and exec function to operate these instructions
/* if the input is from the pipe, then read the input from the temporary file
/* put the output to the temporary file
/*/
int MyShell::ExterInst_oper(vector<string>& Str_arry,string input,int num)
{
	int N = Str_arry.size();
	int end = N;
	//get the range of the instruction in the input string
	for(int i=num;i<N;i++)
		if(Str_arry[i] == "|" || Str_arry[i] == ">" || Str_arry[i] == ">>")
		{
			end = i;
			break;
		}
	string inst;

	//if input is not from the pipe(temporary file)
	if(!Ispipe)
	{
		//put the instructions form the Str_arry to the argc
		char * argc[MAX_INST_NUMBER];
		for(int i=num;i<end;i++)
		{
			argc[i-num] = (char*)Str_arry[i].c_str();
		}
		argc[end-num]=NULL;
		//set up a new child process
		int result=fork();
		if(result == 0)
		{
			//call the execvp in the child process
			//set the stdout to the Tempfile
			FILE *fd = freopen(Tempfile.c_str(), "w", stdout);
			execvp(argc[0],argc);
			fclose(stdout);
			exit(0);
		}
		// the parent process wait until the child process finish
		else wait(NULL);
	}
	//if the input is form the pipe
	else
	{
		char * argc[MAX_INST_NUMBER];
		for(int i=num;i<end;i++)
		{
			argc[i-num] = (char*)Str_arry[i].c_str();
		}
		argc[end-num]=(char*)Tempfile2.c_str();

		//copy the temporary Tempfile to the Tempfile2
		//set the Tempfil2 as the input file
		argc[end+1-num]=NULL;
		ofstream out(Tempfile2);
		ifstream in(Tempfile);
		string buf(( std::istreambuf_iterator<char>(in)),  std::istreambuf_iterator<char>() );
		out << buf;  
		in.close();
		out.close();

		int result=fork();
		if(result == 0)
		{
			//set the stdout to the Tempfile 
			FILE *fd = freopen(Tempfile.c_str(), "w", stdout);
			execvp(argc[0],argc);
			fclose(stdout);
			exit(0);
		}
		else wait(NULL);
		//delete the tempporary file
		remove(Tempfile2.c_str());
	}

	return end;
}

/**************************      Output Redirect Function       **************************/
/* redirect the output to the object file
/* because the output is in the temporary file
/* so just copy the contains from the temporary file to the object file
/*/
int MyShell::redirect(vector<string>& Str_arry, int num)
{
	//get the symbol to judge is '>' or '>>'
	string symbol = Str_arry[num];
	num++;
	string filename=Str_arry[num];
	num++;

	//if found the ~ in the filename, then replace the "~" with the $HOME
	int w = filename.find("~");
	if(w != string::npos)
	{
		filename.replace(w,1,HOME);
	}

	//if the file didn't exist, then create one
	if(access((char*)filename.c_str(),F_OK) == -1)
	{
		ofstream out(filename);
	}

	//get the contains form the temporary file
	ifstream in(Tempfile);
	if (! in.is_open())  
	{ 
		//if the temporary file doesnt't exist, then throw a error
		cout << "Error opening file" << endl; 
		return -1;
	}  
	//get the contains from the file to the string
	std::string buf(( std::istreambuf_iterator<char>(in)),  std::istreambuf_iterator<char>() );  
	if(buf[buf.size()-1] != '\n')
		buf+="\n";

	//if sysmol is '>', then cover
	if(symbol == ">")
	{
		ofstream out(filename);
		out << buf;
	}
	//if symbol is '>>', then add to the end.
	else if(symbol == ">>")
	{
		ofstream out( filename,ios::app); 
		out << buf;
	}
	remove(Tempfile.c_str());
	return num;
}

/**************************     Variable Handle Function       **************************/
/* get the variable name and value form the input string
/* support the variable value from the instructions, like a=$(ls | grep XXX)
/* if use the instructions to assign the variable, then use the operInput() to hadle the instruction
/* could distinguish the "" and '' in the input string
/*/
bool MyShell::GetVar(string input)
{
	int equal=input.find("=");
	if( input[equal-1] == ' ')
	{
		cout << "error: syntax error!" << endl;
		return false;
	}
	int begin = 0;
	//get the variable name
	while(input[begin] == ' ' || input[begin] == '\t')
		begin++;
	string var=input.substr(begin,equal-begin);

	//put the point to the next position
	begin=equal+1;
	while(input[begin] == ' ' || input[begin] == '\t')
		begin++;

	//if use the instructions to assign the variable
	if(input.find("$") != string::npos)
	{
		int begin1=input.find_last_of("(");
		int end1=input.find(")");
		//get the inst from the input
		string inst=input.substr(begin1+1,end1 - begin1-1);

		vector<string> Str_arry;
		char *p1;
		const char* spilit = " \t\n";
		char tem[MAX_INPUT_LENGTH];
		strcpy(tem,inst.c_str());
		p1=strtok(tem,spilit);
		while(p1 != NULL)
		{
			Str_arry.push_back(p1);
			p1=strtok(NULL,spilit);
		}
		//use the operInput to get the output to the temporary file
		operInput(Str_arry,inst,true);
		
		ifstream in(Tempfile);
		if (! in.is_open())  
   		{ 
       		cout << "error: fail to open temporary file" << endl; 
       		return -1;
    	}  
    	//get the contains in the file to the string buf
    	std::string buf(( std::istreambuf_iterator<char>(in)),  std::istreambuf_iterator<char>() );  
    	in.close();
    	remove(Tempfile.c_str());
    	//replace the "\n" in the string with the space
    	int xx=buf.find("\n");
    	while(xx != string::npos)
    	{
    		buf.replace(xx,1," ");
    		xx=buf.find("\n");
    	}
    	//insert a variable to the variable arry
    	return Var.InsertVar(var,buf);
	}
	else
	{
		//if there is "" in the input string
		//then put the strings between the "" all to the variable
		if(input.find("\"") != string::npos)
		{
			begin=input.find("\"");
			begin++;
			int end=input.find("\"",begin);
			string value=input.substr(begin,end-begin);
			return Var.InsertVar(var,value);
		}
		//read the first string to the variable
		else
		{
			int end = begin;
			while(input[end] != ' ' && end < input.size())
				end++;
			string value=input.substr(begin,end-begin);
			return Var.InsertVar(var,value);
		}
	}
}

/***********************************************************************************/
/***************************    Inner Instructions     *****************************/
/***********************************************************************************/

/* Set values of shell options and positional parameters.
/* the positional variable can be assigned with instructions, like: set $(ls | grep XXX)
/* set also support the \" and the \' in the string, like: set "hello world" 'linux system'
/*/
int MyShell::set_oper(vector<string>& Str_arry , string input)
{
	//clear the deque which store the positional parameters
	setDeque.clear();
	//if use instructions to assign the positional parameters
	if(input.find("$") != string::npos)
	{
		//get the inst form the input string
		int begin=input.find_last_of("(");
		int end=input.find(")");
		string inst=input.substr(begin+1,end - begin-1);
		
		//spilit the inst to the Str_arry
		vector<string> Str_arry;
		char *p1;
		const char* spilit = " \t\n";
		char tem[MAX_INPUT_LENGTH];
		strcpy(tem,inst.c_str());
		p1=strtok(tem,spilit);
		while(p1 != NULL)
		{
			Str_arry.push_back(p1);
			p1=strtok(NULL,spilit);
		}

		//call the operInput to operate this instructions
		//stored the output in the temporary file
		operInput(Str_arry,inst,true);

		ifstream in(Tempfile);
		if (! in.is_open())  
	    { 
	       	cout << "error: fail to open temporary file" << endl; 
	       	return -1;
	    }  
	    //get the contains from the file to the string buf
	    std::string buf(( std::istreambuf_iterator<char>(in)),  std::istreambuf_iterator<char>() );  
	    in.close();
	    //delete the temporary file
	    remove(Tempfile.c_str());

	    //spilit the strings in the buf and put them to the setDeque
		char * p=strtok((char*)buf.c_str(),spilit);
		while(p != NULL)
		{
			setDeque.push_back(p);
			p=strtok(NULL,spilit);
		}
	}
	//use strings the assign the positional parameters 
	else
	{
		string str;
		int N = Str_arry.size();
		if( N <= 1)
			return -1;
		int begin=0,end;
		begin = input.find("set");
		begin += 3;
		while(input[begin] == ' ' || input[begin] == '\t')
			begin++;
		//get the begin point of the value string
		while(begin < input.size())
		{
			end = begin + 1;
			//if has \" in the input string, then put the contains in the "" to one positional parameters
			if(input[begin] == '\"')
			{
				while(end < input.size() && input[end] != '\"')
					end++;
				if(end >= input.size())
				{
					cout << "error: wrong syntax about the \" ." << endl;
					return -1; 
				}
				str = input.substr(begin+1,end - begin - 1);
				setDeque.push_back(str);
			}
			//if has \" in the input string, then put the contains in the '' to one positional parameters
			else if(input[begin] == '\'')
			{
				while(end < input.size() && input[end] != '\'')
					end++;
				if(end >= input.size())
				{
					cout << "error: wrong syntax about the \' ." << endl;
					return -1; 
				}
				str = input.substr(begin+1,end - begin - 1);
				setDeque.push_back(str);
			}
			//put each word to the positional parameters
			else
			{
				while(end < input.size() && input[end] != ' ' && input[end] != '\t')
					end++;
				str = input.substr(begin,end - begin);
				setDeque.push_back(str);
			}
			begin = end + 1;
			//skip the space characters 
			while(begin < input.size() && (input[begin] == ' ' || input[begin] == '\t'))
				begin++;
		}
	}
	return -1;
}

/* Write arguments to the standard output.
/* echo operations support the variable, ""  and ''
/*/
int MyShell::echo_oper(vector<string>& Str_arry, string input,int num)
{
	int N = Str_arry.size();
	int end = N;
	//get the instruction range in the input string
	for(int i=num;i<N;i++)
		if(Str_arry[i] == "|" || Str_arry[i] == ">" || Str_arry[i] == ">>")
		{
			end = i;
			break;
		}

	//get a new inst from the Str_arry
	string inst;
	for(int i=num+1;i<end;i++)
	{
		inst += Str_arry[i];
		inst += " ";
	}
	//remove the last space at the end of inst
	if(inst.size() > 0)
		inst.pop_back();

	//replace the variable in the input
	//if can't find the variable used in the input, then throw a error
	if(!replaceVar(inst,0))
	{
		cout << "error: fail to find variable." << endl;
		return end;
	}
	//delete all the " before output the result
	while(inst.find("\"") != string::npos)
		inst.replace(inst.find("\""),1,"");

	//put the output to the temporary file for the pipe operations
	ofstream out(Tempfile);
	out << inst;
	out.close();
	return end;
}

/* Clear the Screen.
/*/
int MyShell::clr_oper(int num)
{
	num++;
	printf("\033c");
	return num;
}

/* Report a snapshot of the current processes.
/* use the ps to compilshed this job
/*/
int MyShell::jobs_oper(int num)
{
	ofstream out(Tempfile);
	num++;
	int result=fork();

	if(result == 0)
	{
		//set the stdout to the temporary file
		FILE *fd = freopen(Tempfile.c_str(), "w", stdout);
		//use the "ps" instructions to get the result
		execlp("ps","ps","-c",NULL);
		fclose(stdout);
		exit(0);
	}
	else
		wait(NULL);
	return num;
}

/* List all the files' name in the target directory.
/* if don't input the directory, then list the currunt directory
/*/
int MyShell::dir_oper(vector<string>& Str_arry,int num)
{
	ofstream out(Tempfile);
	out.close();
	num++;
	int N=Str_arry.size();
	//if don't input the directory, then list the currunt directory
	if(N == 1)
	{
		int result = fork();
		if(result == 0)
		{
			//set the stdout to the tomporary file
			FILE *fd = freopen(Tempfile.c_str(), "w", stdout);
			//call the ls to list the files
			execlp("ls","ls",NULL);
			fclose(stdout);
			exit(0);
		}
		else wait(NULL);

	}
	//list the target directory
	else
	{
		string path=Str_arry[num];
		num++;
		char file_buf[MAX_PATH_LENGTH];
		//get the absolute path of the directory
		realpath(path.c_str(), file_buf);
		path=file_buf;		
		
		struct stat buf;
		//judge whether the file is existed
		if ( lstat((char *)path.c_str(), &buf) == -1 )
		{
       	 	cout << "error: file doesn't exist." << endl;
       	 	return -1;
    	}
    	//to judge whether the target file is a directory
    	if((buf.st_mode & S_IFMT) != S_IFDIR )
    	{
    		cout << "error: target path is not a directory!" << endl;
    		return -1;
    	}
    	else
    	{
    		int result = fork();
			if(result == 0)
			{
				//set the output to the temporary file
				FILE *fd = freopen(Tempfile.c_str(), "w", stdout);
				//use the ls instruction to list the target directory
				execlp("ls","ls",path.c_str(),NULL);
				fclose(stdout);
				exit(0);
			}
			else wait(NULL);
    	}
	}
	return num;
}

/* Shift positional parameters.
/* if don't input the parameter, shitf 1 default
/*/
int MyShell::shift_oper(vector<string>& Str_arry,int num)
{
	num++;
	int shift=1;
	int result = Str_arry.size();
	//get the number input
	if(result > 1)
	{
		string number = Str_arry[num];
		int n = atoi(number.c_str());
		shift = n;
		num++;
	}

	//shift the positional parameters in the setDeque
	int N = setDeque.size();
	if( shift >= N)
		setDeque.clear();
	else
	{
		for(int i=0;i<shift;i++)
		{
			//delete the first element in the setDeque
			setDeque.pop_front();
		}
	}
	return num;
}

/* Change the shell working directory.
/* if don't input the directory, then cd the $HOME default
/*/
int MyShell::cd_oper(vector<string>& Str_arry,int num)
{
	num++;
	int N=Str_arry.size();
	if(N == 1)
	{
		//get to the $HOME directory
		chdir((char*)HOME.c_str());
		//change the currunt path and the $PWD
		CurPath=HOME;
		EnvirVar["PWD"] = CurPath;
		return num;
	}
	else
	{
		string path=Str_arry[num];
		char file_buf[MAX_PATH_LENGTH];
		//get the absolute path of the directory
		realpath(path.c_str(), file_buf);
		path=file_buf;						
		int w = path.find("~");
		if(w != string::npos)
		{
			path.replace(w,1,HOME);
		}
		num++;
		//judge whether the file is existed
		struct stat buf;
		if ( lstat((char *)path.c_str(), &buf) == -1 )
		{
       	 	cout << "error: file doesn't exist." << endl;
       	 	return -1;
    	}
    	//to judge whether the target file is a directory
    	if((buf.st_mode & S_IFMT) != S_IFDIR )
    	{
    		cout << "error: target path is not a directory!" << endl;
    		return -1;
    	}
    	else
    	{
    		//get to the path input
    		chdir((char*)path.c_str());
    		char buf[MAX_PATH_LENGTH];
			getcwd(buf, sizeof(buf));
			//changet the CurPath and $PWD
			CurPath = buf;
			EnvirVar["PWD"] = CurPath;
    		return num;
    	}
	}

	return num;
}

/* Print the name of the current working directory.
/* put the output to the tomporary file
/*/
int MyShell::pwd_oper(vector<string>& Str_arry, int num)
{
	ofstream out(Tempfile);
	num++;
	char buf[MAX_PATH_LENGTH];
    getcwd(buf, sizeof(buf));
    out << buf;
    out.close();
    return num;
}

/* List all the environmental variables.
/* put the output to the temporary files.
/*/
int MyShell::environ_oper(vector<string>& Str_arry, int num)
{
	ofstream out(Tempfile);
	num++;
	string buf;
	int N = EnvirVar.size();
	auto it = EnvirVar.begin();
	//get the environmental variables from the EnvirVar
	for(;it != EnvirVar.end();it++)
		buf += it->first + ": " + it->second + "\n";
	buf.pop_back();
	out << buf;
	out.close();

	return num;
}

/* Print the system date and time.
/* put the output to the temporary files.
/*/
int MyShell::time_oper(vector<string>& Str_arry,int num)
{
	ofstream out(Tempfile);
	out.close();
	num++;
	int result=fork();

	if(result == 0)
	{
		//put the stdout to the temporary files
		FILE *fd = freopen(Tempfile.c_str(), "w", stdout);
		//use "date" instruction to get the output
		execlp("date","date",NULL);
		fclose(stdout);
		exit(0);
	}
	else
		wait(NULL);
	return num;
}

/* Unset values of shell variables.
/*/
int MyShell::unset_oper(vector<string>& Str_arry,int num)
{
	int N = Str_arry.size();
	if(N == 1)
		return -1;
	for(int i=1;i<N;i++)
	{
		string str = Str_arry[i];
		//if find the variable in the Var, then delete it
		if(Var.SearchVar(str))
			Var.DeleteVar(str);
	}
	return -1;
}

/* Exit the shell.
/*/
void MyShell::exit_oper()
{
	cout << "Bye Bye !" << endl;
	exit(0);
}

/*use for some functions still dont't compilshed*/
int MyShell::sorry_oper()
{
	cout << "Sorry, professor... I have't compilshed this function. QAQ " << endl;
	return -1;
}

/* Interpret and execute a shell script.
/* each time get oneline from the shell script
/* before execute the instruction, the instruction will be print first
/* if don't input target file, then there will be a tip
/*/
int MyShell::myshell_oper(vector<string>& Str_arry,int num)
{
	num ++;
	string path,str;
	if(Str_arry.size() == 1)
	{
		cout << "> The File Name: ";
		cin >> path;
		getline(cin,str);
	}
	else
		path = Str_arry[num];

	char file_buf[MAX_PATH_LENGTH];
	//get the absolute path of the directory
	realpath(path.c_str(), file_buf);
	path=file_buf;					
	struct stat buf;
	//judge whether the file is existed
	if ( lstat((char *)path.c_str(), &buf) == -1 )
	{
   	 	cout << "error: file doesn't exist." << endl;
   	 	return -1;
	}
	//to judge whether the target file is a directory
	if((buf.st_mode & S_IFMT) == S_IFDIR )
	{
		cout << "error: target path is a directory!" << endl;
		return -1;
	}
	//to judge whether the target file is a regular file
    else if((buf.st_mode & S_IFMT) == S_IFREG)
    {
    	ifstream in(path.c_str());
    	string line;
    	while(getline(in,line))
    	{
    		//delete the space before each line in the file
			while(line.size() > 0 && line[0] == ' ')
				line.replace(0,1,"");
			if(line.size() > 0)
			{
				cout << " >> \"";
				cout << line << "\"" << endl;
				//spilit the inst
				vector<string> Str_arry;
				char *p;
				const char* spilit = " \t";
				char tem[MAX_INPUT_LENGTH];
				strcpy(tem,line.c_str());
				p=strtok(tem,spilit);
				while(p != NULL)
				{
					Str_arry.push_back(p);
					p=strtok(NULL,spilit);
				}
				if(Str_arry[0] == "exit" || Str_arry[0] == "quit")
				{
					cout << "Bye Bye!" << endl;
					exit(0);
				}
				//use the operinput to get the output in the temporary file
				operInput(Str_arry,line,false);
			}
    	}
    }

	return -1;
}

/* Show this help list
/* help can has a target instruction after it.
/* if help list has this instruction, then the list will be output
/* if not, another tip will be printed.
/* use "more" instructions to print the list.
/*/
int MyShell::help_oper(vector<string>& Str_arry,int num)
{
	//store the help infro list in the string
	string helpinfo;
	helpinfo = "myshell(DTY's), version 0.0.1\n";
	helpinfo+= "These shell commands are defined internally.  Type `help' to see this list.\n";
	helpinfo+=	"Use `info bash' to find out more about the shell in general.\n";
	helpinfo+=	"Use `man -k' or `info' to find out more about commands not in this list.\n";
	helpinfo+=	"\n";
	helpinfo+=	"A star (*) next to a name means that the command is disabled.\n";
	helpinfo+=	"\n";
	helpinfo+=	"               COMMANDS                            INTRODUCTIONS\n";
	helpinfo+=	"bg*:       bg [job_spec ...]               Move jobs to the background.\n";
	helpinfo+=	"cd:        cd [dir]                        Change the shell working directory.\n";
	helpinfo+=	"clr:       clr                             Clear the Screen.\n";
	helpinfo+=	"continue*: continue [n]                    Resume for, while, or until loops.\n";
	helpinfo+=	"echo:      echo [arg ...]                  Write arguments to the standard output.\n";
	helpinfo+=	"dir:       dir [dir]                       List all the files' name in the target directory.\n";
	helpinfo+=	"environ:   environ                         List all the environmental variables.\n";
	helpinfo+=	"exec*:     exec [command [arguments ...]   Replace the shell with the given command.\n";
	helpinfo+=	"exit/quit: exit/quit                       Exit the shell.\n";
	helpinfo+=	"fg*:       fg [job_spec]                   Move job to the foreground.\n";
	helpinfo+=	"help:      help                            Show this help list.\n";
	helpinfo+=	"jobs:      jobs                            Report a snapshot of the current processes.\n";
	helpinfo+=	"myshell:   myshell [filename]              Interpret and execute a shell script.\n";
	helpinfo+=	"pwd:       pwd                             Print the name of the current working directory.\n";
	helpinfo+=	"set:       set [-$] [command or arg ...]   Set values of shell options and positional parameters.\n";
	helpinfo+=	"shift:     shift [n]                       Shift positional parameters.\n";
	helpinfo+=	"time:      time                            Print the system date and time.\n";
	helpinfo+=	"umask*:    umask [-p] [-S] [mode]          Display or set file mode mask.\n";
	helpinfo+=	"unset:     unset [name]                    Unset values of shell variables.";

	string nothelp;

	//get the range of the inst
	int N = Str_arry.size();
	int end = N;
	for(int i=num;i<N;i++)
		if(Str_arry[i] == "|" || Str_arry[i] == ">" || Str_arry[i] == ">>")
		{
			end = i;
			break;
		}
	set<string> innerinst = {"bg","cd","clr","continue","echo","dir","environ","exec","exit","fg","help","myshell",
							 "pwd","set","shift","time","umask","unset","jobs"};
	string inst;

	//if has parameters
	if(end > num + 1)
	{
		inst = Str_arry[num+1];
		//if found the instruction in the innerinst list
		//then put the help list to the temporary file
		if(innerinst.find(inst) != innerinst.end())
		{
			UseMore= true;
			ofstream out(Tempfile);
			out << helpinfo;
		}
		//print the error tips to the temporary file
		else
		{
			nothelp = "myshell: help: no help topics match \'" + inst +"\'. Try \'help help \' or \'man -k " + inst + "\' or \'info " + inst + "\'.";
			ofstream out(Tempfile);
			out << nothelp;
		}
	}
	//if don't have parameters
	else
	{
		UseMore = true;
		ofstream out(Tempfile);
		out << helpinfo;
	}
	return end;
}
