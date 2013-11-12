/*
 * Display and extract functions 
 */

#include "fm_index.h"
#include "fm_extract.h"
#include "fm_search.h"
#include "fm_mng_bits.h"	/* Function to manage bits */
#include "fm_occurences.h"
#include <string.h> /* memcpy */
#define VICINA (2) 


/*
 * Allocates snippet (which must be freed by the caller) and writes
 * text[from..to] on it. Returns in snippet_length the length of the
 * snippet actually extracted (that could be less than to-from+1 if to is
 * larger than the text size). 
 */

int extract(void * indexe, ulong from, ulong to, uchar **dest, 
			ulong *snippet_length) {

	fm_index * index = (fm_index *) indexe;
	ulong written, numchar;
	ulong row; /* numero di riga corrispondente all'ultima position */
	ulong scarto = 0;  	/* lo scarto tra la posizione richiesta e la posizione 
			    	       successiva divisibile per s->skip */
	ulong i,j;
	ulong pos_text = 0; /* last readen position */
	uchar * text;
	
	if ((from >= index->text_size) || (from >= to)){
					*dest = NULL; 
					*snippet_length = 0;
			return FM_OK; // Invalid Position
	}
	
	to = MIN(to, index->text_size-1);
	
	if(index->smalltext) { //uses Boyer-Moore algorithm
		*snippet_length = to-from+1;
		*dest = malloc(sizeof(uchar)*(*snippet_length));
		if (*dest==NULL) return FM_OUTMEM;
		for(i=0;i<*snippet_length;i++) 
				(*dest)[i] = index->text[from+i];
		//memcpy(*dest, index->text+from, (*snippet_length)*sizeof(uchar));
		return FM_OK;
	}
	
	if ((from == 0) && (to == index->text_size-1)) { // potrebbe essere conveniente anche se inferiore
			int error = fm_unbuild(index, dest, snippet_length);
			return error;
	}
	
	if(index->skip == 0) 
			return FM_NOMARKEDCHAR;
			
	if (index->skip == 0)  
			return FM_NOMARKEDCHAR; // attenzione prevedere unbuild
		
	if (to >= index->text_size) 
			to = index->text_size-1;
				
	numchar = to - from + 1;

	/*	Inizia a leggere sequenzialmente le posizioni memorizzate. 
		Appena ne trovi una che e' abbastanza (VICINA*index->skip) vicina a quella cercata
		calcola la riga corrispondente e inizia ad andare indietro. 
		Se non trovi nessuna posizione cosi' vicina usa min_pos e la riga min_row
		corrispondente.			
	*/	
	if (to+VICINA*index->skip >= index->text_size-1) { // vicina alla fine del testo la riga e' la 0
			row = index->bwt_eof_pos;
			scarto = index->text_size - to-1;
	
	} else { /* Inizia a scandire 
		     le posizioni marcate non hanno un ordine particolare 
		     vanno scandite una dopo l'altra finche non se ne trova una 
				sufficientemente VICINA (dato che non abbiamo neppure la certezza 
				che tale posizione sia marcata) per trovare la riga corrispondente.
				Ad ogni passo si ricorda la posizione piu' vicina. 
		   */
	
		   	ulong occ_char_inf = index->bwt_occ[index->specialchar]; /* # occs caratteri inferiori del carattere speciale */
			ulong marked; 			/* numero righe carattere speciale */
			ulong min_pos, min_row; /* posizione e riga piu vicine trovate */
			ulong pos_readen = 0;   /* quante posizioni ho letto? */
			ulong pos_near = VICINA * index->skip;

			min_pos = index->text_size-1; min_row = index->bwt_eof_pos;
		
			/* calcola numero posizioni marcate == numero occ carattere speciale */
		  	if(index->specialchar == index->alpha_size-1) 
    			marked = index->text_size - occ_char_inf; 
  			else 
    			marked = index->bwt_occ[index->specialchar+1] - occ_char_inf;
			
			fm_init_bit_reader(index->start_prologue_occ); /* vado sulla prima */
			/* legge posizioni finche non ne trova una suff vicina */
			while (	pos_readen < marked ) {
			    pos_text = fm_bit_read(index->log2textsize);
				
				pos_readen++;		
				if (pos_text < to)  continue;  /* posizione troppo piccola  */
				if (pos_text - to <= pos_near) break; /* trovata: pos suffientemente vicina */
				
				/* posizione grande la ricordo se e' piu piccola della minima */
				if( pos_text < min_pos ) {
						min_pos = pos_text; 
						min_row = pos_readen + occ_char_inf-1;
					}				
				
			}
		
			if (pos_readen == marked) { // nessuna posizione vicina e' stata trovata uso la + vicina
				row = min_row;
				scarto = min_pos - to;
			}
			 else { // trovata una vicina
				row = pos_readen  + occ_char_inf-1; 
			 	scarto = pos_text - to;
			 }
		}
	
	/* 
		Prendi il testo andando all'indietro da row per il numero di caratteri 
	   	che servono per arrivare alla posizione richiesta 
	 */
	text = malloc((numchar+scarto)*sizeof(uchar));
	if (text == NULL) 
			return FM_OUTMEM;

	/* conosco il primo carattere e' special_char della colonna F in quanto marcato! */
	if (row!=index->bwt_eof_pos) {
			text[0] = index->inv_char_map[index->subchar]; // specifico del tipo di marcamento
			written = go_back(index, row, scarto + numchar - 1, text+1);
			written++;
	} else written = go_back(index, row, scarto + numchar, text);

	numchar = MIN(written, numchar); 
	uchar *desti = malloc(numchar * sizeof(uchar));

	if (desti == NULL) 
			return FM_OUTMEM;
	
	/* reverse the string from the end of text */
	for (j = 0, i = written-1; j<numchar; i--, j++) 
		desti[j] = text[i];
		
	free(text);	
	*snippet_length = numchar;
	*dest = desti;
	
	return FM_OK;
}

