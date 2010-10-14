/*

 SimplePlugin.cpp
 SimplePlugin

 Copyright 2010 Mark Banks. All rights reserved.
 License: see the accompyanying file License.txt
 
 http://banks.id.au/filemaker/plugins/simpleplugin/

 */


#include "FMWrapper/FMXBinaryData.h"
#include "FMWrapper/FMXCalcEngine.h"
#include "FMWrapper/FMXClient.h"
#include "FMWrapper/FMXData.h"
#include "FMWrapper/FMXDateTime.h"
#include "FMWrapper/FMXExtern.h"
#include "FMWrapper/FMXFixPt.h"
#include "FMWrapper/FMXText.h"
#include "FMWrapper/FMXTextStyle.h"
#include "FMWrapper/FMXTypes.h"

#include <string>
#include <iostream>


#pragma mark -
#pragma mark Prototypes
#pragma mark -

fmx::QuadCharAutoPtr PluginID ( void );
fmx::TextAutoPtr PluginPrefix ( void );
fmx::TextAutoPtr PluginOptionsString ( void );

void GetString ( FMX_ExternCallPtr pb );
fmx::errcode RegisterFunction ( std::string prototype, fmx::ExtPluginType function, bool evaluate_on_server = true );
void UnregisterFunctions ( void );
fmx::TextAutoPtr FunctionName ( fmx::TextAutoPtr& signature );
void NumberOfParameters ( fmx::TextAutoPtr& signature, short& required, short& optional );


// pragma mark generates a warning C4068 in Visual Studio so that warning is disabled in VS

#pragma mark -
#pragma mark Enums, defines &  Globals
#pragma mark -

enum {
	kSPOptionsString = 1
}; 


enum {
	kSPManyParameters = -1,
	kSPPrefixLength = 4,
	kSPPluginIDLength = 5,	//	kSPPrefixLength = 4 + 1
	kSPFirstFunction = 1000
};


enum {
	kSPFirstParameter = 0,
	kSPSecondParameter = 1,
	kSPThirdParameter = 2,
	kSPFourthParameter = 3,
	kSPFifthParameter = 4
};


enum errors {
	kSPNoError = 0,
};


//#define PLUGIN_NAME "SimplePlugin"		//	default is the project name (Xcode only)
											//		in Visual Studio set $(TargetName)
//#define PLUGIN_PREFIX "Simp"				//	default is the first four characters of the name
//#define PLUGIN_OPTIONS_STRING "1YnYYnn"	//	ask for everything ( "1nnYnnn" )


// Globals

short g_next_function;	// used to keep track of the funciton id number


#pragma mark -
#pragma mark Functions
#pragma mark -

/* ****************************************************************************
 
 Add code for plug-in functions here. Each function must be registered using 
 RegisterFunction in the Init function.
 
 **************************************************************************** */


FMX_PROC(fmx::errcode) Limit ( short function_id, const fmx::ExprEnv& environment, const fmx::DataVect& parameters, fmx::Data& reply )
{
#pragma usnused ( function_id, environment )
	
	fmx::errcode error = kSPNoError;
	
	// extract the first paramter to the function
	
	fmx::FixPtAutoPtr number;
	number->AssignFixPt ( parameters.AtAsNumber ( kSPFirstParameter ) );
	
	fmx::FixPtAutoPtr lower_limit;
	
	// only get the lower and upper limits if they exist (otherwise we crash)
	
	if ( parameters.Size() >= 2 ) {
		lower_limit->AssignFixPt ( parameters.AtAsNumber ( kSPSecondParameter ) );
	} else {
		lower_limit->AssignInt ( 0 );
	}

	fmx::FixPtAutoPtr upper_limit;
	if ( parameters.Size() == 3 ) {
		upper_limit->AssignFixPt ( parameters.AtAsNumber ( kSPThirdParameter ) );
	} else {
		long precision = number->GetPrecision();
		upper_limit->AssignInt ( precision * 10 );
	}
	
	// apply the limits
	
	if ( *number < *lower_limit ) {
		number = lower_limit;
	} else if ( *number > *upper_limit ) {
		number = upper_limit;
	}
	
	// send the result of the calculation back to FileMaker
	
	reply.SetAsNumber ( *number );
	
	return error;
	
} // Limit


/* ***************************************************************************
 
 Public plug-in functions
 
 **************************************************************************** */

#pragma mark -
#pragma mark Plugin
#pragma mark -

