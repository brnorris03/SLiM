//
//  script_pathelement.cpp
//  SLiM
//
//  Created by Ben Haller on 5/1/15.
//  Copyright (c) 2015 Messer Lab, http://messerlab.org/software/. All rights reserved.
//

#include "script_pathelement.h"
#include "script_functions.h"
#include "script_functionsignature.h"
#include "slim_global.h"

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <fstream>


using std::string;
using std::vector;
using std::endl;


//
//	Script_PathElement
//
#pragma mark Script_PathElement

Script_PathElement::Script_PathElement(void) : base_path_("~")
{
}

Script_PathElement::Script_PathElement(const std::string &p_base_path) : base_path_(p_base_path)
{
}

const std::string *Script_PathElement::ElementType(void) const
{
	return &gStr_Path;
}

bool Script_PathElement::ExternallyOwned(void) const
{
	return false;
}

ScriptObjectElement *Script_PathElement::ScriptCopy(void)
{
	return new Script_PathElement(base_path_);
}

void Script_PathElement::ScriptDelete(void) const
{
	delete this;
}

std::vector<std::string> Script_PathElement::ReadOnlyMembers(void) const
{
	std::vector<std::string> members;
	
	return members;
}

std::vector<std::string> Script_PathElement::ReadWriteMembers(void) const
{
	std::vector<std::string> members;
	
	members.push_back(gStr_path);
	
	return members;
}

bool Script_PathElement::MemberIsReadOnly(GlobalStringID p_member_id) const
{
	if (p_member_id == gID_path)
		return false;
	else
		return ScriptObjectElement::MemberIsReadOnly(p_member_id);
}

ScriptValue *Script_PathElement::GetValueForMember(GlobalStringID p_member_id)
{
	if (p_member_id == gID_path)
		return new ScriptValue_String(base_path_);
	
	// all others, including gID_none
	else
		return ScriptObjectElement::GetValueForMember(p_member_id);
}

void Script_PathElement::SetValueForMember(GlobalStringID p_member_id, ScriptValue *p_value)
{
	if (p_member_id == gID_path)
	{
		ScriptValueType value_type = p_value->Type();
		int value_count = p_value->Count();
		
		if (value_type != ScriptValueType::kValueString)
			SLIM_TERMINATION << "ERROR (Script_PathElement::SetValueForMember): type mismatch in assignment to member 'path'." << slim_terminate();
		if (value_count != 1)
			SLIM_TERMINATION << "ERROR (Script_PathElement::SetValueForMember): value of size() == 1 expected in assignment to member 'path'." << slim_terminate();
		
		base_path_ = p_value->StringAtIndex(0);
		return;
	}
	
	// all others, including gID_none
	else
		return ScriptObjectElement::SetValueForMember(p_member_id, p_value);
}

std::vector<std::string> Script_PathElement::Methods(void) const
{
	std::vector<std::string> methods = ScriptObjectElement::Methods();
	
	methods.push_back(gStr_files);
	methods.push_back(gStr_readFile);
	methods.push_back(gStr_writeFile);
	
	return methods;
}

const FunctionSignature *Script_PathElement::SignatureForMethod(GlobalStringID p_method_id) const
{
	// Signatures are all preallocated, for speed
	static FunctionSignature *filesSig = nullptr;
	static FunctionSignature *readFileSig = nullptr;
	static FunctionSignature *writeFileSig = nullptr;
	
	if (!filesSig)
	{
		filesSig = (new FunctionSignature(gStr_files, FunctionIdentifier::kNoFunction, kScriptValueMaskString))->SetInstanceMethod();
		readFileSig = (new FunctionSignature(gStr_readFile, FunctionIdentifier::kNoFunction, kScriptValueMaskString))->SetInstanceMethod()->AddString_S();
		writeFileSig = (new FunctionSignature(gStr_writeFile, FunctionIdentifier::kNoFunction, kScriptValueMaskNULL))->SetInstanceMethod()->AddString_S()->AddString();
	}
	
	// All of our strings are in the global registry, so we can require a successful lookup
	switch (p_method_id)
	{
		case gID_files:
			return filesSig;
		case gID_readFile:
			return readFileSig;
		case gID_writeFile:
			return writeFileSig;
			
			// all others, including gID_none
		default:
			return ScriptObjectElement::SignatureForMethod(p_method_id);
	}
}

