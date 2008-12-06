/*
** ds-parser.y
**
** new DECORATE parser grammar for Lemon
**
**---------------------------------------------------------------------------
** Copyright 2008 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/


%token_type    {FToken}
%token_prefix	DS_
%extra_argument { Baggage *context }
%syntax_error { context->ScriptPosition.Message(MSG_ERROR, "Syntax Error"); }
%name DSParse



// main
// ----
	main ::= translation_unit.



// translation_unit
// ----------------
	translation_unit ::= . /* empty */
	translation_unit ::= translation_unit toplevel_declaration.

// ===========================================================================
//
// Top level definitions can be:
//	Constants
//	Enums
//	Classes
//  Functions
//	The semicolon is treated as an empty declaration so that it can be
//	optionally added after closing braces.
//
// ===========================================================================

// toplevel_declaration
// --------------------

	toplevel_declaration ::= SEMICOLON.
	toplevel_declaration ::= constant_definition(A).
	{
		DefineGlobalConstant(A);
	}

	toplevel_declaration ::= enum_definition(A).
	{
		DefineGlobalConstant(A);
	}

	toplevel_declaration ::= class_definition(A).
	{
		A->DefineClass(*context);
	}

	/*
	toplevel_declaration ::= global_function_prototype(A).
	{
		DefineGlobalFunction(A);
	}
	*/


// ===========================================================================
//
// Constant definition
//
// ===========================================================================

%type constant_definition { FsConstant* }
%destructor constant_definition { delete $$; }

	constant_definition(A) ::= CONST const_type_expression(B) IDENTIFIER(C) ASSIGN value_expression(D) SEMICOLON.
	{
		A = new FsConstant(B, C.NameValue(), D, C.ScriptPosition());
	}

// ===========================================================================
//
// Enum definition
//
// ===========================================================================

%type enum_definition { FsEnum* }
%destructor enum_definition { delete $$; }
%type enum_list { FsEnum* }
%destructor enum_list { delete $$; }
%type enum_line { FsConstant* }
%destructor enum_line { delete $$; }

// enum_definition
// ---------------

	enum_definition(A) ::= ENUM LBRACE enum_list(B) RBRACE.
	{
		A = B;
	}

	enum_definition(A) ::= ENUM LBRACE enum_list(B) COMMA RBRACE.
	{
		A = B;
	}

// enum_list
// ---------

	enum_list(A) ::= enum_list(B) COMMA enum_line(C).
	{
		B->Append(C);
		A = B;
	}

	enum_list(A) ::= enum_line(B).
	{
		A = new FsEnum;
		A->Append(B);
	}

// enum_line
// ---------

	enum_line(A) ::= IDENTIFIER(B) ASSIGN value_expression(C).
	{
		A = new FsConstant(TK_Int, B.NameValue(), C, B.ScriptPosition());
	}

	enum_line(A) ::= IDENTIFIER(B).
	{
		A = new FsConstant(TK_Int, B.NameValue(), NULL, B.ScriptPosition());
	}

// ===========================================================================
//
// Class definition
//
// ===========================================================================

%type class_definition { FsClass* }
%destructor class_definition { delete $$; }
%type class_header { FsClass* }
%destructor class_header { delete $$; }
%type class_body { FsClass* } // intentionally no destructor!

// class_definition
// ----------------

	class_definition(A) ::= class_header(B) LBRACE class_body RBRACE.
	{
		A = B;
		context->Class = NULL;
	}

// class_header
// ------------

	class_header(A) ::= CLASS quotable_identifier(B) maybe_native(C).
	{
		A = new FsClass(B.NameValue(), NULL, false, !!C, B.ScriptPosition(), context);
	}

	class_header(A) ::= CLASS quotable_identifier(B) COLON quotable_identifier(C) maybe_native(D).
	{
		A = new FsClass(B.NameValue(), C.NameValue(), false, !!D, B.ScriptPosition(), context);
	}

	class_header(A) ::= ACTOR quotable_identifier(B) maybe_native(C).
	{
		A = new FsClass(B.NameValue(), NAME_None, true, !!C, B.ScriptPosition(), context);
	}

	class_header(A) ::= ACTOR quotable_identifier(B) COLON quotable_identifier(C) maybe_native(D).
	{
		A = new FsClass(B.NameValue(), C.NameValue(), true, !!D, B.ScriptPosition(), context);
	}

