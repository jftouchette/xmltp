/*************************************************************************
 * xmltp_hosts.c
 * -------------
 *
 *  Copyright (c) 2003 Jocelyn Blanchet
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
 * Look for a host name and a port number from a logical xmltp server name.
 *
 * LIST OF CHANGES:
 * ----------------
 *
 * 2003feb04, jbl:	Initial creation.
 * 
 *************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "xmltp_hosts.h"

#define BUF_SIZE	2048
#define STRING_SIZE	256



/******************************************************
 * Split each line into 3 parts
 ******************************************************/

int sf_split_line (	char	*buf,
			char	*logical_name,
			char	*host_name,
			char 	*port_number )
{

int step;
int on_data_ind;
int i,j;

step = 0;
on_data_ind = 0;
i = 0;
j=0;

while ( i <= BUF_SIZE && buf[i] != '#' && buf[i] != '\0' && buf[i] != '\n' ) {

	/* printf("[%d],[%d],[%c],[%d][%d]\n", i, j, buf[i], step, on_data_ind);  */

	/* We skip tab and space to advance on next field */
	if (buf[i] == '\t' || buf [i] == ' ')	{

		/* Are we already on data (not space ot tab) */
		if (on_data_ind == 1) {

				on_data_ind = 0;

				/* Were we on the last field */
				if (step == 3) {
					/* clean exit */
					step++;				/* Step = 4, indicate that we have all the fields */
					port_number[j] = '\0';
					break;
				}
				else {
					switch (step) {
						case 1: logical_name[j] = '\0';
							break;
						case 2: host_name[j] = '\0';
							break;
						case 3: port_number[j] = '\0';
							break;
						default: 
							break;
					}
					i++;
				}
		}
		else {
			/*skip space or tab */ 
			i++;
		}
	}
	else {
		/* We are on a field */

		if (on_data_ind == 0) {		/* Starting a new field */
			on_data_ind = 1;
			j=0;
			step++;
		}

		/* Data transfert */
		switch (step) {
			case 1: logical_name[j++] = buf[i++];
				break;
			case 2: host_name[j++] = buf[i++];
				break;
			case 3: port_number[j++] = buf[i++];
				break;
			default: 
				break;
		}
	}
} /* --- End while --- */ 


/* Special processing if we get out of the loop before */
/* processing the end of the last field */

if ( buf[i] == '#' || buf[i] == '\0' || buf[i] == '\n') {

	/* Are we already on data (not space ot tab) */
	if (on_data_ind == 1) {

			/* Were we on the last field */
			if (step == 3) {
				/* clean exit */
				step++;				/* Step = 4, indicate that we have all the fields */
				port_number[j] = '\0';
			}
			else {
				step++;
				i++;
			}
	}
	else {
		/*skip space or tab */ 
		i++;
	}
}

if (step == 4) {
	return 0;
}
else {
	return 1;
}

} /* ------ End of sf_split_line ----------- */




/**************************************************
 *
 * read a host name and a port number from 
 * a xmltp_host file.
 *
 * format: logical_name<tab/space>host_name<tab/space>port_number
 * 	empty line and comment (#) are supported.
 *
 * Logical name found on an invalid format line will be
 * reported as not found.
 * 
 * Return values
 * -------------
 *
 * 0: Success
 * 1: File not found or any open error
 * 2: Logical name not found
 *
 ***************************************************/

int xmltp_hosts(	char	*xmltp_host_file_name,	/* File name */
			char	*logical_name,		/* xmltp hosts logical name */
			char	*host_name,		/* regular unix hosts name  */
			char	*port_number )		/* host name */
	    
{

FILE *fp;
char buf[BUF_SIZE+1];

char	t_logical_name[STRING_SIZE+1];
char	t_host_name[STRING_SIZE+1];
char	t_port_number[STRING_SIZE+1];

strcpy(host_name, "");
strcpy(port_number, "");

if (host_name == NULL) {
	printf("Host_name is nULL\n");
}


if ( (fp=fopen(xmltp_host_file_name, "r")) == NULL ) {
	return 1;
}

while (fgets(buf, BUF_SIZE, fp) != NULL)  {

	/* if line start with a comment or empty line skip line */
	if ( buf[0] != '#' && (strcmp(buf,"\n") != 0) ) {

		/* Line must be split now to check for a match */
		if (sf_split_line(buf, t_logical_name, t_host_name, t_port_number) == 0) {

			/* We end on the first match */
			if (strncmp(t_logical_name, logical_name, 32) == 0) {
				strncpy(host_name, t_host_name, STRING_SIZE);
				strncpy(port_number, t_port_number, 10);

				fclose(fp);
				return 0;
			}
		}
	}


} /* end of while */

fclose(fp);

return 2;

} /* ----- End of xmltp_hosts ---------- */



/******
main()
{
char	host_name[STRING_SIZE];
char	port_number[STRING_SIZE];
int	rc;

rc = xmltp_hosts("./hosts.txt", "ALLO", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "ALLO2", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "ALLO3", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "ALLO4", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "ALLO5", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "ALLO6", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "ALLO7", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "XYZ", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("./hosts.txt", "ALLO8", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

rc = xmltp_hosts("../durpcexc3/xmltp_hosts", "XMLOCIGX", host_name, port_number);
printf("host:[%s], port:[%s], rc:[%d]\n", host_name, port_number, rc);

}  ----- End main ------ */
