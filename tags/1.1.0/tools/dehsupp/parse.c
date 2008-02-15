/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included which follows the "include" declaration
** in the input file. */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#line 1 "parse.y"

#include <malloc.h>
#include "dehsupp.h"
#line 14 "parse.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 67
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE struct Token
typedef union {
  ParseTOKENTYPE yy0;
  int yy64;
  int yy133;
} YYMINORTYPE;
#define YYSTACKDEPTH 100
#define ParseARG_SDECL
#define ParseARG_PDECL
#define ParseARG_FETCH
#define ParseARG_STORE
#define YYNSTATE 170
#define YYNRULE 87
#define YYERRORSYMBOL 34
#define YYERRSYMDT yy133
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* Next are that tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   167,  166,  164,  162,  161,  159,  158,  157,  156,  153,
 /*    10 */   152,  150,   56,   22,   24,   26,   23,   28,   27,   87,
 /*    20 */    26,   23,   28,   27,   57,   66,   77,   88,   99,  147,
 /*    30 */   101,   35,   92,   65,   98,   67,   25,   22,   24,   26,
 /*    40 */    23,   28,   27,   24,   26,   23,   28,   27,   83,   97,
 /*    50 */    25,   22,   24,   26,   23,   28,   27,   60,   58,  101,
 /*    60 */    35,  108,   29,   16,   25,   22,   24,   26,   23,   28,
 /*    70 */    27,  100,   53,  117,   31,  140,  106,   25,   22,   24,
 /*    80 */    26,   23,   28,   27,    9,   44,  155,   73,   32,  104,
 /*    90 */    84,   25,   22,   24,   26,   23,   28,   27,   21,   21,
 /*   100 */    63,   48,  169,   85,   95,   17,   17,   52,  145,  137,
 /*   110 */   144,  144,   79,  111,  112,  113,   75,   39,   82,   54,
 /*   120 */    38,   93,   76,   18,   47,   10,   49,   91,   78,   81,
 /*   130 */    72,   89,   28,   27,   62,   80,   71,   30,   32,   31,
 /*   140 */   258,    1,  107,  102,   19,   69,  146,  168,   33,  143,
 /*   150 */    59,  151,  165,  109,    4,  142,   46,   55,  103,   94,
 /*   160 */    41,    8,   51,   45,   43,    2,  114,   50,   42,  141,
 /*   170 */    96,   40,  116,  105,  115,   12,  118,   13,  138,   37,
 /*   180 */    34,   36,  131,  163,   15,  121,  110,  128,  119,   20,
 /*   190 */   139,  149,  126,    5,   86,  160,  148,  136,  120,    7,
 /*   200 */   134,  135,   90,  259,    6,  130,   61,  124,  154,  122,
 /*   210 */   133,   64,  125,  259,  123,  129,   74,   11,  127,   70,
 /*   220 */    14,    3,  259,  259,  259,  132,   68,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    37,   38,   39,   40,   41,   42,   43,   44,   45,   46,
 /*    10 */    47,   48,   10,    2,    3,    4,    5,    6,    7,   17,
 /*    20 */     4,    5,    6,    7,   22,   23,   24,   25,   26,   49,
 /*    30 */    50,   51,   30,   31,   32,   33,    1,    2,    3,    4,
 /*    40 */     5,    6,    7,    3,    4,    5,    6,    7,   13,   34,
 /*    50 */     1,    2,    3,    4,    5,    6,    7,   34,   49,   50,
 /*    60 */    51,   58,   13,   13,    1,    2,    3,    4,    5,    6,
 /*    70 */     7,   34,   57,   58,   51,   12,   21,    1,    2,    3,
 /*    80 */     4,    5,    6,    7,   13,   62,   63,   34,   51,   13,
 /*    90 */    19,    1,    2,    3,    4,    5,    6,    7,    4,    4,
 /*   100 */    34,   64,   65,   34,   34,   11,   11,   54,   14,   15,
 /*   110 */    16,   16,   13,   27,   28,   29,   13,   51,   19,   53,
 /*   120 */    51,   13,   19,   13,   55,   13,   56,   19,   13,   19,
 /*   130 */    34,   19,    6,    7,   19,   34,   13,   13,   51,   51,
 /*   140 */    35,   36,   19,   19,   13,   34,   21,   20,   51,   51,
 /*   150 */    19,   63,   65,   20,   13,   51,   60,   51,   19,   19,
 /*   160 */    51,   18,   61,   52,   51,   18,   20,   51,   51,   51,
 /*   170 */    19,   51,   20,   59,   21,   18,   20,   18,   20,   51,
 /*   180 */    51,   51,   21,   20,   18,   21,   21,   14,   20,   13,
 /*   190 */    21,   20,   20,   11,   19,   21,   12,   20,   20,   18,
 /*   200 */    21,   21,   19,   66,   18,   20,   19,   14,   20,   20,
 /*   210 */    20,   19,   21,   66,   20,   20,   19,   18,   20,   19,
 /*   220 */    18,   18,   66,   66,   66,   20,   19,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 107