// class_body
// ----------

	class_body(A) ::= .
	{
		A = context->GetClass();
	}


	class_body(A) ::= class_body(B) constant_definition(C).
	{
		B->DefineConstant(C);
		A = B;
	}

	class_body(A) ::= class_body(B) enum_definition(C).
	{
		B->DefineConstant(C);
		A = B;
	}

	class_body(A) ::= class_body(B) info_definition.
	{
		A = B;
	}

	class_body(A) ::= class_body(B) properties_definition.
	{
		A = B;
	}

	/*
	class_body(A) ::= class_body(B) function_prototype(C).
	{
		B->AddFunction(C);
		A = B;
	}
	*/

	class_body(A) ::= class_body(B) states_definition.
	{
		A = B;
	}

// ===========================================================================
//
// Info block definition
//
// ===========================================================================

%type info_body { FsClass* }	// intentionally no destructor


info_definition ::= INFO LBRACE info_body RBRACE.



// info_body
// ---------
	
info_body(A) ::= .
{
	A = context->GetClass();
}

info_body(A) ::= info_body(B) IDENTIFIER(C) LPAREN property_args(D) RPAREN.
{
	B->AddProperty(*context, NAME_None, C.NameValue(), D, C.ScriptPosition(), true);
	delete D;
	A = B;
}


// ===========================================================================
//
// Property block definition
//
// ===========================================================================

%type prop_body { FsClass* }	// intentionally no destructor


properties_definition ::= DEFAULTPROPERTIES LBRACE prop_body RBRACE.
	

// prop_body
// ---------
	
	prop_body(A) ::= .
	{
		A = context->GetClass();
	}

	prop_body(A) ::= prop_body(B) property_identifier(C) LPAREN RPAREN.
	{
		B->AddProperty(*context, NAME_None, C.NameValue(), NULL, C.ScriptPosition(), false);
		A = B;
	}

	prop_body(A) ::= prop_body(B) property_identifier(C) DOT property_identifier(D) LPAREN RPAREN.
	{
		B->AddProperty(*context, C.NameValue(), D.NameValue(), NULL, C.ScriptPosition(), false);
		A = B;
	}

	prop_body(A) ::= prop_body(B) property_identifier(C) LBRACKET value_expression(D) RBRACKET.
	{
		B->AddExpressionProperty(*context, NAME_None, C.NameValue(), D, C.ScriptPosition());
		A = B;
	}

	prop_body(A) ::= prop_body(B) property_identifier(C) LPAREN property_args(D) RPAREN.
	{
		B->AddProperty(*context, NAME_None, C.NameValue(), D, C.ScriptPosition(), false);
		delete D;
		A = B;
	}

	prop_body(A) ::= prop_body(B) property_identifier(C) DOT property_identifier(D) LPAREN property_args(E) RPAREN.
	{
		B->AddProperty(*context, C.NameValue(), D.NameValue(), E, C.ScriptPosition(), false);
		delete E;
		A = B;
	}

	prop_body(A) ::= prop_body(B) PLUS property_identifier(C).
	{
		B->AddFlag(NAME_Actor, C.NameValue(), true, C.ScriptPosition());
		A = B;
	}

	prop_body(A) ::= prop_body(B) MINUS property_identifier(C).
	{
		B->AddFlag(NAME_Actor, C.NameValue(), false, C.ScriptPosition());
		A = B;
	}

	prop_body(A) ::= prop_body(B) PLUS property_identifier(C) DOT property_identifier(D).
	{
		B->AddFlag(C.NameValue(), D.NameValue(), true, C.ScriptPosition());
		A = B;
	}

	prop_body(A) ::= prop_body(B) MINUS property_identifier(C) DOT property_identifier(D).
	{
		B->AddFlag(C.NameValue(), D.NameValue(), false, C.ScriptPosition());
		A = B;
	}

	prop_body(A) ::= prop_body(B) error.
	{
		A = B;
		context->ScriptPosition.Message(MSG_ERROR, "Invalid property definition");
	}

