#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

/*************************************** GLOBAL CONSTANTS ***************************************/
const int   NAME_LENGTH = 20;
const int   MAX_NAMES = 100;
const int   MAXINPUT = 5000;
const char* MAINPROMPT = "-> ";
const char* ALTPROMPT = "> ";
const char  COMMENTCHAR = ';';
const int   TABCODE = 9;

/************************************ STRUCTS/TYPEDEFS/ENUMS ************************************/
typedef int NUMBER;
typedef int NAME;
struct EXPLISTREC;
typedef EXPLISTREC* EXPLIST;
enum EXPTYPE {VALEXP,VAREXP,APEXP};

struct EXPREC
{
	EXPTYPE etype;
	NUMBER num;
	NAME varble;
	NAME optr;
	EXPLIST args;
};
typedef EXPREC* EXP;

struct EXPLISTREC
{
	EXP head;
	EXPLIST tail;
};

struct VALUELISTREC
{
	NUMBER head;
	VALUELISTREC* tail;
};
typedef VALUELISTREC* VALUELIST;

struct NAMELISTREC
{
	NAME head;
	NAMELISTREC* tail;
};
typedef NAMELISTREC* NAMELIST;

struct ENVREC
{
	NAMELIST vars;
	VALUELIST values;
};
typedef ENVREC* ENV;

struct FUNDEFREC
{
	NAME funname;
	NAMELIST formals;
	EXP body;
	FUNDEFREC* nextfundef;
};
typedef FUNDEFREC* FUNDEF;

/*************************************** GLOBAL VARIABLES ***************************************/
FUNDEF fundefs;
ENV globalEnv;
EXP currentExp;
char userinput[MAXINPUT];
int inputleng, pos;
char* printNames[MAX_NAMES];
int numNames, numBuiltins;
int quitBool;

/*********************************** DATA STRUCTURE OPERATIONS ***********************************/

// mkVALEXP - return an EXP of type VALEXP with number n
EXP mkVALEXP(NUMBER n)
{
   EXP e;
   e = new EXPREC;
   e->etype = VALEXP;
   e->num = n;
   return e;
}

// mkVAREXP - return an EXP of type VAREXP with variable nm
EXP mkVAREXP(NAME nm)
{
   EXP e;
   e = new EXPREC;
   e->etype = VAREXP;
   e->varble = nm;
   return e;
}

// mkAPEXP - return EXP of type APEXP w/ operator op and arguments el
EXP mkAPEXP(NAME op, EXPLIST el)
{
   EXP  e;
   e = new EXPREC;
   e->etype = APEXP;
   e->optr = op;
   e->args = el;
   return e;
}

// mkExplist - return an EXPLIST with head e and tail el
EXPLIST mkExplist(EXP e, EXPLIST el)
{
   EXPLIST newel;
   newel =new EXPLISTREC;
   newel->head = e;
   newel->tail = el;
   return newel;
}

// mkNamelist - return a NAMELIST with head n and tail nl
NAMELIST mkNamelist(NAME nm, NAMELIST nl)
{
   NAMELIST newnl;
   newnl = new NAMELISTREC;
   newnl->head = nm;
   newnl->tail = nl;
   return newnl;
}

// mkValuelist - return an VALUELIST with head n and tail vl
VALUELIST mkValuelist(NUMBER n,  VALUELIST vl)
{
   VALUELIST newvl;
   newvl = new VALUELISTREC;
   newvl->head = n;
   newvl->tail = vl;
   return newvl;
}

// mkEnv - return an ENV with variables nl and values vl
ENV mkEnv(NAMELIST nl, VALUELIST vl)
{
   ENV rho;
   rho = new ENVREC;
   rho->vars = nl;
   rho->values = vl;
   return rho;
}

// lengthVL - return length of VALUELIST vl
int lengthVL(VALUELIST vl)
{
   int i = 0;
   while (vl != 0)
   {
	   i++;
	   vl = vl->tail;
   }
   return i;
}

// lengthNL - return length of NAMELIST nl
int lengthNL ( NAMELIST nl)
{
   int i = 0;
   while( nl !=0 )
   {
	   ++i;
	   nl = nl->tail;
   }
   return i;
}

/**************************************** NAME MANAGEMENT ****************************************/

// fetchFun - get function definition of fname from fundefs
FUNDEF fetchFun(NAME fname)
{
   FUNDEF  f = fundefs;
   while (f != 0)
   {
	   if (f->funname == fname )
	      return f;
	   f = f->nextfundef;
   }
   return 0;
}

