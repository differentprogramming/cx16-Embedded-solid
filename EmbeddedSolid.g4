/* This language is based on a unified model where there is a single language
 * that is both capable of being highly compressed running in a vm in bank
 * switched memory AND of being fast, running in machine code in flat memory.
 *
 * That might be unrealistic.  Perhaps the compact compiler will need to be
 * smaller and simpler, avoiding static typing and then once that works the
 * static typed compiler can be written in it, perhaps using more memory than
 * would be available in a rom.
 *
 * Or, or maybe I DO make a more complex self-hosting compiler but forgo the
 * idea of fitting it in rom or in a single rom slot.  In that case maybe I can
 * put generators and exceptions back in.
 *
 * One thing that's bothering me is the idea of a language for games using 
 * garbage collection.
 */
grammar EmbeddedSolid;

program 
    : declaration EOF
    ;

declaration
    : DECLARE CLASS IDENT
        (((FIELD IDENT (OF type)?)
         |((METHOD (MESSAGE_SELECTOR IDENT)+) | (METHOD postfix_selector))
           )
           statements
        ) +
      END
    | DECLARE MESSAGE (MESSAGE_SELECTOR type?)+ (RETURNS type)?
    | DECLARE MESSAGE IDENT (RETURNS type)? //declares postfix_selector
    | DECLARE RECORD IDENT
        (FIELD IDENT (OF type)?)+
      END
    | NEAR? top_level_function_definition
    | var_dec_and_assign
    | const_dec_and_assign
    ;

var_dec_and_assign
    : VAR var_assign_or_declare (COMMA var_assign_or_declare)* SEMI
    ;

const_dec_and_assign
    //expression can only contain constants
    : CONSTANT IDENT ASSIGN expression_vl SEMI
    ;

statement
    : expression SEMI
    | SWITCH (((CASE constant_exp)|DEFAULT) statements )* 
      END SEMI
    | IF expression THEN statements 
      (ELSEIF expression THEN statements)*
      (ELSE statements)?
      END SEMI
    | WHILE expression DO statements END SEMI
    | (BREAK|CONTINUE) SEMI
    | DO statements UNTIL expression SEMI 
    | FOR assign_exp? (SEMI expression? SEMI expression?)? DO statements END SEMI
    | var_dec_and_assign
    ;
var_assign_or_declare 
    : IDENT ((COMMA IDENT)* ASSIGN expression_vl)?
    ;
statements
    : statement * (RETURN expression (COMMA expression)*)?
    ;

expression
    : expression_vvl 
      ( MULTI_SEND_START (( MESSAGE_SELECTOR expression_l )| postfix_selector) 
        (MULTI_SEND_CONTINUE (( MESSAGE_SELECTOR expression_l )| postfix_selector) )* )?
    ;

expression_vvl 
    : expression_vl ( CASCADE expression_vl )*
    | assign_exp
    ;

lexp 
    : IDENT (index)*
    ;

assign_exp 
    : lexp (COMMA lexp)* ASSIGN expression_vl 
    ;
        
message 
    : ( MESSAGE_SELECTOR expression_l ) +
    | postfix_selector
    ;

expression_vl
    : expression_l ( MESSAGE_SELECTOR expression_l ) *
    ;

expression_l 
    :  expression_m postfix_selector *
    ;
        
postfix_selector
    : IDENT //also has to be declared as a message
    ;

expression_m
    : expression_h (bi_op expression_h) *
    | SEND expression TO expression_h
    ;

bi_op
    : PLUS
    | MINUS
    | MUL
    | DIV
    | EQ
    | RSHIFT
    | LSHIFT
    | BOR
    | BXOR
    | BAND
    | NEQ
    | LE
    | LT
    | GT
    | GE
    | DOTDOT 
    | AND
    | OR
    | BACKSLASH //for drop
    ;

pre_op
    : MINUS
    | MUL
    | BAND
    | NOT
    | BNOT
    ;

index 
    : ( LBRACK ( expression ( COMMA expression) * )* RBRACK )
    ;
expression_h
    : expression_h2
    | expression_h2 
      ( ( LP ( expression ( COMMA expression) * )* RP ) |
         index )+
    | NEAR? NEW explicit_type 
      ( ( LP ( expression ( COMMA expression) * )* RP ) |
         index )* //initializer is only for class
    ;
expression_h2 
    : SELF 
    | SUPER + 
    | MESSAGE message 
    | constant_exp
    | LP expression RP
    | pre_op expression_h
    | function_definition
    | NEAR? LCURLY ((constant_exp MAP_TO)? expression (COMMA (constant_exp MAP_TO)? expression)*)? RCURLY
    | NEAR? (LIST|ARRAY|CTREE) (OF type)? LCURLY (expression (COMMA expression)*)? RCURLY
    ;

function_definition
    : FUNCTION IDENT? LP (IDENT (OF type)? (COMMA IDENT (OF type)?)*)? RP (RETURNS type)? statements END
    ;

top_level_function_definition
    : FUNCTION IDENT LP (IDENT (OF type)? (COMMA IDENT (OF type)?)*)? RP (RETURNS type)? statements END
    ;

constant_exp
    : NUM_INT
    | YES
    | NO
    | ATOMCONST
    | NUM_REAL
    | STRING_LITERAL
    | NIL  
    | IDENT
    ;

simple_type
    : ( BIG
    | INTEGER
    | BYTE
    | REAL
    | STRING
    | WHETHER
    | ATOM
    | BOXED)
    ;