// ===========================================================================
//
// Property arguments
//
// ===========================================================================


%type property_args { FPropArgs* }
%destructor property_args { delete $$; }

// property_args
// -------------

	property_args(A) ::= value_expression(B).
	{
		A = new FPropArgs;
		A->Push(B);
	}

	property_args(A) ::= property_args(B) COMMA value_expression(C).
	{
		B->Push(C);
		A = B;
	}

// ===========================================================================
//
// Function modifiers
//
// ===========================================================================

%type maybe_native { int }

	maybe_native(A) ::=.
	{
		A = 0;
	}

	maybe_native(A) ::= NATIVE.
	{
		A = 1;
	}

// ===========================================================================
//
// Quotable identifiers are needed so that actor classes can use non
// alphanumeric characters. This was allowed previously and is needed
// for compatibility.
//
// ===========================================================================

%type quotable_identifier { FToken }
 
	quotable_identifier(A) ::= IDENTIFIER(B).
	{
		A = B;
	}

	quotable_identifier(A) ::= STRINGCONST(B).
	{
		A = B.StringToIdentifier();
	}

	// 'Actor' needs to be both a valid identifier for class names and a keyword.
	quotable_identifier(A) ::= ACTOR(B).	
	{
		A = B.MakeIdentifier(NAME_Actor);
	}

// ===========================================================================
//
// For the property parser we also need some keywords as valid identifiers
//
// ===========================================================================

%type property_identifier { FToken }

	property_identifier(A) ::= IDENTIFIER(B).
	{
		A = B;
	}

	property_identifier(A) ::= COLOR(B).
	{
		A = B.MakeIdentifier(NAME_Color);
	}

	property_identifier(A) ::= FLOAT(B).
	{
		A = B.MakeIdentifier(NAME_Float);
	}

	property_identifier(A) ::= PROJECTILE(B).
	{
		A = B.MakeIdentifier(NAME_Projectile);
	}


// ===========================================================================
//
// basic types are those that can be used for constants.
//
// ===========================================================================

%type const_type_expression { int }

	const_type_expression(A) ::= basic_type_expression(B).
	{
		A = B;
	}

	const_type_expression(A) ::= STRING.
	{
		A = VAL_String;
	}

	/*
	const_type_expression(A) ::= VECTOR.
	{
		A = TK_Vector;
	}
	*/

%type basic_type_expression { int }

	basic_type_expression(A) ::= INT.
	{
		A = VAL_Int;
	}

	basic_type_expression(A) ::= FLOAT.
	{
		A = VAL_Float;
	}

	basic_type_expression(A) ::= BOOL.
	{
		A = VAL_Int;
	}

// ===========================================================================
//
// Values (constants and expressions)
//
// ===========================================================================

%right ASSIGN ADDASSIGN SUBASSIGN MULASSIGN DIVASSIGN MODASSIGN ANDASSIGN ORASSIGN XORASSIGN LSHASSIGN RSHASSIGN URSHASSIGN.
%left QUESTION.
%left OROR.
%left ANDAND.
%left OR.
%left XOR.
%left AND.
%left EQ NE.
%left GT GE LT LE.
%left LSHIFT RSHIFT URSHIFT.
%left PLUS MINUS.
%left MUL DIVIDE MOD.
%left INCR DECR.
%right TILDE NOT.
%left DOT.

