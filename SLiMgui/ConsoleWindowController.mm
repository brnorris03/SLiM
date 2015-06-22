//
//  ConsoleWindowController.m
//  SLiM
//
//  Created by Ben Haller on 6/13/15.
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


#import "ConsoleWindowController.h"
#import "SLiMScriptTextView.h"

#include "script.h"
#include "slim_global.h"
#include "script_interpreter.h"
#include "script_functionsignature.h"

#include <iostream>
#include <sstream>
#include <stdexcept>


using std::string;
using std::vector;
using std::map;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::istream;
using std::ostream;


// User defaults keys
NSString *defaultsShowTokensKey = @"ShowTokens";
NSString *defaultsShowParseKey = @"ShowParse";
NSString *defaultsShowExecutionKey = @"ShowExecution";
NSString *defaultsSuppressScriptCheckSuccessPanelKey = @"SuppressScriptCheckSuccessPanel";


@implementation ConsoleWindowController

@synthesize delegate, browserController, scriptWindow, mainSplitView, scriptTextView, outputTextView;

+ (void)initialize
{
	[[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
															 [NSNumber numberWithBool:NO], defaultsShowTokensKey,
															 [NSNumber numberWithBool:NO], defaultsShowParseKey,
															 [NSNumber numberWithBool:NO], defaultsShowExecutionKey,
															 [NSNumber numberWithBool:NO], defaultsSuppressScriptCheckSuccessPanelKey,
															 nil]];
}

- (instancetype)init
{
	if (self = [super init])
	{
	}
	
	return self;
}