/*
   read len chars before the position given by row using the LF mapping
   stop if the beginning of the file is encountered
   return the number of chars actually read 
*/
ulong go_back(fm_index *index, ulong row, ulong len, uchar *dest) {
	
  ulong written, curr_row, n, occ_sb[256], occ_b[256];
  uchar c, c_sb, cs;
 
  if (row != index->bwt_eof_pos) curr_row = EOF_shift(row);
  else curr_row = 0;
  

  for( written=0; written < len; ) {
  
    // fetches info from the header of the superbucket
    get_info_sb(curr_row, occ_sb, index);  
    // fetches occ into occ_b properly remapped and returns
    // the remapped code for occ_b of the char in the  specified position
    c = get_info_b(NULL_CHAR, curr_row,occ_b, WHAT_CHAR_IS, index);  
    assert(c < index->alpha_size_sb);
  
    c_sb = index->inv_map_sb[c];
    assert(c_sb < index->alpha_size);
	cs = c_sb;
  
	if (cs == index->specialchar) cs = index->subchar; // remap special char
		
    dest[written++] = index->inv_char_map[cs]; 	// store char    
    n = occ_sb[c_sb] + occ_b[c];         	    // # of occ before curr_row

	curr_row = index->bwt_occ[c_sb] + n - 1; // get next row
    if(curr_row == index->bwt_eof_pos) break;    
    curr_row = EOF_shift(curr_row);        
	
  }

  return written;
}


/*
   write in dest[] the first "len" chars of "row" (unless
   the EOF is encountered).
   return the number of chars actually read 
*/
ulong go_forw(fm_index *s, ulong row, ulong len, uchar *dest) {
	
  uchar get_firstcolumn_char(fm_index *s, ulong);
  ulong fl_map(fm_index *s, ulong, uchar);
  
  ulong written;
  uchar c,cs;

  for(written=0;written<len; ) {
  
    c = get_firstcolumn_char(s, row);
    assert(c < s->alpha_size);
	cs = c;
	if (cs == s->specialchar) cs = s->subchar; // rimappa correttamente specialchar
    dest[written++] = s->inv_char_map[cs];
    // compute the first to last mapping
    row = fl_map(s, row,c);
    // adjust row to take the EOF symbol into account
	if(row == 0) break; // row = -1
    if(row <= s->bwt_eof_pos) row -= 1;
     }
 
  return written;
}