/*
 initalise the plug-in
 perform any setup and register functions
 */

fmx::errcode Init ( FMX_ExternCallPtr pb ) 
{
#pragma unused ( pb )
	
	fmx::errcode error = kSPNoError;
	fmx::errcode enable = kCurrentExtnVersion; // kDoNotEnable to prevent the plugin loading

	
	// perform any initialisation and set-up globals
	
	
	
	/*
	 register plug-in functions
	 
	 functions must always be registered in the same order (to avoid breaking existing 
	 calculation in FM).
	 */

	error = RegisterFunction ( "Limit ( number {; lowerLimit ; upperLimit } )", Limit );

	return enable;
	
} // Init


/*
 clean up anything set up or allocated in Init
 plug-in functions are un-registered automatically before this function is called
 */

void Shutdown ( void ) 
{
}


/*
 the main entry point for the plug-in
 
 calls from fmp go either here or directly to registered plugin function
 see also the options for FMX_ExternCallSwitch in FMXExtern.h
 
 only edit to add additonal call handlers
 */

void FMX_ENTRYPT FMExternCallProc ( FMX_ExternCallPtr pb )
{
	
	switch ( pb->whichCall ) 
	{
		case kFMXT_GetString:
			GetString ( pb );
			break;
			
		case kFMXT_Init:
			g_next_function = kSPFirstFunction;
			pb->result = Init ( pb );
			break;
			
		case kFMXT_Shutdown:
			UnregisterFunctions ( );
			Shutdown ( );
			break;
			
	}

}	// FMExternCallProc


/* ***************************************************************************
 You should not need to edit anything in this section.
 *************************************************************************** */

#pragma mark -
#pragma mark Private Functions
#pragma mark -


// get the plug-in name or options string and hand back to FileMaker

void GetString ( FMX_ExternCallPtr pb )
{
	fmx::TextAutoPtr string;
	
	switch ( pb->parm1 )
	{
		case kSPOptionsString:
		case kFMXT_OptionsStr:
			string->SetText ( *PluginOptionsString() );
			break;
			
		case kFMXT_NameStr:
		case kFMXT_AppConfigStr:
			string->Assign ( PLUGIN_NAME );
			break;
			
//		default:
			
	}
	
	string->GetUnicode ( (fmx::unichar*)(pb->result), 0, fmx::Text::kSize_End );
	
} // GetString



/*
 register plug-in functions
 
 RegisterFunction takes three parameters: 
 1. the external function signature as it should appear in the calcuation dialogs but
 without the prefix i.e. Explode ( timer ) rather than Bomb_Explode ( timer )
 2.	the plug-in function to call when the function is used in FileMaker
 3. (optional) should the function be available via FileMaker Server? default: true
 */

fmx::errcode RegisterFunction ( std::string prototype, fmx::ExtPluginType function, bool evaluate_on_server )
{
	fmx::TextAutoPtr underscore;
	underscore->Assign ( "_" );

	fmx::TextAutoPtr signature;
	signature->Assign ( prototype.c_str() );
	signature->InsertText ( *PluginPrefix(), 0 );
	signature->InsertText ( *underscore, kSPPrefixLength );
	
	fmx::TextAutoPtr name;
	name->SetText ( *FunctionName ( signature ) );
	
	short required_parameters = 0;
	short optional_rarameters = 0;
	NumberOfParameters ( signature, required_parameters, optional_rarameters );
	
	fmx::ulong function_flags = fmx::ExprEnv::kDisplayInAllDialogs;
	if ( evaluate_on_server ) {
		function_flags |= fmx::ExprEnv::kMayEvaluateOnServer;
	}
	
	fmx::errcode error = fmx::ExprEnv::RegisterExternalFunction ( *PluginID(),
																 g_next_function, 
																 *name, 
																 *signature, 
																 required_parameters, 
																 required_parameters + optional_rarameters, 
																 function_flags, 
																 function 
																 );
	
	if ( error != kSPNoError ) {
		std::cerr << "Error registering: " << prototype << "! Error #: " << error << std::endl;
	}

	++g_next_function;

	return error;
	
} // RegisterFunction


// unregister all registered functions

void UnregisterFunctions ( void ) 
{	
	for ( short i = kSPFirstFunction ; i < g_next_function ; i++ ) {
		fmx::ExprEnv::UnRegisterExternalFunction ( *PluginID(), i );
	}	
}