%type value_expression { FxExpression* }
%destructor value_expression { delete $$; }
%type value_expression_1 { FxExpression* }
%destructor value_expression_1 { delete $$; }
%type value_expression_2 { FxExpression* }
%destructor value_expression_2 { delete $$; }
%type value_expression_3 { FxExpression* }
%destructor value_expression_3 { delete $$; }
%type value_expression_4 { FxExpression* }
%destructor value_expression_4 { delete $$; }
%type value_expression_5 { FxExpression* }
%destructor value_expression_5 { delete $$; }
%type value_expression_6 { FxExpression* }
%destructor value_expression_6 { delete $$; }
%type value_expression_7 { FxExpression* }
%destructor value_expression_7 { delete $$; }
%type value_expression_8 { FxExpression* }
%destructor value_expression_8 { delete $$; }
%type value_expression_9 { FxExpression* }
%destructor value_expression_9 { delete $$; }
%type value_expression_10 { FxExpression* }
%destructor value_expression_10 { delete $$; }
%type value_expression_11 { FxExpression* }
%destructor value_expression_11 { delete $$; }
%type value_expression_12 { FxExpression* }
%destructor value_expression_12 { delete $$; }
%type value_expression_13 { FxExpression* }
%destructor value_expression_13 { delete $$; }
%type value_expression_14 { FxExpression* }
%destructor value_expression_14 { delete $$; }

value_expression(A) ::= value_expression_1(B).
{
	A = B;
}

// first level: assignment operators
// ---------------------------------

	/* (disabled for now!)
	value_expression_1(A) ::= value_expression_2(B) ASSIGN value_expression_1(C).
	{
		A = new FxAssignment('=', B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) ADDASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_AddEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) SUBASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_SubEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) MULASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_MulEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) DIVASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_DivEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) MODASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_ModEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) ANDASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_AndEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) ORASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_OrEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) XORASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_XorEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) LSHASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_LShiftEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) RSHASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_RShiftEq, B, C);
	}

	value_expression_1(A) ::= value_expression_2(B) URSHASSIGN value_expression_1(C).
	{
		A = new FxAssignment(TK_URShiftEq, B, C);
	}
	*/

	value_expression_1(A) ::= value_expression_2(B).
	{
		A = B;
	}

// Level 2: Conditionals
// ---------------------

	value_expression_2(A) ::= value_expression_3(B) QUESTION value_expression_3(C) COLON value_expression_2(D).
	{
		A = new FxConditional(B, C, D);
	}

	value_expression_2(A) ::= value_expression_3(B).
	{
		A = B;
	}


// Level 3: ||
// -----------

	value_expression_3(A) ::= value_expression_3(B) OROR value_expression_4(C).
	{
		A = new FxBinaryLogical(TK_OrOr, B, C);
	}


	value_expression_3(A) ::= value_expression_4(B).
	{
		A = B;
	}

// Level 4: &&
// -----------

	value_expression_4(A) ::= value_expression_4(B) ANDAND value_expression_5(C).
	{
		A = new FxBinaryLogical(TK_AndAnd, B, C);
	}

	value_expression_4(A) ::= value_expression_5(B).
	{
		A = B;
	}

// Level 5: |
// ----------

	value_expression_5(A) ::= value_expression_5(B) OR value_expression_6(C).
	{
		A = new FxBinaryInt('|', B, C);
	}

	value_expression_5(A) ::= value_expression_6(B).
	{
		A = B;
	}

// Level 6: ^
// ----------

	value_expression_6(A) ::= value_expression_6(B) XOR value_expression_7(C).
	{
		A = new FxBinaryInt('^', B, C);
	}

	value_expression_6(A) ::= value_expression_7(B).
	{
		A = B;
	}

// Level 7: &
// ----------

	value_expression_7(A) ::= value_expression_7(B) AND value_expression_8(C).
	{
		A = new FxBinaryInt('&', B, C);
	}

	value_expression_7(A) ::= value_expression_8(B).
	{
		A = B;
	}

// Level 8: ==, !=
// ---------------

	value_expression_8(A) ::= value_expression_8(B) EQ value_expression_9(C).
	{
		A = new FxCompareEq(TK_Eq, B, C);
	}

	value_expression_8(A) ::= value_expression_8(B) NE value_expression_9(C).
	{
		A = new FxCompareEq(TK_Neq, B, C);
	}

	value_expression_8(A) ::= value_expression_9(B).
	{
		A = B;
	}

