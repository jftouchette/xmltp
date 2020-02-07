/***********************************************
 *
 * xmltp_encoder.c
 *
 *  Copyright (c) 2002-2003 Jocelyn Blanchet
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  (The file "COPYING" or "LICENSE" in a directory above this source file
 *  should contain a copy of the GNU Library General Public License text).
 *  -------------------------------------------------------------------------
 *
 * Encode and decode xml markup delimiter.
 *
 * LIST OF CHANGES:
 * ----------------
 *
 * 2002aug08, jbl:	Initial creation.
 * 2003jun13, jbl:	Added: "'", '"'
 * 			Fix a hang bug in xmltp_decode
 * 			when & present but no matching
 * 			escape string.
 * 
 ***********************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------ PRIVATE defines: */

#define TO_ESCAPE_1	'>'
#define TO_ESCAPE_2	'<'
#define TO_ESCAPE_3	'&'	/* Also use to search string to decode */
#define TO_ESCAPE_4	'\''
#define TO_ESCAPE_5	'"'

#define ESCAPE_1	"&gt;"
#define ESCAPE_2	"&lt;"
#define ESCAPE_3	"&amp;"
#define ESCAPE_4	"&apos;"
#define ESCAPE_5	"&quot;"

#define MAX_STRING_SIZE 255



/**********************************************
  This function replace the XML markup
  delimiters with a new sequence of
  character.

  If no escaping was done calling function
  need to use the original string because no
  move in the result buffer will be done.

  Overflow in the result buffer is not check.
  The validation is done on the size of the
  original string. Result string cannot exceed
  len(str) * 4.

  Return values:
  	0: OK no escape found or empty string
	1: OK escape found
	2: Impossible error

 **********************************************/

int xmltp_encode(char *str, char *cnv_buf)
{
	int	rc = 0,

		slen = 0,	/* Len of original string (str)     */

		from,		/* Use to move in str, point at the */
				/* start of the next part to move   */  

		to,		/* Use to move in str, point at the */
				/* end of the next part to move     */  

		cnv_to;		/* Use to move in the cnv_buf       */


	slen = strlen(str);

	/* Dont loose your time with an empty string */
	if (0 == slen) {
		return (0);
	}

	if (MAX_STRING_SIZE == slen) {
		str[MAX_STRING_SIZE] = '\0';
		/* xml2db_write_log_msg_mm(ALOG_WARN, 0, "String must be truncated to 255", "sf_escape_mk_delim"); */
	}

	/* printf("(%s)\n", str); */

	/* We need to scan the whole original string */
	for (from=0, to=0, cnv_to=0; to<slen; to++) {

		/* printf("from: %d, to: %d, cnv_to: %d, slen: %d\n", 
				from,to,cnv_to,slen); */

		if (	str[to] == TO_ESCAPE_1 || 
			str[to] == TO_ESCAPE_2 || 
			str[to] == TO_ESCAPE_3 || 
			str[to] == TO_ESCAPE_4 || 
			str[to] == TO_ESCAPE_5 ) {

			memcpy(( char *) &(cnv_buf[cnv_to]), (char *) &(str[from]), to-from);
			cnv_to += to-from;

			switch (str[to]) {
				case TO_ESCAPE_1:	strcpy( (char *) &(cnv_buf[cnv_to]), ESCAPE_1);
							cnv_to += strlen(ESCAPE_1);
							break;
				case TO_ESCAPE_2:	strcpy( (char *) &(cnv_buf[cnv_to]), ESCAPE_2);
							cnv_to += strlen(ESCAPE_2);
							break;
				case TO_ESCAPE_3:	strcpy( (char *) &(cnv_buf[cnv_to]), ESCAPE_3);
							cnv_to += strlen(ESCAPE_3);
							break;
				case TO_ESCAPE_4:	strcpy( (char *) &(cnv_buf[cnv_to]), ESCAPE_4);
							cnv_to += strlen(ESCAPE_4);
							break;
				case TO_ESCAPE_5:	strcpy( (char *) &(cnv_buf[cnv_to]), ESCAPE_5);
							cnv_to += strlen(ESCAPE_5);
							break;

				/* Impossible to reach here */
				default:		return 2;
							break;

			} /* End of switch */

			/* Skip the escape character */
			from = to + 1;

		} /* End of if */
	} /* End of for */

	/* If we found at least one escape character in the original string */
	if (from != 0) {

		rc = 1;

		/* The last part of str need to be moved */
		if (from <= to) {
			memcpy( (char *) &(cnv_buf[cnv_to]), (char *)  &(str[from]), to-from+1);
			cnv_to += to-from+1;
		}
		cnv_buf[cnv_to] = '\0';
	}
	else {
		rc = 0;
	}

	return rc;

} /* End of xmltp_encode() */