- (void)awakeFromNib
{
	// Tell Cocoa that we can go full-screen
	[scriptWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	
	// Turn off all automatic substitution stuff, etc.; turning them off in the nib doesn't seem to actually turn them off, on 10.10.1
	[scriptTextView setAutomaticDashSubstitutionEnabled:NO];
	[scriptTextView setAutomaticDataDetectionEnabled:NO];
	[scriptTextView setAutomaticLinkDetectionEnabled:NO];
	[scriptTextView setAutomaticQuoteSubstitutionEnabled:NO];
	[scriptTextView setAutomaticSpellingCorrectionEnabled:NO];
	[scriptTextView setAutomaticTextReplacementEnabled:NO];
	[scriptTextView setContinuousSpellCheckingEnabled:NO];
	[scriptTextView setGrammarCheckingEnabled:NO];
	[scriptTextView turnOffLigatures:nil];
	
	[outputTextView setAutomaticDashSubstitutionEnabled:NO];
	[outputTextView setAutomaticDataDetectionEnabled:NO];
	[outputTextView setAutomaticLinkDetectionEnabled:NO];
	[outputTextView setAutomaticQuoteSubstitutionEnabled:NO];
	[outputTextView setAutomaticSpellingCorrectionEnabled:NO];
	[outputTextView setAutomaticTextReplacementEnabled:NO];
	[outputTextView setContinuousSpellCheckingEnabled:NO];
	[outputTextView setGrammarCheckingEnabled:NO];
	[outputTextView turnOffLigatures:nil];
	
	// Fix textview fonts and typing attributes
	[scriptTextView setFont:[NSFont fontWithName:@"Menlo" size:11.0]];
	[outputTextView setFont:[NSFont fontWithName:@"Menlo" size:11.0]];
	
	[scriptTextView setTypingAttributes:[SLiMScriptTextView consoleTextAttributesWithColor:nil]];
	[outputTextView setTypingAttributes:[SLiMScriptTextView consoleTextAttributesWithColor:nil]];
	
	// Fix text container insets to look a bit nicer; {0,0} by default
	[scriptTextView setTextContainerInset:NSMakeSize(0.0, 5.0)];
	[outputTextView setTextContainerInset:NSMakeSize(0.0, 5.0)];
	
	// Show a welcome message
	[outputTextView showWelcomeMessage];
	
	if (delegate)
		[delegate appendWelcomeMessageAddendum];
	
	// And show our prompt
	[outputTextView showPrompt];
	
	// Execute a null statement to get our symbols set up, for code completion etc.
	// Note this has the side effect of creating a random number generator g_rng for our use; we have a separate one from SLiM.
	[self validateSymbolTable];
}

- (void)dealloc
{
	delete global_symbols;
	global_symbols = nil;
	
	[scriptWindow setDelegate:nil];
	[browserController setDelegate:nil];
	
	[self setDelegate:nil];
	[self setBrowserController:nil];
	[self setScriptWindow:nil];
	[self setMainSplitView:nil];
	[self setScriptTextView:nil];
	[self setOutputTextView:nil];
	
	[super dealloc];
}

- (void)showWindow
{
	[scriptWindow makeKeyAndOrderFront:nil];
	[scriptWindow makeFirstResponder:outputTextView];
}

- (ConsoleTextView *)textView;
{
	return outputTextView;
}

- (SymbolTable *)symbols
{
	return global_symbols;
}

- (void)invalidateSymbolTable
{
	if (global_symbols)
	{
		delete global_symbols;
		global_symbols = nil;
	}
	
	[browserController reloadBrowser];
}

- (void)validateSymbolTable
{
	if (!global_symbols)
	{
		NSString *errorString = nil;
		
		[self _executeScriptString:@";" tokenString:NULL parseString:NULL executionString:NULL errorString:&errorString addOptionalSemicolon:NO];
		
		if (errorString)
			NSLog(@"Error in validateSymbolTable: %@", errorString);
	}
	
	[browserController reloadBrowser];
}

- (NSString *)_executeScriptString:(NSString *)scriptString tokenString:(NSString **)tokenString parseString:(NSString **)parseString executionString:(NSString **)executionString errorString:(NSString **)errorString addOptionalSemicolon:(BOOL)addSemicolon
{
	string script_string([scriptString UTF8String]);
	Script script(script_string, 0);
	ScriptValue *result;
	string output;
	
	// Tokenize
	try
	{
		script.Tokenize();
		
		// Add a semicolon if needed; this allows input like "6+7" in the console
		if (addSemicolon)
			script.AddOptionalSemicolon();
		
		if (tokenString)
		{
			ostringstream token_stream;
			
			script.PrintTokens(token_stream);
			
			string token_stream_string = token_stream.str();
			
			*tokenString = [NSString stringWithUTF8String:token_stream_string.c_str()];
		}
	}
	catch (std::runtime_error err)
	{
		string raise_msg = GetUntrimmedRaiseMessage();
		
		*errorString = [NSString stringWithUTF8String:raise_msg.c_str()];
		return nil;
	}
	
	// Parse, an "interpreter block" bounded by an EOF rather than a "script block" that requires braces
	try
	{
		script.ParseInterpreterBlockToAST();
		
		if (parseString)
		{
			ostringstream parse_stream;
			
			script.PrintAST(parse_stream);
			
			string parse_stream_string = parse_stream.str();
			
			*parseString = [NSString stringWithUTF8String:parse_stream_string.c_str()];
		}
	}
	catch (std::runtime_error err)
	{
		string raise_msg = GetUntrimmedRaiseMessage();
		
		*errorString = [NSString stringWithUTF8String:raise_msg.c_str()];
		return nil;
	}
	
	// Ensure that we have a symbol table; this used to be done by ScriptInterpreter but now we're responsible
	if (!global_symbols)
		global_symbols = new SymbolTable();
	
	// Interpret the parsed block
	if (delegate)
		[delegate willExecuteScript];
	
	ScriptInterpreter interpreter(script, *global_symbols);		// give the interpreter the symbol table
	
	// Give our delegate a chance to add variables and other context to the interpreter
	if (delegate)
		[delegate injectIntoInterpreter:&interpreter];
	
	try
	{
		if (executionString)
			interpreter.SetShouldLogExecution(true);
		
		result = interpreter.EvaluateInterpreterBlock();
		output = interpreter.ExecutionOutput();
		
		// reload outline view to show new global symbols, in case they have changed
		[browserController reloadBrowser];
		
		if (executionString)
		{
			string execution_log = interpreter.ExecutionLog();
			
			*executionString = [NSString stringWithUTF8String:execution_log.c_str()];
		}
	}
	catch (std::runtime_error err)
	{
		if (delegate)
			[delegate didExecuteScript];
		
		output = interpreter.ExecutionOutput();
		
		string raise_msg = GetUntrimmedRaiseMessage();
		*errorString = [NSString stringWithUTF8String:raise_msg.c_str()];
		
		return [NSString stringWithUTF8String:output.c_str()];
	}
	
	if (delegate)
		[delegate didExecuteScript];
	
	return [NSString stringWithUTF8String:output.c_str()];
}

- (void)executeScriptString:(NSString *)scriptString addOptionalSemicolon:(BOOL)addSemicolon
{
	NSTextStorage *ts = [outputTextView textStorage];
	NSString *tokenString = nil, *parseString = nil, *executionString = nil, *errorString = nil;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	BOOL showTokens = [defaults boolForKey:defaultsShowTokensKey];
	BOOL showParse = [defaults boolForKey:defaultsShowParseKey];
	BOOL showExecution = [defaults boolForKey:defaultsShowExecutionKey];
	
	NSString *result = [self _executeScriptString:scriptString
									  tokenString:(showTokens ? &tokenString : NULL)
									  parseString:(showParse ? &parseString : NULL)
								  executionString:(showExecution ? &executionString : NULL)
									  errorString:&errorString
							 addOptionalSemicolon:addSemicolon];
	
	// make the attributed strings we will append
	NSAttributedString *outputString1 = [[NSAttributedString alloc] initWithString:@"\n" attributes:[ConsoleTextView inputAttrs]];
	NSAttributedString *outputString2 = (result ? [[NSAttributedString alloc] initWithString:result attributes:[ConsoleTextView outputAttrs]] : nil);
	NSAttributedString *errorAttrString = (errorString ? [[NSAttributedString alloc] initWithString:errorString attributes:[ConsoleTextView errorAttrs]] : nil);
	NSAttributedString *tokenAttrString = (tokenString ? [[NSAttributedString alloc] initWithString:tokenString attributes:[ConsoleTextView tokensAttrs]] : nil);
	NSAttributedString *parseAttrString = (parseString ? [[NSAttributedString alloc] initWithString:parseString attributes:[ConsoleTextView parseAttrs]] : nil);
	NSAttributedString *executionAttrString = (executionString ? [[NSAttributedString alloc] initWithString:executionString attributes:[ConsoleTextView executionAttrs]] : nil);;
	
	// do the editing
	[ts beginEditing];
	[ts replaceCharactersInRange:NSMakeRange([ts length], 0) withAttributedString:outputString1];
	[outputTextView appendSpacer];
	
	if (tokenAttrString)
	{
		[ts replaceCharactersInRange:NSMakeRange([ts length], 0) withAttributedString:tokenAttrString];
		[outputTextView appendSpacer];
	}
	if (parseAttrString)
	{
		[ts replaceCharactersInRange:NSMakeRange([ts length], 0) withAttributedString:parseAttrString];
		[outputTextView appendSpacer];
	}
	if (executionAttrString)
	{
		[ts replaceCharactersInRange:NSMakeRange([ts length], 0) withAttributedString:executionAttrString];
		[outputTextView appendSpacer];
	}
	if ([outputString2 length])
	{
		[ts replaceCharactersInRange:NSMakeRange([ts length], 0) withAttributedString:outputString2];
		[outputTextView appendSpacer];
	}
	if (errorAttrString)
	{
		[ts replaceCharactersInRange:NSMakeRange([ts length], 0) withAttributedString:errorAttrString];
		[outputTextView appendSpacer];
	}
	
	[ts endEditing];
	
	// clean up
	[outputString1 release];
	[tokenAttrString release];
	[parseAttrString release];
	[executionAttrString release];
	[outputString2 release];
	[errorAttrString release];
	
	// and show a new prompt
	[outputTextView showPrompt];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
	SEL sel = [menuItem action];
	BOOL continuousPlayOn = (delegate ? [delegate continuousPlayOn] : NO);
	BOOL generationPlayOn = (delegate ? [delegate generationPlayOn] : NO);
	
	if (sel == @selector(checkScript:))
		return !(continuousPlayOn || generationPlayOn);
	if (sel == @selector(executeAll:))
		return !(continuousPlayOn || generationPlayOn);
	if (sel == @selector(executeSelection:))
		return !(continuousPlayOn || generationPlayOn);
	
	return YES;
}


//
//	Actions
//
#pragma mark Actions

- (IBAction)checkScript:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *currentScriptString = [scriptTextView string];
	const char *cstr = [currentScriptString UTF8String];
	NSString *errorDiagnostic = nil;
	
	if (!cstr)
	{
		errorDiagnostic = [@"The script string could not be read, possibly due to an encoding problem." retain];
	}
	else
	{
		Script script(cstr, 0);
		
		try {
			script.Tokenize();
			script.ParseInterpreterBlockToAST();
		}
		catch (std::runtime_error err)
		{
			string raise_msg = GetTrimmedRaiseMessage();
			
			errorDiagnostic = [[NSString stringWithUTF8String:raise_msg.c_str()] retain];
		}
	}
	
	if (delegate)
		[delegate checkScriptDidSucceed:!errorDiagnostic];
	
	if (errorDiagnostic)
	{
		// On failure, we show an alert describing the error, and highlight the relevant script line
		NSAlert *alert = [[NSAlert alloc] init];
		
		[alert setAlertStyle:NSWarningAlertStyle];
		[alert setMessageText:@"Script error"];
		[alert setInformativeText:errorDiagnostic];
		[alert addButtonWithTitle:@"OK"];
		
		[alert beginSheetModalForWindow:scriptWindow completionHandler:^(NSModalResponse returnCode) { [alert autorelease]; }];
		
		[scriptTextView selectErrorRange];
	}
	else
	{
		// On success, we optionally show a success alert sheet
		if (![defaults boolForKey:defaultsSuppressScriptCheckSuccessPanelKey])
		{
			NSAlert *alert = [[NSAlert alloc] init];
			
			[alert setAlertStyle:NSInformationalAlertStyle];
			[alert setMessageText:@"No script errors"];
			[alert setInformativeText:@"No errors found."];
			[alert addButtonWithTitle:@"OK"];
			[alert setShowsSuppressionButton:YES];
			
			[alert beginSheetModalForWindow:scriptWindow completionHandler:^(NSModalResponse returnCode) {
				if ([[alert suppressionButton] state] == NSOnState)
					[defaults setBool:YES forKey:defaultsSuppressScriptCheckSuccessPanelKey];
				[alert autorelease];
			}];
		}
	}
	
	[errorDiagnostic release];
}

