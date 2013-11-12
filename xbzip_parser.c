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

/* ------------- xbzip functions and types---------- */
#include "xbzip.h"

// ------------------------------------------------------------------------
// Some useful macros to manage trees with arbitrary fan-out
// -------------------------------------------------------------------------
#define treeinsert_child(_u_,_childu_){							\
	{	if(_u_->leftmost_child == NULL){						\
		_u_->leftmost_child = _childu_;							\
		_u_->last_child = _childu_;								\
	} else {													\
		_u_->last_child->next_sibling = _childu_;				\
		_u_->last_child = _childu_;								\
		} } }



#define create_node(_u_,_TYPE_,_el_,_el_len_,_parent_,_pos_){			\
	{																	\
		if(ud->buffer_nodes_fill == BUFFER_NODES_SIZE){					\
			ud->buffer_nodes = (Tree_node *) malloc(sizeof(Tree_node) * BUFFER_NODES_SIZE);	\
			ud->buffer_nodes_fill=0;									\
			}															\
		_u_ = (Tree_node *)ud->buffer_nodes;							\
		ud->buffer_nodes++;												\
		ud->buffer_nodes_fill++;										\
		_u_->str = _el_;												\
		_u_->len_str = _el_len_;										\
		_u_->position = _pos_;											\
		_pos_ = _pos_ + 1;												\
		_u_->type = _TYPE_;												\
		_u_->parent = _parent_;											\
		_u_->last_child = NULL;											\
		_u_->leftmost_child = NULL;										\
		_u_->next_sibling = NULL;										\
		_u_->upward_path = NULL;										\
		if(_parent_ != NULL) {											\
			if (_parent_->upward_path != NULL){							\
				_u_->upward_path = (char *) malloc(_parent_->len_str + strlen(_parent_->upward_path) + 1); \
				memcpy(_u_->upward_path,_parent_->str,_parent_->len_str);	\
				memcpy(_u_->upward_path + _parent_->len_str,_parent_->upward_path, strlen(_parent_->upward_path));			\
				_u_->upward_path[_parent_->len_str + strlen(_parent_->upward_path)]= '\0';				\
				} else {												\
				_u_->upward_path = (char *) malloc(_parent_->len_str + 1); \
				memcpy(_u_->upward_path,_parent_->str,_parent_->len_str);	\
				_u_->upward_path[_parent_->len_str]= '\0';				\
					}													\
			}															\
		if(_parent_ != NULL) treeinsert_child(_parent_,_u_);			\
	} }




// ------------------------------------------------------------------------
// A macro to manage memory for attr and their values
// If needed, the current buffer is left and a new buffer is allocated
// -------------------------------------------------------------------------

#define update_ta_buffer(_len_){														\
	if(ud->ta_buffer_fill + _len_ + 1 > ud->ta_buffer_size){							\
		ud->ta_buffer_size = (ceil(_len_ / BUFFER_TA_SIZE) + 2) * BUFFER_TA_SIZE;		\
		ud->ta_buffer = malloc(sizeof(UChar) * ud->ta_buffer_size);						\
		ud->ta_buffer_fill=0; }	}			


// ------------------------------------------------------------------------
// Initialize the data structure passed to the XML parsing functions
// -------------------------------------------------------------------------

#define init_userdata(_x_){														\
	_x_->top_stack = -1;														\
	_x_->counter = 0;															\
	_x_->parser = p;															\
	_x_->main_text = text;														\
	_x_->text_buffer_len = 0;													\
	_x_->ta_buffer = (UChar *) malloc(sizeof(UChar) * BUFFER_TA_SIZE);			\
	_x_->ta_buffer_size = BUFFER_TA_SIZE;										\
	_x_->ta_buffer_fill = 0;													\
	_x_->buffer_nodes = (Tree_node *) malloc(sizeof(Tree_node) * BUFFER_NODES_SIZE); \
	_x_->buffer_nodes_fill=0;													\
	_x_->empty_tag_flag = 0;															\
	_x_->empty_tag_string[0] = (UChar) 255;										\
	_x_->empty_tag_string[1] = (UChar) 0;										\
	}

//**************************************************************************
// XML handlers
//
// The goal is to construct a DOM representation of the XML document
// which is sufficiently space-light to be managed efficiently.
// This representation will be used to compute the XBW-transform.
//
// In case of an empty tag, we fill it with the special char 255
// We remove it when unrolling the XBWT
//**************************************************************************

void default_hndl(void *data, const char *s, int slen) 
{
	user_data *ud;
	
	ud = (user_data *) data;
	if(ud == NULL)
		fatal_error("Unspecified -data- in char_hndl() function\n");

	if(ud->text_buffer_len == 0)
		ud->text_buffer = ud->main_text + XML_GetCurrentByteIndex(ud->parser);
	ud->text_buffer_len += slen;

	ud->empty_tag_flag = 0; // we are not in an empty tag

	s= NULL; // dummy to manage a warning
}  

