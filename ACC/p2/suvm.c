/*------------------------------------------------------------------------------
  HEIG-Vd - CoE@SNU              Summer University              July 11-22, 2016

  The Art of Compiler Construction


  suVM - virtual machine to execute suPL binary code

*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "supllib.h"

/// @{
/// @name memories
Stack *global;            ///< storage for global variables
Stack *stack;             ///< the current stack
Stack *operands;          ///< the operand stack
/// @}

/// @{
/// @name execution context
CodeBlock *code;          ///< currently executing codeblock
int pc;                   ///< program counter (offset into code
/// @}


/// @{
/// @name code management

char *fn_pfx = NULL;
CodeBlock *entry = NULL;

/// @}

/// @{
/// @name output control

int trace_ops = 0;        ///< print each operation for each step
int trace_mem = 0;        ///< print memory contents for each step
int trace_stack = 0;      ///< print operand stack for each step

/// @}


CodeBlock* load(const char *label, const char *fn_prefix)
{
  // check if code is already loaded
  CodeBlock *cb = entry;
  while ((cb != NULL) && (strcmp(label, cb->label) != 0)) cb = cb->next;

  // if not load the code and insert it into the list of codeblocks
  if (cb == NULL) {
    cb = load_codeblock(label, fn_prefix);
    if (cb != NULL) {
      cb->next = entry;
      entry = cb;
    }
  }

  return cb;
}

/// @brief execution loop. The memories (global, current), the operand
///        stack (operands), and the execution context (code, pc) must
///        be set up.
void execute(void)
{
  Operation *op;

  while (1) {
    op = get_op(code, pc);
    if (op == NULL) {
      printf("Error: invalid code address %d in\n", pc);
      dump_codeblock(code);
      return;
    }

    if (trace_ops) {
      printf("---   operation   ---\n");
      dump_operation(op);
    }
    if (trace_stack) {
      printf("--- operand stack ---\n");
      dump_stack(operands);
    }
    if (trace_mem) {
      printf("--- global memory ---\n");
      dump_stack(global);
      printf("--- current stack ---\n");
      dump_stack(stack);
    }
    if (trace_ops || trace_stack || trace_mem) {
      printf("---------------------\n");
    }


    int next_pc = pc+1;

    switch (op->opc) {
      case opHalt:
        { //
          // halt execution
          //
          return;
        }
        break;

      case opAdd:
      case opSub:
      case opMul:
      case opDiv:
      case opMod:
      case opPow:
        { // 
          // pop b
          // pop a
          // push a op b
          //
          if (num_values(operands) < 2) {
            printf("Error: operand stack underflow at pc %d in\n", pc);
            dump_codeblock(code);
            return;
          }
          int b = pop_value(operands);
          int a = pop_value(operands); 
          int r;

          switch (op->opc) {
            case opAdd: r = a + b; break;
            case opSub: r = a - b; break;
            case opMul: r = a * b; break;
            case opDiv: r = a / b; break;
            case opMod: r = a % b; break;
            case opPow: r = powl(a, b); break;
          }

          push_value(operands, r);
        }
        break;

      case opPush:
        { //
          // push operand
          //
          push_value(operands, (int)(long int)op->operand);
        }
        break;

      case opPop:
        { //
          // pop and discard value
          //
          pop_value(operands);
        }
        break;

      case opLoad:
      case opStore:
        { //
          // opLoad:
          //   push mem[operand]
          //
          // opStore:
          //   pop v
          //   mem[operand] = v
          
          int offset = (int)(long int)op->operand;

          // bit 31: 1 => global variable
          //         0 => variable on stack
          Stack *mem = stack;
          if (offset & (1 << 31)) {
            mem = global;
            offset &= 0x7fffffff;
          }

          int v;
          if (op->opc == opLoad) {
            v = load_value(mem, offset);
            push_value(operands, v);
          } else {
            if (num_values(operands) < 1) {
              printf("Error: operand stack underflow at pc %d in\n", pc);
              dump_codeblock(code);
              return;
            }
            v = pop_value(operands);
            store_value(mem, offset, v);
          }
        }
        break;

      case opCall:
        {
          // create a new execution stack
          stack = init_stack(stack);

          // save return information
          stack->retcb = code;
          stack->retpc = next_pc;

          // load code and set pc to first instruction
          code = load((char*)op->operand, fn_pfx);
          next_pc = 0;
        }
        break;

      case opReturn:
        { 
          // set execution context to next instruction in caller
          code = stack->retcb;
          next_pc = stack->retpc;

          // activate caller stack and discard stack of callee
          Stack *s = stack;
          stack = stack->uplink;
          delete_stack(s);
        }
        break;

      case opJump:
      case opJeq:
      case opJle:
      case opJlt:
        { // 
          // opJeg, opJle, opJlt            opJump
          // pop b                        
          // pop a
          // flag = a ?? b 
          // if (flag) newpc = pc+operand   newpc = pc+operand
          //
          int flag = 1;

          if (op->opc != opJump) {
            if (num_values(operands) < 2) {
              printf("Error: operand stack underflow at pc %d in\n", pc);
              dump_codeblock(code);
              return;
            }

            int b = pop_value(operands);
            int a = pop_value(operands); 

            switch (op->opc) {
              case opJeq: flag = a == b; break;
              case opJle: flag = a <= b; break;
              case opJlt: flag = a <  b; break;
            }
          }

          if (flag) next_pc = pc + (int)(long int)op->operand;
        }
        break;

      case opRead:
        { //
          // read v from stdin
          // mem[op] = v
          //
          int v, r;
          r = scanf("%d", &v);
          if (r == EOF) {
            printf("Error: cannot read from stdin at pc %d in\n", pc);
            dump_codeblock(code);
            return;
          }
          if (r == 0) {
            printf("Error: invalid input at pc %d in\n", pc);
            dump_codeblock(code);
            return;
          }

          int offset = (int)(long int)op->operand;

          // bit 31: 1 => global variable
          //         0 => variable on stack
          Stack *mem = stack;
          if (offset & (1 << 31)) {
            mem = global;
            offset &= 0x7fffffff;
          }

          store_value(mem, offset, v);
        }
        break;

      case opWrite:
        { //
          // pop v
          // print v to stdout
          //
          if (num_values(operands) < 1) {
            printf("Error: operand stack underflow at pc %d in\n", pc);
            dump_codeblock(code);
            return;
          }

          int v = pop_value(operands);
          printf("%d\n", v);
        }
        break;

      case opPrint:
        { //
          // print operand to stdout
          //
          printf("%s", (char*)op->operand);
        }
        break;

      default:
        { //
          // invalid/unimplemented opcode
          //
            printf("Error: invalid/unimplemented opcode at pc %d in\n", pc);
            dump_codeblock(code);
            return;
        }
    }
    pc = next_pc;
  } 
}

int main(int argc, char *argv[])
{
  char *fn = NULL;
  int i = 1, help = 0;
  while (i < argc) {
    if (strcmp("--help", argv[i]) == 0) help = 1;
    else if (strcmp("--trace-ops", argv[i]) == 0) trace_ops = 1;
    else if (strcmp("--trace-mem", argv[i]) == 0) trace_mem = 1;
    else if (strcmp("--trace-stack", argv[i]) == 0) trace_stack = 1;
    else fn = argv[i];
    i++;
  }
  if (help || (fn == NULL)) {
    printf("Syntax: suvm [options] <suPL binary>\n\n");
    printf("  --help              print this help\n");
    printf("  --trace-ops         trace operations\n");
    printf("  --trace-mem         trace memory\n");
    printf("  --trace-stack       trace operand stack\n");
    if (help) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
  }

  global = init_stack(NULL);
  delete_stack(global);

  global = init_stack(NULL);
  push_value(global, 7);
  delete_stack(global);

  global = init_stack(NULL);
  store_value(global, 4, 7);
  delete_stack(global);

  // prepare filename prefix (cut off extension)
  fn_pfx = strdup(fn);
  char *dot = strrchr(fn_pfx, '.');
  if (dot != NULL) *dot = '\0';

  // load main code
  entry = load("", fn_pfx);
  if (entry != NULL) {
    dump_codeblock(entry);

    printf("\n\nExecution begins\n\n");

    // initialize stacks
    operands = init_stack(NULL);

    global = init_stack(NULL);
    stack = init_stack(global);

    // start executing at pc=0
    code = entry;
    pc = 0;

    execute();

    // cleanup
    delete_stack(operands);
    delete_stack(global);
    delete_stack(stack);

    CodeBlock *cb;
    do {
      cb = entry;
      entry = entry->next;
      delete_codeblock(cb);
    } while (entry != NULL);
  } else {
    printf("Error: cannot load code.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