- (IBAction)showScriptHelp:(id)sender
{
	NSTextStorage *ts = [outputTextView textStorage];
	NSAttributedString *scriptAttrString = [[NSAttributedString alloc] initWithString:@"help()" attributes:[ConsoleTextView inputAttrs]];
	NSUInteger promptEnd = [outputTextView promptRangeEnd];
	
	[ts beginEditing];
	[ts replaceCharactersInRange:NSMakeRange(promptEnd, [ts length] - promptEnd) withAttributedString:scriptAttrString];
	[ts endEditing];
	
	[outputTextView registerNewHistoryItem:@"help()"];
	[self executeScriptString:@"help()" addOptionalSemicolon:YES];
}

- (IBAction)clearOutput:(id)sender
{
	[outputTextView clearOutput];
}

- (IBAction)toggleConsoleVisibility:(id)sender
{
	if ([scriptWindow isVisible])
		[scriptWindow performClose:nil];
	else
		[scriptWindow makeKeyAndOrderFront:nil];
}

- (IBAction)executeAll:(id)sender
{
	NSTextStorage *ts = [outputTextView textStorage];
	NSString *fullScriptString = [scriptTextView string];
	NSAttributedString *scriptAttrString = [[NSAttributedString alloc] initWithString:fullScriptString attributes:[ConsoleTextView inputAttrs]];
	NSUInteger promptEnd = [outputTextView promptRangeEnd];
	
	[ts beginEditing];
	[ts replaceCharactersInRange:NSMakeRange(promptEnd, [ts length] - promptEnd) withAttributedString:scriptAttrString];
	[ts endEditing];
	
	[outputTextView registerNewHistoryItem:fullScriptString];
	[self executeScriptString:fullScriptString addOptionalSemicolon:NO];
}

