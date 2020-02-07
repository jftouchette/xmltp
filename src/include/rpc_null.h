/*******************************************************************

 rpc_null.h

 Null string that will be use in a RPC call to indicate the 
 data type of the parameter.

 RPC_NULL_DEFAULT is quoted for char type, not quoted for int type.
 All other NULL string must not be quoted.

 LIST OF CHANGES:

 1996-04-25, jbl: Creation
 1996-02-06, jbl: Move NULL_D from not supported to supported.

 *******************************************************************/

#define RPC_NULL_DEFAULT	"NULL"		/* For compatibility with old */
						/* version only.	      */
#define RPC_CS_INT_NULL		"NULL_I"	/* NULL integer 4 bytes */	
#define RPC_CS_CHAR_NULL	"NULL_C"	/* NULL chararter */
#define RPC_CS_FLOAT_NULL	"NULL_F"	/* NULL float 8 bytes */
#define RPC_CS_DATETIME_NULL	"NULL_D"	/* NULL datetime */

#define RPC_CS_DATETIME4_NULL	"NULL_D4"	/* Not supported */
#define RPC_CS_SMALLINT_NULL	"NULL_SI"	/* Not supported */
#define RPC_CS_REAL_NULL	"NULL_R"	/* Not supported */