static const short yy_shift_ofst[] = {
 /*     0 */    -1,    2,   95,   95,   94,   94,   95,   95,   55,   95,
 /*    10 */    95,  169,  173,  165,  164,  161,   95,   95,   95,   95,
 /*    20 */    86,   95,   95,   95,   95,   95,   95,   95,   95,   95,
 /*    30 */    55,   49,   35,   76,   63,   90,   90,   90,   90,   90,
 /*    40 */    90,   11,   40,   16,   71,   99,  103,  110,  112,  108,
 /*    50 */   126,  115,  123,  124,  131,  126,  182,  181,  184,  185,
 /*    60 */   187,  188,  190,  192,  195,  199,  202,  203,  205,  207,
 /*    70 */   198,  191,  200,  197,  194,  193,  189,  186,  180,  179,
 /*    80 */   183,  178,  177,  174,  171,  175,  168,  166,  159,  163,
 /*    90 */   158,  156,  157,  153,  152,  151,  146,  140,  147,  143,
 /*   100 */   139,  141,  133,  127,  125,   50,  176,  172,
};
#define YY_REDUCE_USE_DFLT (-38)
#define YY_REDUCE_MAX 30
static const short yy_reduce_ofst[] = {
 /*     0 */   105,  -37,   23,   37,  -20,    9,   69,   66,   15,   88,
 /*    10 */    87,  101,   96,   70,   53,  111,  130,  129,  128,  120,
 /*    20 */   114,  118,  117,  116,  113,  109,  106,  104,   98,   97,
 /*    30 */     3,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   171,  170,  247,  253,  185,  185,  218,  208,  228,  257,
 /*    10 */   257,  242,  237,  223,  213,  203,  257,  257,  257,  257,
 /*    20 */   257,  257,  257,  257,  257,  257,  257,  257,  257,  257,
 /*    30 */   257,  257,  257,  257,  257,  189,  231,  220,  219,  209,
 /*    40 */   210,  196,  198,  197,  257,  257,  257,  257,  257,  257,
 /*    50 */   192,  257,  257,  257,  257,  193,  257,  257,  257,  257,
 /*    60 */   257,  257,  257,  257,  257,  257,  257,  257,  257,  257,
 /*    70 */   257,  257,  257,  257,  257,  257,  257,  257,  257,  257,
 /*    80 */   257,  257,  257,  257,  257,  257,  257,  257,  257,  257,
 /*    90 */   257,  257,  257,  257,  257,  257,  257,  257,  257,  257,
 /*   100 */   257,  186,  257,  257,  257,  257,  257,  257,  230,  226,
 /*   110 */   224,  232,  233,  234,  222,  225,  227,  229,  221,  217,
 /*   120 */   216,  214,  235,  212,  239,  215,  211,  236,  238,  207,
 /*   130 */   206,  204,  202,  240,  205,  244,  201,  190,  241,  243,
 /*   140 */   200,  199,  195,  194,  191,  188,  250,  187,  184,  245,
 /*   150 */   183,  249,  182,  181,  246,  248,  180,  179,  178,  177,
 /*   160 */   256,  176,  175,  251,  174,  255,  173,  172,  252,  254,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammer, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  int stateno;       /* The state-number */
  int major;         /* The major token value.  This is the code
                     ** number for the token at this stack level */
  YYMINORTYPE minor; /* The user-supplied minor token value.  This
                     ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "OR",            "XOR",           "AND",         
  "MINUS",         "PLUS",          "MULTIPLY",      "DIVIDE",      
  "NEG",           "EOI",           "PRINT",         "LPAREN",      
  "RPAREN",        "COMMA",         "STRING",        "ENDL",        
  "NUM",           "Actions",       "LBRACE",        "RBRACE",      
  "SEMICOLON",     "SYM",           "OrgHeights",    "ActionList",  
  "CodePConv",     "OrgSprNames",   "StateMap",      "FirstState",  
  "SpawnState",    "DeathState",    "SoundMap",      "InfoNames",   
  "ThingBits",     "RenderStyles",  "error",         "main",        
  "translation_unit",  "external_declaration",  "print_statement",  "actions_def", 
  "org_heights_def",  "action_list_def",  "codep_conv_def",  "org_spr_names_def",
  "state_map_def",  "sound_map_def",  "info_names_def",  "thing_bits_def",
  "render_styles_def",  "print_list",    "print_item",    "exp",         
  "actions_list",  "org_heights_list",  "action_list_list",  "codep_conv_list",
  "org_spr_names_list",  "state_map_list",  "state_map_entry",  "state_type",  
  "sound_map_list",  "info_names_list",  "thing_bits_list",  "thing_bits_entry",
  "render_styles_list",  "render_styles_entry",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= translation_unit",
 /*   1 */ "translation_unit ::=",
 /*   2 */ "translation_unit ::= translation_unit external_declaration",
 /*   3 */ "external_declaration ::= print_statement",
 /*   4 */ "external_declaration ::= actions_def",
 /*   5 */ "external_declaration ::= org_heights_def",
 /*   6 */ "external_declaration ::= action_list_def",
 /*   7 */ "external_declaration ::= codep_conv_def",
 /*   8 */ "external_declaration ::= org_spr_names_def",
 /*   9 */ "external_declaration ::= state_map_def",
 /*  10 */ "external_declaration ::= sound_map_def",
 /*  11 */ "external_declaration ::= info_names_def",
 /*  12 */ "external_declaration ::= thing_bits_def",
 /*  13 */ "external_declaration ::= render_styles_def",
 /*  14 */ "print_statement ::= PRINT LPAREN print_list RPAREN",
 /*  15 */ "print_list ::=",
 /*  16 */ "print_list ::= print_item",
 /*  17 */ "print_list ::= print_item COMMA print_list",
 /*  18 */ "print_item ::= STRING",
 /*  19 */ "print_item ::= exp",
 /*  20 */ "print_item ::= ENDL",
 /*  21 */ "exp ::= NUM",
 /*  22 */ "exp ::= exp PLUS exp",
 /*  23 */ "exp ::= exp MINUS exp",
 /*  24 */ "exp ::= exp MULTIPLY exp",
 /*  25 */ "exp ::= exp DIVIDE exp",
 /*  26 */ "exp ::= exp OR exp",
 /*  27 */ "exp ::= exp AND exp",
 /*  28 */ "exp ::= exp XOR exp",
 /*  29 */ "exp ::= MINUS exp",
 /*  30 */ "exp ::= LPAREN exp RPAREN",
 /*  31 */ "actions_def ::= Actions LBRACE actions_list RBRACE SEMICOLON",
 /*  32 */ "actions_def ::= Actions LBRACE error RBRACE SEMICOLON",
 /*  33 */ "actions_list ::=",
 /*  34 */ "actions_list ::= SYM",
 /*  35 */ "actions_list ::= actions_list COMMA SYM",
 /*  36 */ "org_heights_def ::= OrgHeights LBRACE org_heights_list RBRACE SEMICOLON",
 /*  37 */ "org_heights_def ::= OrgHeights LBRACE error RBRACE SEMICOLON",
 /*  38 */ "org_heights_list ::=",
 /*  39 */ "org_heights_list ::= exp",
 /*  40 */ "org_heights_list ::= org_heights_list COMMA exp",
 /*  41 */ "action_list_def ::= ActionList LBRACE action_list_list RBRACE SEMICOLON",
 /*  42 */ "action_list_def ::= ActionList LBRACE error RBRACE SEMICOLON",
 /*  43 */ "action_list_list ::=",
 /*  44 */ "action_list_list ::= SYM",
 /*  45 */ "action_list_list ::= action_list_list COMMA SYM",
 /*  46 */ "codep_conv_def ::= CodePConv LBRACE codep_conv_list RBRACE SEMICOLON",
 /*  47 */ "codep_conv_def ::= CodePConv LBRACE error RBRACE SEMICOLON",
 /*  48 */ "codep_conv_list ::=",
 /*  49 */ "codep_conv_list ::= exp",
 /*  50 */ "codep_conv_list ::= codep_conv_list COMMA exp",
 /*  51 */ "org_spr_names_def ::= OrgSprNames LBRACE org_spr_names_list RBRACE SEMICOLON",
 /*  52 */ "org_spr_names_def ::= OrgSprNames LBRACE error RBRACE SEMICOLON",
 /*  53 */ "org_spr_names_list ::=",
 /*  54 */ "org_spr_names_list ::= SYM",
 /*  55 */ "org_spr_names_list ::= org_spr_names_list COMMA SYM",
 /*  56 */ "state_map_def ::= StateMap LBRACE state_map_list RBRACE SEMICOLON",
 /*  57 */ "state_map_def ::= StateMap LBRACE error RBRACE SEMICOLON",
 /*  58 */ "state_map_list ::=",
 /*  59 */ "state_map_list ::= state_map_entry",
 /*  60 */ "state_map_list ::= state_map_list COMMA state_map_entry",
 /*  61 */ "state_map_entry ::= SYM COMMA state_type COMMA exp",
 /*  62 */ "state_type ::= FirstState",
 /*  63 */ "state_type ::= SpawnState",
 /*  64 */ "state_type ::= DeathState",
 /*  65 */ "sound_map_def ::= SoundMap LBRACE sound_map_list RBRACE SEMICOLON",
 /*  66 */ "sound_map_def ::= SoundMap LBRACE error RBRACE SEMICOLON",
 /*  67 */ "sound_map_list ::=",
 /*  68 */ "sound_map_list ::= STRING",
 /*  69 */ "sound_map_list ::= sound_map_list COMMA STRING",
 /*  70 */ "info_names_def ::= InfoNames LBRACE info_names_list RBRACE SEMICOLON",
 /*  71 */ "info_names_def ::= InfoNames LBRACE error RBRACE SEMICOLON",
 /*  72 */ "info_names_list ::=",
 /*  73 */ "info_names_list ::= SYM",
 /*  74 */ "info_names_list ::= info_names_list COMMA SYM",
 /*  75 */ "thing_bits_def ::= ThingBits LBRACE thing_bits_list RBRACE SEMICOLON",
 /*  76 */ "thing_bits_def ::= ThingBits LBRACE error RBRACE SEMICOLON",
 /*  77 */ "thing_bits_list ::=",
 /*  78 */ "thing_bits_list ::= thing_bits_entry",
 /*  79 */ "thing_bits_list ::= thing_bits_list COMMA thing_bits_entry",
 /*  80 */ "thing_bits_entry ::= exp COMMA exp COMMA SYM",
 /*  81 */ "render_styles_def ::= RenderStyles LBRACE render_styles_list RBRACE SEMICOLON",
 /*  82 */ "render_styles_def ::= RenderStyles LBRACE error RBRACE SEMICOLON",
 /*  83 */ "render_styles_list ::=",
 /*  84 */ "render_styles_list ::= render_styles_entry",
 /*  85 */ "render_styles_list ::= render_styles_list COMMA render_styles_entry",
 /*  86 */ "render_styles_entry ::= exp COMMA SYM",
};
#endif /* NDEBUG */

