/******************************************************************
 * Copyright (C) 2002-2007 Ra-Tek. All rights reserved.
 *               By Roger Aarenstrup
 *             www.rogeraarenstrup.com
 ******************************************************************
 */
#include <stdio.h>
#include "display.h"

static int debug = DISPLAY_NO_DEBUG_INFO;

/*
** display_init
** ============
*/
void 
display_init(int debug_mode)
{
    debug = debug_mode;
}

/*
** display_register
** ================
*/
void 
display_register(void)
{
    printf("\n     ----------<State Register Clients!>----------\n");
}

/*
** display_source_added
** ====================
*/
void 
display_source_added(char *application,
                     char *variable_name,
                     int variable_size,
                     int client)
{
    printf("\n     -------<Source added!>---<%s>-------\n", application);
    printf("     ---------<%s>---<size:%d>---<client:%d>-------\n", 
           variable_name, 
           variable_size,
           client);
}

/*
** display_remove_source
** =====================
*/
void 
display_remove_source(int client)
{
    printf("     -------<Source removed!>---<client:%d>-------\n", client);
}

/*
** display_sink_added
** ==================
*/
void 
display_sink_added(char *application,
                   char *variable_name,
                   int variable_size,
                   int client)
{
    printf("\n        -------<Sink added!>---<%s>-------\n", application);
    printf("        ---------<%s>---<size:%d>---<client:%d>-------\n", 
           variable_name, 
           variable_size,
           client);
}

/*
** display_remove_sink
** ===================
*/
void 
display_remove_sink(int client)
{
    printf("        -------<Sink removed!>---<client:%d>-------\n", client);
}

/*
** display_remove_sink
** ===================
*/
void
display_refuse_sink(int client)
{
    printf("        -------<Sink refused!>---<client:%d>-------\n", client);
}

/*
** display_starting_simulation
** ===========================
*/
void 
display_starting_simulation(void)
{
    printf("\n     ----------<Starting simulation!>----------\n");
}

/*
** display_server_stopped
** ======================
*/
void display_server_stopped(void)
{  
    printf("\n     ----------<Server stopped!>----------\n");
}

/*
** display_list_complete
** =====================
*/
void 
display_list_complete(void)
{
    printf("\n     ---------<<List complete!>>---------\n");
}

/*
** display_non_fatal_error
** =======================
*/
void 
display_non_fatal_error(char *disp_msg)
{
    printf("*** ERROR *** : ");
    printf(disp_msg);
    printf("\n");
}

/*
** display_fatal_error
** ===================
*/
void 
display_fatal_error(char *disp_msg)
{
    printf("*** FATAL ERROR *** : ");
    printf(disp_msg);
    printf("\n");
}


/******************************************
 *            Debug displays              *
 ******************************************
 */

/*
** display_dbg_before_select
** =========================
*/
void 
display_dbg_before_select(void)
{
    if (debug == DISPLAY_SHOW_DEBUG_INFO)
    {
        printf("=================================\n");
    }
}

/*
** display_dbg_after_select
** ========================
*/
void 
display_dbg_after_select(int nr_eager)
{
    if (debug == DISPLAY_SHOW_DEBUG_INFO)
    {
        printf("  SELECT --->Nr eager [%d]\n", nr_eager);
        printf("=================================\n\n");
    }
}        

/*
** display_dbg_info
** ================
*/
void 
display_dbg_info(char *info)
{
    if (debug == DISPLAY_SHOW_DEBUG_INFO)
    {
        printf("%s", info);
    }
}
