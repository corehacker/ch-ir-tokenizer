/*******************************************************************************
 * Copyright (c) 2014, Sandeep Prakash <sxp121331@utdallas.edu>
 *
 * \file   ch-ir-tokenizer.c
 *
 * \author sandeepprakash
 *
 * \date   Feb 11, 2014
 *
 * \brief
 *
 * General Information
 * ===================
 * This package contains source code for the application that parses set of files
 * in a given directory and tokenizes them. It will keep track of the number of
 * occurances of each token and prints out statistics like their frequency of
 * occurance, total unique tokens, tokens occuring only once, etc.
 *
 * The source code is POSIX compliant and uses the standard C99 data types.
 *
 * The application is named "ch-ir-tokenizer".
 *
 * The program uses libraries developed by me which have been made open source
 * earlier in my Masters. The code is found on github here:
 *    https://github.com/corehacker/c-modules
 * The libraries that are being used are ch-pal, a platform abstraction layer and
 * ch-utils, general utilities like lists, queues and hashmap.
 *
 * Building The Sourcecode
 * =======================
 * 1. Unzip the tar ball in the current directory.
 * 2. Navigate to code/tokenizer. This is where Makefile is located.
 *    % cd code/tokenizer
 * 3. Issue make command after issuing make clean.
 *    % make -f Makefile_independent clean
 *    % make -f Makefile_independent
 *    After successful execution of the above commands, the executable
 *    "ch-ir-tokenizer" will be created in the current directory.
 *
 * Execution
 * =========
 * 1. Requirements:
 *    a. Export the LD_LIBRARY_PATH environment variable. A utility script is
 *       provided for ease.
 *       % chmod 755 export.sh
 *       % source export.sh
 *
 * 2. Application Usage:
 *    Usage:
 *    ./ch-ir-tokenizer <Directory To Parse> [<Hashmap Table Size>]
 *       Directory To Parse - Absolute or relative directory path to parse files.
 *       Hashmap Table Size - Table size of the hashmap. Smaller the table size
 *                            slower is the run time. [Optional]
 *
 ******************************************************************************/

#include <ctype.h>
#include <ch-pal/exp_pal.h>
#include <ch-utils/exp_list.h>
#include <ch-utils/exp_hashmap.h>

#define DEFAULT_DIRECTORY_TO_PARSE     "./Cranfield"
#define MAX_TOKEN_SIZE                 (2048)
#define MAX_LINE_SIZE                  (16384)
#define MAX_FILENAME_LEN               (16384)
#define DEFAULT_HASHMAP_TABLE_SIZE     (1000)

static uint32_t ui_top_30_count = 0;

typedef struct _TOKEN_STATS_X
{
   uint8_t *puc_token;

   uint32_t ui_num_occurances;
} TOKEN_STATS_X;

typedef struct _TOKENIZER_CTXT_X
{
   HM_HDL hl_token_hm;

   LIST_HDL hl_token_list;

   uint32_t ui_num_unique_tokens;

   uint32_t ui_one_occur_token;

   uint32_t ui_num_tokens;

   uint32_t ui_num_docs;

   TOKEN_STATS_X xa_top_30[30];
} TOKENIZER_CTXT_X;

static void handle_token (
   TOKENIZER_CTXT_X *px_tok_ctxt,
   char *token);

static bool does_token_contain_only_numerals(
   char *token);

static void parse_line(
   TOKENIZER_CTXT_X *px_tok_ctxt,
   char *line);

static void parse_file(
   TOKENIZER_CTXT_X *px_tok_ctxt,
   char *filename);

static LIST_RET_E fn_list_compare_fn_cbk (
  LIST_NODE_DATA_X *px_app_list_node_data,
  LIST_NODE_DATA_X *px_curr_list_node_data,
  void *p_app_data);

static HM_RET_E fn_hm_for_each_cbk (
   HM_NODE_DATA_X *px_curr_node_data,
   void *p_app_data);

static LIST_RET_E fn_list_for_all_cbk(
   LIST_NODE_DATA_X *px_node_data,
   void *p_app_data);

static LIST_RET_E fn_list_for_all_delete_cbk (
   LIST_NODE_DATA_X *px_node_data,
   void *p_app_data);

static void print_usage(
   int i_argc,
   char **ppc_argv);