// Level 9: <, >, <=, >=
// ---------------------

	value_expression_9(A) ::= value_expression_9(B) GT value_expression_10(C).
	{
		A = new FxCompareRel('>', B, C);
	}

	value_expression_9(A) ::= value_expression_9(B) GE value_expression_10(C).
	{
		A = new FxCompareRel(TK_Geq, B, C);
	}

	value_expression_9(A) ::= value_expression_9(B) LT value_expression_10(C).
	{
		A = new FxCompareRel('<', B, C);
	}

	value_expression_9(A) ::= value_expression_9(B) LE value_expression_10(C).
	{
		A = new FxCompareRel(TK_Leq, B, C);
	}

	value_expression_9(A) ::= value_expression_10(B).
	{
		A = B;
	}

// Level 10: Shifting
// ------------------

	value_expression_10(A) ::= value_expression_10(B) LSHIFT value_expression_11(C).
	{
		A = new FxBinaryInt(TK_LShift, B, C);
	}

	value_expression_10(A) ::= value_expression_10(B) RSHIFT value_expression_11(C).
	{
		A = new FxBinaryInt(TK_RShift, B, C);
	}

	value_expression_10(A) ::= value_expression_10(B) URSHIFT value_expression_11(C).
	{
		A = new FxBinaryInt(TK_URShift, B, C);
	}

	value_expression_10(A) ::= value_expression_11(B).
	{
		A = B;
	}

// Level 11: +, -
// --------------

	value_expression_11(A) ::= value_expression_11(B) PLUS value_expression_12(C).
	{
		A = new FxAddSub('+', B, C);
	}

	value_expression_11(A) ::= value_expression_11(B) MINUS value_expression_12(C).
	{
		A = new FxAddSub('-', B, C);
	}

	value_expression_11(A) ::= value_expression_12(B).
	{
		A = B;
	}

// Level 12: *, /, %
// -----------------

	value_expression_12(A) ::= value_expression_12(B) MUL value_expression_13(C).
	{
		A = new FxMulDiv('*', B, C);
	}

	value_expression_12(A) ::= value_expression_12(B) DIVIDE value_expression_13(C).
	{
		A = new FxMulDiv('/', B, C);
	}

	value_expression_12(A) ::= value_expression_12(B) MOD value_expression_13(C).
	{
		A = new FxMulDiv('%', B, C);
	}

	value_expression_12(A) ::= value_expression_13(B).
	{
		A = B;
	}

// Level 13: Unary
// ---------------

	value_expression_13(A) ::= TILDE value_expression_13(B).
	{
		A = new FxUnaryNotBitwise(B);
	}

	value_expression_13(A) ::= NOT value_expression_13(B).
	{
		A = new FxUnaryNotBoolean(B);
	}

	value_expression_13(A) ::= MINUS value_expression_13(B).
	{
		A = new FxMinusSign(B);
	}

	value_expression_13(A) ::= PLUS value_expression_13(B).
	{
		A = new FxPlusSign(B);
	}

	/*
	value_expression_13(A) ::= MUL value_expression_13(B).
	{
		A = new FxPointerDereference(B);
	}

	value_expression_13(A) ::= AND value_expression_13(B).
	{
		A = new FxAddress(B);
	}

	value_expression_13(A) ::= INCR value_expression_13(B).
	{
		A = new FxPrePost(TK_Incr, B);
	}

	value_expression_13(A) ::= DECR value_expression_13(B).
	{
		A = new FxPrePost(TK_Decr, B);
	}
	*/

	value_expression_13(A) ::= value_expression_14(B).
	{
		A = B;
	}

