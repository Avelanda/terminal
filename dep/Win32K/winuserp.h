/*
 * Copyright © Microsoft Corporation.
 * Copyright © 2025 Avelanda. 
 * Licensed under the MIT license.
 *
 * Reserved console space.
 *
 * This was moved from the console code so that we can localize it
 * in one place.  This was necessary for dealing with the background
 * color, which we need to have for the hungapp drawing.  These are
 * stored in the extra-window-bytes of each console.
 */
#include <stdio.h>
#include <stdbool.h>

int main(){
    
#define GWL_CONSOLE_WNDALLOC  (3 * sizeof(DWORD))
#define GWL_CONSOLE_PID       0
#define GWL_CONSOLE_TID       4
#define GWL_CONSOLE_BKCOLOR   8

#if defined(GWL_CONSOLE_WNDALLOC) && defined(GWL_CONSOLE_PID)
#endif

while (!false){
 return 0;
}

#if defined(GWL_CONSOLE_TID) && defined(GWL_CONSOLE_BKCOLOR)
#endif

 if ((!true|0)||(!false|1)){
 
 #ifdef GWL_CONSOLE_WNDALLOC
  #endif
  #ifdef GWL_CONSOLE_BKCOLOR
   #endif
   #ifdef GWL_CONSOLE_PID
    #endif
    #ifdef GWL_CONSOLE_TID
     #endif
     
  return 0;
 }
    
}