static void handle_token (
   TOKENIZER_CTXT_X *px_tok_ctxt,
   char *token)
{
   HM_RET_E e_hm_ret = eHM_RET_FAILURE;
   HM_NODE_DATA_X x_node_data = { eHM_KEY_TYPE_INVALID };
   TOKEN_STATS_X *px_token_stats = NULL;
   uint32_t ui_token_len = 0;

   px_tok_ctxt->ui_num_tokens++;

   (void) pal_memset (&x_node_data, 0x00, sizeof(x_node_data));
   x_node_data.e_hm_key_type = eHM_KEY_TYPE_STRING;
   x_node_data.u_hm_key.puc_str_key = token;
   e_hm_ret = hm_search_node (px_tok_ctxt->hl_token_hm, &x_node_data);
   if (eHM_RET_HM_NODE_FOUND == e_hm_ret)
   {
      px_token_stats = (TOKEN_STATS_X *) x_node_data.p_data;
      px_token_stats->ui_num_occurances++;
   }
   else
   {
      px_token_stats = pal_malloc (sizeof(TOKEN_STATS_X), NULL);

      ui_token_len = pal_strlen (token);
      px_token_stats->puc_token = pal_malloc (ui_token_len, NULL);

      pal_strncpy(px_token_stats->puc_token, token, ui_token_len);
      px_token_stats->ui_num_occurances++;
      //printf ("Count: %d\n", px_token_stats->ui_num_occurances);

      (void) pal_memset (&x_node_data, 0x00, sizeof(x_node_data));
      x_node_data.e_hm_key_type = eHM_KEY_TYPE_STRING;
      x_node_data.u_hm_key.puc_str_key = token;
      x_node_data.p_data = px_token_stats;
      x_node_data.ui_data_size = sizeof(*px_token_stats);
      e_hm_ret = hm_add_node (px_tok_ctxt->hl_token_hm, &x_node_data);
      if (eHM_RET_SUCCESS == e_hm_ret)
      {
      }
      else
      {
         printf ("Key \"%s\" Add failed: %p\n", token, x_node_data.p_data);
      }
   }
}

static bool does_token_contain_only_numerals(
   char *token)
{
   bool b_only_numerals = false;
   char c = 0;
   uint32_t ui_i = 0;

   ui_i = pal_strlen (token) - 1;

   while (ui_i > 0)
   {
      c = token[ui_i];

      if (c < '0' || c > '9')
      {
         break;
      }

      ui_i--;
   }

   if (0 == ui_i)
   {
      b_only_numerals = true;
   }

   return b_only_numerals;
}

static void parse_line(
   TOKENIZER_CTXT_X *px_tok_ctxt,
   char *line)
{
   HM_RET_E e_hm_ret = eHM_RET_FAILURE;
   char c = 0;
   uint32_t ui_i = 0;
   bool b_ignore = false;
   bool b_break = false;
   char ca_token[MAX_TOKEN_SIZE] = {0};
   uint32_t ui_token_len = 0;
   HM_NODE_DATA_X x_node_data = {eHM_KEY_TYPE_INVALID};
   TOKEN_STATS_X *px_token_stats = NULL;
   bool b_only_numerals = false;

   while (1)
   {
      c = line[ui_i];

      switch (c)
      {
         case '\0':
         {
            if (ui_token_len > 0)
            {
               ca_token [ui_token_len] = '\0';

               if ('\'' == ca_token[0] && '\'' == ca_token [ui_token_len - 1])
               {
                  pal_memmove (ca_token, &ca_token[1], ui_token_len - 2);
                  ca_token [ui_token_len - 2] = '\0';
               }

               handle_token (px_tok_ctxt, ca_token);

               (void) pal_memset (ca_token, 0x00, sizeof(ca_token));
               ui_token_len = 0;
            }
            b_break = true;
            break;
         }
         case '<':
         {
            b_ignore = true;
            break;
         }
         case '>':
         {
            b_ignore = true;
            break;
         }
         case '.':
         {
            if (ui_token_len > 0)
            {
               ca_token [ui_token_len] = '\0';

               b_only_numerals = does_token_contain_only_numerals (ca_token);
               if ((true == b_only_numerals) && ('\0' != line[ui_i + 1]) &&
                     ((line[ui_i + 1] >= '0') && (line[ui_i + 1] <= '9')))
               {
                  /*
                   * Handle the following case:
                   *    1. 10.901
                   */
                  ca_token [ui_token_len] = '.';
                  ui_token_len++;
               }
               else
               {

                  handle_token (px_tok_ctxt, ca_token);

                  (void) pal_memset (ca_token, 0x00, sizeof(ca_token));
                  ui_token_len = 0;
               }
            }
            break;
         }
         case ',':
         case '!':
         case ' ':
         case '(':
         case ')':
         case '/':
            /*
             * All these are considered as delimiters.
             */
         {
            if (ui_token_len > 0)
            {
               ca_token [ui_token_len] = '\0';

               if ('\'' == ca_token[0] && '\'' == ca_token [ui_token_len - 1])
               {
                  pal_memmove (ca_token, &ca_token[1], ui_token_len - 3);
                  ca_token [ui_token_len - 2] = '\0';
               }

               handle_token (px_tok_ctxt, ca_token);

               (void) pal_memset (ca_token, 0x00, sizeof(ca_token));
               ui_token_len = 0;
            }
            break;
         }
         default:
         {
            if (true == b_ignore)
            {
               break;
            }
            else
            {
               ca_token[ui_token_len] = tolower (c);
               ui_token_len++;
            }
         }
      }

      if (true == b_break)
      {
         break;
      }

      ui_i++;
   }
}