/*
	compute the first-to-last map using binary search
*/
ulong fl_map(fm_index *s, ulong row, uchar ch) {

  ulong i, n, rank, first, last, middle;
  ulong occ_sb[s->alpha_size], occ_b[s->alpha_size];
  uchar c_b,c_sb;              // char returned by get_info
  uchar ch_b;

  // rank of c in first column 
  rank = 1 + row - s->bwt_occ[ch];
  // get position in the last column using binary search
  first = 0; last = s->text_size;
  // invariant: the desired position is within first and last
  while(first<last) {
    middle = (first+last)/2;
    /* -------------------------------------------------------------
       get the char in position middle. As a byproduct, occ_sb
       and occ_b are initialized
       ------------------------------------------------------------- */
    get_info_sb(middle, occ_sb, s);   // init occ_sb[]  
    c_b = get_info_b(NULL_CHAR, middle, occ_b, WHAT_CHAR_IS, s); // init occ_b[] 
    assert(c_b < s->alpha_size_sb);
    c_sb = s->inv_map_sb[c_b];           
    assert(c_sb < s->alpha_size);  // c_sb is the char in position middle 
  
    /* --------------------------------------------------------------
       count the # of occ of ch in [0,middle]
       -------------------------------------------------------------- */
    if(s->bool_map_sb[ch]==0)
     n=occ_sb[ch];          // no occ of ch in this superbucket
    else {
      ch_b=0;                        // get remapped code for ch
      for(i=0;i<ch;i++)                  
	if(s->bool_map_sb[i]) ch_b++;
      assert(ch_b<s->alpha_size_sb);
      n = occ_sb[ch] + occ_b[ch_b];  // # of occ of ch in [0,middle]
    }
    /* --- update first or last ------------- */
    if(n>rank)
      last=middle;
    else if (n<rank)
      first=middle+1;
    else {                      // there are exactly "rank" c's in [0,middle]
      if(c_sb==ch) 
	first=last=middle;      // found!
      else 
	last=middle;            // restrict to [first,middle)
    }
  }
  // first is the desired row in the last column
  return first;
}


/*
   return the first character of a given row.
   This routine can be improved using binary search!
*/
uchar get_firstcolumn_char(fm_index *s, ulong row)
{
  int i;

  for(i=1;i<s->alpha_size;i++)
    if(row<s->bwt_occ[i])
      return i-1;
  return s->alpha_size-1;
}


int display(void *indexe, uchar *pattern, ulong length, ulong nums, ulong *numocc, 
			uchar **snippet_text, ulong **snippet_len) {

	multi_count *groups;
	int i, num_groups = 0, error;
	uchar *snippets;
	ulong *snip_len, j, h, len;
	fm_index * index = (fm_index *) indexe;

	*numocc = 0;
	len = length + 2*nums;

	if(index->smalltext) { //uses Boyer-Moore algorithm
		ulong *occ, to, numch, k;
		int error = fm_boyermoore(index, pattern, length, &occ, numocc);
		if(error<0) return error;
		snip_len = (ulong *) malloc (sizeof (ulong) * (*numocc));
		if (snip_len == NULL)
			return FM_OUTMEM;
		
		
		*snippet_text = (uchar *) malloc (sizeof (uchar) * len *(*numocc));
		snippets = *snippet_text;
		
		if (snippets == NULL)
			return FM_OUTMEM;
		
		for(k=0;k<*numocc;k++) {
			if(occ[k]<nums) 
				to = 0;
			else 
				to = occ[k]-nums;
				
			if(occ[k]+nums+length-1>index->text_size-1) 
				numch =  index->text_size - to;
			else 
				numch = occ[k]+nums+length-to;
				
			memcpy(snippets, index->text+to, numch);
			snip_len[k] = numch;
			snippets += numch;
		}
		
		free(occ);
		*snippet_len = snip_len;
		return FM_OK;
	}
		

	/* count */
	num_groups = fm_multi_count (index, pattern, length, &groups);

	if (num_groups <= 0)
		return num_groups;

	for (i = 0; i < num_groups; i++)
		*numocc += groups[i].elements;

	snip_len = (ulong *) malloc (sizeof (ulong) * (*numocc));
	if (snip_len == NULL)
		return FM_OUTMEM;

	snippets = (uchar *) malloc (sizeof (uchar) * len *(*numocc));
	if (snippets == NULL)
		return FM_OUTMEM;
	
	h = 0;
	for (i = 0; i < num_groups; i++)
	{
		
		for(j=0; j<groups[i].elements; j++) {
	
			error = 
			fm_snippet(index, groups[i].first_row + j, length, nums, 
						snippets + h*len, &(snip_len[h]));
		
			if (error < 0)
			{
				free(snippets);
				free(snip_len);
				return error;
			}
			h++;	
		}
	}
	
	*snippet_text = snippets;
	*snippet_len = snip_len;
	free (groups);
	return FM_OK;

}




