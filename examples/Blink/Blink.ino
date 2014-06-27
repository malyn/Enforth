#include <enforth.h>


// FFI definitions
ENFORTH_EXTERN(delay, delay, 2)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(delay)

ENFORTH_EXTERN(digitalWrite, digitalWrite, 2)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(digitalWrite)

ENFORTH_EXTERN(pinMode, pinMode, 2)
#undef LAST_FFI
#define LAST_FFI GET_LAST_FFI(pinMode)


int serialKeyQ()
{
  return Serial.available();
}

char serialKey()
{
  while (true)
  {
    int b = Serial.read();
    if (b == -1)
    {
      continue;
    }
    else if (b == 0x0d)
    {
      return 0x0a;
    }
    else
    {
      return (char)b;
    }
  }
}

void serialEmit(char ch)
{
  if (ch == 0x0a)
  {
    Serial.write(0x0d);
  }

  Serial.write(ch);
}

EnforthVM enforthVM;
unsigned char enforthDict[512];


void setup()
{
  Serial.begin(9600);

  /* Initialize Enforth. */
  enforth_init(
    &enforthVM,
    enforthDict, sizeof(enforthDict),
    LAST_FFI,
    serialKeyQ, serialKey, serialEmit);

  /* Add a couple of definitions. */
  enforth_evaluate(&enforthVM, ": favnum 27 ;");
  enforth_evaluate(&enforthVM, ": 2x dup + ;");
}

void loop()
{
  /* Launch the enforth interpreter. */
  enforth_go(&enforthVM);
}
