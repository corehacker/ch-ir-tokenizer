General Information                                                              
===================                                                              
This package contains source code for the application that parses set of files
in a given directory and tokenizes them. It will keep track of the number of
occurances of each token and prints out statistics like their frequency of
occurance, total unique tokens, tokens occuring only once, etc.                                                                                 
                                                                                 
The source code is POSIX compliant and uses the standard C99 data types.         
                                                                                 
The application is named "ch-ir-tokenizer".

The program uses libraries developed by me which have been made open source 
earlier in my Masters. The code is found on github here:
   https://github.com/corehacker/c-modules
The libraries that are being used are ch-pal, a platform abstraction layer and
ch-utils, general utilities like lists, queues and hashmap.
                                                                                 
Code Organization                                                                
=================                                                                
The code is organized into sub-directories to help in organizing modules and
application code. Following is the structure of the code:                                          
                                                                                 
ch-ir-tokenizer

  code                                                                          
   |                                                                             
   + tokenizer                      - The actual application code.   
   |  |
   |  + ch-ir-tokenizer.c
   |  |
   |  + Makefile
   |  |
   |  ...
   |                                                                             
   + modules                                                                                                                                    
      |                                                                          
      + internal
         |
         + c-modules                                                              
            |                                                                       
            + ch-pal                - The platform abstraction layer.                  
            |                         (static library.)                                         
            + ch-utils              - General utility modules like lists, queues,         
                                      and hashmap. (static library.)                                
                                                                                 
Building The Sourcecode                                                          
=======================                                                          
1. Issue make command after issuing make clean.                                  
   % ./configure
   % make clean
   % make
   After successful execution of the above commands, the executable 
   "ch-ir-tokenizer" will be created in the current directory.                                     
   
Execution                                                                        
=========                                                                        
1. Requirements:                                                                 
   a. Export the LD_LIBRARY_PATH environment variable. A utility script is
      provided for ease.
      % chmod 755 export.sh
      % source export.sh
                                                                                 
2. Application Usage:                                                            
   Usage:                                                                        
   ./ch-ir-tokenizer <Directory To Parse> [<Hashmap Table Size>]
      Directory To Parse - Absolute or relative directory path to parse files.
      Hashmap Table Size - Table size of the hashmap. Smaller the table size 
                           slower is the run time. [Optional]
                                                                                 
Sample Execution
================
./ch-ir-tokenizer /people/cs/s/sanda/cs6322/Cranfield
30 most frequent words:
|---------+----------------------+------------+----------|
| Sl. No. |                Token | Occurances | Frequency|
|---------+----------------------+------------+----------|
|       1 |                  the |      19444 |  8.2604% | 
|       2 |                   of |      12675 |  5.3847% | 
|       3 |                  and |       6668 |  2.8328% | 
|       4 |                    a |       6239 |  2.6505% | 
|       5 |                   in |       4628 |  1.9661% | 
|       6 |                   to |       4532 |  1.9253% | 
|       7 |                   is |       4112 |  1.7469% | 
|       8 |                  for |       3492 |  1.4835% | 
|       9 |                  are |       2428 |  1.0315% | 
|      10 |                 with |       2265 |  0.9622% | 
|      11 |                   on |       1941 |  0.8246% | 
|      12 |                   at |       1834 |  0.7791% | 
|      13 |                   by |       1747 |  0.7422% | 
|      14 |                 flow |       1736 |  0.7375% | 
|      15 |                 that |       1564 |  0.6644% | 
|      16 |                   an |       1386 |  0.5888% | 
|      17 |                   be |       1271 |  0.5400% | 
|      18 |             pressure |       1132 |  0.4809% | 
|      19 |                 from |       1116 |  0.4741% | 
|      20 |                   as |       1111 |  0.4720% | 
|      21 |                 this |       1080 |  0.4588% | 
|      22 |                which |        974 |  0.4138% | 
|      23 |               number |        963 |  0.4091% | 
|      24 |             boundary |        897 |  0.3811% | 
|      25 |                    j |        892 |  0.3789% | 
|      26 |              results |        885 |  0.3760% | 
|      27 |                   it |        854 |  0.3628% | 
|      28 |                 mach |        816 |  0.3467% | 
|      29 |               theory |        775 |  0.3292% | 
|      30 |                layer |        728 |  0.3093% | 
|---------+----------------------+------------+----------|


Total Unique Tokens: 12470

Total Tokens: 235388

Tokens Occuring Only Once: 6060

Time Taken for Tokenization: 500 ms

Total Time Taken: 1388 ms

Copyright                                                                        
=========                                                                        
Copyright Sandeep Prakash (c), 2014                                              
Sandeep Prakash - 123sandy@gmail.com