/*
   display the text sourronding a given pattern. Is is assumed that 
   the pattern starts at "row" (we do not know its position in 
   the input text), and we get the clen chars preceeding and 
   following it.
*/
   
int fm_snippet(fm_index *s, ulong row, ulong plen, ulong clen, uchar *dest, 
			   ulong *snippet_length) {

  ulong back, forw, i;
  uchar * temptext;
			   
  temptext = malloc(sizeof(uchar) * clen);
  if (temptext==NULL) return FM_OUTMEM;
			   	
  /* --- get clen chars preceding the current position --- */
  back = go_back(s, row, clen, temptext);
  assert(back <= clen);

  for(i=0; i<back; i++) // reverse temptext
	  	dest[i] = temptext[back-(i+1)];
  free(temptext);
  
  /* --- get plen+clen chars from the current position --- */
  forw = go_forw(s, row, clen+plen, dest+back);
  assert(forw <= clen+plen);
  if(forw<plen) return FM_GENERR;
  
  *snippet_length = back+forw;
  return FM_OK;
}




/* UNBUILD */

/* 
   read prologue of a .bwi file
   Output
     s->numbucs, s->buclist
*/
int read_prologue(fm_index *s)
{
  bucket_lev1 *sb;  
  ulong i, k, offset;
	  
  /* alloc superbuckets */
  s->buclist_lev1 = (bucket_lev1 *) malloc(s->num_bucs_lev1 * sizeof(bucket_lev1));
  if(s->buclist_lev1==NULL) return FM_OUTMEM; 

  /* alloc aux array for each superbucket */
  for(i=0; i< s->num_bucs_lev1; i++){
    sb = &(s->buclist_lev1[i]);

    /* allocate space for array of occurrences */
    sb->occ = (ulong *) malloc((s->alpha_size)* sizeof(ulong));
	if(sb->occ==NULL) return FM_OUTMEM;

    /* allocate space for array of boolean char map */
    sb->bool_char_map = (uchar *)malloc((s->alpha_size)*sizeof(uchar));
	if(sb->bool_char_map == NULL) return FM_OUTMEM;
  }  
	
  offset = s->start_prologue_info_sb;
   
  for(i=0; i<s->num_bucs_lev1; i++) {
    sb = &(s->buclist_lev1[i]);
 	fm_init_bit_reader(s->compress + offset);
  
    for(k=0; k<s->alpha_size; k++)     /* boolean char_map */
      sb->bool_char_map[k] = fm_bit_read(1);

   	if(i>0)   {                         // read prefix-occ 
      	for(k=0;k<s->alpha_size;k++) 
	  		memcpy(sb->occ, s->compress + offset + s->sb_bitmap_size, s->alpha_size*sizeof(ulong));
		offset += (s->alpha_size * sizeof(ulong) + s->sb_bitmap_size);
	} else offset += s->sb_bitmap_size;
	  
  }

  /* alloc array for the starting positions of the buckets */
  s->start_lev2 =  (ulong *) malloc((s->num_bucs_lev2)* sizeof(ulong));
  if(s->start_lev2 == NULL) return FM_OUTMEM;
 
  fm_init_bit_reader(s->compress + s->start_prologue_info_b);
  
  /* read the start positions of the buckets */
  for(i=0;i<s->num_bucs_lev2;i++) 
    s->start_lev2[i] = fm_bit_read(s->var_byte_rappr);
  
  return FM_OK;
}  