- (IBAction)executeSelection:(id)sender
{
	NSTextStorage *ts = [outputTextView textStorage];
	NSString *fullScriptString = [scriptTextView string];
	NSUInteger scriptLength = [fullScriptString length];
	NSRange selectedRange = [scriptTextView selectedRange];
	NSCharacterSet *newlineChars = [NSCharacterSet newlineCharacterSet];
	NSRange executionRange = selectedRange;	// indices of the first and last characters to execute
	
	// If the selection is an insertion point, execute the whole line
	if (executionRange.length == 0)
	{
		// start at the start of the selection and move backwards to the beginning of the line
		while (executionRange.location > 0)
		{
			unichar ch = [fullScriptString characterAtIndex:executionRange.location - 1];
			
			if ([newlineChars characterIsMember:ch])
				break;
			
			--executionRange.location;
			++executionRange.length;
		}
		
		// now move the end of the selection backwards to remove any newlines from the end of the range to execute
		while (executionRange.length > 0)
		{
			unichar ch = [fullScriptString characterAtIndex:executionRange.location + executionRange.length - 1];
			
			if (![newlineChars characterIsMember:ch])
				break;
			
			--executionRange.length;
		}
		
		// now move the end of the selection forwards to the end of the line, not including the newline
		while (executionRange.location + executionRange.length < scriptLength)
		{
			unichar ch = [fullScriptString characterAtIndex:executionRange.location + executionRange.length];
			
			if ([newlineChars characterIsMember:ch])
				break;
			
			++executionRange.length;
		}
	}
	
	// now execute the range we have found
	if (executionRange.length > 0)
	{
		NSString *scriptString = [fullScriptString substringWithRange:executionRange];
		NSAttributedString *scriptAttrString = [[NSAttributedString alloc] initWithString:scriptString attributes:[ConsoleTextView inputAttrs]];
		NSUInteger promptEnd = [outputTextView promptRangeEnd];
		
		[ts beginEditing];
		[ts replaceCharactersInRange:NSMakeRange(promptEnd, [ts length] - promptEnd) withAttributedString:scriptAttrString];
		[ts endEditing];
		
		[outputTextView registerNewHistoryItem:scriptString];
		[self executeScriptString:scriptString addOptionalSemicolon:YES];
	}
	else
	{
		NSBeep();
	}
}


