/* ====================================================================
 * The Prothon License Agreement, Version 1.1
 *
 * Copyright (c) 2004 Hahn Creative Applications, http://hahnca.com.
 * All rights reserved. 
 *
 * 1. This LICENSE AGREEMENT is between Hahn Creative Applications ("HCA"),
 * and the Individual or Organization ("Licensee") accessing and otherwise
 * using Prothon software in source or binary form and its associated
 * documentation.
 * 
 * 2. Subject to the terms and conditions of this License Agreement, HCA
 * hereby grants Licensee a nonexclusive, royalty-free, world-wide license
 * to reproduce, analyze, test, perform and/or display publicly, prepare
 * derivative works, distribute, and otherwise use Prothon alone or in any
 * derivative version, provided, however, that HCA's License Agreement and
 * HCA's notice of copyright, i.e., "Copyright (c) 2004 Hahn Creative
 * Applications; All Rights Reserved" are retained in Prothon alone or
 * in any derivative version prepared by Licensee.
 * 
 * 3. In the event Licensee prepares a derivative work that is based on or
 * incorporates Prothon or any part thereof, and wants to make the
 * derivative work available to others as provided herein, then Licensee
 * hereby agrees to include in any such work a brief summary of the
 * changes made to Prothon.
 * 
 * 4. HCA is making Prothon available to Licensee on an "AS IS" basis.
 * HCA MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.  BY WAY
 * OF EXAMPLE, BUT NOT LIMITATION, HCA MAKES NO AND DISCLAIMS ANY
 * REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS FOR ANY
 * PARTICULAR PURPOSE OR THAT THE USE OF PROTHON WILL NOT INFRINGE ANY
 * THIRD PARTY RIGHTS.
 * 
 * 5. HCA SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PROTHON
 * FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS A
 * RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PROTHON, OR ANY
 * DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.
 * 
 * 6. This License Agreement will automatically terminate upon a material
 * breach of its terms and conditions.
 * 
 * 7. Nothing in this License Agreement shall be deemed to create any
 * relationship of agency, partnership, or joint venture between HCA and
 * Licensee.  This License Agreement does not grant permission to use HCA
 * trademarks or trade name in a trademark sense to endorse or promote
 * products or services of Licensee, or any third party.
 * 
 * 8. By copying, installing or otherwise using Prothon, Licensee agrees
 * to be bound by the terms and conditions of this License Agreement.
 * ====================================================================
 */


// bytecodes.h

#ifndef BYTECODES_H
#define BYTECODES_H