#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#if YYSTACKDEPTH<=0
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
#line 10 "parse.y"
{ if ((yypminor->yy0).string) free((yypminor->yy0).string); }
#line 527 "parse.c"
      break;
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor( yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      int iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  if( stateno>YY_REDUCE_MAX ||
	  (i = yy_reduce_ofst[stateno])==YY_REDUCE_USE_DFLT ){
	return yy_default[stateno];
  }
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }else{
	return yy_action[i];
  }
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer ot the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#if YYSTACKDEPTH>0
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," (%d)%s",yypParser->yystack[i].stateno,yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 35, 1 },
  { 36, 0 },
  { 36, 2 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 37, 1 },
  { 38, 4 },
  { 49, 0 },
  { 49, 1 },
  { 49, 3 },
  { 50, 1 },
  { 50, 1 },
  { 50, 1 },
  { 51, 1 },
  { 51, 3 },
  { 51, 3 },
  { 51, 3 },
  { 51, 3 },
  { 51, 3 },
  { 51, 3 },
  { 51, 3 },
  { 51, 2 },
  { 51, 3 },
  { 39, 5 },
  { 39, 5 },
  { 52, 0 },
  { 52, 1 },
  { 52, 3 },
  { 40, 5 },
  { 40, 5 },
  { 53, 0 },
  { 53, 1 },
  { 53, 3 },
  { 41, 5 },
  { 41, 5 },
  { 54, 0 },
  { 54, 1 },
  { 54, 3 },
  { 42, 5 },
  { 42, 5 },
  { 55, 0 },
  { 55, 1 },
  { 55, 3 },
  { 43, 5 },
  { 43, 5 },
  { 56, 0 },
  { 56, 1 },
  { 56, 3 },
  { 44, 5 },
  { 44, 5 },
  { 57, 0 },
  { 57, 1 },
  { 57, 3 },
  { 58, 5 },
  { 59, 1 },
  { 59, 1 },
  { 59, 1 },
  { 45, 5 },
  { 45, 5 },
  { 60, 0 },
  { 60, 1 },
  { 60, 3 },
  { 46, 5 },
  { 46, 5 },
  { 61, 0 },
  { 61, 1 },
  { 61, 3 },
  { 47, 5 },
  { 47, 5 },
  { 62, 0 },
  { 62, 1 },
  { 62, 3 },
  { 63, 5 },
  { 48, 5 },
  { 48, 5 },
  { 64, 0 },
  { 64, 1 },
  { 64, 3 },
  { 65, 3 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  memset(&yygotominor, 0, sizeof(yygotominor));


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 14:
#line 39 "parse.y"
{
	printf ("\n");
  yy_destructor(10,&yymsp[-3].minor);
  yy_destructor(11,&yymsp[-2].minor);
  yy_destructor(12,&yymsp[0].minor);
}
#line 881 "parse.c"
        break;
      case 18:
#line 47 "parse.y"
{ printf ("%s", yymsp[0].minor.yy0.string); }
#line 886 "parse.c"
        break;
      case 19:
#line 48 "parse.y"
{ printf ("%d", yymsp[0].minor.yy64); }
#line 891 "parse.c"
        break;
      case 20:
#line 49 "parse.y"
{ printf ("\n");   yy_destructor(15,&yymsp[0].minor);
}
#line 897 "parse.c"
        break;
      case 21:
#line 52 "parse.y"
{ yygotominor.yy64 = yymsp[0].minor.yy0.val; }
#line 902 "parse.c"
        break;
      case 22:
#line 53 "parse.y"
{ yygotominor.yy64 = yymsp[-2].minor.yy64 + yymsp[0].minor.yy64;   yy_destructor(5,&yymsp[-1].minor);
}
#line 908 "parse.c"
        break;
      case 23:
#line 54 "parse.y"
{ yygotominor.yy64 = yymsp[-2].minor.yy64 - yymsp[0].minor.yy64;   yy_destructor(4,&yymsp[-1].minor);
}
#line 914 "parse.c"
        break;
      case 24:
#line 55 "parse.y"
{ yygotominor.yy64 = yymsp[-2].minor.yy64 * yymsp[0].minor.yy64;   yy_destructor(6,&yymsp[-1].minor);
}
#line 920 "parse.c"
        break;
      case 25:
#line 56 "parse.y"
{ yygotominor.yy64 = yymsp[-2].minor.yy64 / yymsp[0].minor.yy64;   yy_destructor(7,&yymsp[-1].minor);
}
#line 926 "parse.c"
        break;
      case 26:
#line 57 "parse.y"
{ yygotominor.yy64 = yymsp[-2].minor.yy64 | yymsp[0].minor.yy64;   yy_destructor(1,&yymsp[-1].minor);
}
#line 932 "parse.c"
        break;
      case 27:
#line 58 "parse.y"
{ yygotominor.yy64 = yymsp[-2].minor.yy64 & yymsp[0].minor.yy64;   yy_destructor(3,&yymsp[-1].minor);
}
#line 938 "parse.c"
        break;
      case 28:
#line 59 "parse.y"
{ yygotominor.yy64 = yymsp[-2].minor.yy64 ^ yymsp[0].minor.yy64;   yy_destructor(2,&yymsp[-1].minor);
}
#line 944 "parse.c"
        break;
      case 29:
#line 60 "parse.y"
{ yygotominor.yy64 = -yymsp[0].minor.yy64;   yy_destructor(4,&yymsp[-1].minor);
}
#line 950 "parse.c"
        break;
      case 30:
#line 61 "parse.y"
{ yygotominor.yy64 = yymsp[-1].minor.yy64;   yy_destructor(11,&yymsp[-2].minor);
  yy_destructor(12,&yymsp[0].minor);
}
#line 957 "parse.c"
        break;
      case 34:
#line 68 "parse.y"
{ AddAction (yymsp[0].minor.yy0.string); }
#line 962 "parse.c"
        break;
      case 35:
#line 69 "parse.y"
{ AddAction (yymsp[0].minor.yy0.string);   yy_destructor(13,&yymsp[-1].minor);
}
#line 968 "parse.c"
        break;
      case 39:
#line 76 "parse.y"
{ AddHeight (yymsp[0].minor.yy64); }
#line 973 "parse.c"
        break;
      case 40:
#line 77 "parse.y"
{ AddHeight (yymsp[0].minor.yy64);   yy_destructor(13,&yymsp[-1].minor);
}
#line 979 "parse.c"
        break;
      case 44:
#line 84 "parse.y"
{ AddActionMap (yymsp[0].minor.yy0.string); }
#line 984 "parse.c"
        break;
      case 45:
#line 85 "parse.y"
{ AddActionMap (yymsp[0].minor.yy0.string);   yy_destructor(13,&yymsp[-1].minor);
}
#line 990 "parse.c"
        break;
      case 49:
#line 92 "parse.y"
{ AddCodeP (yymsp[0].minor.yy64); }
#line 995 "parse.c"
        break;
      case 50:
#line 93 "parse.y"
{ AddCodeP (yymsp[0].minor.yy64);   yy_destructor(13,&yymsp[-1].minor);
}
#line 1001 "parse.c"
        break;
      case 54:
#line 100 "parse.y"
{ AddSpriteName (yymsp[0].minor.yy0.string); }
#line 1006 "parse.c"
        break;
      case 55:
#line 101 "parse.y"
{ AddSpriteName (yymsp[0].minor.yy0.string);   yy_destructor(13,&yymsp[-1].minor);
}
#line 1012 "parse.c"
        break;
      case 61:
#line 111 "parse.y"
{ AddStateMap (yymsp[-4].minor.yy0.string, yymsp[-2].minor.yy64, yymsp[0].minor.yy64);   yy_destructor(13,&yymsp[-3].minor);
  yy_destructor(13,&yymsp[-1].minor);
}
#line 1019 "parse.c"
        break;
      case 62:
#line 114 "parse.y"
{ yygotominor.yy64 = 0;   yy_destructor(27,&yymsp[0].minor);
}
#line 1025 "parse.c"
        break;
      case 63:
#line 115 "parse.y"
{ yygotominor.yy64 = 1;   yy_destructor(28,&yymsp[0].minor);
}
#line 1031 "parse.c"
        break;
      case 64:
#line 116 "parse.y"
{ yygotominor.yy64 = 2;   yy_destructor(29,&yymsp[0].minor);
}
#line 1037 "parse.c"
        break;
      case 68:
#line 123 "parse.y"
{ AddSoundMap (yymsp[0].minor.yy0.string); }
#line 1042 "parse.c"
        break;
      case 69:
#line 124 "parse.y"
{ AddSoundMap (yymsp[0].minor.yy0.string);   yy_destructor(13,&yymsp[-1].minor);
}
#line 1048 "parse.c"
        break;
      case 73:
#line 131 "parse.y"
{ AddInfoName (yymsp[0].minor.yy0.string); }
#line 1053 "parse.c"
        break;
      case 74:
#line 132 "parse.y"
{ AddInfoName (yymsp[0].minor.yy0.string);   yy_destructor(13,&yymsp[-1].minor);
}
#line 1059 "parse.c"
        break;
      case 80:
#line 142 "parse.y"
{ AddThingBits (yymsp[0].minor.yy0.string, yymsp[-4].minor.yy64, yymsp[-2].minor.yy64);   yy_destructor(13,&yymsp[-3].minor);
  yy_destructor(13,&yymsp[-1].minor);
}
#line 1066 "parse.c"
        break;
      case 86:
#line 152 "parse.y"
{ AddRenderStyle (yymsp[0].minor.yy0.string, yymsp[-2].minor.yy64);   yy_destructor(13,&yymsp[-1].minor);
}
#line 1072 "parse.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = yyact;
      yymsp->major = yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 8 "parse.y"
 yyerror("Syntax error"); 
#line 1132 "parse.c"
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      memset(&yyminorunion, 0, sizeof(yyminorunion));
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
#ifdef YYERRORSYMBOL
      int yymx;
#endif
      assert( yyact == YY_ERROR_ACTION );
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
