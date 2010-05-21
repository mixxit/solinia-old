/*

  
  
  					W3C Sample Code Library libwww WAIS CLIENT


!
  Declaration of W3C Sample Code WAIS MODULE
!
*/

/*
**	(c) COPYRIGHT MIT 1995.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This is the include file for the basic WAIS module that can be used together
with the core of the W3C Sample Code Library.
It contains all WAIS specific modules which are required to compile and build
the WAIS DLL.
*/

#ifndef WWWWAIS_H
#define WWWWAIS_H

/*
*/

/*
.
  System dependencies
.

The wwwsys.h file includes system-specific include
files and flags for I/O to network and disk. The only reason for this file
is that the Internet world is more complicated than Posix and ANSI.
*/

#include "wwwsys.h"

/*
.
  Library Includes
.
*/

#include "HTWAIS.h"			        /* WAIS client state machine */

/*

End of WAIS module
*/

#endif

/*

  

  @(#) $Id: WWWWAIS.html,v 2.8 1998-05-04 19:37:58 frystyk Exp $

*/
