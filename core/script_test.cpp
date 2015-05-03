//
//  script_test.cpp
//  SLiM
//
//  Created by Ben Haller on 4/7/15.
//  Copyright (c) 2015 Philipp Messer.  All rights reserved.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of SLiM.
//
//	SLiM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	SLiM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with SLiM.  If not, see <http://www.gnu.org/licenses/>.


#include "script_test.h"
#include "script.h"
#include "script_value.h"
#include "script_interpreter.h"
#include "slim_global.h"

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>


using std::string;
using std::vector;
using std::map;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::istream;
using std::ostream;


// Helper functions for testing
void AssertScriptSuccess(string p_script_string, ScriptValue *p_correct_result);
void AssertScriptRaise(string p_script_string);


// Instantiates and runs the script, and prints an error if the result does not match expectations
void AssertScriptSuccess(string p_script_string, ScriptValue *p_correct_result)
{
	Script script(1, 1, p_script_string, 0);
	ScriptValue *result = nullptr;
	
	try {
		script.Tokenize();
	}
	catch (std::runtime_error err)
	{
		std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : raise during Tokenize(): " << GetTrimmedRaiseMessage() << endl;
		return;
	}
	
	try {
		script.ParseInterpreterBlockToAST();
	}
	catch (std::runtime_error err)
	{
		std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : raise during ParseToAST(): " << GetTrimmedRaiseMessage() << endl;
		return;
	}
	
	try {
		ScriptInterpreter interpreter(script);
		
		// note InjectIntoInterpreter() is not called here; we want a pristine environment to test the language itself
		
		result = interpreter.EvaluateInterpreterBlock();
		
		// We have to copy the result; it lives in the interpreter's symbol table, which will be deleted when we leave this local scope
		result = result->CopyValues();
	}
	catch (std::runtime_error err)
	{
		std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : raise during EvaluateInterpreterBlock(): " << GetTrimmedRaiseMessage() << endl;
		return;
	}
	
	// Check that the result is actually what we want it to be
	if (result == nullptr)
	{
		std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : return value is nullptr" << endl;
		return;
	}
	if (result->Type() != p_correct_result->Type())
	{
		std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : unexpected return type (" << result->Type() << ", expected " << p_correct_result->Type() << ")" << endl;
		return;
	}
	if (result->Count() != p_correct_result->Count())
	{
		std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : unexpected return length (" << result->Count() << ", expected " << p_correct_result->Count() << ")" << endl;
		return;
	}
	
	for (int value_index = 0; value_index < result->Count(); ++value_index)
	{
		if (CompareScriptValues(result, value_index, p_correct_result, value_index) != 0)
		{
			std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : mismatched values (" << *result << "), expected (" << *p_correct_result << ")" << endl;
			return;
		}
	}
	
	std::cerr << p_script_string << " == " << p_correct_result->Type() << "(" << *p_correct_result << ") : \e[32mSUCCESS\e[0m" << endl;
}

// Instantiates and runs the script, and prints an error if the script does not cause an exception to be raised
void AssertScriptRaise(string p_script_string)
{
	Script script(1, 1, p_script_string, 0);
	
	try {
		script.Tokenize();
		script.ParseInterpreterBlockToAST();
		
		ScriptInterpreter interpreter(script);
		
		// note InjectIntoInterpreter() is not called here; we want a pristine environment to test the language itself
		
		interpreter.EvaluateInterpreterBlock();
		
		std::cerr << p_script_string << " : \e[31mFAILURE\e[0m : no raise during EvaluateInterpreterBlock()." << endl;
	}
	catch (std::runtime_error err)
	{
		std::cerr << p_script_string << " == (expected raise) " << GetTrimmedRaiseMessage() << " : \e[32mSUCCESS\e[0m" << endl;
		return;
	}
}