// Level 14: lowest
// ----------------

	/*
	value_expression_14(A) ::= value_expression_14(B) INCR.
	{
		A = new FxPrePost(-TK_Incr, B);
	}

	value_expression_14(A) ::= value_expression_14(B) DECR.
	{
		A = new FxPrePost(-TK_Decr, B);
	}
	*/

	value_expression_14(A) ::= LPAREN value_expression(B) RPAREN.
	{
		A = B;
	}

	value_expression_14(A) ::= LPAREN error RPAREN.
	{
		A = new FxConstant(0, context->ScriptPosition);
		context->ScriptPosition.Message(MSG_ERROR, "Error in expression");
	}

	value_expression_14(A) ::= INT LPAREN value_expression(B) RPAREN.
	{
		A = new FxIntCast(B);
	}

	value_expression_14(A) ::= FLOAT LPAREN value_expression(B) RPAREN.
	{
		A = new FxFloatCast(B);
	}

	/*
	value_expression_14(A) ::= VECTOR LPAREN value_expression(B) COMMA value_expression(C) COMMA value_expression(D) RPAREN.
	{
		A = new FxVector(B, C, D);
	}
	*/

	value_expression_14(A) ::= ABS LPAREN value_expression(B) RPAREN.
	{
		A = new FxAbs(B);
	}

	value_expression_14(A) ::= RANDOM LPAREN value_expression(B) COMMA value_expression(C) RPAREN.
	{
		A = new FxRandom(NULL, B, C, B->ScriptPosition);
	}

	value_expression_14(A) ::= RANDOM LBRACKET IDENTIFIER(I) RBRACKET LPAREN value_expression(B) COMMA value_expression(C) RPAREN.
	{
		A = new FxRandom(MakeRNG(I), B, C, B->ScriptPosition);
	}

	value_expression_14(A) ::= RANDOM(R) LPAREN RPAREN.
	{
		A = new FxRandom(NULL, NULL, NULL, R.ScriptPosition());
	}

	value_expression_14(A) ::= RANDOM LBRACKET IDENTIFIER(I) RBRACKET LPAREN RPAREN.
	{
		A = new FxRandom(MakeRNG(I), NULL, NULL, I.ScriptPosition());
	}

	value_expression_14(A) ::= RANDOM2 LPAREN value_expression(B) RPAREN.
	{
		A = new FxRandom2(NULL, B, B->ScriptPosition);
	}

	value_expression_14(A) ::= RANDOM2 LBRACKET IDENTIFIER(I) RBRACKET LPAREN value_expression(B) RPAREN.
	{
		A = new FxRandom2(MakeRNG(I), B, B->ScriptPosition);
	}

	value_expression_14(A) ::= RANDOM2(R) LPAREN RPAREN.
	{
		A = new FxRandom2(NULL, NULL, R.ScriptPosition());
	}

	value_expression_14(A) ::= RANDOM2 LBRACKET IDENTIFIER(I) RBRACKET LPAREN RPAREN.
	{
		A = new FxRandom2(MakeRNG(I), NULL, I.ScriptPosition());
	}

	/*
	value_expression_14(A) ::= CLASS LT IDENTIFIER(B) GT LPAREN value_expression(C) RPAREN.
	{
		A = new FxDynamicCast(&B, C);
	}

	value_expression_14(A) ::= IDENTIFIER(B) LPAREN arguments(C) RPAREN.
	{
		A = new FxFunctionCall(NULL, &B, false, false, C);
	}

	value_expression_14(A) ::= DCOLON IDENTIFIER(B) LPAREN arguments(C) RPAREN.
	{
		A = new FxFunctionCall(NULL, &B, false, true, C);
	}

	value_expression_14(A) ::= SUPER DOT IDENTIFIER(B) LPAREN arguments(C) RPAREN.
	{
		A = new FxFunctionCall(NULL, &B, true, false, C);
	}

	value_expression_14(A) ::= value_expression_14(self) DOT IDENTIFIER(B) LPAREN arguments(C) RPAREN.
	{
		A = new FxFunctionCall(self, &B, false, false, C);
	}

	value_expression_14(A) ::= DEFAULT DOT IDENTIFIER(B).
	{
		FxExpression *Expr = new FxClassDefaults(new FxSelf(B.ScriptPosition()), B.ScriptPosition());
		A = new FxDotIdentifier(Expr, &B);
	}

	value_expression_14(A) ::= value_expression_14(B) DOT DEFAULT.
	{
		A = new FxClassDefaults(B, B->ScriptPosition);
	}

	value_expression_14(A) ::= value_expression_14(B) DOT IDENTIFIER(C).
	{
		A = new FxDotIdentifier(B, &C);
	}

	value_expression_14(A) ::= IDENTIFIER(B) DCOLON IDENTIFIER(C).
	{
		A = new FxScopeIdentifier(&B, &C);
	}

	value_expression_14(A) ::= DCOLON IDENTIFIER(C).
	{
		A = new FxScopeIdentifier(NULL, &C);
	}


	value_expression_14(A) ::= SELF(B).
	{
		A = new FxSelf(B.ScriptPosition());
	}

	value_expression_14(A) ::= NULL(B).
	{
		A = new FxNull(B.ScriptPosition());
	}
	*/

	value_expression_14(A) ::= TRUE(B).
	{
		A = new FxConstant(1, B.ScriptPosition());
	}

	value_expression_14(A) ::= FALSE(B).
	{
		A = new FxConstant(0, B.ScriptPosition());
	}

	value_expression_14(A) ::= IDENTIFIER(B).
	{
		A = new FxIdentifier(B.NameValue(), B.ScriptPosition());
	}

	value_expression_14(A) ::= STRINGCONST(B).
	{
		A = new FxStringConstant(B.StringValue(), B.ScriptPosition());
	}

	value_expression_14(A) ::= FLOATCONST(B).
	{
		A = new FxConstant(B.FloatValue(), B.ScriptPosition());
	}

	value_expression_14(A) ::= INTCONST(B).
	{
		A = new FxConstant(B.IntValue(), B.ScriptPosition());
	}