typedef enum {

// NOP
// no operation
// param: total word count
// |opcode|<unused>|<unused>|...
OP_NOP,

// IMPORT(path)
// compile and import module into Modules and globals
// (uses existing module if found)
// name for Modules and globals is last label in path
// param: total word count
// |opcode|path symbol|path symbol|...|
OP_IMPORT,

// IMPORT_AS(path, label)
// compile and import module into Modules and globals
// (uses existing module if found)
// name for Modules is last label in path
// name for globals is given "as" label
// param: total word count
// |opcode|path symbol|path symbol|...|label symbol|
OP_IMPORT_AS,

// IMPORT_PUSH(path)
// compile and import module into Modules and push on stack
// (uses existing module if found)
// name for Modules is last label in path
// does NOT import into globals
// stack: <empty> -> module-obj
// param: total word count
// |opcode|path symbol|path symbol|...
OP_IMPORT_PUSH,

// IMPORT_ATTR(path)
// Import attr from module on top-of-stack into globals
// name for globals is last label in path
// leaves module object unchanged on stack
// param: total word count
// |opcode|path symbol|path symbol|...|
OP_IMPORT_ATTR,

// IMPORT_ATTR_AS(path,label)
// Import attr from module on top-of-stack into globals
// name for globals is given "as" label
// leaves module object unchanged on stack
// param: total word count
// |opcode|path symbol|path symbol|...|label symbol|
OP_IMPORT_ATTR_AS,

// PUSH(object)
// push object-ids onto stack
// param: total word count
// |opcode|object-id|object-id|...|
OP_PUSH,

// OBJ(protos)
// Push new execution frame identical to last except with default-local set to
// a new object (usually a prototype) made with prototypes from list protos.
// New obj also stored at destination reference location on stack.
// stack length is ((num proto refs)+1)*2 since ref is two stack items
// stack: dst-ref, proto-ref2, proto-ref3, ... proto-refN -> <empty>
// param: total word count == 3
// |opcode|func-obj|Num proto refs|
OP_OBJ,

// WITH (self)
// Push new execution frame identical to last except with locals
// equal to top-of-stack.  Similar to a call.
// Code is in a simple function-object that only has code.
// stack: local-scope-obj -> <empty>
// param: total word count (==2)
// |opcode|func-obj|
OP_WITH,

// DEF_(label, formal params, code)
// define a function and store it in reference attr
// fparam-label is symbol or OBJ(PARAM_STAR), or OBJ(PARAM_STAR_STAR)
// fparam-value can be object or zero for empty
// stack: obj, ref, fparam-label, fparam-value, fparam-label, fparam-value,  ... -> <empty>
// param: total word count (==3)
// |opcode|# of fparam pairs|func-obj|
OP_DEF,

// GEN(label, formal params, code)
// Same as OP_DEF except that it also wraps the new function in a gen object
// the gen object also contains the execution frame stack
// stack: obj, ref, fparam-label, fparam-value, fparam-label, fparam-value,  ... -> <empty>
// param: total word count (==3)
// |opcode|# of fparam pairs|func-obj|
OP_GEN,

/********/ OP_PARAM_WORDS_BOUNDARY, /********/ 

//*** Note: A ref (2 words on stack) is:
//***		1) A containing object and ...
//***       2) A symbol object that can directly look up an object
//***          attribute or a slice object that can be used by 
//***          functions in the object to reference sequence data.
//***
//***       The combined obj/reference represents attributes or data 
//***		contained in the object that can be read from or written to.

// PROP()
// Push new execution frame identical to last except with default-local set to
// a new object that will hold property methods.
// New obj also stored at destination reference location on stack.
// stack: dst-ref -> <empty>
// param: <unused>
// |opcode|func-obj|
OP_PROP,

// COMMAND(label)
// call function as a command, label is name of function in local scope
// stack: arg1-key, arg1-value,  arg2-key, arg2-value, ... -> result
// param: number of arguments on stack
// |opcode|label|
OP_COMMAND,

// PUSH_LOCAL_REF(symbol)
// push local reference (local-obj/ref) onto stack
// stack: -> local-scope,symbol
// param: <unused>
// |opcode|symbol-key|
OP_PUSH_LOCAL_REF,

// PUSH_SELF_REF(self.symbol)
// push self or inherited self obj reference (self/ref) onto stack
// stack: -> self-obj,symbol
// param: <unused>
// |opcode|symbol-key|
OP_PUSH_SELF_REF,

// PUSH_SUPER_REF(super.symbol)
// push inherited super obj reference (self/ref) onto stack
// stack: -> super-proto,symbol
// param: <unused>
// |opcode|symbol-key|
OP_PUSH_SUPER_REF,

// PUSH_OUTER_REF(outer.symbol)
// push inherited super obj reference (super/ref) onto stack
// stack: -> super-obj,symbol
// param: <unused>
// |opcode|symbol-key|
OP_PUSH_OUTER_REF,

// PUSH_CALLER_REF(caller.symbol)
// push inherited super obj reference (super/ref) onto stack
// stack: -> super-obj,symbol
// param: <unused>
// |opcode|symbol-key|
OP_PUSH_CALLER_REF,

// BEQ(object-id)
// branch relative param words in code if top-of-stack equals obj-id
// stack: object-id  (unchanged)
// if eq: PC += param, else: PC += 2
// param: relative distance to jump (signed 24-bits)
// |opcode|object-id|
OP_BEQ,

// BREAK (label)
// break for FOR or WHILE loop
// placeholder for branch during compile phase
// replaced with BR, NOP
// param: unused
// |opcode|(char*) label|
OP_BREAK,

// CONTINUE (label)
// continue for FOR or WHILE loop
// placeholder for branch during compile phase
// replaced with BR, NOP
// param: unused
// |opcode|(char*) label|
OP_CONTINUE,

// CMDCALL(func_obj, params)
// (almost same as OP_OBJCALL)
// call function as a command, with unbound function object, bind target, & params on stack
// use locals container for self
// params are in pairs of optional-label/value
// pop all and leave result on stack
// if bind-tgt is NULL, use current self as bind-tgt
// symbol is provided for error message
// stack: func-object,bind-tgt,label,value,label,value,... -> result
// param: num param pairs on stack
// |opcode|symbol|
OP_CMDCALL,

/********/ OP_TWO_WORDS_BOUNDARY, /********/ 

// CLRSP
// clear sp
// stack: -> <empty>
// param: <unused>
// |opcode|
OP_CLRSP,

// POP
// delete top-of-stack
// param: words to pop
// |opcode|
OP_POP,

// SWAP
// swap top two items on stap
// stack: item1, item2 -> item2, item1
// param: <unused>
// |opcode|
OP_SWAP,

// DUPLICATE
// duplicate item on stack
// stack: item -> item,item
// param: sp offset of item to duplicate, -1: tos, -2: second item on stack, ...
// |opcode|
OP_DUPLICATE,

// RESTORE_TMP
// push temporary register onto stack
// stack: <empty> -> item
// param: <unused>
// |opcode|
OP_RESTORE_TMP,

// PUSH_SELF
// push self scope object-id onto stack
// param: <unused>
// |opcode|
OP_PUSH_SELF,

// PUSH_CALLER
// push caller scope object-id onto stack
// param: <unused>
// |opcode|
OP_PUSH_CALLER,

// OP_CHAIN_CMP
// make chain compare object from items on stack
// stack: VALUE_OBJ, True/False -> cmp_object
// if True:  cmp_object = new_object with proto == True object, add getCmp_(): return VALUE_OBJ
// else: cmp_object = False object
// param: <unused>
// |opcode|
OP_CHAIN_CMP,

// OBJBIND()
// bind function (function obj) on stack with obj on stack
// leave bound method result on stack
// if bind-tgt is NULL, use self as bind-tgt
// stack: function-obj,bind-tgt -> bound-function
// param: <unused>
// |opcode|
OP_OBJBIND,

// BIND()
// bind function (attribute ref pair) on stack with bind-tgt on stack
// leave bound method result on stack
// if bind-tgt is NULL, use obj of attr-ref-pair as bind-tgt
// stack: obj,func-symbol,bind-tgt -> bound-function
// param: <unused>
// |opcode|
OP_BIND,

// OBJCALL(func_obj, params)
// call function, with unbound function object, bind target, & params on stack
// use locals container for self
// params are in pairs of optional-label/value
// pop all and leave result on stack
// if bind-tgt is NULL, use current self as bind-tgt
// stack: func-object,bind-tgt,label,value,label,value,... -> result
// param: num param pairs on stack
// |opcode|
OP_OBJCALL,

// CALL(self, func, params)
// call function, with self-obj, func-symbol, bind target, & params on stack
// params are in pairs of optional-label/value
// label is symbol-obj, OBJ(NOKEY), OBJ(PARAM_STAR), or OBJ(PARAM_STAR_STAR)	
// pop all and leave result on stack
// if bind-tgt is NULL, use self-obj as bind-tgt
// stack: self-obj,func-symbol,bind-tgt,label,value,label,value,... -> result
// param: num param pairs on stack
// |opcode|
OP_CALL,

// ASSIGN(values, references)
// assign values on stack to references on stack
// pop references when done
// stack: value, value, ..., reference, reference, ... -> value, value, ...
// param: number of value/reference pairs
// |opcode|
OP_ASSIGN,

// OP_SEQ_ASSIGN(sequence, symbol refs)
// assign values in sequence on stack to local references on stack
// each local reference is a single symbol object
// if not a sequence treat as sequence of one object
// sequence length must match param length or error
// pop obj and references when done
// used in for loop and list comprehension
// stack: sequence-obj, reference, reference, ...  ->  <empty>
// param: number of references
// |opcode|
OP_SEQ_ASSIGN,

// OP_SEQ_ASSIGN(sequence, attr refs)
// assign values in sequence on stack to attr references on stack
// if not a sequence treat as sequence of one object
// sequence length must match param length or error
// pop obj and references when done
// used in exapanding assignment from sequence to arg list
// stack: sequence-obj, attr-reference, attr-reference, ...  ->  <empty>
// param: number of references
// |opcode|
OP_ASSIGN_EXP,

// BR
// branch relative param words in code unconditionally
// PC += param
// param: relative distance to jump (signed 24-bits)
// |opcode|
OP_BR,

// NEWTUPLE
// make a new tuple object out of objects on top of stack
// stack: obj, obj, obj, ... -> tuple-obj
// param: number of objects on stack
// |opcode|
OP_NEWTUPLE,

// NEWLIST
// make a new list object out of objects on top of stack
// stack: obj, obj, obj, ... -> list-obj
// param: number of objects on stack
// |opcode|
OP_NEWLIST,

// NEWDICT
// make a new dictionary object out of objects on top of stack
// objects are in pairs of key:value
// stack: key, value, key, value, ... -> list-obj
// param: number of objects on stack
// |opcode|
OP_NEWDICT,

// NEWSLICE
// make a new slice object out of 3 objects on top of stack
// stack: value, value, value -> slice-obj
// param: unused
// |opcode|
OP_NEWSLICE,

// APPEND
// append item object on top of stack to list lower in stack
// pop item after appending, used in list comprehension
// stack: list, ..., item -> list, ...
// param: offset of list from top of stack, -2 -> list is immediately under item, etc.
// |opcode|
OP_APPEND,

// TRYEXCEPT
// push try frame on exception stack
// param: distance to first matching except opcode
// |opcode|
OP_TRYEXCEPT,

// EXCEPT(exception(s), reference)
// ecxeption handling code follows this command
// execute handler or skip over it
// stack: exception proto(or list of protos), label -> 
// param: distance to end of handler code
// |opcode|
OP_EXCEPT,

// RERAISE
// reraise exception (does nothing when nothing to re-raise)
// param: <unused>
// |opcode|
OP_RERAISE,

// TRYFINALLY
// push try frame on exception stack
// param: distance to matching final opcode
// |opcode|
OP_TRYFINALLY,

// EXEC(string)
// compile and execute string on top-of-stack
// stack: string-object -> <empty>
// param: <unused>
// |opcode|
OP_EXEC,

// DEREF
// replace 2 reference objects (obj/symbol) on stack with object referred to (attr value)
// stack: obj,ref -> obj(ref)
// param: <unused>
// |opcode|
OP_DEREF,

// RAISE(eception object)
// raise exception object
// stack: exception-object -> <none>
// param: <unused>
// |opcode|
OP_RAISE,

// DEL(attr)
// delete attr(s) pointed to by obj/reference on top-of stack 
// delete top-of-stack
// stack: obj, reference(3 words) -> <empty>
// param: <unused>
// |opcode|
OP_DEL,

// YIELD(value)
// return value on top-of stack leaving exec frame ready to run
// delete top-of-stack
// stack: obj -> <empty>
// param: <unused>
// |opcode|
OP_YIELD,

// RETURN(value)
// return value on top-of stack and kill exec frame
// delete top-of-stack
// stack: obj -> <empty>
// param: <unused>
// |opcode|
OP_RETURN

} opcode_t;

#endif // #define BYTECODES_H
