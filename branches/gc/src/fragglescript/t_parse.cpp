// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Parsing.
//
// Takes lines of code, or groups of lines and runs them.
// The main core of FraggleScript
//
// By Simon Howard
//
//----------------------------------------------------------------------------

/*
FraggleScript is from SMMU which is under the GPL. Technically, therefore, 
combining the FraggleScript code with the non-free ZDoom code is a violation of the GPL.

As this may be a problem for you, I hereby grant an exception to my copyright on the 
SMMU source (including FraggleScript). You may use any code from SMMU in GZDoom, provided that:

    * For any binary release of the port, the source code is also made available.
    * The copyright notice is kept on any file containing my code.
*/

/* includes ************************/
#include <stdarg.h>
#include "t_script.h"
#include "s_sound.h"
#include "v_text.h"
#include "c_cvars.h"
#include "i_system.h"
#include "a_pickups.h"


CVAR(Bool, script_debug, false, 0)

// inline for speed
// haleyjd: updated
#define isnum(c) ( ((c)>='0' && (c)<='9') || (c)=='.')
// isop: is an 'operator' character, eg '=', '%'
#define isop(c)   !( ( (c)<='Z' && (c)>='A') || ( (c)<='z' && (c)>='a') || \
( (c)<='9' && (c)>='0') || ( (c)=='_') )

// for simplicity:
#define tt (tokentype[num_tokens-1])
#define tok (tokens[num_tokens-1])

//==========================================================================
//
// next_token: end this token, go onto the next
//
//==========================================================================

void DFraggleThinker::next_token()
{
	if(tok[0] || tt==string_)
    {
		num_tokens++;
		tokens[num_tokens-1] = tokens[num_tokens-2]
			+ strlen(tokens[num_tokens-2]) + 1;
		tok[0] = 0;
    }
	
	// get to the next token, ignoring spaces, newlines,
	// useless chars, comments etc
	
	while(1)
    {
		// empty whitespace
		if(*rover && (*rover==' ' || *rover<32))
		{
			while((*rover==' ' || *rover<32) && *rover) rover++;
		}
		// end-of-script?
		if(!*rover)
		{
			if(tokens[0][0])
			{
			/*
			C_Printf("%s %i %i\n", tokens[0],
			rover-current_script->data, current_script->len);
				*/
				// line contains text, but no semicolon: an error
				script_error("missing ';'\n");
			}
			// empty line, end of command-list
			return;
		}
		// 11/8 comments moved to new preprocessor
		
		break;  // otherwise
    }
	
	if(num_tokens>1 && *rover == '(' && tokentype[num_tokens-2] == name_)
		tokentype[num_tokens-2] = function;
	
	if(*rover == '{' || *rover == '}')
    {
		if(*rover == '{')
		{
			bracetype = bracket_open;
			current_section = find_section_start(rover);
		}
		else            // closing brace
		{
			bracetype = bracket_close;
			current_section = find_section_end(rover);
		}
		if(!current_section)
		{
			I_Error("section not found!\n");
			return;
		}
    }
	else if(*rover == ':')  // label
    {
		// ignore the label : reset
		num_tokens = 1;
		tokens[0][0] = 0; tt = name_;
		rover++;        // ignore
    }
	else if(*rover == '\"')
    {
		tt = string_;
		if(tokentype[num_tokens-2] == string_)
			num_tokens--;   // join strings
		rover++;
    }
	else
    {
		tt = isop(*rover) ? operator_ : isnum(*rover) ? number : name_;
    }
}

//==========================================================================
//
// return an escape sequence (prefixed by a '\')
// do not use all C escape sequences
//
//==========================================================================

static char escape_sequence(char c)
{
	if(c == 'n') return '\n';
	if(c == '\\') return '\\';
	if(c == '"') return '"';
	if(c == '?') return '?';
	if(c == 'a') return '\a'; // alert beep
	if(c == 't') return '\t'; //tab
	
	return c;
}

//==========================================================================
//
// add_char: add one character to the current token
//
//==========================================================================

void DFraggleThinker::add_char(char c)
{
	char *out = tok + strlen(tok);
	
	*out++ = c;
	*out = 0;
}

//==========================================================================
//
// get_tokens.
// Take a string, break it into tokens.

// individual tokens are stored inside the tokens[] array
// tokentype is also used to hold the type for each token:

