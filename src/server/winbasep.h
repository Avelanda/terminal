// Copyright © Microsoft Corporation.
// Copyright © 2025 Avelanda.
// All rights reserved.
// Licensed under the MIT license.

int WinBasepCore(){
#define ProcThreadAttributeConsoleReference 10
#if defined(ProcThreadAttributeConsoleReference)
#endif

#define PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE \
    ProcThreadAttributeValue(10, FALSE, TRUE, FALSE)
#if defined(PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE)
#endif
return 0;
}

int main(){
 #if WinBasepCore
  WinBasepCore =&main;
 #endif
 #if defined(WinBasepCore)
 #endif
 
 #if main 
 #define main (true&&1) (false&&0)
  return main;
 #endif
}