// ===========================================================================
//
// States
//
// ===========================================================================


%type state_label { FString* }
%destructor state_label { delete $$; }
%type maybe_bright { bool }
%include { struct offsetxy { int x,y; }; }
%type maybe_offset { offsetxy }
%type maybe_codeptr { CodePtr* }
%destructor maybe_codeptr { delete $$; }
%type codeptr_paramlist { CodePtr* }
%destructor codeptr_paramlist { delete $$; }

states_definition ::= STATES LBRACE stateblock RBRACE.

// stateblock
// ----------

	stateblock ::= .
	{
		if (context->StateSet) context->ScriptPosition.Message(MSG_FATAL, "Multiple state declarations not allowed");
		context->StateSet=true;
	}

	stateblock ::= stateblock state.

	stateblock ::= stateblock state_label(A) COLON.
	{
		context->statedef.AddStateLabel(*A);
		delete A;
	}

	stateblock ::= stateblock STOP.
	{
		context->statedef.SetStop();
	}

	stateblock ::= stateblock WAIT.
	{
		context->statedef.SetWait();
	}

	stateblock ::= stateblock FAIL.
	{
		context->statedef.SetWait();
	}

	stateblock ::= stateblock LOOP.
	{
		context->statedef.SetLoop();
	}

	stateblock ::= stateblock GOTO state_label(A).
	{
		context->statedef.SetGotoLabel(*A);
		delete A;
	}

	stateblock ::= stateblock GOTO state_label(A) PLUS INTCONST(B).
	{
		A->AppendFormat("+%d", B.IntValue());
		context->statedef.SetGotoLabel(*A);
		delete A;
	}