// newFunDef - add new function fname w/ parameters nl, body e
void newFunDef(NAME fname, NAMELIST nl, EXP e)
{
  FUNDEF f;
   f = fetchFun(fname);
   if (f == 0) /* fname not yet defined as a function */
   {
	 f = new FUNDEFREC;
	 f->nextfundef = fundefs; // place new FUNDEFREC
	 fundefs = f;        // on fundefs list
   }
   f->funname = fname;
   f->formals = nl;
   f->body = e;
}

// initNames - place all pre-defined names into printNames
void initNames()
{
   int i = 0;
   fundefs = 0;
   printNames[i] = (char*)"if";     i++;  //  0
   printNames[i] = (char*)"while";  i++;  //  1
   printNames[i] = (char*)"set";    i++;  //  2
   printNames[i] = (char*)"begin";  i++;  //  3
   printNames[i] = (char*)"+";      i++;  //  4
   printNames[i] = (char*)"-";      i++;  //  5
   printNames[i] = (char*)"*";      i++;  //  6
   printNames[i] = (char*)"/";      i++;  //  7
   printNames[i] = (char*)"=";      i++;  //  8
   printNames[i] = (char*)"<";      i++;  //  9
   printNames[i] = (char*)">";      i++;  // 10
   printNames[i] = (char*)"print";        // 11
   numNames = i;
   numBuiltins = i;
}

// install - insert new name into printNames
NAME install (char* nm)
{
   int i = 0;
   while (i <= numNames)
   {
	   if (strcmp( nm,printNames[i] ) == 0)
	      break;
      i++;
   }

   if (i > numNames)
   {
      numNames = i;
      printNames[i] = new char[strlen(nm) + 1];
      strcpy(printNames[i], nm);
   }

   return i;
}

// prName - print name nm
void prName(NAME nm)
{
	cout << printNames[nm];
}

/********************************************* INPUT *********************************************/

// isDelim - check if c is a delimiter
int isDelim(char c)
{
   return ((c == '(') || (c == ')') ||(c == ' ') || (c == COMMENTCHAR));
}

// skipblanks - return next non-blank position in userinput
int skipblanks(int p)
{
   while (userinput[p] == ' ')
	++p;
   return p;
}

// matches - check if string nm matches userinput[s .. s+leng]
int matches (int s, int leng,  char* nm)
{
   int i = 0;
   while (i < leng)
   {
      if( userinput[s] != nm[i] )
         return 0;
      ++i;
      ++s;
   }
   if (!isDelim(userinput[s]))
	   return 0;
   return 1;
}

// nextchar - read next char - filter tabs and comments
void nextchar(char& c)
{
   scanf("%c", &c);
   if (c == COMMENTCHAR)
   {
	   while (c != '\n')
		scanf("%c",&c);
   }
}

// readParens - read char's, ignoring newlines, to matching ')'
void readParens()
{
   int parencnt = 1;
   char c;

   do
   {
      if (c == '\n')
         cout << ALTPROMPT;
      cout.flush();
      nextchar(c);
      pos++;
      if (pos == MAXINPUT)
      {
         cout << "User input too long\n";
         exit(1);
      }
      if (c == '\n')
         userinput[pos] = ' ';
      else
         userinput[pos] = c;
      if (c == '(')
         ++parencnt;
      if (c == ')')
         parencnt--;
	}
   while (parencnt != 0);
}

// readInput - read char's into userinput
void readInput()
{
   char c;
   cout << MAINPROMPT;
   cout.flush();
   pos = 0;
   do
	{
	   ++pos;
	   if (pos == MAXINPUT )
	   {
         cout << "User input too long\n";
         exit(1);
	   }
	   nextchar(c);
	   if (c == '\n' )
	      userinput[pos] = ' ';
	   else
		   userinput[pos] = c;
	   if (userinput[pos] == '(' )
		  readParens();
	}
	while (c != '\n');
	inputleng = pos;
	userinput[pos+1] = COMMENTCHAR; // sentinel
}

// reader - read char's into userinput; be sure input not blank 
void reader ()
{
   do
   {
      readInput();
      pos = skipblanks(1);
   }
   while (pos > inputleng);
}

// parseName - return (installed) NAME starting at userinput[pos]
NAME parseName()
{
   char nm[20]; // array to accumulate characters
   int leng; // length of name
   leng = 0;
   while ((pos <= inputleng) && !isDelim(userinput[pos]))
   { 
	   nm[leng] = userinput[pos];
	   ++pos;
	   ++leng;
   }
   if (leng == 0)
   {
	   cout << "Error: expected name, instead read: " << userinput[pos] << endl;
	   exit(1);
   }
   nm[leng] = '\0';
   pos = skipblanks(pos); // skip blanks after name
  
   return install(nm);
}