// automaticlly generate the PluginID from the prefix

fmx::QuadCharAutoPtr PluginID ( void )
{
	fmx::TextAutoPtr prefix;
	prefix->SetText ( *PluginPrefix() );
	char buffer[kSPPluginIDLength];
	prefix->GetBytes ( buffer, kSPPluginIDLength );
	fmx::QuadCharAutoPtr id ( buffer[0], buffer[1], buffer[2], buffer[3] );

	return id;
}


// use the defined prefix if it exists otherwise use the first four characters of the name

fmx::TextAutoPtr PluginPrefix ( void )
{
	fmx::TextAutoPtr prefix;

#ifdef PLUGIN_PREFIX
	prefix->Assign ( PLUGIN_PREFIX );
#else
	prefix->Assign ( PLUGIN_NAME );
	prefix->DeleteText ( kSPPrefixLength );
#endif	
	
	return prefix;
}


// use the options string defined above otherwise turn everything on

fmx::TextAutoPtr PluginOptionsString ( void )
{
	fmx::TextAutoPtr optionsString;
	
#ifdef PLUGIN_OPTIONS_STRING
	optionsString->Assign ( PLUGIN_OPTIONS_STRING );
#else
	optionsString->Assign ( "1YnYYnn" );
#endif	

	optionsString->InsertText ( *PluginPrefix(), 0 );
	
	return optionsString;
}


// extract the function name from a function signature/prototype

fmx::TextAutoPtr FunctionName ( fmx::TextAutoPtr& signature )
{
	
	fmx::TextAutoPtr separator;
	separator->Assign ( "(" );
	
	fmx::ulong parameters_start = signature->Find ( *separator, 0 );
	if ( parameters_start == fmx::Text::kSize_Invalid ) {
		parameters_start = fmx::Text::kSize_End;
	} else {
		
		// there may or may not be spaces between the function name and the bracket
		
		fmx::TextAutoPtr space;
		space->Assign ( " " );
		
		fmx::ulong last = parameters_start - 1;
		while ( signature->Find ( *space, last ) == last ) {
			--last;
		}
		parameters_start = last + 1;
	}
	
	fmx::TextAutoPtr name;
	name->SetText ( *signature, 0, parameters_start );
	
	return name;
	
} // FunctionName


// calculate the number of required and optional parameters from a function signature/prototye

void NumberOfParameters ( fmx::TextAutoPtr& signature, short& required, short& optional )
{
	required = 0;
	optional = 0;
	
	fmx::TextAutoPtr separator;
	separator->Assign ( "(" );
	
	fmx::ulong parameters_start = signature->Find ( *separator, 0 );
	if ( parameters_start == fmx::Text::kSize_Invalid ) {
		return;
	}
	
	// we have parameters
	
	fmx::TextAutoPtr semi_colon;
	semi_colon->Assign ( ";" );
	
	fmx::TextAutoPtr curly_bracket;
	curly_bracket->Assign ( "{" );	
	
	bool has_optional_parameters = false;
	fmx::ulong next = parameters_start;
	
	while ( next != fmx::Text::kSize_Invalid ) {
		
		++next;
		fmx::ulong next_semi_colon = signature->Find ( *semi_colon, next );
		fmx::ulong next_curly_bracket = signature->Find ( *curly_bracket, next );
		
		if ( next_curly_bracket < next_semi_colon && has_optional_parameters == false ) {
			
			next = signature->Find ( *semi_colon, next_curly_bracket + 1 );
			++required;
			has_optional_parameters = true;
			
			fmx::TextAutoPtr elipsis;
			elipsis->Assign ( "…" );
			
			if ( signature->Find ( *elipsis, next_curly_bracket + 1 ) != fmx::Text::kSize_Invalid ) {
				optional = -1;
				next = fmx::Text::kSize_Invalid;
			} else {
				
				fmx::TextAutoPtr faux_elipsis;
				faux_elipsis->Assign ( "..." );
				
				if ( signature->Find ( *faux_elipsis, next_curly_bracket + 1 ) != fmx::Text::kSize_Invalid ) {
					optional = kSPManyParameters;
					next = fmx::Text::kSize_Invalid;
				}
			}
			
			
		} else {
			next = next_semi_colon;
			
			if ( has_optional_parameters == true ) {
				++optional;
			} else {
				++required;
			}
		}
		
	}
	
} // NumberOfParameters