std::string Script_PathElement::ResolvedBasePath(void) const
{
	string path = base_path_;
	
	// if there is a leading '~', replace it with the user's home directory; not sure if this works on Windows...
	if ((path.length() > 0) && (path.at(0) == '~'))
	{
		const char *homedir;
		
		if ((homedir = getenv("HOME")) == NULL)
			homedir = getpwuid(getuid())->pw_dir;
		
		if (strlen(homedir))
			path.replace(0, 1, homedir);
	}
	
	return path;
}

ScriptValue *Script_PathElement::ExecuteMethod(GlobalStringID p_method_id, ScriptValue *const *const p_arguments, int p_argument_count, ScriptInterpreter &p_interpreter)
{
	// All of our strings are in the global registry, so we can require a successful lookup
	switch (p_method_id)
	{
		case gID_files:
		{
			string path = ResolvedBasePath();
			
			// this code modified from GNU: http://www.gnu.org/software/libc/manual/html_node/Simple-Directory-Lister.html#Simple-Directory-Lister
			// I'm not sure if it works on Windows... sigh...
			DIR *dp;
			struct dirent *ep;
			
			dp = opendir(path.c_str());
			
			if (dp != NULL)
			{
				ScriptValue_String *return_string = new ScriptValue_String();
				
				while ((ep = readdir(dp)))
					return_string->PushString(ep->d_name);
				
				(void)closedir(dp);
				
				return return_string;
			}
			else
			{
				// would be nice to emit an error message, but at present we don't have access to the stream...
				//p_output_stream << "Path " << path << " could not be opened." << endl;
				return gStaticScriptValueNULLInvisible;
			}
		}
		case gID_readFile:
		{
			// the first argument is the filename
			ScriptValue *arg0_value = p_arguments[0];
			int arg0_count = arg0_value->Count();
			
			if (arg0_count != 1)
				SLIM_TERMINATION << "ERROR (Script_PathElement::ExecuteMethod): method " << StringForGlobalStringID(p_method_id) << "() requires that its first argument's size() == 1." << slim_terminate();
			
			string filename = arg0_value->StringAtIndex(0);
			string file_path = ResolvedBasePath() + "/" + filename;
			
			// read the contents in
			std::ifstream file_stream(file_path.c_str());
			
			if (!file_stream.is_open())
			{
				// not a fatal error, just a warning log
				p_interpreter.ExecutionOutputStream() << "WARNING: File at path " << file_path << " could not be read." << endl;
				return gStaticScriptValueNULLInvisible;
			}
			
			ScriptValue_String *string_result = new ScriptValue_String();
			string line;
			
			while (getline(file_stream, line))
				string_result->PushString(line);
			
			if (file_stream.bad())
			{
				// not a fatal error, just a warning log
				p_interpreter.ExecutionOutputStream() << "WARNING: Stream errors occurred while reading file at path " << file_path << "." << endl;
			}
			
			return string_result;
		}
		case gID_writeFile:
		{
			// the first argument is the filename
			ScriptValue *arg0_value = p_arguments[0];
			int arg0_count = arg0_value->Count();
			
			if (arg0_count != 1)
				SLIM_TERMINATION << "ERROR (Script_PathElement::ExecuteMethod): method " << StringForGlobalStringID(p_method_id) << "() requires that its first argument's size() == 1." << slim_terminate();
			
			string filename = arg0_value->StringAtIndex(0);
			string file_path = ResolvedBasePath() + "/" + filename;
			
			// the second argument is the file contents to write
			ScriptValue *arg1_value = p_arguments[1];
			int arg1_count = arg1_value->Count();
			
			// write the contents out
			std::ofstream file_stream(file_path.c_str());
			
			if (!file_stream.is_open())
			{
				// Not a fatal error, just a warning log
				p_interpreter.ExecutionOutputStream() << "WARNING (Script_PathElement::ExecuteMethod): File at path " << file_path << " could not be opened." << endl;
				return gStaticScriptValueNULLInvisible;
			}
			
			for (int value_index = 0; value_index < arg1_count; ++value_index)
			{
				if (value_index > 0)
					file_stream << endl;
				
				file_stream << arg1_value->StringAtIndex(value_index);
			}
			
			if (file_stream.bad())
			{
				// Not a fatal error, just a warning log
				p_interpreter.ExecutionOutputStream() << "WARNING (Script_PathElement::ExecuteMethod): Stream errors occurred while reading file at path " << file_path << "." << endl;
			}
			
			return gStaticScriptValueNULLInvisible;
		}
			
			// all others, including gID_none
		default:
			return ScriptObjectElement::ExecuteMethod(p_method_id, p_arguments, p_argument_count, p_interpreter);
	}
}



































































