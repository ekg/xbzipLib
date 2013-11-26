/***************************************************************************
 *   Copyright (C) 2005 by Paolo Ferragina, Università di Pisa             *
 *   Contact address: ferragina@di.unipi.it								   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "xbzip_types.h"

/** Initialize the hash table according to the number of estimated
 *   tokens; the load factor is set to 10 and the lists are managed
 *   via the MTF-rule. 
 *
 * @param ht pointer to an (empty) hash table.
 * @param n estimated number of items to be inserted.
 */

void HHashtable_init(HHash_table *ht, int n)
{
  
  int i;

  ht->size  = (int) (1.2 * n + 13);     // Load factor ~ 80%
  ht->card = 0;

  ht->table = (Hash_nodeptr_array) malloc(ht->size * sizeof(Hash_node *));

  if (ht->table == NULL) {
    fprintf(stderr,"Fatal Error: Hash table allocation\n");
    exit(-1); }

  for (i=0; i < ht->size; i++)
    ht->table[i] = NULL;
}


/** Print the content of the hash table ht: token, length, count
 * occurrences, tagged cw and its length.
 *
 * @param ht pointer to the hash table to be printed out.
 */

void HHashtable_print(HHash_table *ht)
{  
  int i;
  Hash_node *p;

  printf("\n\n================== Hash Table ====================\n");
  printf("Table size = %d, number of stored objects = %d\n\n",ht->size,ht->card);

  for(i=0; i<ht->size;i++)
	  for(p = ht->table[i]; p ; p = p->next)
		printf("token = \"%s\", len= %d, code= %d\n", p->str,p->len_str,p->code);
}



/** Computes the hash value for the given string. The function was
    proposed by Ramakrishnais and Zobel in a paper appeared in:
    Int. Conf. on DB Systems for advanced applications, 1997.
 * @param s input string whose hash value has to be computed.
 * @param len length of s.
 * @param ht pointer to an hash table. 
 */
int HHashtable_func(char *s, int len, HHash_table *ht)
{   
  register int hfn;
  int hfi;
  int table_size = ht->size;

  hfn = 11;
  for (hfi=0; hfi<len ; hfi++)
    hfn = hfn ^ ((hfn<<5) + (hfn>>2) + (unsigned char) *s++);
  hfn = abs(hfn % table_size);
  return(hfn);
}


/** Searches for the given string into the passed hash table (NULL if not).
 * @param s string to be searched.
 * @param slen length of s (to manage also the NULL char).
 * @param ht pointer to the hash table to be searched.
 */
Hash_node *HHashtable_search(char *s, int slen, HHash_table *ht)
{   
  Hash_node *hsp;  

  for (hsp = ht->table[HHashtable_func(s,slen,ht)]; hsp; hsp = hsp->next)
    if ((slen == hsp->len_str)  && (memcmp(s,hsp->str,slen) == 0)) 
	return(hsp);
  return((Hash_node *)0);
}



/** Inserts the token in the hash table and returns 1 if new, 0
 *  otherwise; it also updates its code field and its number 
 *  of occurrences. 
 *
 * @param s string to be inserted.
 * @param slen length of s (to manage also the NULL char).
 * @param ht pointer to an hash table, it will be updated.
 */
int HHashtable_insert(char *s, int slen, int code, HHash_table *ht)   
{   
  int hiv;
  Hash_node *hip;
  Hash_node *table_head;
  Hash_node *hip_pred;
  char *token;

  hiv = HHashtable_func(s,slen,ht);  // compute the hash value
  hip = HHashtable_search(s,slen,ht);   // check string occurrence

  if (hip) {
    hip->count_occ += 1;
    hip->code = code;
    
    // move-to-front the searched token
    if (ht->table[hiv] != hip){

      // Find hip predecessor which surely does exist
      // because hip is not the head of the list
      for (hip_pred = ht->table[hiv]; hip_pred->next != hip; 
	   hip_pred = hip_pred->next) ;

      hip_pred->next = hip->next;   // jump hip in the old list
      table_head = ht->table[hiv];   // save the old head of the list-entry
      ht->table[hiv] = hip;        // hip is the new head of the list
      hip->next = table_head;         
    }

    return(0);

  } else {    //------ The token is new -------
    hip = (Hash_node *) malloc(sizeof(Hash_node));

    token = (char *) malloc(slen+1);
    memcpy(token,s,slen);
	token[slen] = '\0';

    if (hip == NULL){
      fprintf(stderr,"Error: Insert hash table\n");
	  exit(-1); } 

    hip->len_str = slen;
    hip->count_occ = 1;
    hip->str = token;
    hip->next = ht->table[hiv];
    hip->code = code;
	ht->table[hiv] = hip;
    ht->card += 1;
	
    return(1);
  }
}


/** Frees all elements of a hashtable. After this call, ht is an empty,
 * uninitialized HHash_table.
 * 
 * @param ht pointer to a HHash_table
 */
void HHashtable_clear(HHash_table *ht)
{
  int i;

  for ( i = 0; i < ht->size; i++ ) {
    Hash_node *hn = ht->table[i];
    while ( hn ) {
      Hash_node *toFree = hn;
      hn = hn->next;
      free(toFree->str);
      free(toFree);
    }
  }
  ht->card = 0;
  ht->size = 0;
  free(ht->table);
}