//   name: a piece of text which starts with an alphabet letter.
//         probably a variable name. Some are converted into
//         function types later on in find_brackets
//   number: a number. like '12' or '1337'
//   operator: an operator such as '&&' or '+'. All FraggleScript
//             operators are either one character, or two character
//             (if 2 character, 2 of the same char or ending in '=')
//   string: a text string that was enclosed in quote "" marks in
//           the original text
//   unset: shouldn't ever end up being set really.
//   function: a function name (found in second stage parsing)
//
//==========================================================================

void DFraggleThinker::get_tokens(char *s)
{
	rover = s;
	num_tokens = 1;
	tokens[0][0] = 0; tt = name_;
	
	current_section = NULL;   // default to no section found
	
	next_token();
	linestart = rover;      // save the start
	
	if(*rover)
		while(1)
		{
			if(killscript) return;
			if(current_section)
			{
				// a { or } section brace has been found
				break;        // stop parsing now
			}
			else if(tt != string_)
			{
				if(*rover == ';') break;     // check for end of command ';'
			}
			
			switch(tt)
			{
			case unset:
			case string_:
				while(*rover != '\"')     // dedicated loop for speed
				{
					if(*rover == '\\')       // escape sequences
					{
						rover++;
						if (*rover>='0' && *rover<='9')
						{
							add_char(TEXTCOLOR_ESCAPE);
							add_char(*rover+'A'-'0');
						}
						else add_char(escape_sequence(*rover));
					}
					else
						add_char(*rover);
					rover++;
				}
				rover++;
				next_token();       // end of this token
				continue;
				
			case operator_:
				// all 2-character operators either end in '=' or
				// are 2 of the same character
				// do not allow 2-characters for brackets '(' ')'
				// which are still being considered as operators
				
				// operators are only 2-char max, do not need
				// a seperate loop
				
				if((*tok && *rover != '=' && *rover!=*tok) ||
					*tok == '(' || *tok == ')')
				{
					// end of operator
					next_token();
					continue;
				}
				add_char(*rover);
				break;
				
			case number:
				
				// haleyjd: 8-17
				// add while number chars are read
				
				while(isnum(*rover))       // dedicated loop
					add_char(*rover++);
				next_token();
				continue;
				
			case name_:
				
				// add the chars
				
				while(!isop(*rover))        // dedicated loop
					add_char(*rover++);
				next_token();
				continue;
				
			default:
				break;
			}
			rover++;
		}
		
	// check for empty last token
	
	if(!tok[0])
	{
		num_tokens = num_tokens - 1;
	}
	
	rover++;
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::print_tokens()	// DEBUG
{
	int i;
	for (i = 0; i < num_tokens; i++)
	{
		Printf("\n'%s' \t\t --", tokens[i]);
		switch (tokentype[i])
		{
		case string_:
			Printf("string");
			break;
		case operator_:
			Printf("operator");
			break;
		case name_:
			Printf("name");
			break;
		case number:
			Printf("number");
			break;
		case unset:
			Printf("duh");
			break;
		case function:
			Printf("function name");
			break;
		}
	}
	Printf("\n");
	if (current_section)
		Printf("current section: offset %i\n", (int) (current_section->start - current_script->data));
}


//==========================================================================
//
// run_script
//
// the function called by t_script.c
//
//==========================================================================

void DFraggleThinker::run_script(script_t *script)
{
	// set current script
	current_script = script;
	
	// start at the beginning of the script
	rover = current_script->data;
	
	// haleyjd: no last if so it sure wasn't true
	current_script->lastiftrue = false;
	
	parse_script(); // run it
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::continue_script(script_t *script, char *continue_point)
{
	current_script = script;
	
	// continue from place specified
	rover = continue_point;
	
	parse_script(); // run 
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::parse_script()
{
	// check for valid rover
	if(rover < current_script->data || 
		rover > current_script->data+current_script->len)
    {
		script_error("parse_script: trying to continue from point outside script!\n");
		return;
    }
	
	trigger_obj = current_script->trigger;  // set trigger
	
	parse_data(current_script->data,
		current_script->data+current_script->len);
	
	// dont clear global vars!
	if(current_script->scriptnum != -1)
		clear_variables(current_script);        // free variables
	
	// haleyjd
	current_script->lastiftrue = false;
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::parse_data(char *data, char *end)
{
	char *token_alloc;      // allocated memory for tokens
	
	killscript = false;     // dont kill the script straight away
	
	// allocate space for the tokens
	token_alloc = new char[current_script->len + T_MAXTOKENS];
	
	prev_section = NULL;  // clear it
	
	while(*rover)   // go through the script executing each statement
    {
		// past end of script?
		if(rover > end)
			break;
		
		// reset the tokens before getting the next line
		tokens[0] = token_alloc;
		
		prev_section = current_section; // store from prev. statement
		
		// get the line and tokens
		get_tokens(rover);
		
		if(killscript) break;
		
		if(!num_tokens)
		{
			if(current_section)       // no tokens but a brace
			{
				// possible } at end of loop:
				// refer to spec.c
				spec_brace();
			}
			
			continue;  // continue to next statement
		}
		
		if(script_debug) print_tokens();   // debug
		run_statement();         // run the statement
    }
	delete token_alloc;
}

//==========================================================================
//
// decide what to do with it

// NB this stuff is a bit hardcoded:
//    it could be nicer really but i'm
//    aiming for speed

// if() and while() will be mistaken for functions
// during token processing
//
//==========================================================================

void DFraggleThinker::run_statement()
{
	if(tokentype[0] == function)
    {
		if(!strcmp(tokens[0], "if"))
		{
			current_script->lastiftrue = spec_if();
			return;
		}
		else if(!strcmp(tokens[0], "elseif"))
		{
			if(!prev_section || 
				(prev_section->type != st_if && 
				prev_section->type != st_elseif))
			{
				script_error("elseif statement without if\n");
				return;
			}
			current_script->lastiftrue = 
				spec_elseif(current_script->lastiftrue);
			return;
		}
		else if(!strcmp(tokens[0], "else"))
		{
			if(!prev_section ||
				(prev_section->type != st_if &&
				prev_section->type != st_elseif))
			{
				script_error("else statement without if\n");
				return;
			}
			spec_else(current_script->lastiftrue);
			current_script->lastiftrue = true;
			return;
		}
		else if(!strcmp(tokens[0], "while"))
		{
			spec_while();
			return;
		}
		else if(!strcmp(tokens[0], "for"))
		{
			spec_for();
			return;
		}
    }
	else if(tokentype[0] == name_)
    {
		// NB: goto is a function so is not here

		// Allow else without '()'
        if (!strcmp(tokens[0], "else"))
        {
			if(!prev_section ||
				(prev_section->type != st_if &&
				prev_section->type != st_elseif))
			{
				script_error("else statement without if\n");
				return;
			}
			spec_else(current_script->lastiftrue);
			current_script->lastiftrue = true;
			return;
        }

		// if a variable declaration, return now
		if(spec_variable()) return;
    }
	
	// just a plain expression
	evaluate_expression(0, num_tokens-1);
}

/***************** Evaluating Expressions ************************/

//==========================================================================
//
// find a token, ignoring things in brackets        
//
//==========================================================================

int DFraggleThinker::find_operator(int start, int stop, char *value)
{
	int i;
	int bracketlevel = 0;
	
	for(i=start; i<=stop; i++)
    {
		// only interested in operators
		if(tokentype[i] != operator_) continue;
		
		// use bracketlevel to check the number of brackets
		// which we are inside
		bracketlevel += tokens[i][0]=='(' ? 1 :
		tokens[i][0]==')' ? -1 : 0;
		
		// only check when we are not in brackets
		if(!bracketlevel && !strcmp(value, tokens[i]))
			return i;
    }
	
	return -1;
}

//==========================================================================
//
// go through tokens the same as find_operator, but backwards
//
//==========================================================================

int DFraggleThinker::find_operator_backwards(int start, int stop, char *value)
{
	int i;
	int bracketlevel = 0;
	
	for(i=stop; i>=start; i--)      // check backwards
    {
		// operators only
		
		if(tokentype[i] != operator_) continue;
		
		// use bracketlevel to check the number of brackets
		// which we are inside
		
		bracketlevel += tokens[i][0]=='(' ? -1 :
		tokens[i][0]==')' ? 1 : 0;
		
		// only check when we are not in brackets
		// if we find what we want, return it
		
		if(!bracketlevel && !strcmp(value, tokens[i]))
			return i;
    }
	
	return -1;
}

//==========================================================================
//
// simple_evaluate is used once evalute_expression gets to the level
// where it is evaluating just one token

// converts number tokens into svalue_ts and returns
// the same with string tokens
// name tokens are considered to be variables and
// attempts are made to find the value of that variable
// command tokens are executed (does not return a svalue_t)
//
//==========================================================================

svalue_t DFraggleThinker::simple_evaluate(int n)
{
	svalue_t returnvar;
	svariable_t *var;
	
	switch(tokentype[n])
    {
    case string_: returnvar.type = svt_string;
		returnvar.value.s = tokens[n];
		return returnvar;
		
		// haleyjd: 8-17
    case number:
		if(strchr(tokens[n], '.'))
		{
			returnvar.type = svt_fixed;
			returnvar.value.f = (fixed_t)(atof(tokens[n]) * FRACUNIT);
		}
		else
		{
			returnvar.type = svt_int;
			returnvar.value.i = atoi(tokens[n]);
		}
		return returnvar;
		
    case name_:   var = find_variable(tokens[n]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", tokens[n]);
			return nullvar;
		}
		else
			return getvariablevalue(var);
		
    default: return nullvar;
    }
}

//==========================================================================
//
// pointless_brackets checks to see if there are brackets surrounding
// an expression. eg. "(2+4)" is the same as just "2+4"
//
// because of the recursive nature of evaluate_expression, this function is
// neccesary as evaluating expressions such as "2*(2+4)" will inevitably
// lead to evaluating "(2+4)"
//
//==========================================================================

void DFraggleThinker::pointless_brackets(int *start, int *stop)
{
	int bracket_level, i;
	
	// check that the start and end are brackets
	
	while(tokens[*start][0] == '(' && tokens[*stop][0] == ')')
    {
		
		bracket_level = 0;
		
		// confirm there are pointless brackets..
		// if they are, bracket_level will only get to 0
		// at the last token
		// check up to <*stop rather than <=*stop to ignore
		// the last token
		
		for(i = *start; i<*stop; i++)
		{
			if(tokentype[i] != operator_) continue; // ops only
			bracket_level += (tokens[i][0] == '(');
			bracket_level -= (tokens[i][0] == ')');
			if(bracket_level == 0) return; // stop if braces stop before end
		}
		
		// move both brackets in
		
		*start = *start + 1;
		*stop = *stop - 1;
    }
}

//==========================================================================
//
// evaluate_expresion is the basic function used to evaluate
// a FraggleScript expression.
// start and stop denote the tokens which are to be evaluated.
//
// works by recursion: it finds operators in the expression
// (checking for each in turn), then splits the expression into
// 2 parts, left and right of the operator found.
// The handler function for that particular operator is then
// called, which in turn calls evaluate_expression again to
// evaluate each side. When it reaches the level of being asked
// to evaluate just 1 token, it calls simple_evaluate
//
//==========================================================================

svalue_t DFraggleThinker::evaluate_expression(int start, int stop)
{
	int i, n;
	
	if(killscript) return nullvar;  // killing the script
	
	// possible pointless brackets
	if(tokentype[start] == operator_ && tokentype[stop] == operator_)
		pointless_brackets(&start, &stop);
	
	if(start == stop)       // only 1 thing to evaluate
    {
		return simple_evaluate(start);
    }
	
	// go through each operator in order of precedence
	
	for(i=0; i<num_operators; i++)
    {
		// check backwards for the token. it has to be
		// done backwards for left-to-right reading: eg so
		// 5-3-2 is (5-3)-2 not 5-(3-2)
		
		if (operators[i].direction==forward)
		{
			n = find_operator(start, stop, operators[i].string);
		}
		else
		{
			n = find_operator_backwards(start, stop, operators[i].string);
		}

		if( n != -1)
		{
			// C_Printf("operator %s, %i-%i-%i\n", operators[count].string, start, n, stop);
			
			// call the operator function and evaluate this chunk of tokens
			
			return (this->*(operators[i].handler))(start, n, stop);
		}
    }
	
	if(tokentype[start] == function)
		return evaluate_function(start, stop);
	
	// error ?
	{        
		char tempstr[1024]="";
		
		for(i=start; i<=stop; i++)
			sprintf(tempstr,"%s %s", tempstr, tokens[i]);
		script_error("couldnt evaluate expression: %s\n",tempstr);
		return nullvar;
	}
	
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::script_error(char *s, ...)
{
	va_list args;
	FString tempstr;
	int linenum = 1;
	
	va_start(args, s);
	
	if(killscript) return;  //already killing script
	
	// find the line number
	if(rover >= current_script->data &&
		rover <= current_script->data+current_script->len)
    {
		char *temp;
		for(temp = current_script->data; temp<linestart; temp++)
			if(*temp == '\n') linenum++;    // count EOLs
    }
	
	// print the error
	tempstr.VFormat(s, args);

	Printf(PRINT_BOLD,"FS Error: %s at Script %d, Line %d\n", tempstr.GetChars(), current_script->scriptnum, linenum);
	
	
	// make a noise
	S_Sound(CHAN_VOICE, "script/error", 1, ATTN_STATIC);
	
	killscript = true;
}




// EOF