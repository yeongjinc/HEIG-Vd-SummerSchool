/*------------------------------------------------------------------------------
  HEIG-Vd - CoE@SNU              Summer University              July 11-22, 2016

  The Art of Compiler Construction


  suPL support routines for the parser and the VM

*/

#ifndef __SUPLLIB_H__
#define __SUPLLIB_H__

#include <string.h>

//------------------------------------------------------------------------------
// suPL types
//

/// @brief suPL types
typedef enum __type {
  tVoid,                        ///< void
  tInteger,                     ///< integer
} EType;

//------------------------------------------------------------------------------
// symbol scope
//

/// @brief variable scope
typedef enum __scope {
  sLocal,                       ///< local variable
  sGlobal,                      ///< global variable
} EScope;


//------------------------------------------------------------------------------
// various lists
//

/// @brief list of identifiers. The list owns the strings.
typedef struct __idlist {
  char *id;                     ///< identifier
  struct __idlist *next;        ///< link to next element
} IDlist;

/// @brief free an IDlist. The list owns the strings, i.e., frees them here.
/// @param l list to free
void free_idlist(IDlist *l);


//------------------------------------------------------------------------------
// stack management
//
// Not a pure stack as it also allows for direct access (see load/store_value
// below). During compile-time, the global and local variables are pushed onto
// the respective stacks to obtain memory addresses (push_value returns the
// stack offset). At runtime, those offsets are then used to directly load/store
// values from memory locations.

/// @brief operand/variable stack
typedef struct __stack {               
  int *values;                  ///< contents of stack
  int top;                      ///< top of stack (index into 'values' array)
  int elem;                     ///< number of elements this stack can hold

  struct __stack *uplink;       ///< pointer to upper stack (static at compile
                                ///< time, dynamic at runtime)
} Stack;

/// @brief initalize a new stack
/// @param uplink link to (statically) upper stack
/// @retval Stack* newly created stack
Stack*  init_stack(Stack *uplink);

/// @brief delete a stack
/// @param s stack to delete
void delete_stack(Stack *s);

/// @brief resize a stack to hold @a s->elem elements
/// @param s stack to resize
void resize_stack(Stack *s);

/// @brief return the number of values on stack
/// @retval number of elements on stack
int num_values(Stack *s);

/// @brief push a value onto a stack
/// @param val value
/// @retval offset of element relative to stack base (in bytes)
int push_value(Stack *s, int val);

/// @brief pop a value from a stack
/// @retval int popped value
int pop_value(Stack *s);

/// @brief store a value on the stack
/// @param offset offset of value on stack (in bytes)
/// @param val value
void store_value(Stack *s, int offset, int val);

/// @brief load a value from the stack
/// @param offset offset of value on stack (in bytes)
/// @retval int value
int load_value(Stack *s, int offset);

/// @brief print stack in human-readable form to stdout
/// @param s stack to print
void dump_stack(Stack *s);


//------------------------------------------------------------------------------
// symbol management
//

struct __symtab;

/// @brief a suPL symbol
typedef struct __symbol {       ///< symbol
  const char *id;               ///< symbol ID
  EType type;                   ///< type of symbol
  int offset;                   ///< location of symbol relative to active stack
  struct __symbol *next;        ///< link to next symbol in table
  struct __symtab *symtab;      ///< link to symbol table
} Symbol;

/// @brief symbol table
typedef struct __symtab {
  Symbol *s;                    ///< link to first symbol
  int nsym;                     ///< number of symbols in symbol table
  Stack *stack;                 ///< link to stack holding the data of this ST
  struct __symtab *parent;      ///< link to parent symbol table
} Symtab;

/// @brief create a new (empty) symbol table
/// @param parent link to parent symbol table
/// @retval Symbtab* newly created symbol table 
Symtab* init_symtab(Stack *stack, Symtab *parent);

/// @brief delete a symbol table including all its symbols
/// @param st symbol table to delete
void delete_symtab(Symtab *st);

/// @brief next symbol in symbol table @a st
/// @param prev previous symbol (or NULL for first)
/// @retval Symbol* next symbol or
/// @retval NULL if no mor symbols
Symbol* next_symbol(Symtab *st, Symbol *prev);

/// @brief find symbol in the symbol table
/// @param st link to symbol table
/// @param id ID of symbol
/// @param scope scope of search
/// @retval NULL if symbol was not found
/// @retval Symbol* link to symbol
Symbol* find_symbol(Symtab *st, const char *id, EScope scope);