/**********************************************
  Return values:
  	0: OK no escape found or empty string
	1: OK escape found

 **********************************************/

int xmltp_decode(char *str, char *cnv_buf)
{

	char	*p_search;	/* Point to & if found NULL if not */
	char	*p_in;		/* To move in str */
	char	*p_out;		/* To move in cnv_buf */
	int	sz;		/* Size to move */

	p_in = str;
	p_out = cnv_buf;

	/* We loop until we dont find a & */
	while ( (p_search = strchr(p_in, TO_ESCAPE_3) ) != NULL ) {

		/* Move what is before & to the output buffer */
		sz=p_search - p_in;
		memcpy(p_out, p_in, sz);
		p_in+=sz;
		p_out+=sz;
			
		/* Restore the original character */
		if (strncmp(p_search, ESCAPE_1, strlen(ESCAPE_1)) == 0) {
			p_out[0] = TO_ESCAPE_1;
			p_in += strlen(ESCAPE_1);
		}
		else if (strncmp(p_search, ESCAPE_2, strlen(ESCAPE_2)) == 0) {
			p_out[0] = TO_ESCAPE_2;
			p_in += strlen(ESCAPE_2);
		}
		else if (strncmp(p_search, ESCAPE_3, strlen(ESCAPE_3)) == 0) {
			p_out[0] = TO_ESCAPE_3;
			p_in += strlen(ESCAPE_3);
		}
		else if (strncmp(p_search, ESCAPE_4, strlen(ESCAPE_4)) == 0) {
			p_out[0] = TO_ESCAPE_4;
			p_in += strlen(ESCAPE_4);
		}
		else if (strncmp(p_search, ESCAPE_5, strlen(ESCAPE_5)) == 0) {
			p_out[0] = TO_ESCAPE_5;
			p_in += strlen(ESCAPE_5);
		}
		else {
			/* & found but no matching escape pattern */
			/* should not append but if it does       */
			/* we just copy the &.                    */
			p_out[0] = TO_ESCAPE_3;
			p_in += 1;
		}

		++p_out;

	} /* End of while */

	/* Move the rest of the input buffer to the output buffer */
	/* or do nothing if no decoding was done.                 */

	if (p_in != str) {

		sz = strlen(p_in);
		memcpy(p_out, p_in, sz );
		p_out+= sz;
		p_out[0] = '\0';

		return 1;
	}
	else {
		return 0;
	}

} /* End of xmltp_decode() */

/***
main()
{
char t_buf[256];
int rc;


printf("TEST TEST TEST TEST\n");
fflush(stdout);

rc = xmltp_decode("ABCD&lt;EFGH&gt;IJKL", t_buf);
printf("RC:[%ld], buf:[%s]\n", rc, t_buf);
fflush(stdout);

rc = xmltp_decode("ABCD&lt;EFGH&gt;IJKL&amp;XXXX&quot;YYYYYYY&apos;", t_buf);
printf("RC:[%ld], buf:[%s]\n", rc, t_buf);
fflush(stdout);

rc = xmltp_decode("&amp;&amp;&lt;ABCD&lt;EFGH&gt;IJKL&lt;&lt;&gt;&amp;", t_buf);
printf("RC:[%ld], buf:[%s]\n", rc, t_buf);
fflush(stdout);

rc = xmltp_decode("ABCDEFGHIJKL", t_buf);
printf("RC:[%ld], buf:[%s]\n", rc, t_buf);
fflush(stdout);

rc = xmltp_decode("A&lt BCDE&&&FGH&IJKL", t_buf);
printf("RC:[%ld], buf:[%s]\n", rc, t_buf);
fflush(stdout);

rc = xmltp_encode("ABC&DEF<HIJ>KLM'OPQ\"", t_buf);
printf("RC:[%ld], buf:[%s]\n", rc, t_buf);
fflush(stdout);

}
****/
