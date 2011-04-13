/*  `Cest.c`
 *  ========
 *  `Cest` is a very dirty ‘testing’ library for ISO C. Nothing fancy, simply a collection of preprocessor macros
 *  and a miniscule bit of code to sequentially run your tests.
 *  
 *  ### Running
 *  Running `CEST()`s boils down to statically building your .test.c files against Cest.c, and then executing the
 *  resulting binary. As an (non-functional in this project, due to Paws conventions explained in the README)
 *  example, you might run something like (from your project directory, itself adjacent to the Cest.c directory):
 *      
 *      C -O0 -std=c99 -pedantic-errors -Wall ../Cest.c/Source/Cest.c \
 *        Source/Paws.c/Types/list/ll.tests.c \
 *        Source/Paws.c/Types/list/list.tests.c && \
 *      ./list.tests.o
 *      
 *  That example simply builds the Cest.c implementation, which includes `main()`, and then compiles your test
 *  files statically with Cest.c into a single binary you can run. This binary registers each of your tests in
 *  turn, and then executes `main()`, running all of said tests.
 *  
 *  ### Writing
 *  Writing test files using `Cest` is very simple; simply include the `Cest.c` declarations …
 *      
 *      #include "MYCODE.c"
 *      
 *      #define DECLARATIONS
 *      # include "Cest.c"
 *      
 *      # include <string.h>
 *      #undef  DECLARATIONS
 *      
 *  … and then declare `CEST()`s using the macro thusly:
 *      
 *      CEST(LL, allocate) {
 *        
 *        ASSERT(something);
 *        
 *      SUCCEED; }}
 *      
 *  Ensure that you always finish a `CEST()` with the `SUCCEED;` statement. This ensures your test is marked as
 *  passing if none of the `ASSERT()`ions `FAIL`. (Important note: the `CEST()` macro includes an opening left-
 *  bracket, which you must manually pair with a right-bracket. Generally, you wrap the `CEST` in an extra set of
 *  brackets, in addition, to attempt to ensure that syntax hilighting tools treat it properly as a body-code.)
 */

#if !defined(CEST_DECLARED)
# define     CEST_DECLARED

#define constructor __attribute__((constructor))

/*  This is the core `CEST()` macro that you utilize to declare new `CEST()`s. It expects a name for the test,
 *  split into a NAMESPACE (32 characters, think module name) and NAME (216 characters, think test description.)
 *  
 *  Make sure to end your `CEST()` blocks with a `SUCCEED;` statement!
 */