type
    : simple_type
    | OBJECT
    | ARRAY NUM_INT? (OF type)?
    | TABLE (OF type)?
    | LIST (OF type)?
    | CTREE (OF type)?
    | FUNCTION LP (type (COMMA type)*)? RP (RETURNS type)?
    | NEAR? POINTER TO type
    | RECORD IDENT
    ;    

explicit_type
    : simple_type
    | CLASS IDENT
    | ARRAY NUM_INT? (OF type)?
    | TABLE (OF type)?
    | LIST (OF type)?
    | CTREE (OF type)?
    | POINTER TO type
    | RECORD IDENT
    ;    

NUM_INT
    : ( '0' .. '9') +
    ;

AND
   : 'and'
   ;

ARRAY
    : 'array'
    ;

ATOM
    : 'atom'
    ;

BOXED 
    : 'boxed' 
    ;     

BREAK
    : 'break'
    ;


CASE
    : 'case'
    ;

CLASS
    : 'class'
    ;

END
    : 'end'
    ;

CONTINUE
    : 'cont'
    ;

CONSTANT
    : 'const'
    ;

DECLARE
    : 'decl'
    ;

DEFAULT
    : 'default'
    ;

DO
   : 'do'
   ;

ELSEIF
   : 'elseif'
   ;

ELSE
   : 'else'
   ;


FIELD
   : 'field'
   ;

FOR
   : 'for'
   ;

FUNCTION
   : 'func'
   ;

IF
   : 'if'
   ;

BIG
    : 'big'
    ;

INTEGER
   : 'int'
   ;

BYTE
   : 'byte'
   ;

LIST
    : 'list'
    ;

CTREE
    : 'ctree'
    ;

MESSAGE
    : 'message' 
    ;

METHOD
    : 'method'
    ;

MOD 
    : 'mod'
    ;

NEAR
    : 'near'
    ;

NEW
    : 'new'
    ;

NIL
    : 'nil'
    ;

NOT
   : 'not'
   ;

NO
    : 'no'
    ;

OBJECT
    : 'object'
    ;

OF
   : 'of'
   ;

OR
   : 'or'
   ;

POINTER 
    : 'ptr'
    ;

REAL
   : 'real'
   ;

RECORD
   : 'record'
   ;

REPEAT
   : 'repeat'
   ;

RETURNS
    : 'returns'
    ;

RETURN
    : 'return'
    ;

SELF
    : 'self'
    ;  

SEND
    : 'send'
    ;

SET
   : 'set'
   ;
   
STRING
    : 'string'
    ;

SUPER
    : 'super'
    ;

SWITCH
    : 'switch'
    ;

TABLE 
    : 'table'
    ;

THEN
   : 'then'
   ;

TO
   : 'to'
   ;

UNTIL
   : 'until'
   ;

VAR
   : 'var'
   ;

WHETHER
    : 'whether'  
    ;

WHILE
   : 'while'
   ;

YES
    : 'yes'
    ;
        

PLUS
   : '+'
   ;


MINUS
   : '-'
   ;


MUL
   : '*'
   ;


DIV
   : '/'
   ;

BACKSLASH
    : '\\'
    ;

EQ
   : '=='
   ;

MAP_TO
   : '=>'
   ;

RSHIFT
   : '>>'
   ;

LSHIFT
   : '<<'
   ;


ASSIGN
   : '='
   ;

BOR
    : '|'
    ;

BXOR
    : '^'
    ;

BAND
    : '&'
    ;

BNOT
    : '~'
    ;

COMMA
   : ','
   ;


SEMI
   : ';'
   ;


NEQ
   : '!='
   ;


LE
   : '<='
   ;

LT
   : '<'
   ;


GT
   : '>'
   ;

GE
   : '>='
   ;

LP
   : '('
   ;


RP
   : ')'
   ;


LBRACK
   : '['
   ;



RBRACK
   : ']'
   ;


DOTDOT
   : '..'
   ;

LCURLY
   : '{'
   ;


RCURLY
   : '}'
   ;

CASCADE
    : '|>'
    ;

MULTI_SEND_START
    : '::>'
    ;

MULTI_SEND_CONTINUE
    : ':>'
    ;

COMMENT_1
   : '/*' .*? '*/' -> skip
   ;

LINE_COMMENT
    : '%' ~('\r'|'\n')*
    ('\r\n'|'\r'|'\n')
    -> skip
    ;
    
WS
   : [ \t\r\n] -> skip
   ;


/* Special selectors
 * doesNotUnderstand: if it exists gets sent a version of the message repackaged so every element is boxed, ie list{selector,params...}
 * new goes to basicNew by default then to initialize
 * ProtoObject initialize does nothing
 */

MESSAGE_SELECTOR
    : IDENT ':'
    ;

ATOMCONST
    : '$' IDENT
    ;

 
IDENT
   : ('a' .. 'z' | 'A' .. 'Z' | '_') ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_')*
   ;


STRING_LITERAL
   : '"' ('\\"' | ~ ('"'))* '"'
   ;


NUM_REAL
   : ('0' .. '9') + (('.' ('0' .. '9') + (EXPONENT)?) | EXPONENT)
     | (('.' ('0' .. '9') + (EXPONENT)?))
   ;


fragment EXPONENT
   : ('e') ('+' | '-')? ('0' .. '9') +
   ;