/* 
   	retrieve the bwt by uncompressing the data in the input file
   	Output
     	s->bwt  
*/ 
int uncompress_data(fm_index *s)
{
  int uncompress_superbucket(fm_index *s, ulong numsb, uchar *out);
  ulong i;
  int error;

  s->bwt = (uchar *) malloc(s->text_size);
  if(s->bwt == NULL)
    	return FM_OUTMEM;
 
  for(i=0; i < s->num_bucs_lev1; i++){
    	error = uncompress_superbucket(s, i, s->bwt+i*s->bucket_size_lev1);
  		if(error < 0) return error;
  }
  
  return FM_OK;
} 


/* 
   expand the superbucket num. the uncompressed data is written
   in the array out[] which should be of the appropriate size  
   (i.e. s->bucket_size_lev1 unless num is the last superbucket) 
*/
int uncompress_superbucket(fm_index *s, ulong numsb, uchar *out)
{
  bucket_lev1 sb;  
  uchar *dest, c;
  ulong sb_start, sb_end, start,  b2, temp_occ[s->alpha_size];
  int i, k, error, len, is_odd, temp_len;
 
  assert(numsb<s->num_bucs_lev1);
  sb = s->buclist_lev1[numsb];    	/* current superbucket */
  sb_start = numsb*s->bucket_size_lev1;/* starting position of superbucket */
  sb_end = MIN(sb_start+s->bucket_size_lev1, s->text_size);    
  b2 = sb_start/s->bucket_size_lev2; /* initial level 2 bucket */
	
  s->alpha_size_sb = 0;                /* build inverse char map for superbucket */
  for(k=0; k<s->alpha_size; k++) 
    if(sb.bool_char_map[k]) 
      s->inv_map_sb[s->alpha_size_sb++] = k;

  for(start=sb_start; start < sb_end; start += s->bucket_size_lev2, b2++) {
   
	len = MIN(s->bucket_size_lev2, sb_end-start); // length of bucket
    dest = out + (start - sb_start);                       
	
	fm_init_bit_reader((s->compress) + s->start_lev2[b2]); // go to start of bucket
  	is_odd = 0;
  	if((b2%2 == 0) && (start != sb_start) && (b2 != s->num_bucs_lev2-1))
		is_odd = 1; 
	
    if((start != sb_start) && (!is_odd)) // if not the first bucket and not odd skip occ
      for(k=0; k<s->alpha_size_sb; k++) 
         error = fm_integer_decode(s->int_dec_bits); /* non servono se non mtf2 */
	
	/* Compute bucket inv map */
  	s->alpha_size_b = 0;
    for(i=0; i< s->alpha_size_sb;i++)     
      if( fm_bit_read(1) ) 
		  s->inv_map_b[s->alpha_size_b++] = i;
	  
	assert(s->alpha_size_sb >= s->alpha_size_b);

    /* Applies the proper decompression routine */
    switch (s->type_compression) 
	{
      case MULTIH: /* Bzip compression of mtf-ranks */
    	/* Initialize Mtf start */
		for (i = 0; i < s->alpha_size_b; i++)
			s->mtf[i] = i;
		if(is_odd)
			temp_len = 0;
		else 
			temp_len = len-1;
		
		get_b_multihuf(temp_len, temp_occ, s, is_odd); /* temp_occ is not needed 
	  											       get_b_m modify s->mtf_seq */
	 	break;

  	  default:  
			return FM_COMPNOTSUP;
      }

    /* remap the bucket according to the superbucket s->inv_map_sb */
    for(i=0; i<len; i++) { 
      assert(s->mtf_seq[i] < s->alpha_size_sb);
	  c = s->inv_map_sb[s->mtf_seq[i]]; /* compute remapped char */
      assert(c < s->alpha_size);          
      if (is_odd) dest[len-(i+1)] = c;	/* reverse this is an odd bucket */
	  else dest[i] = c;                      
    } 
	#if TESTINFO
	/* rimuovimi */
	ulong occu[256];
	fprintf(stderr, "\n\n\n\n\nBUCKET %lu ",start);
	for(i=0;i<256;i++) occu[i] = 0;
	for(i=0;i<len; i++) occu[dest[i]]++;
	for(i=0;i<256;i++) if(occu[i]!= 0) fprintf(stderr, " (%lu = %lu) ",i, occu[i]);
	/* fine rimuovimi */
	#endif
	}
	return FM_OK;
}