void RunSLiMScriptTests(void)
{
	// test literals, built-in identifiers, and tokenization
	AssertScriptSuccess("3;", new ScriptValue_Int(3));
	AssertScriptSuccess("3e2;", new ScriptValue_Int(300));
	AssertScriptSuccess("3.1;", new ScriptValue_Float(3.1));
	AssertScriptSuccess("3.1e2;", new ScriptValue_Float(3.1e2));
	AssertScriptSuccess("3.1e-2;", new ScriptValue_Float(3.1e-2));
	AssertScriptSuccess("\"foo\";", new ScriptValue_String("foo"));
	AssertScriptSuccess("\"foo\\tbar\";", new ScriptValue_String("foo\tbar"));
	AssertScriptSuccess("T;", new ScriptValue_Logical(true));
	AssertScriptSuccess("F;", new ScriptValue_Logical(false));
	AssertScriptRaise("$foo;");
	
	// test vector-to-singleton comparisons for integers
	AssertScriptSuccess("rep(1:3, 2) == 2;", new ScriptValue_Logical(false, true, false, false, true, false));
	AssertScriptSuccess("rep(1:3, 2) != 2;", new ScriptValue_Logical(true, false, true, true, false, true));
	AssertScriptSuccess("rep(1:3, 2) < 2;", new ScriptValue_Logical(true, false, false, true, false, false));
	AssertScriptSuccess("rep(1:3, 2) <= 2;", new ScriptValue_Logical(true, true, false, true, true, false));
	AssertScriptSuccess("rep(1:3, 2) > 2;", new ScriptValue_Logical(false, false, true, false, false, true));
	AssertScriptSuccess("rep(1:3, 2) >= 2;", new ScriptValue_Logical(false, true, true, false, true, true));
	
	AssertScriptSuccess("2 == rep(1:3, 2);", new ScriptValue_Logical(false, true, false, false, true, false));
	AssertScriptSuccess("2 != rep(1:3, 2);", new ScriptValue_Logical(true, false, true, true, false, true));
	AssertScriptSuccess("2 > rep(1:3, 2);", new ScriptValue_Logical(true, false, false, true, false, false));
	AssertScriptSuccess("2 >= rep(1:3, 2);", new ScriptValue_Logical(true, true, false, true, true, false));
	AssertScriptSuccess("2 < rep(1:3, 2);", new ScriptValue_Logical(false, false, true, false, false, true));
	AssertScriptSuccess("2 <= rep(1:3, 2);", new ScriptValue_Logical(false, true, true, false, true, true));
	
	// tests for the + operator
	AssertScriptSuccess("1+1;", new ScriptValue_Int(2));
	AssertScriptSuccess("1+-1;", new ScriptValue_Int(0));
	AssertScriptSuccess("(0:2)+10;", new ScriptValue_Int(10, 11, 12));
	AssertScriptSuccess("10+(0:2);", new ScriptValue_Int(10, 11, 12));
	AssertScriptSuccess("(15:13)+(0:2);", new ScriptValue_Int(15, 15, 15));
	AssertScriptRaise("(15:12)+(0:2);");
	AssertScriptRaise("NULL+(0:2);");		// FIXME should this be an error?
	AssertScriptSuccess("1+1.0;", new ScriptValue_Float(2));
	AssertScriptSuccess("1.0+1;", new ScriptValue_Float(2));
	AssertScriptSuccess("1.0+-1.0;", new ScriptValue_Float(0));
	AssertScriptSuccess("(0:2.0)+10;", new ScriptValue_Float(10, 11, 12));
	AssertScriptSuccess("10.0+(0:2);", new ScriptValue_Float(10, 11, 12));
	AssertScriptSuccess("(15.0:13)+(0:2.0);", new ScriptValue_Float(15, 15, 15));
	AssertScriptRaise("(15:12.0)+(0:2);");
	AssertScriptRaise("NULL+(0:2.0);");		// FIXME should this be an error?
	AssertScriptSuccess("\"foo\"+5;", new ScriptValue_String("foo5"));
	AssertScriptSuccess("\"foo\"+5.0;", new ScriptValue_String("foo5"));
	AssertScriptSuccess("\"foo\"+5.1;", new ScriptValue_String("foo5.1"));
	AssertScriptSuccess("5+\"foo\";", new ScriptValue_String("5foo"));
	AssertScriptSuccess("5.0+\"foo\";", new ScriptValue_String("5foo"));
	AssertScriptSuccess("5.1+\"foo\";", new ScriptValue_String("5.1foo"));
	AssertScriptSuccess("\"foo\"+1:3;", new ScriptValue_String("foo1", "foo2", "foo3"));
	AssertScriptSuccess("1:3+\"foo\";", new ScriptValue_String("1foo", "2foo", "3foo"));
	AssertScriptSuccess("NULL+\"foo\";", new ScriptValue_String());		// FIXME should this be an error?
	AssertScriptSuccess("\"foo\"+\"bar\";", new ScriptValue_String("foobar"));
	AssertScriptSuccess("\"foo\"+c(\"bar\", \"baz\");", new ScriptValue_String("foobar", "foobaz"));
	AssertScriptSuccess("c(\"bar\", \"baz\")+\"foo\";", new ScriptValue_String("barfoo", "bazfoo"));
	AssertScriptSuccess("c(\"bar\", \"baz\")+T;", new ScriptValue_String("barT", "bazT"));
	AssertScriptSuccess("F+c(\"bar\", \"baz\");", new ScriptValue_String("Fbar", "Fbaz"));
	AssertScriptRaise("T+F;");
	AssertScriptRaise("T+T;");
	AssertScriptRaise("F+F;");
	AssertScriptSuccess("+5;", new ScriptValue_Int(5));
	AssertScriptSuccess("+5.0;", new ScriptValue_Float(5));
	AssertScriptRaise("+\"foo\";");
	AssertScriptRaise("+T;");
	AssertScriptSuccess("3+4+5;", new ScriptValue_Int(12));
	
	// test for the - operator
	AssertScriptSuccess("1-1;", new ScriptValue_Int(0));
	AssertScriptSuccess("1--1;", new ScriptValue_Int(2));
	AssertScriptSuccess("(0:2)-10;", new ScriptValue_Int(-10, -9, -8));
	AssertScriptSuccess("10-(0:2);", new ScriptValue_Int(10, 9, 8));
	AssertScriptSuccess("(15:13)-(0:2);", new ScriptValue_Int(15, 13, 11));
	AssertScriptRaise("(15:12)-(0:2);");
	AssertScriptRaise("NULL-(0:2);");		// FIXME should this be an error?
	AssertScriptSuccess("1-1.0;", new ScriptValue_Float(0));
	AssertScriptSuccess("1.0-1;", new ScriptValue_Float(0));
	AssertScriptSuccess("1.0--1.0;", new ScriptValue_Float(2));
	AssertScriptSuccess("(0:2.0)-10;", new ScriptValue_Float(-10, -9, -8));
	AssertScriptSuccess("10.0-(0:2);", new ScriptValue_Float(10, 9, 8));
	AssertScriptSuccess("(15.0:13)-(0:2.0);", new ScriptValue_Float(15, 13, 11));
	AssertScriptRaise("(15:12.0)-(0:2);");
	AssertScriptRaise("NULL-(0:2.0);");		// FIXME should this be an error?
	AssertScriptRaise("\"foo\"-1;");
	AssertScriptRaise("T-F;");
	AssertScriptRaise("T-T;");
	AssertScriptRaise("F-F;");
	AssertScriptSuccess("-5;", new ScriptValue_Int(-5));
	AssertScriptSuccess("-5.0;", new ScriptValue_Float(-5));
	AssertScriptRaise("-\"foo\";");
	AssertScriptRaise("-T;");
	AssertScriptSuccess("3-4-5;", new ScriptValue_Int(-6));
	
	// test the seq() function
	AssertScriptSuccess("seq(1, 5);", new ScriptValue_Int(1, 2, 3, 4, 5));
	AssertScriptSuccess("seq(5, 1);", new ScriptValue_Int(5, 4, 3, 2, 1));
	AssertScriptSuccess("seq(1.1, 5);", new ScriptValue_Float(1.1, 2.1, 3.1, 4.1));
	AssertScriptSuccess("seq(1, 5.1);", new ScriptValue_Float(1, 2, 3, 4, 5));
	AssertScriptSuccess("seq(1, 10, 2);", new ScriptValue_Int(1, 3, 5, 7, 9));
	AssertScriptRaise("seq(1, 10, -2);");
	AssertScriptSuccess("seq(10, 1, -2);", new ScriptValue_Int(10, 8, 6, 4, 2));
	AssertScriptSuccess("(seq(1, 2, 0.2) - c(1, 1.2, 1.4, 1.6, 1.8, 2.0)) < 0.000000001;", new ScriptValue_Logical(true, true, true, true, true, true));
	AssertScriptRaise("seq(1, 2, -0.2);");
	AssertScriptSuccess("(seq(2, 1, -0.2) - c(2.0, 1.8, 1.6, 1.4, 1.2, 1)) < 0.000000001;", new ScriptValue_Logical(true, true, true, true, true, true));
	AssertScriptRaise("seq(\"foo\", 2, 1);");
	AssertScriptRaise("seq(1, \"foo\", 2);");
	AssertScriptRaise("seq(2, 1, \"foo\");");
	AssertScriptRaise("seq(T, 2, 1);");
	AssertScriptRaise("seq(1, T, 2);");
	AssertScriptRaise("seq(2, 1, T);");
	// FIXME test with NULL
	
	// test for the rev() function
	AssertScriptSuccess("rev(6:10);", new ScriptValue_Int(10,9,8,7,6));
	AssertScriptSuccess("rev(-(6:10));", new ScriptValue_Int(-10,-9,-8,-7,-6));
	AssertScriptSuccess("rev(c(\"foo\",\"bar\",\"baz\"));", new ScriptValue_String("baz","bar","foo"));
	AssertScriptSuccess("rev(-1);", new ScriptValue_Int(-1));
	AssertScriptSuccess("rev(1.0);", new ScriptValue_Float(1));
	AssertScriptSuccess("rev(\"foo\");", new ScriptValue_String("foo"));
	AssertScriptSuccess("rev(6.0:10);", new ScriptValue_Float(10,9,8,7,6));
	AssertScriptSuccess("rev(c(T,T,T,F));", new ScriptValue_Logical(false, true, true, true));


}
































































