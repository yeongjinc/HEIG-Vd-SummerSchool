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
  Funclist *fnl  = NULL;
  Funclist *ftemp = NULL;
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

%type<n>    NUMBER number params exprl
%type<str>  ident IDENT string STRING call
%type<idl>  identl vardecl
%type<t>    type
%type<op> 	condition
%type<bp> 	IF WHILE
%type<fl> 	fun funcl

%%

program     :                                 { stack = init_stack(NULL); symtab = init_symtab(stack, NULL); fnl = NULL; }
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
			| funcl 						  { /* do nothing */  }
            ;

vardecl     : type identl                     {
												if($type == tVoid) {
												  char *error = NULL;
												  asprintf(&error, "Void variable.");
												  yyerror(error);
												  free(error);
												  YYABORT;
												}
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

funcl 		: fun 							  { $$ = (Funclist*)calloc(1, sizeof(Funclist));
												$$->id = strdup($fun->id);
												$$->rettype = $fun->rettype;
												$$->narg = $fun->narg;
												fnl = $$;
											  }
			| funcl fun 					  {
												$$ = (Funclist*)calloc(1, sizeof(Funclist));
												$$->id = strdup($fun->id);
												$$->rettype = $fun->rettype;
												$$->narg = $fun->narg;
												$$->next = $1;
												fnl = $$;
											  }
			;

fun 		:
	   		type ident 						  {
												if(find_func(fnl, $ident) != NULL) {
													char *error = NULL;
                                                    asprintf(&error, "Duplicated function name '%s'.", $ident);
                                                    yyerror(error);
                                                    free(error);
                                                    YYABORT;
												}
												ftemp = (Funclist*)calloc(1, sizeof(Funclist));
												ftemp->id = $ident;
												ftemp->rettype = $type;

												cb = init_codeblock($ident);
												stack = init_stack(stack);
												symtab = init_symtab(stack, symtab);
											  }
			'(' params ')' 					  {
												ftemp->narg = $params;
											  }
			stmtblock
											  {
											  	if(ftemp->rettype == tVoid)
													add_op(cb, opReturn, NULL);
											  	dump_codeblock(cb);
												save_codeblock(cb, fn_pfx);
												stack = stack->uplink;
												symtab = symtab->parent;
												$$ = ftemp;
											  }
			;

params 		: %empty  						  { $$ = 0; }
		 	| vardecl 						  { int cnt = 0;
												IDlist *l = $vardecl;
												while(l) { cnt++; l = l->next; }
										      	$$ = cnt;
											  }
			;

identl      : ident                           { $$ = (IDlist*)calloc(1, sizeof(IDlist)); $$->id = $ident; }
            | identl ',' ident                { $$ = (IDlist*)calloc(1, sizeof(IDlist)); $$->id = $ident; $$->next = $1; }
            ;

stmtl 		: %empty 						  { /* do nothing */ }
			| stmtl stmt 					  { /* do nothing */ }
			;

stmtblock   :
              '{' 							  { symtab = init_symtab(stack, symtab); }
			  stmtl 						  { symtab = symtab->parent; }
			  '}'
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

if 			:  IF '(' condition ')' 		{
	  											$IF = (BPrecord *)calloc(1, sizeof(BPrecord));
												Operation *tb = add_op(cb, $condition, NULL);
												Operation *fb = add_op(cb, opJump, NULL);
												$IF->ttrue = add_backpatch($IF->ttrue, tb);
												$IF->tfalse = add_backpatch($IF->tfalse, fb);
												pending_backpatch(cb, $IF->ttrue);
	  										}
	  		stmtblock 						{
												Operation *next = add_op(cb, opJump, NULL);
												$IF->end = add_backpatch($IF->end, next);
												pending_backpatch(cb, $IF->tfalse);
											}
			else 							{
												pending_backpatch(cb, $IF->end);
											}
			;

else 		: %empty 						{ /* do nothing */ }
	   		| ELSE 							{ /* do nothing */ }
			stmtblock 						{ /* do nothing */ }
			;

while 		: WHILE   						{
												$WHILE = (BPrecord *)calloc(1, sizeof(BPrecord));
												$WHILE->pos = cb->nops;
											}
			'(' condition ')'				{
												Operation *tb = add_op(cb, $condition, NULL);
												Operation *fb = add_op(cb, opJump, NULL);
												$WHILE->ttrue = add_backpatch($WHILE->ttrue, tb);
												$WHILE->tfalse = add_backpatch($WHILE->tfalse, fb);
												pending_backpatch(cb, $WHILE->ttrue);
											}
			stmtblock 						{
												Operation *ret = get_op(cb, $WHILE->pos);
												add_op(cb, opJump, ret);
												pending_backpatch(cb, $WHILE->tfalse);
											}
			;

exprl 		: %empty 						{ $$ = 0; }
			| expression 					{ $$ = 1; }
			| exprl ',' expression 			{ $$ = $$ + 1; }
			;

call 		:
	   		ident '(' exprl ')' 			{
												Funclist* f = find_func(fnl, $ident);
												if(f == NULL) {
												  char *error = NULL;
                                                  asprintf(&error, "Undeclared function '%s'.", $ident);
                                                  yyerror(error);
                                                  free(error);
                                                  YYABORT;
												}
												if(f->narg != $exprl) {
												  char *error = NULL;
                                                  asprintf(&error, "Function '%s' : Expected number of parameters %d, actual %d", $ident, f->narg, (int)$exprl);
                                                  yyerror(error);
                                                  free(error);
                                                  YYABORT;
												}
												add_op(cb, opCall, $ident);
											}
			;
												/* TODO Check # */
return 		: RETURN ';' 					{ add_op(cb, opReturn, NULL); }
		 	| RETURN expression ';' 		{ add_op(cb, opReturn, NULL); }
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
			;

condition 	: expression '=' '=' expression { $$ = opJeq; }
		   	| expression '<' '=' expression { $$ = opJle; }
			| expression '<' expression 	{ $$ = opJlt; }
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

