%{
/* Cognitive Grammar Lexer for OpenCog */
#include <stdio.h>
#include <string.h>
#include "cognitive_grammar.tab.h"
%}

%option noyywrap
%option yylineno

DIGIT       [0-9]
LETTER      [a-zA-Z]
IDENTIFIER  {LETTER}({LETTER}|{DIGIT}|_)*
NUMBER      {DIGIT}+\.?{DIGIT}*
STRING      \"[^\"]*\"
WHITESPACE  [ \t\r]

%%

    /* Keywords for cognitive grammar constructs */
"concept"       { return CONCEPT; }
"predicate"     { return PREDICATE; }
"link"          { return LINK; }
"node"          { return NODE; }
"eval"          { return EVAL; }
"exec"          { return EXEC; }
"variable"      { return VARIABLE; }
"truth"         { return TRUTH; }
"attention"     { return ATTENTION; }
"schema"        { return SCHEMA; }
"rule"          { return RULE; }
"pattern"       { return PATTERN; }
"bind"          { return BIND; }
"and"           { return AND; }
"or"            { return OR; }
"not"           { return NOT; }
"implies"       { return IMPLIES; }
"equivalent"    { return EQUIVALENT; }
"forall"        { return FORALL; }
"exists"        { return EXISTS; }

    /* Cognitive construction grammar elements */
"construction"  { return CONSTRUCTION; }
"frame"         { return FRAME; }
"semantic"      { return SEMANTIC; }
"syntactic"     { return SYNTACTIC; }
"role"          { return ROLE; }
"filler"        { return FILLER; }
"constraint"    { return CONSTRAINT; }

    /* Truth value operations */
"strength"      { return STRENGTH; }
"confidence"    { return CONFIDENCE; }

    /* Attention value components */
"sti"           { return STI; }
"lti"           { return LTI; }
"vlti"          { return VLTI; }

    /* Operators */
"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"["             { return LBRACKET; }
"]"             { return RBRACKET; }
":"             { return COLON; }
";"             { return SEMICOLON; }
","             { return COMMA; }
"="             { return EQUALS; }
"->"            { return ARROW; }
"<->"           { return DOUBLE_ARROW; }

    /* Literals */
{IDENTIFIER}    { yylval.string = strdup(yytext); return IDENTIFIER; }
{NUMBER}        { yylval.number = atof(yytext); return NUMBER; }
{STRING}        { 
                  yylval.string = strdup(yytext + 1);
                  yylval.string[strlen(yylval.string) - 1] = '\0';
                  return STRING;
                }

    /* Comments */
"#".*           { /* Single-line comment */ }
"/*"([^*]|\*[^/])*"*/" { /* Multi-line comment */ }

    /* Whitespace */
{WHITESPACE}    { /* Ignore whitespace */ }
\n              { yylineno++; }

    /* Default action for unrecognized characters */
.               { fprintf(stderr, "Unrecognized character: %s\n", yytext); }

%%

/* Additional lexer functions */
/* yyerror is defined in the parser, not here */
