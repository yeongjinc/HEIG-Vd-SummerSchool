/*------------------------------------------------------------------------------
  HEIG-Vd - CoE@SNU              Summer University              July 11-22, 2016

  The Art of Compiler Construction


  suPL parser definition

*/

%locations

%code top{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YYDEBUG 1

extern char *yytext;
}

%code requires {
#include "supllib.h"
}

%union {
  long int n;
  char     *str;
  IDlist   *idl;
  EType    t;
  Funclist *fl;
  BPrecord *bp;
  EOpcode  op;
}

%code {
  Stack   *stack = NULL;
  Symtab *symtab = NULL;
  CodeBlock *cb  = NULL;

  char *fn_pfx   = NULL;
  EType rettype  = tVoid;
}

%start program

%token INTEGER VOID
%token NUMBER
%token IDENT
%token STRING

%token IF
%token ELSE
%token WHILE
%token RETURN
%token READ
%token WRITE
%token PRINT

%left '+' '-'
%left '*' '/'
%right '='
//TODO conditional operator

%type<n>    NUMBER number
%type<str>  ident IDENT string STRING call
%type<idl>  identl vardecl
%type<t>    type
%type<op> 	condition
%type<bpr> 	IF WHILE

%%

program     :                                 { stack = init_stack(NULL); symtab = init_symtab(stack, NULL); }
              decll                           { cb = init_codeblock("");
                                                stack = init_stack(stack); symtab = init_symtab(stack, symtab);
                                                rettype = tVoid;
                                              }
              stmtblock                       { add_op(cb, opHalt, NULL);
                                                dump_codeblock(cb); save_codeblock(cb, fn_pfx);
                                                Stack *pstck = stack; stack = stack->uplink; delete_stack(pstck);
                                                Symtab *pst = symtab; symtab = symtab->parent; delete_symtab(pst);
                                              }
            ;

decll       : %empty
            | decll vardecl ';'               { delete_idlist($vardecl); }
			| decll fundecl
            ;

vardecl     : type identl                     {
                                                IDlist *l = $identl;
                                                while (l) {
                                                  if (insert_symbol(symtab, l->id, $type) == NULL) {
                                                    char *error = NULL;
                                                    asprintf(&error, "Duplicated identifier '%s'.", l->id);
                                                    yyerror(error);
                                                    free(error);
                                                    YYABORT;
                                                  }
                                                  l = l->next;
                                                }
                                                $$ = $identl;
                                              }
            ;

type        : INTEGER                         { $$ = tInteger; }
            | VOID                            { $$ = tVoid; }
            ;

fundecl 	: type ident '(' ')' stmtblock
		 	| type ident '(' vardecl ')' stmtblock
			;

identl      : ident                           { $$ = (IDlist*)calloc(1, sizeof(IDlist)); $$->id = $ident; }
            | identl ',' ident                { $$ = (IDlist*)calloc(1, sizeof(IDlist)); $$->id = $ident; $$->next = $1; }
            ;

exprl 		: expression
			| exprl ',' expression
			;

stmtl 		: %empty
			| stmtl stmt
			;

stmtblock   :
              '{' stmtl '}'
            ;

stmt 		: vardecl ';' 					  { delete_idlist($vardecl); }
	   		| assign 						  { /* do nothing */ }
			| if
			| while
			| call ';'
			| return
			| read
			| write
			| print
			;

assign 		: ident '=' expression ';' 		{
												Symbol* symbol = find_symbol(symtab, $ident, sGlobal);
											  	if(symbol == NULL) {
												  char *error = NULL;
                                                  asprintf(&error, "Undeclared identifier '%s'.", $ident);
                                                  yyerror(error);
                                                  free(error);
                                                  YYABORT;
											    }
												add_op(cb, opStore, symbol);
		 									}
		 	;

if 			: IF '(' condition ')' stmtblock 		{ printf("IF\n"); }
	  		| IF '(' condition ')' stmtblock ELSE stmtblock
			;

while 		: WHILE '(' condition ')' stmtblock
			;

call 		: ident '(' ')'
	   		| ident '(' exprl ')'
			;

return 		: RETURN ';'
		 	| RETURN expression ';'
			;

read 		: READ ident ';' 				{
	   											Symbol* symbol = find_symbol(symtab, $ident, sGlobal);
											  	if(symbol == NULL) {
												  char *error = NULL;
                                                  asprintf(&error, "Undeclared identifier '%s'.", $ident);
                                                  yyerror(error);
                                                  free(error);
                                                  YYABORT;
											    }
												add_op(cb, opRead, symbol);
	   										}
	   		;

write 		: WRITE expression ';' 			{ add_op(cb, opWrite, NULL); }
			;

print 		: PRINT string ';' 				{ add_op(cb, opPrint, (void *)$string); }
			;

expression 	: number 						{ add_op(cb, opPush, $number); }
			| ident 						{ Symbol* symbol = find_symbol(symtab, $ident, sGlobal);
											  if(symbol == NULL) {
												char *error = NULL;
                                                asprintf(&error, "Undeclared identifier '%s'.", $ident);
                                                yyerror(error);
                                                free(error);
                                                YYABORT;
											  }
											  add_op(cb, opLoad, symbol);
											}
			| expression '+' expression 	{ add_op(cb, opAdd, NULL); }
			| expression '-' expression 	{ add_op(cb, opSub, NULL); }
			| expression '*' expression 	{ add_op(cb, opMul, NULL); }
			| expression '/' expression 	{ add_op(cb, opDiv, NULL); }
			| expression '%' expression 	{ add_op(cb, opMod, NULL); }
			| expression '^' expression 	{ add_op(cb, opPow, NULL); }
			| '(' expression ')' 			{ /* do nothing */ }
			| call

condition 	: expression '=' '=' expression { printf("Condition ==\n"); }
		   	| expression '<' '=' expression
			| expression '<' expression
			| expression '>' '=' expression //TODO
			| expression '>' expression //TODO
			;

number 		: NUMBER 						{ /* do nothing */ }
		 	;

ident       : IDENT 						{ /* do nothing */ }
            ;

string 		: STRING 						{ /* do nothing */ }
		 	;

%%

int main(int argc, char *argv[])
{
  extern FILE *yyin;
  argv++; argc--;

  while (argc > 0) {
    // prepare filename prefix (cut off extension)
    fn_pfx = strdup(argv[0]);
    char *dot = strrchr(fn_pfx, '.');
    if (dot != NULL) *dot = '\0';

    // open source file
    yyin = fopen(argv[0], "r");
    yydebug = 0;

    // parse
    yyparse();

    // next input
    free(fn_pfx);
    argv++; argc--;
  }

  return 0;
}

int yyerror(const char *msg)
{
  printf("Parse error at %d:%d: %s\n", yylloc.first_line, yylloc.first_column, msg);
  return 0;
}