//
//	VariableBrowserController delegate methods
//
#pragma mark VariableBrowserController delegate

- (SymbolTable *)symbolTable
{
	return global_symbols;
}


//
//	NSTextView delegate methods
//
#pragma mark NSTextView delegate

- (NSRange)textView:(NSTextView *)textView willChangeSelectionFromCharacterRange:(NSRange)oldRange toCharacterRange:(NSRange)newRange
{
	if (textView == outputTextView)
	{
		// prevent a zero-length selection (i.e. an insertion point) in the history
		if ((newRange.length == 0) && (newRange.location < [outputTextView promptRangeEnd]))
			return NSMakeRange([[outputTextView string] length], 0);
	}
	
	return newRange;
}

- (BOOL)textView:(NSTextView *)textView shouldChangeTextInRange:(NSRange)affectedCharRange replacementString:(NSString *)replacementString
{
	if (textView == outputTextView)
	{
		// prevent the user from changing anything above the current prompt
		if (affectedCharRange.location < [outputTextView promptRangeEnd])
			return NO;
	}
	
	return YES;
}

- (void)executeConsoleInput:(ConsoleTextView *)textView
{
	if (textView == outputTextView)
	{
		NSString *outputString = [outputTextView string];
		NSString *scriptString = [outputString substringFromIndex:[outputTextView promptRangeEnd]];
		
		[outputTextView registerNewHistoryItem:scriptString];
		[self executeScriptString:scriptString addOptionalSemicolon:YES];
	}
}

- (void)textDidChange:(NSNotification *)notification
{
	NSTextView *textView = (NSTextView *)[notification object];
	
	// Syntax color the script string
	if (textView == scriptTextView)
	{
		[scriptTextView syntaxColorForSLiMScript];
	}
}

- (SymbolTable *)globalSymbolsForCompletion
{
	return global_symbols;
}

- (std::vector<FunctionSignature*> *)injectedFunctionSignatures
{
	return [delegate injectedFunctionSignatures];
}


//
//	NSWindow delegate methods
//
#pragma mark NSWindow delegate

- (void)windowWillClose:(NSNotification *)notification
{
	NSWindow *closingWindow = [notification object];
	
	if (closingWindow == scriptWindow)
	{
		// If we're closing, the variable browser closes too, since it is an inspector on our state
		if ([[browserController browserWindow] isVisible])
			[browserController toggleBrowserVisibility:self];
		
		// Let our delegate do some; SLiMscribe quits, SLiMgui toggles its console button
		if (delegate)
			[delegate consoleWindowWillClose];
	}
}


//
//	NSSplitView delegate methods
//
#pragma mark NSSplitView delegate

- (BOOL)splitView:(NSSplitView *)splitView canCollapseSubview:(NSView *)subview
{
	return YES;
}

- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex
{
	return proposedMax - 230;
}

- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex
{
	return proposedMin + 230;
}

@end



































































