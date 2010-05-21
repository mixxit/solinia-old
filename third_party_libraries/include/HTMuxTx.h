/*

  
  					W3C Sample Code Library libwww Buffered MUX Writer Stream


!
  Buffered MUX Writer Stream
!
*/

/*
**	(c) COPYRIGHT MIT 1995.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

The MUX Writer Stream is an output stream
&nbsp;which knows how to write to a MUX channel. It is part of the
Transport interface and may be registered as
part of a Transport Object. In the
default initialization module, you can find the
HTTransportInit() function which sets up this stream as a default
transport for handling unbuffered socket write operations. 

This module is implemented by HTMuxTx.c, and it is
a part of the W3C Sample Code
Library.
*/

#ifndef HTMUXWRITE_H
#define HTMUXWRITE_H

/*

*/

#define MUX_BUFFER_SIZE    2048

extern HTOutput_new HTMuxBuffer_new;

/*
*/

#endif  /* HTMUXWRITE_H */

/*

  

  @(#) $Id: HTMuxTx.html,v 2.4 2005-11-11 14:03:15 vbancrof Exp $

*/