// isDigits - check if sequence of digits begins at pos
int isDigits(int pos)
{
   if ((userinput[pos] < '0') ||  (userinput[pos] > '9'))
	   return 0;
   while ((userinput[pos] >='0') && (userinput[pos] <= '9'))
	   ++pos;

   if (!isDelim(userinput[pos]))
	   return 0;
   return 1;
}

// isNumber - check if a number begins at pos
int isNumber(int pos)
{
  return (isDigits(pos) || ((userinput[pos] == '-') && isDigits(pos+1))
            || ((userinput[pos] == '+') && isDigits(pos+1)));
}

// parseVal - return number starting at userinput[pos]
NUMBER parseVal()
{
   int n = 0, sign = 1;
   if (userinput[pos] == '+')
      ++pos;
   if (userinput[pos] == '-')
   {
	   sign = -1;
	   ++pos;
   }
   while ((userinput[pos] >= '0') && (userinput[pos] <= '9'))
   {
	   n = 10 * n + userinput[pos] -'0';
	   ++pos;
   }
   pos = skipblanks(pos); // skip blanks after number
   return (NUMBER)n*sign;
}

EXPLIST parseEL();

// parseExp - return EXP starting at userinput[pos]
EXP parseExp()
{
   NAME nm;
   EXPLIST el;
   pos = skipblanks(pos);
   if (userinput[pos] == '(')
   {  // means it must be APEXP
	  pos = skipblanks(pos+1); // skip '( ..'
	  nm = parseName();
	  el = parseEL();
	  return (mkAPEXP(nm, el));
   }
   //either a value or a variable
   if (isNumber(pos))
	  return (mkVALEXP((NUMBER)parseVal()));  // VALEXP
   return (mkVAREXP((NAME)parseName())); // VAREXP
}

// parseEL - return EXPLIST starting at userinput[pos]
EXPLIST parseEL()
{
   EXP e;
   EXPLIST el;
   if (userinput[pos] == ')')
   {
	   pos = skipblanks(pos+1); // skip ') ..'
	   return 0;
   }
   e = parseExp();
   el = parseEL();
   return mkExplist(e, el);
}

// parseNL - return NAMELIST starting at userinput[pos]
NAMELIST parseNL()
{
   NAME nm;
   NAMELIST nl;
   if (userinput[pos] == ')')
   {
	   pos = skipblanks(pos+1); // skip ') ..'
	   return 0;
   }
   nm = parseName();
   nl = parseNL();
   return mkNamelist(nm, nl);
}

// parseDef - parse function definition at userinput[pos]
NAME parseDef()
{
   pos = skipblanks(pos+1); // skip '( ..'
   pos = skipblanks(pos+6); // skip 'define ..'
   NAME fname = parseName();
   pos = skipblanks(pos);
   pos = skipblanks(pos+1); // skip '( ..'
   NAMELIST nl = parseNL();
   EXP e = parseExp();
   newFunDef(fname, nl, e);
   pos = skipblanks(pos);
   pos = skipblanks(pos+1); // skip ') ..'
   return fname;
}

/***************************************** ENVIRONMENTS *****************************************/

// emptyEnv - return an environment with no bindings
ENV emptyEnv()
{
   return mkEnv(0, 0);
}

// bindVar - bind variable nm to value n in environment rho
void bindVar(NAME nm, NUMBER n, ENV rho)
{
   rho->vars = mkNamelist(nm, rho->vars);
   rho->values = mkValuelist(n, rho->values);
}

// findVar - look up nm in rho
VALUELIST findVar(NAME nm, ENV rho)
{
   NAMELIST x = rho->vars;
   VALUELIST y = rho->values;
   while (x != 0)
   {
      if (nm == x->head)
      {
         return y;
      }
      x = x->tail;
      y = y->tail;
   }
   return 0;
}

// assign - assign value n to variable nm in rho
void  assign(NAME nm, NUMBER n, ENV rho)
{
   VALUELIST varloc;
   varloc = findVar(nm, rho);
   varloc->head = n;
}

// fetch - return number bound to nm in rho
NUMBER fetch(NAME nm, ENV rho)
{
   VALUELIST  vl;
   vl = findVar(nm, rho);
   return (vl->head);
}

// isBound - check if nm is bound in rho
int isBound(NAME nm, ENV rho)
{
   return ( findVar(nm, rho) != 0 );
}

/******************************************** NUMBERS ********************************************/

// prValue - print number n
void prValue(NUMBER n)
{
   cout << n;
}

// isTrueVal - return true if n is a true (non-zero) value
int isTrueVal(NUMBER n)
{
   return (n != 0);
}

// arity - return number of arguments expected by op
int arity(int op)
{
	if (op == 11) // print
	   return 1;
	return 2;
}