void start_hndl(void *data, const char *el, const char **attr) 
{
	register int i,j;
	Tree_node *u,*v;
	user_data *ud;

	ud = (user_data *) data;
	if(ud == NULL)
		fatal_error("Unspecified -data- in start_hndl() function\n");
	
	ud->empty_tag_flag = 1; // we are possibly in an empty tag

	// Create a node for the pending text, and null the buffer
	if (ud->text_buffer_len != 0) {
		create_node(u, TEXT, ud->text_buffer, (ud->text_buffer_len), 
					(ud->stack_nodes[ud->top_stack]), ud->counter );
		ud->text_buffer_len=0;
		}

	// create node for current Tag as '<Tag'
	create_node(u, TAGATTR, ud->main_text + XML_GetCurrentByteIndex(ud->parser), strlen(el)+1, 
				ud->stack_nodes[ud->top_stack], ud->counter);

	ud->stack_nodes[++ud->top_stack] = u;

	if(ud->top_stack > MAX_NESTING)
		fatal_error("The document is too much nested!\n");

	// Manage the attributes of the current tag
	for (i = 0; attr[i]; i += 2) {

		// create node for current Attr as '@Attr'
		j=strlen(attr[i]);
		update_ta_buffer(j+2);
		ud->ta_buffer[ud->ta_buffer_fill] = '@';
		memcpy(ud->ta_buffer + ud->ta_buffer_fill + 1,attr[i],j);
		ud->ta_buffer[ud->ta_buffer_fill + 1 + j] = '\0';
		create_node(u, TAGATTR, (ud->ta_buffer + ud->ta_buffer_fill), j+1, 
					(ud->stack_nodes[ud->top_stack]), ud->counter);
		ud->ta_buffer_fill += (j+2);

		// create node for the attribute value and assume not \0 into it
		j=strlen(attr[i+1]);
		update_ta_buffer(j+1);
		memcpy(ud->ta_buffer + ud->ta_buffer_fill, attr[i+1], j);
		ud->ta_buffer[ud->ta_buffer_fill + j] = '\0';
		create_node(v, TEXT, (ud->ta_buffer + ud->ta_buffer_fill), j, u, ud->counter); 
		ud->ta_buffer_fill += (j+1);
		}
		// do not update the stack
}


void end_hndl(void *data, const char *el) 
{
	char *strndup(const char *s, size_t n);
	Tree_node *u;
	user_data *ud;

	ud = (user_data *) data;
	if( (ud == NULL) || (el == NULL) )
		fatal_error("Unspecified -data- in end_hndl() function\n");

	if (ud->empty_tag_flag == 1) {
		if (ud->text_buffer_len != 0)
			fatal_error("\nI'm in an empty tag but the text buffer is not empty! (END_HNDL)\n\n");
		ud->text_buffer_len = 1;
		ud->text_buffer = strndup(ud->empty_tag_string,2);
		ud->empty_tag_flag = 0; // reset
		}

	// Manage the pending text
	if (ud->text_buffer_len != 0) {
		create_node(u, TEXT, ud->text_buffer, ud->text_buffer_len, 
					(ud->stack_nodes[ud->top_stack]), ud->counter );
		ud->text_buffer_len=0;
		}

	// Remove the closed tag from the stack
	ud->top_stack--;
	if(ud->top_stack < 0) 
		fatal_error("\nError in managing the stack!\n");

	ud->empty_tag_flag = 0; // we are not in an empty tag
}  


void char_hndl(void *data, const char *s, int slen) 
{
	user_data *ud;

	ud = (user_data *) data;
	if(ud == NULL)
		fatal_error("Unspecified -data- in char_hndl() function\n");
	
	ud->empty_tag_flag = 0; // we are not in an empty tag

	// Catch internal/external entities and keep them expanded
	if(ud->main_text[XML_GetCurrentByteIndex(ud->parser)] == '&'){
		s=ud->main_text+XML_GetCurrentByteIndex(ud->parser);
		for(slen=0; s[slen] != ';' ; slen++) ;
		slen++;
		}

	if(ud->text_buffer_len == 0)
		ud->text_buffer = ud->main_text + XML_GetCurrentByteIndex(ud->parser);
	ud->text_buffer_len += slen;
}  




//**************************************************************************
// Building the DOM tree of the XML document given its text source
// 
// We proceed by parsing the XML source via Expat and then constructing
// a light-space tree. The key idea is to "merge" text parts which come
// split due to parser features.
//**************************************************************************


Tree_node *xml2tree(UChar *text, int text_len, int *treesize)
{	
	XML_Parser p;
	char *t;
	Tree_node *root,*rp = NULL;
	user_data ud[1];

	p = XML_ParserCreate(NULL);
	if (! p) fatal_error("Error in parser allocation!");
	XML_SetElementHandler(p, start_hndl, end_hndl);
	XML_SetCharacterDataHandler(p, char_hndl);
	XML_SetDefaultHandler(p, default_hndl);

	//Initialize the user data passed to the parser
	init_userdata(ud);
	XML_SetUserData(p, (void *) ud);

	// Create the root of the tree
	t = (UChar *) malloc(sizeof(UChar) * 10);
	memcpy(t,"<xml_xbwt",9);
	t[9]='\0';
	create_node(root,TAGATTR,t,strlen(t),rp, ud->counter);
	ud->stack_nodes[++ud->top_stack] = root;

	// Issue the parsing of the whole text
	if (! XML_Parse(p, text, text_len, 1)) {
	  fprintf(stderr, "Parse error at line %d:\n%s\n",
      XML_GetCurrentLineNumber(p),
      XML_ErrorString(XML_GetErrorCode(p)));
	  exit(-1);
	}

	// keep track of the number of tree nodes and leaves
	*treesize = ud->counter;
	XML_ParserFree(p);
	return root;
}


//**************************************************************************
// Serializes the DOM tree in pre-order
//
// Pre-order visit of the DOM tree, in order to ensure that first
// visited nodes are put in front of the array for subsequent "stable" sort.
//**************************************************************************
void tree2nodearray(Tree_node *u, Tree_node *array[], int *cursor)
{
	if(!u) {return;}
	else { 
		array[*cursor] = u; 
		(*cursor)++;
		}
	tree2nodearray(u->leftmost_child, array, cursor);
	tree2nodearray(u->next_sibling, array, cursor);
}