// state_label
// -----------

	state_label(A) ::= IDENTIFIER(B).
	{
		A = new FString(B.NameValue());
	}

	state_label(A) ::= IDENTIFIER(B) DCOLON IDENTIFIER(C).
	{
		A = new FString;
		A->Format("%s::%s", B.NameValue().GetChars(), C.NameValue().GetChars());
	}

	state_label(A) ::= SUPER DCOLON IDENTIFIER(C).
	{
		A = new FString;
		A->Format("super::%s", C.NameValue().GetChars());
	}

	state_label(A) ::= ACTOR DCOLON IDENTIFIER(C).
	{
		A = new FString;
		A->Format("actor::%s", C.NameValue().GetChars());
	}

	state_label(A) ::= state_label(B) DOT IDENTIFIER(C).
	{
		A = B;
		(*A) << '.' << C.NameValue().GetChars();
	}

// state
// -----

	state ::= quotable_identifier(Sprite) quotable_identifier(frame) value_expression(tics) maybe_bright(b) maybe_offset(xy) maybe_codeptr(codeptr) SEMICOLON.
	{
		FState state;
		
		state.sprite = GetSpriteIndex(Sprite.NameValue());
		state.Misc1 = xy.x; 
		state.Misc2 = xy.y;
		state.Frame = b? SF_FULLBRIGHT:0;
		state.DefineFlags = 0;
		state.NextState = NULL;

		FCompileContext ctx(context->Info->Class);
		tics = tics->CreateCast(ctx, VAL_Int);
		state.Tics = tics==NULL? 0 : clamp<int>(tics->EvalExpression(NULL).GetInt(), -1, 32767);
		SAFE_DELETE(tics);

		InstallCodePtr(&state, codeptr, context->Info->Class, Sprite.ScriptPosition());	
		if (codeptr != NULL) delete codeptr;
		context->statedef.AddStates(&state, frame.NameValue());
	}

	state ::= quotable_identifier error SEMICOLON.
	{
		context->ScriptPosition.Message(MSG_ERROR, "Error in frame definition for state");
	}

	state ::= quotable_identifier quotable_identifier error SEMICOLON.
	{
		context->ScriptPosition.Message(MSG_ERROR, "numeric expression expected for duration");
	}

	state ::= error SEMICOLON.
	{
		context->ScriptPosition.Message(MSG_ERROR, "Error in state definition");
	}

// state helpers
// -------------

	maybe_bright(A) ::= .
	{
		A = false;
	}

	maybe_bright(A) ::= BRIGHT.
	{
		A = true;
	}

	maybe_offset(A) ::= .
	{
		A.x = A.y = 0;
	}

	maybe_offset(A) ::= OFFSET LPAREN value_expression(X) COMMA value_expression(Y) RPAREN.
	{
		FCompileContext ctx(context->Info->Class);

		X = X->CreateCast(ctx, VAL_Int);
		A.x = X == NULL? 0 : X->EvalExpression(NULL).GetInt();
		SAFE_DELETE(X);

		Y = Y->CreateCast(ctx, VAL_Int);
		A.y = Y == NULL? 0 : Y->EvalExpression(NULL).GetInt();
		SAFE_DELETE(Y);
	}

	maybe_offset(A) ::= OFFSET LPAREN error RPAREN.
	{
		A.x = A.y = 0;
		context->ScriptPosition.Message(MSG_ERROR, "Error in offset definition for state");
	}


	maybe_codeptr(A) ::= .
	{
		A = NULL;
	}

	maybe_codeptr(A) ::= IDENTIFIER(B).
	{
		A = new CodePtr;
		A->funcname = B.NameValue();
	}

	maybe_codeptr(A) ::= IDENTIFIER(B) LPAREN codeptr_paramlist(C) RPAREN.
	{
		A = C;
		A->funcname = B.NameValue();
	}

	maybe_codeptr(A) ::= IDENTIFIER LPAREN error RPAREN.
	{
		A = NULL;
		context->ScriptPosition.Message(MSG_ERROR, "Error in action function parameters");
	}

// codeptr parameters
// ------------------

	codeptr_paramlist(A) ::= value_expression(B).
	{
		A = new CodePtr;
		A->parameters.Push(B);
	}

	codeptr_paramlist(A) ::= codeptr_paramlist(C) COMMA value_expression(B).
	{
		A = C;
		A->parameters.Push(B);
	}