// applyValueOp - apply VALUEOP op to arguments in VALUELIST vl
NUMBER applyValueOp(int op, VALUELIST vl)
{
   NUMBER  n, n1, n2;

   if (arity(op) != lengthVL(vl) )// correct number of parameters
   {
	  cout << "Wrong number of arguments to ";
	  prName(op);
	  cout <<endl;
    }
   n1 = vl->head; // 1st actual
   if (arity(op) == 2) 
   {
      n2 = vl->tail->head; //2nd actual
   }
   switch (op)
   {
	 case 4: n = n1+n2; break;
	 case 5: n = n1-n2; break;//do it
	 case 6: n = n1*n2; break;//do it
	 case 7: n = n1/n2; break;//do it
	 case 8: n = (n1 == n2); break;
	 case 9: n = (n1 < n2); break;
	 case 10: n = (n1 > n2); break;
	 case 11: prValue(n1); cout<<endl; n = n1; break;
   };//switch
   return n;
}

/****************************************** EVALUATION ******************************************/

NUMBER eval ( EXP e,  ENV rho);

// evalList - evaluate each expression in el
//this evaluates a list of expressions and 
//return the correspoding values in a valuelist
VALUELIST evalList (EXPLIST el, ENV rho)
{
    if (el == 0)
    {
       return 0;
    }
    NUMBER h = eval(el->head, rho);
    VALUELIST v = evalList(el->tail, rho);
    return mkValuelist(h, v);
}

// applyUserFun - look up definition of nm and apply to actuals
NUMBER applyUserFun(NAME nm, VALUELIST actuals)
{
    FUNDEF f = fetchFun(nm);
    ENV rho = mkEnv(f->formals, actuals);
    return eval(f->body, rho);
}

// applyCtrlOp - apply CONTROLOP op to args in rho
NUMBER applyCtrlOp(int op, EXPLIST args, ENV rho)
{
   if (op == 0) // (if ex1 ex2 ex3)
   {
      if (isTrueVal(eval(args->head, rho))) //tail = next, head = first of new lsit
      {
         return eval(args->tail->head, rho);
      }
      return eval(args->tail->tail->head, rho);
   }
   if (op == 1) // (while ex1 ex2)
   {
      while (isTrueVal(eval(args->head, rho)) != 0)
      {
         eval(args->tail->head, rho);
      }
      return isTrueVal(eval(args->head, rho));
   }
   if (op == 2) //(set x e) //CHECK
   {
      NUMBER v = eval(args->tail->head, rho);
      NAME x = args->head->varble;
      if (isBound(x, rho))
      {
         assign(x, v, rho); //check ?
      }
      else if (isBound(x, globalEnv))
      {
         assign(x, v, globalEnv);
      }
      else
      {
         bindVar(x, v, globalEnv);
      }
      return v;
   }
   if (op == 3) // ((begin e1 e2 ...en))
   {
      EXPLIST temp = args;
      NUMBER x;
      while (temp != 0)
      {
         x = eval(temp->head, rho);
         temp = temp->tail;
      }
      return x;
      //CHECK THIS ONE
   }
   //unfinished? continue
}

// eval - return value of expression e in local environment rho
NUMBER eval(EXP e, ENV rho)
{
   switch (e->etype)
   {
      case VALEXP: return (e->num);
      case VAREXP:   if (isBound(e->varble, rho))
                        return fetch(e->varble, rho);
                     if (isBound(e->varble, globalEnv))
                        return fetch(e->varble, globalEnv);
                     cout << "Undefined variable" <<endl;  
                     exit(1);
      case APEXP: if (e->optr > numBuiltins) //putting this first allows for operator overloading
                     return applyUserFun(e->optr, evalList(e->args, rho));
                  if (e->optr < 4) //applied control operator
                     return applyCtrlOp(e->optr, e->args, rho);
                  return applyValueOp(e->optr, evalList(e->args, rho));
   }
   return 0;
}

/************************************* READ-EVAL-PRINT LOOP *************************************/
int main()
{
   initNames();
   globalEnv = emptyEnv();
   quitBool = 0;

   while (!quitBool)
   {
      reader();
      if (matches(pos, 4, (char*)"quit"))
         quitBool = 1;
      else if ((userinput[pos] == '(') && matches(skipblanks(pos+1), 6, (char*)"define"))
      {
         prName(parseDef());
         cout << endl;
      }
      else
      {
			currentExp = parseExp();
			prValue(eval(currentExp, emptyEnv()));
			cout << endl << endl;
		}
	}
   return 0;
}

/*********************************** FUNCTION IMPLEMENTATIONS ***********************************/