/// @brief insert a symbol with ID @a id into symbol table @a st
/// @param st symbol table to insert symbol into
/// @param id ID of symbol to insert
/// @param type type of symbol to insert
/// @retval NULL if a symbol with the given name already exists in the table
/// @retval Symbol* link to inserted symbol
Symbol* insert_symbol(Symtab *st, const char *id, EType type);


//------------------------------------------------------------------------------
// code generation
//
#define SUX_MAGIC 0x16071122

/// @brief suPL VM opcodes
typedef enum __opcode {         ///< syntax operands semantics
  opHalt = 0,                   ///< opHalt    0     stop program 

  opAdd,                        ///< opAdd     0     push p2 + p1
  opSub,                        ///< opSub     0     push p2 - p1 
  opMul,                        ///< opMul     0     push p2 * p1
  opDiv,                        ///< opDiv     0     push p2 / p1
  opMod,                        ///< opMod     0     push p2 % p1
  opPow,                        ///< opPow     0     push power(p2, p1)

  opPush,                       ///< opPush    1     push op
  opPop,                        ///< opPop     0     pop and discard

  opLoad,                       ///< opLoad    1     push mem[op] 
  opStore,                      ///< opStore   1     mem[op] = pop

  opCall,                       ///< opCall    1     goto code[op]; save retadr
  opReturn,                     ///< opReturn  0     return to caller

  opJump,                       ///< opJump    1     goto code[op]
  opJeq,                        ///< opJeg     1     if p2 == p1 goto code[op]
  opJle,                        ///< opJle     1     if p2 <= p1 goto code[op]
  opJlt,                        ///< opJlt     1     if p2 <  p1 goto code[op]

  opRead,                       ///< opRead    1     mem[op] = int from stdin
  opWrite,                      ///< opWrite   0     write p1 to stdout
  opPrint,                      ///< opPrint   1     write string op to stdout

  opMax
} EOpcode;

/// @brief Operation
typedef struct __operation {
  int id;                       ///< operation ID
  int bin;                      ///< flag: 1 for binary (loaded) ops,0 otherwise
  EOpcode opc;                  ///< opcode
  void *operand;                ///< operand (type varies; see dump_operation)
} Operation;

/// @brief Oplist: a list of operations
typedef struct __oplist {
  Operation *op;                ///< operation
  struct __oplist *next;        ///< link to next element
} Oplist;

/// @brief BPrecord structure to manage backpatching
typedef struct __bprec {
  Oplist *ttrue;                ///< true-branch backpatch list
  Oplist *tfalse;               ///< false-branch backpatch list
  Oplist *end;                  ///< end backpatch list
  int pos;                      ///< position of operation for back branch
} BPrecord;

/// @brief CodeBlock structure
typedef struct __codeblock {
  const char *label;            ///< name of this codeblock
  Operation *code;              ///< array holding operations

  Oplist *bp_pending;           ///< pending backpatch list

  int nops;                     ///< current position in code array
  int elem;                     ///< max. number of elements (size of code)

  int stacksize;                ///< size of runtime stack (== number of locals)
} CodeBlock;

/// @brief initalize a new codeblock
CodeBlock* init_codeblock(const char *label);

/// @brief delete codeblock @a cb
void delete_codeblock(CodeBlock *cb);

/// @brief resize codeblock @a cb to hold @a cb->elem operations
void resize_codeblock(CodeBlock *cb);

/// @brief add an operation to codeblock @a cb
/// @param cb codeblock
/// @param opc opcode
/// @param opa operand
/// @retval Operation* link to added operation
Operation* add_op(CodeBlock *cb, EOpcode opc, void* opa);

/// @brief get operation at a specific position @a idx
/// @param idx operation to retrieve
/// @retval Operation* operatino if it exists
/// @retval NULL otherwise
Operation* get_op(CodeBlock *cb, int idx);

/// @brief add a list of operations to backpatch on next add_op
/// @param bpl list of operations to backpatch
void pending_backpatch(CodeBlock *cb, Oplist *bpl);

/// @brief add an operation to a given operation list for backpatching
/// @param list current list
/// @param op operation to add
/// @retval Oplist* new list including @a op
Oplist* add_backpatch(Oplist *list, Operation *op);

/// @brief print codeblock in human-readable form to stdout
/// @param cb codeblock to print
void dump_codeblock(CodeBlock *cb);

/// @brief load codeblock from file
/// @param label label (function name, "" for main routine)
/// @param fn_prefix filename prefix
CodeBlock* load_codeblock(const char *label, const char *fn_prefix);

/// @brief save codeblock in binary form
/// @param cb codeblock
/// @param fn_prefix filename prefix
void save_codeblock(CodeBlock *cb, const char *fn_prefix);

#endif // __SUPL_H__
