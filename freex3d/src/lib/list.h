/*

  FreeWRL support library.
  Linked lists.

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#ifndef __LIBFREEWRL_LIST_H__
#define __LIBFREEWRL_LIST_H__


////////////////////////////////////////////////////////////////////////////////////////////
// LIST
////////////////////////////////////////////////////////////////////////////////////////////
/* singly linked */

typedef struct _s_list_t {

    void *elem;
    struct _s_list_t *next;

} s_list_t;

#define ml_elem(_item) (_item->elem)
#define ml_next(_item) (_item->next)

typedef void f_free_t(void *ptr);
#ifdef DEBUG_MALLOC
extern s_list_t* _ml_new(const void *elem, int line, char *fi);
#define ml_new(elem) _ml_new(elem,__LINE__,__FILE__)
#else
extern s_list_t* ml_new(const void *elem);
#endif
extern int       ml_count(s_list_t *list);
extern s_list_t* ml_prev(s_list_t *list, s_list_t *item);
extern s_list_t* ml_last(s_list_t *list);
extern s_list_t* ml_find(s_list_t *list, s_list_t *item);
extern s_list_t* ml_find_elem(s_list_t *list, void *elem);
extern s_list_t* ml_insert(s_list_t *list, s_list_t *point, s_list_t *item);
extern s_list_t* ml_append(s_list_t *list, s_list_t *item);
extern void      ml_delete(s_list_t *list, s_list_t *item);
extern s_list_t* ml_delete_self(s_list_t *list, s_list_t *item);
extern void      ml_delete2(s_list_t *list, s_list_t *item, f_free_t f);
extern void      ml_delete_all(s_list_t *list);
extern void      ml_delete_all2(s_list_t *list, f_free_t f);
extern s_list_t* ml_get(s_list_t *list, int index);
extern void ml_enqueue(s_list_t **list, s_list_t *item);
extern s_list_t* ml_dequeue(s_list_t **list);
extern void ml_free(s_list_t *item);

#define ml_foreach(_list,_action) {\
					s_list_t *__l;\
					s_list_t *next;\
					for(__l=_list;__l!=NULL;) {\
						next = ml_next(__l); /* we need to get next from __l before action deletes element */ \
						_action;\
						__l = next; \
					}\
				  }
extern void ml_dump(s_list_t *list);
extern void ml_dump_char(s_list_t *list);


/* circlularly doubly linked */
typedef struct _cd_list_t {
    void *elem;
    struct _cd_list_t *next;
    struct _cd_list_t *prev;
} cd_list_t;

#define cdl_elem(_item) (_item->elem)
#define cdl_next(_item) (_item->next)
#define cdl_prev(_item) (_item->prev)
#define cdl_last(_head) (_head->prev)

extern cd_list_t* cdl_new(const void *elem);
extern int       cdl_count(cd_list_t *head);
extern cd_list_t* cdl_find(cd_list_t *head, cd_list_t *item);
extern cd_list_t* cdl_find_elem(cd_list_t *head, void *elem);
extern cd_list_t* cdl_insert(cd_list_t *head, cd_list_t *point, cd_list_t *item);
extern cd_list_t* cdl_append(cd_list_t *head, cd_list_t *item);
extern cd_list_t* cdl_delete(cd_list_t *head, cd_list_t *item);
extern cd_list_t* cdl_delete2(cd_list_t *head, cd_list_t *item, f_free_t f);
extern void      cdl_delete_all(cd_list_t *head);
extern void      cdl_delete_all2(cd_list_t *head, f_free_t f);
extern cd_list_t* cdl_get(cd_list_t *head, int index);

#define cdl_foreach(_head,_action) {\
					cd_list_t *__l;\
					cd_list_t *next;\
					__l=head;\
					if(__l) do {\
						next = cdl_next(__l); /* we need to get next from __l before action deletes element */ \
						_action;\
						__l = next; \
					}while(__l != head);\
				  }
extern void cdl_dump(cd_list_t *list);
extern void cdl_dump_char(cd_list_t *list);



#endif /* __LIBFREEWRL_LIST_H__ */