/*
	compute the lf mapping (see paper)
   	Input
    	 s->bwt, s->bwt_occ[], s->text_size, s->alpha_size
   	Output
    	 s->lf   
*/  
int fm_compute_lf(fm_index * s)
{
  ulong i, occ_tmp[ALPHASIZE];
	
  /* alloc memory */
  s->lf = (ulong *) malloc(s->text_size*sizeof(ulong));
  if(s->lf == NULL)
    return FM_OUTMEM;

  /* copy bwt_occ */
  for(i=0;i<ALPHASIZE;i++)
    occ_tmp[i] = s->bwt_occ[i];

  /* now computes lf mapping */
  for(i=0;i<s->text_size;i++)
    s->lf[i] = occ_tmp[s->bwt[i]]++;
  
  return FM_OK;
}


/* 
   compute the inverse bwt using the lf mapping       
   Input
     s->bwt, s->bwt_eof_pos, s->text_size, s->lf
   Output
     s->text
*/    
int fm_invert_bwt(fm_index *s)
{
  ulong j, i;
    
  /* alloc memory */
  s->text = (uchar *) malloc(s->text_size*sizeof(uchar));
  if(s->text == NULL)
    return FM_OUTMEM;

  for(j=0,i=s->text_size-1;i>0;i--) {
    s->text[i] = s->bwt[j];
    j = s->lf[j];              // No account for EOF

    assert(j<s->text_size);

    if(j<s->bwt_eof_pos) j++; // EOF is not accounted in c[] and thus lf[]
                              // reflects the matrix without the first row.
                              // Since EOF is not coded, the lf[] is correct
                              // after bwt_eof_pos but it is -1 before.
                              // The ++ takes care of this situation.

  }
  /* i == 0 */
  s->text[i] = s->bwt[j];
  assert(j<s->text_size);
  j = s->lf[j];              // No account for EOF
  if(j<s->bwt_eof_pos) j++;
	  
  assert(j==s->bwt_eof_pos);
  free(s->lf); s->lf = NULL;
  free(s->bwt); s->bwt = NULL;
  return FM_OK;
}


static void free_unbuild_mem(fm_index *s) { 
	ulong i;
	bucket_lev1 *sb;
	
	free(s->start_lev2);

	for(i=0; i< s->num_bucs_lev1; i++) {
		sb = &(s->buclist_lev1[i]);
		free(sb->occ);
		free(sb->bool_char_map);
	}
	free(s->buclist_lev1);
}


int fm_unbuild(fm_index *s, uchar ** text, ulong *length) {
	
	int error;
	ulong  i;
	
	if ((error = read_prologue(s)) < 0 ) {
			free_unbuild_mem(s);
			return error;
	}

	if ((error = uncompress_data(s)) < 0 ) {
			free_unbuild_mem(s);
			return error;
	}

	if ((error = fm_compute_lf(s)) < 0 ) {
			free_unbuild_mem(s);
			return error;
	}

	if ((error = fm_invert_bwt(s)) < 0 ) {
			free_unbuild_mem(s);
			return error;
	}

	/* remap text */	
	for(i=0; i<s->text_size; i++) {
		if(s->text[i] == s->specialchar) s->text[i] = s->subchar;
    	s->text[i] = s->inv_char_map[s->text[i]];
	}

  	*text = s->text;
	s->text = NULL;
	free_unbuild_mem(s); /* libera memoria allocata */
	
	*length = s->text_size;
	return FM_OK;
}