#define CEST(NAMESPACE, NAME) \
  static cest_state NAMESPACE ## __test__ ## NAME(); \
  static /* inline */ constructor void Cest_registrar_for__ ## NAMESPACE ## __test__ ## NAME() { \
    Cest.enroll(Cest.create(#NAMESPACE, #NAME, NAMESPACE ## __test__ ## NAME)); } \
  cest_state NAMESPACE ## __test__ ## NAME() { cest _this_test = Cest.of(#NAMESPACE, #NAME); //}

/*  This simple macro causes a test to `FAIL` if the passed expression returns `false`. */
#define ASSERT(FACT) \
  if (!(FACT)) \
    FAIL//;

#define ASSERT_NOT(FACT) ASSERT( !(FACT) )//;

#define     ASSERT_EQUAL(THING1, THING2) ASSERT(         (THING1) == (THING2) )//;
#define ASSERT_NOT_EQUAL(THING1, THING2) ASSERT(         (THING1) != (THING2) )//;
#define  ASSERT_STREQUAL(THING1, THING2) ASSERT( strcmp( (THING1) ,  (THING2) ) == 0)//;

#define      ASSERT_ZERO(THING)     ASSERT_EQUAL((THING), 0)//;
#define  ASSERT_NOT_ZERO(THING) ASSERT_NOT_EQUAL((THING), 0)//;
#define      ASSERT_NULL(THING)     ASSERT_EQUAL((THING), NULL)//;
#define  ASSERT_NOT_NULL(THING) ASSERT_NOT_EQUAL((THING), NULL)//;


          struct cest;
typedef   struct cest*   cest;
          struct cest_node;
typedef   struct cest_node*   cest_node;

#define FAIL    return failure//;
#define SUCCEED return success//;
#define PEND    return pending//;

typedef enum cest_state { failure, success, pending } cest_state;

/*  This is the datatype utilized to store your `CEST()`s at runtime. It simply wraps a function-pointer to your
 *  `CEST()` in with the `CEST()`’s name. */
struct cest {
  cest_state   (* function)( void );
  char            namespace[32];
  char            name[216]; /* `256 - 32 - "__test__".length == 216` */
};

// For now, we implement a shitty global linked-list of tests to run. Not my
// favourite implementation, but it will serve for the moment.
struct cest_node {
  cest        cest;
  cest_node   next;
};

/*  `Cest` is the runtime ‘family’ object to interface with this `Cest.c` library. Every `Cest.c` function must
 *  be called through `Cest->...`, including family functions and data functions.
 *  
 *  This also stores the root of the linked-list `CEST()` storage (`>first`).
 */
struct Cest {
  // ==== `Cest` family functions
  /*  `>run_all()`  simply climbs through the record of enrolled `CEST()`s, executing each set of tests. It
   *  prints colorized status information to standard-out as it runs, and returns the number of failed tests
   *  (with 0 indicating success of all tests, testable with `!()`). */
  int          (* run_all)    ( void );
  
  /*  `>of(namespace, name)` returns a pointer to the `cest` instance on the heap for the given pairing of
   *  `namespace` and `name`. Returns `NULL` in the case of no existing `cest` for that pairing. */
  cest         (* of)         ( char namespace[], char name[] );
  
  /*  `>create(namespace, name, &function)` returns a heap pointer to a new `struct cest`, initialized with
   *  copies of the parameters you provide. */
  cest         (* create)     ( char namespace[], char name[], cest_state (*function)(void) );
  
  // ==== `struct cest` data functions
  /*  `>enroll(cest)` takes a pointer to a `struct cest` and enrolls it in the queue to be executed by
   *  `>run_all()`. */
  void         (* enroll)     ( cest );
  /*  `>execute(cest)` is simply a shortcut to `(cest->function)()`. It causes the underlying function pointer to
   *  be dereferenced and executed. */
  cest_state   (* execute)    ( cest );
  
  /*  `>complete(cest, state)` marks the passed `cest` instance as “completed,” with the passed state. */
  cest_state   (* complete)   ( cest, cest_state state );
  
  // ==== Data members
  /*  The `struct cest_node` pointed to by `>first` will be the first `CEST()` to be executed when `>run_all()`
   *  gets called. */
  cest_node       first;
} extern Cest;



#endif
#if !defined(DECLARATIONS) && !defined(CEST_IMPLEMENTED) /* ============================================= BODY */
# define                                      CEST_IMPLEMENTED
# define DECLARATIONS
#   include <stdlib.h>
#   include <stdio.h>
#   include <string.h>
# undef  DECLARATIONS

/* A safer `strcpy()`, using `strncpy()` and `sizeof()` */
#define STRCPY(TO, FROM) \
  strncpy(TO, FROM, sizeof(TO)); TO[sizeof(TO) - 1] = '\0'//;

#define CSI "\033["
#define SGR "m"
static const struct { char failure[6]; char success[6]; char pending[6]; char reset[6]; }
ANSIEscapes = {
  .failure    = CSI "31" SGR,
  .success    = CSI "32" SGR,
  .pending    = CSI "33" SGR,
  .reset      = CSI "0"  SGR
};


int           Cest__run_all     (void);
cest          Cest__of          (char[], char[]);
cest          Cest__create      (char[], char[], cest_state (*)(void));

void          Cest__enroll      (cest);
cest_state    cest__execute     (cest);
cest_state    cest__complete    (cest, cest_state);

struct Cest Cest = {
  .run_all    = Cest__run_all,
  .of         = Cest__of,
  .create     = Cest__create,
  
  .enroll     = Cest__enroll,
  .execute    = cest__execute,
  .complete   = cest__complete,
  
  .first      = NULL
};

int Cest__run_all(void) {   int total, successes, pends; cest_state return_value; cest current;
                            struct cest_node *current_node = Cest.first;
  
  for (total = 0, successes = 0, pends = 0; current_node != NULL; total++, current_node = current_node->next) {
    current = current_node->cest;
    return_value = Cest.execute(current);
    if (return_value) { successes++; }
    if (return_value - 1) { pends++; }
    
    printf("%s->%s%s%s()\n", current->namespace,
      return_value ? (return_value - 1 ? ANSIEscapes.pending : ANSIEscapes.success) : ANSIEscapes.failure,
      current->name, ANSIEscapes.reset); }
  
  printf("%s%d successes%s (of %d)\n",
    successes < total ? ANSIEscapes.failure : (pends ? ANSIEscapes.pending : ANSIEscapes.success),
    successes, ANSIEscapes.reset, total);
  
return total - successes; }

static // »
cest _Cest__of(char namespace[], char name[], cest_node last_node);
cest  Cest__of(char namespace[], char name[]) { return // »
     _Cest__of(     namespace  ,      name  ,           Cest.first); }
cest _Cest__of(char namespace[], char name[], cest_node last_node) { cest last = last_node->cest;
  if( strcmp(last->namespace, namespace) == 0
  &&  strcmp(last->name, name) == 0 ) return last;
  if( last_node->next != NULL )       return // »
     _Cest__of(     namespace  ,      name  ,           last_node->next);
                                 else exit(1337); } // Serious error condition.

cest Cest__create(char namespace[], char name[], cest_state (*function)(void)) {    cest this;
  
  this = malloc(sizeof(struct cest));
  
         this->function     = function;
  STRCPY(this->namespace    , namespace);
  STRCPY(this->name         , name);
  
return this; }

void Cest__enroll(cest a_cest) {    struct cest_node this_node = { .cest = a_cest, .next = NULL },
                                                    *current = NULL, *this;
  
         this     = malloc(sizeof(struct cest_node));
  memcpy(this, &this_node, sizeof(struct cest_node));
  
  if (Cest.first == NULL)
    Cest.first = this;
  else {
    current = Cest.first;
    while (current->next != NULL)
      current = current->next;
    current->next = this; }
  
return; }

cest_state cest__execute(cest this) { return this->function(); }

cest_state cest__complete(cest this, cest_state state) {
  // UNIMP: partial commit.
}


#if !defined(CEST__NO_AUTO)
  /*  This is compiled into every tests-executable, unless `CEST__NO_AUTO` is defined at the point that `Cest.c`
   *  is included. It simply executes `Cest->run_all()` and applies the number of failed tests (if any) as the
   *  exit value of the program. */
  int main() { return Cest.run_all(); }
#endif

#endif