static void parse_file(
   TOKENIZER_CTXT_X *px_tok_ctxt,
   char *filename)
{
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_FILE_HDL hl_file_hdl = NULL;
   uint8_t uca_line[MAX_LINE_SIZE] = {0};
   uint32_t ui_line_len = 0;

   e_pal_ret = pal_fopen (&hl_file_hdl, (const uint8_t *) filename,
      (const uint8_t *) "r");
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      goto LBL_CLEANUP;
   }

   while (1)
   {
      e_pal_ret = pal_freadline (hl_file_hdl, uca_line, sizeof(uca_line),
         &ui_line_len);
      if (ePAL_RET_FILE_EOF_REACHED == e_pal_ret)
      {
         break;
      }
      parse_line (px_tok_ctxt, (char *) uca_line);
   }

   e_pal_ret = pal_fclose (hl_file_hdl);

LBL_CLEANUP:
   return;
}

static LIST_RET_E fn_list_compare_fn_cbk (
  LIST_NODE_DATA_X *px_app_list_node_data,
  LIST_NODE_DATA_X *px_curr_list_node_data,
  void *p_app_data)
{
   LIST_RET_E e_list_ret = eLIST_RET_FAILURE;
   TOKEN_STATS_X *px_app_node_data = NULL;
   TOKEN_STATS_X *px_curr_node_data = NULL;

   if ((NULL == px_curr_list_node_data) || (NULL == px_app_list_node_data))
   {
      printf("Invalid Args");
      e_list_ret = eLIST_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_app_node_data = (TOKEN_STATS_X *) px_app_list_node_data->p_data;
   px_curr_node_data = (TOKEN_STATS_X *) px_curr_list_node_data->p_data;

   if (px_curr_node_data->ui_num_occurances
      < px_app_node_data->ui_num_occurances)
   {
      e_list_ret = eLIST_RET_LIST_NODE_FOUND;
   }
   else
   {
      e_list_ret = eLIST_RET_LIST_NODE_NOT_FOUND;
   }

CLEAN_RETURN:
   return e_list_ret;
}

static HM_RET_E fn_hm_for_each_cbk (
   HM_NODE_DATA_X *px_curr_node_data,
   void *p_app_data)
{
   HM_RET_E e_hm_ret = eHM_RET_FAILURE;
   TOKEN_STATS_X *px_token_stats = NULL;
   TOKEN_STATS_X *px_token_stats_list = NULL;
   TOKENIZER_CTXT_X *px_tok_ctxt = NULL;
   LIST_NODE_DATA_X x_list_node_data = {0};
   LIST_RET_E e_list_ret = eLIST_RET_FAILURE;
   uint32_t ui_token_len = 0;

   px_tok_ctxt = (TOKENIZER_CTXT_X *) p_app_data;
   px_token_stats = (TOKEN_STATS_X *) px_curr_node_data->p_data;

   px_token_stats_list = pal_malloc (sizeof(TOKEN_STATS_X), NULL);

   ui_token_len = pal_strlen(px_token_stats->puc_token) + 1;
   px_token_stats_list->puc_token = pal_malloc (ui_token_len, NULL);
   pal_strncpy (px_token_stats_list->puc_token, px_token_stats->puc_token,
      ui_token_len);
   px_token_stats_list->ui_num_occurances = px_token_stats->ui_num_occurances;

   if (1 == px_token_stats->ui_num_occurances)
   {
      px_tok_ctxt->ui_one_occur_token++;
   }

   x_list_node_data.p_data = px_token_stats_list;
   x_list_node_data.ui_data_size = sizeof(*px_token_stats_list);
   e_list_ret = list_node_insert_sorted (px_tok_ctxt->hl_token_list,
      &x_list_node_data, fn_list_compare_fn_cbk, p_app_data);
   if (eLIST_RET_SUCCESS != e_list_ret)
   {
      if (eLIST_RET_LIST_FULL == e_list_ret)
      {
         printf ("List Full\n");
      }
      else
      {
         printf ("list_node_insert_sorted failed: %d\n", e_list_ret);
      }
      e_hm_ret = eHM_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_hm_ret = eHM_RET_SUCCESS;
   }
LBL_CLEANUP:
   return e_hm_ret;
}

static LIST_RET_E fn_list_for_all_cbk(
   LIST_NODE_DATA_X *px_node_data,
   void *p_app_data)
{
   LIST_RET_E e_error = eLIST_RET_FAILURE;
   TOKEN_STATS_X *px_list_node_data = NULL;
   TOKEN_STATS_X *px_token_stats = NULL;
   TOKENIZER_CTXT_X *px_tok_ctxt = NULL;
   double d_frequency = 0.0;

   if (NULL == px_node_data)
   {
      e_error = eLIST_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if ((NULL == px_node_data->p_data) || (0 == px_node_data->ui_data_size ||
      (sizeof (TOKEN_STATS_X) != px_node_data->ui_data_size)))
   {
      e_error = eLIST_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_list_node_data = (TOKEN_STATS_X *) px_node_data->p_data;
   px_tok_ctxt = (TOKENIZER_CTXT_X *) p_app_data;

   ui_top_30_count++;

   if (31 == ui_top_30_count)
   {
      ui_top_30_count = 0;
      e_error = eLIST_RET_FAILURE;
   }
   else
   {
      d_frequency = (((double) px_list_node_data->ui_num_occurances /
            (double) px_tok_ctxt->ui_num_tokens) * (double) 100);
      printf ("| %7d | %20s | %10d | %7.4lf%% | \n", ui_top_30_count,
         px_list_node_data->puc_token, px_list_node_data->ui_num_occurances,
         d_frequency);
      e_error = eLIST_RET_SUCCESS;
   }


CLEAN_RETURN:
   return e_error;
}

static LIST_RET_E fn_list_for_all_delete_cbk (
   LIST_NODE_DATA_X *px_node_data,
   void *p_app_data)
{
   LIST_RET_E e_error = eLIST_RET_FAILURE;
   TOKEN_STATS_X *px_list_node_data = NULL;
   HM_NODE_DATA_X x_hm_node_data = {eHM_KEY_TYPE_INVALID};
   TOKENIZER_CTXT_X *px_tok_ctxt = NULL;
   HM_RET_E e_hm_ret = eHM_RET_FAILURE;
   TOKEN_STATS_X *px_token_stats = NULL;

   if (NULL == px_node_data)
   {
      e_error = eLIST_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if ((NULL == px_node_data->p_data) || (0 == px_node_data->ui_data_size ||
      (sizeof (TOKEN_STATS_X) != px_node_data->ui_data_size)))
   {
      e_error = eLIST_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_list_node_data = (TOKEN_STATS_X *) px_node_data->p_data;
   px_tok_ctxt = (TOKENIZER_CTXT_X *) p_app_data;

   x_hm_node_data.e_hm_key_type = eHM_KEY_TYPE_STRING;
   x_hm_node_data.u_hm_key.puc_str_key = px_list_node_data->puc_token;
   e_hm_ret = hm_delete_node(px_tok_ctxt->hl_token_hm, &x_hm_node_data);
   if (eHM_RET_SUCCESS == e_hm_ret)
   {
      if (NULL != x_hm_node_data.p_data)
      {
         px_token_stats = (TOKEN_STATS_X *) x_hm_node_data.p_data;
         if (NULL != px_token_stats->puc_token)
         {
            pal_free (px_token_stats->puc_token);
            px_token_stats->puc_token = NULL;
         }

         pal_free (x_hm_node_data.p_data);
         x_hm_node_data.p_data = NULL;
      }
   }

   e_error = eLIST_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static void print_usage(
   int i_argc,
   char **ppc_argv)
{
   printf ("\n Usage:"
      "\n \t%s <Directory To Parse> [<Hashmap Table Size (Default: %d)>]"
      "\n \t\tDirectory To Parse - Absolute or relative directory path to parse files."
      "\n \t\tHashmap Table Size - Table size of the hashmap. Smaller the table "
      "size slower is the run time. [Optional: Default: %d]",
      ppc_argv[0], DEFAULT_HASHMAP_TABLE_SIZE, DEFAULT_HASHMAP_TABLE_SIZE);
   printf ("\n");
}

int main(
   int i_argc,
   char **ppc_argv)
{
   int i_ret_val = -1;
   DIR *p_dir = NULL;
   struct dirent *px_dirent = NULL;
   char ca_filename[MAX_FILENAME_LEN] = {0};
   TOKENIZER_CTXT_X x_tok_ctxt = {NULL};
   HM_RET_E e_hm_ret = eHM_RET_FAILURE;
   HM_INIT_PARAMS_X x_hm_init_params = {0};
   LIST_RET_E e_list_ret = eLIST_RET_FAILURE;
   LIST_INIT_PARAMS_X x_init_params = {0};
   uint32_t ui_start_time_ms = 0;
   uint32_t ui_end_time_ms = 0;
   uint32_t ui_diff_time_ms = 0;
   uint32_t ui_diff_time_tokenization_ms = 0;
   LIST_NODE_DATA_X x_list_node_data = {0};
   TOKEN_STATS_X *px_list_node_data = NULL;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if (i_argc < 2 || i_argc > 3)
   {
      print_usage (i_argc, ppc_argv);
      i_ret_val = -1;
      goto LBL_CLEANUP;
   }

   pal_env_init ();

   if (NULL != ppc_argv[2])
   {
      e_pal_ret = pal_atoi((uint8_t *) ppc_argv[2],
         (int32_t *) &(x_hm_init_params.ui_hm_table_size));
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         x_hm_init_params.ui_hm_table_size = DEFAULT_HASHMAP_TABLE_SIZE;
      }
   }
   else
   {
      x_hm_init_params.ui_hm_table_size = DEFAULT_HASHMAP_TABLE_SIZE;
   }


   x_hm_init_params.e_hm_key_type = eHM_KEY_TYPE_STRING;
   e_hm_ret = hm_create(&(x_tok_ctxt.hl_token_hm), &x_hm_init_params);
   if (eHM_RET_SUCCESS != e_hm_ret)
   {
      i_ret_val = -1;
      goto LBL_CLEANUP;
   }

   if (NULL == ppc_argv [1])
   {
      p_dir = opendir ((const char *) DEFAULT_DIRECTORY_TO_PARSE);
   }
   else
   {
      p_dir = opendir ((const char *) ppc_argv [1]);
   }
   ui_start_time_ms = pal_get_system_time_ms();
   if (NULL != p_dir)
   {

      while ((px_dirent = readdir (p_dir)) != NULL)
      {
         if (8 == px_dirent->d_type)
         {
            (void) pal_memset(ca_filename, 0x00, sizeof(ca_filename));

            if (NULL == ppc_argv [1])
            {
               snprintf (ca_filename, sizeof(ca_filename), "%s/%s",
                  DEFAULT_DIRECTORY_TO_PARSE, px_dirent->d_name);
            }
            else
            {
               snprintf (ca_filename, sizeof(ca_filename), "%s/%s",
                  (const char *) ppc_argv [1], px_dirent->d_name);
            }

            parse_file (&x_tok_ctxt, ca_filename);
         }
      }

      (void) closedir (p_dir);
   }

   ui_end_time_ms = pal_get_system_time_ms();
   ui_diff_time_tokenization_ms = ui_end_time_ms - ui_start_time_ms;

   e_hm_ret = hm_get_total_count(x_tok_ctxt.hl_token_hm, &(x_tok_ctxt.ui_num_unique_tokens));

   x_init_params.ui_list_max_elements = x_tok_ctxt.ui_num_tokens;
   e_list_ret = list_create(&(x_tok_ctxt.hl_token_list), &x_init_params);
   if ((eLIST_RET_SUCCESS != e_list_ret) || (NULL == x_tok_ctxt.hl_token_list))
   {
      printf("list_create failed: %d\n", e_list_ret);
      goto LBL_CLEANUP;
   }
   printf("30 most frequent words:\n");
   printf ("|-%7s-+-%20s-+-%10s-+-%7s|\n", "-------",
               "--------------------", "----------","---------");
   printf ("| %7s | %20s | %10s | %7s|\n", "Sl. No.",
            "Token", "Occurances","Frequency");
   printf ("|-%7s-+-%20s-+-%10s-+-%7s|\n", "-------",
               "--------------------", "----------","---------");
   e_hm_ret = hm_for_each(x_tok_ctxt.hl_token_hm, fn_hm_for_each_cbk, &x_tok_ctxt);
   if (eHM_RET_SUCCESS != e_hm_ret)
   {
      printf ("hm_for_each failed: %d\n", e_hm_ret);
   }

   e_list_ret = list_for_all_nodes (x_tok_ctxt.hl_token_list,
      fn_list_for_all_cbk, &x_tok_ctxt);
   if (eLIST_RET_SUCCESS != e_list_ret)
   {
      if (eLIST_RET_LIST_EMPTY == e_list_ret)
      {
         printf ("List Empty\n");
         i_ret_val = 0;
      }
      else
      {
         i_ret_val = -1;
      }
   }

   ui_end_time_ms = pal_get_system_time_ms();
   ui_diff_time_ms = ui_end_time_ms - ui_start_time_ms;

   printf ("|-%7s-+-%20s-+-%10s-+-%7s|\n", "-------",
                  "--------------------", "----------","---------");

   printf ("\n\nTotal Unique Tokens: %d\n", x_tok_ctxt.ui_num_unique_tokens);
   printf ("\nTotal Tokens: %d\n", x_tok_ctxt.ui_num_tokens);
   printf ("\nTokens Occuring Only Once: %d\n", x_tok_ctxt.ui_one_occur_token);
   printf ("\nTime Taken for Tokenization: %d ms\n", ui_diff_time_tokenization_ms);
   printf ("\nTotal Time Taken: %d ms\n", ui_diff_time_ms);

   /*
    * Do cleanup
    */
   // Clean up all the hash map items by iterating through the keys in the list.
   e_list_ret = list_for_all_nodes (x_tok_ctxt.hl_token_list,
      fn_list_for_all_delete_cbk, &x_tok_ctxt);
   if (eLIST_RET_SUCCESS != e_list_ret)
   {
      if (eLIST_RET_LIST_EMPTY == e_list_ret)
      {
         printf ("List Empty\n");
      }
   }

   // Cleanup elements in the list.
   while (1)
   {
      e_list_ret = list_node_delete_at_tail(x_tok_ctxt.hl_token_list, &x_list_node_data);
      if (eLIST_RET_SUCCESS != e_list_ret)
      {
         break;
      }
      else
      {
         if (NULL != x_list_node_data.p_data)
         {
            px_list_node_data = (TOKEN_STATS_X *) x_list_node_data.p_data;

            if (NULL != px_list_node_data->puc_token)
            {
               pal_free (px_list_node_data->puc_token);
               px_list_node_data->puc_token = NULL;
            }
            pal_free (x_list_node_data.p_data);
            x_list_node_data.p_data;
         }
      }
   }

   // Cleanup data structures.
   list_delete(x_tok_ctxt.hl_token_list);
   hm_delete(x_tok_ctxt.hl_token_hm);
   pal_env_deinit ();
   i_ret_val = 0;

LBL_CLEANUP:
   return i_ret_val;
}
