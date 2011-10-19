/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * Modified by Likewise Software Corporation 2007.
 */

/* 
 * MT safe
 */

/*
#include "config.h"

#include "glib.h"
#include "galias.h"
*/
#include "lwghash.h"
#include "lwgprimes.h"


#define HASH_TABLE_MIN_SIZE 11
#define HASH_TABLE_MAX_SIZE 13845163


typedef struct _GHashNode      GHashNode;

struct _GHashNode
{
  lwgpointer   key;
  lwgpointer   value;
  GHashNode *next;
};

struct _LWGHashTable
{
  lwgint             size;
  lwgint             nnodes;
  GHashNode      **nodes;
  LWGHashFunc        hash_func;
  LWGEqualFunc       key_equal_func;
  volatile lwgint    ref_count;
  LWGDestroyNotify   key_destroy_func;
  LWGDestroyNotify   value_destroy_func;
};

#define LWG_HASH_TABLE_RESIZE(hash_table)				\
   LWG_STMT_START {						\
     if ((hash_table->size >= 3 * hash_table->nnodes &&	        \
	  hash_table->size > HASH_TABLE_MIN_SIZE) ||		\
	 (3 * hash_table->size <= hash_table->nnodes &&	        \
	  hash_table->size < HASH_TABLE_MAX_SIZE))		\
	   lwg_hash_table_resize (hash_table);			\
   } LWG_STMT_END

static void		lwg_hash_table_resize	  (LWGHashTable	  *hash_table);
static GHashNode**	lwg_hash_table_lookup_node  (LWGHashTable     *hash_table,
                                                   lwgconstpointer   key);
static GHashNode*	lwg_hash_node_new		  (lwgpointer	   key,
                                                   lwgpointer        value);
static void		lwg_hash_node_destroy	  (GHashNode	  *hash_node,
                                                   LWGDestroyNotify  key_destroy_func,
                                                   LWGDestroyNotify  value_destroy_func);
static void		lwg_hash_nodes_destroy	  (GHashNode	  *hash_node,
						  LWGDestroyNotify   key_destroy_func,
						  LWGDestroyNotify   value_destroy_func);
static lwguint lwg_hash_table_foreach_remove_or_steal (LWGHashTable     *hash_table,
                                                   LWGHRFunc	   func,
                                                   lwgpointer	   user_data,
                                                   lwgboolean        notify);


/**
 * lwg_hash_table_new:
 * @hash_func: a function to create a hash value from a key.
 *   Hash values are used to determine where keys are stored within the
 *   #LWGHashTable data structure. The lwg_direct_hash(), lwg_int_hash() and 
 *   lwg_str_hash() functions are provided for some common types of keys. 
 *   If hash_func is %NULL, lwg_direct_hash() is used.
 * @key_equal_func: a function to check two keys for equality.  This is
 *   used when looking up keys in the #LWGHashTable.  The lwg_direct_equal(),
 *   lwg_int_equal() and lwg_str_equal() functions are provided for the most
 *   common types of keys. If @key_equal_func is %NULL, keys are compared
 *   directly in a similar fashion to lwg_direct_equal(), but without the
 *   overhead of a function call.
 *
 * Creates a new #LWGHashTable with a reference count of 1.
 * 
 * Return value: a new #LWGHashTable.
 **/
LWGHashTable*
lwg_hash_table_new (LWGHashFunc    hash_func,
		  LWGEqualFunc   key_equal_func)
{
  return lwg_hash_table_new_full (hash_func, key_equal_func, NULL, NULL);
}


/**
 * lwg_hash_table_new_full:
 * @hash_func: a function to create a hash value from a key.
 * @key_equal_func: a function to check two keys for equality.
 * @key_destroy_func: a function to free the memory allocated for the key 
 *   used when removing the entry from the #LWGHashTable or %NULL if you 
 *   don't want to supply such a function.
 * @value_destroy_func: a function to free the memory allocated for the 
 *   value used when removing the entry from the #LWGHashTable or %NULL if 
 *   you don't want to supply such a function.
 * 
 * Creates a new #LWGHashTable like lwg_hash_table_new() with a reference count
 * of 1 and allows to specify functions to free the memory allocated for the
 * key and value that get called when removing the entry from the #LWGHashTable.
 * 
 * Return value: a new #LWGHashTable.
 **/
LWGHashTable*
lwg_hash_table_new_full (LWGHashFunc       hash_func,
		       LWGEqualFunc      key_equal_func,
		       LWGDestroyNotify  key_destroy_func,
		       LWGDestroyNotify  value_destroy_func)
{
  LWGHashTable *hash_table;
  
  hash_table = lwg_slice_new (LWGHashTable);
  hash_table->size               = HASH_TABLE_MIN_SIZE;
  hash_table->nnodes             = 0;
  hash_table->hash_func          = hash_func ? hash_func : lwg_direct_hash;
  hash_table->key_equal_func     = key_equal_func;
  hash_table->ref_count          = 1;
  hash_table->key_destroy_func   = key_destroy_func;
  hash_table->value_destroy_func = value_destroy_func;
  hash_table->nodes              = lwg_new0 (GHashNode*, hash_table->size);
  
  return hash_table;
}


/**
 * lwg_hash_table_ref:
 * @hash_table: a valid #LWGHashTable.
 * 
 * Atomically increments the reference count of @hash_table by one.
 * This function is MT-safe and may be called from any thread.
 * 
 * Return value: the passed in #LWGHashTable.
 * 
 * Since: 2.10
 **/
LWGHashTable*
lwg_hash_table_ref (LWGHashTable *hash_table)
{
  lwg_return_val_if_fail (hash_table != NULL, NULL);
  lwg_return_val_if_fail (hash_table->ref_count > 0, hash_table);

  lwg_atomic_int_add (&hash_table->ref_count, 1);
  return hash_table;
}

/**
 * lwg_hash_table_unref:
 * @hash_table: a valid #LWGHashTable.
 * 
 * Atomically decrements the reference count of @hash_table by one.
 * If the reference count drops to 0, all keys and values will be
 * destroyed, and all memory allocated by the hash table is released.
 * This function is MT-safe and may be called from any thread.
 * 
 * Since: 2.10
 **/
void
lwg_hash_table_unref (LWGHashTable *hash_table)
{
  lwg_return_if_fail (hash_table != NULL);
  lwg_return_if_fail (hash_table->ref_count > 0);

  if (lwg_atomic_int_exchange_and_add (&hash_table->ref_count, -1) - 1 == 0)
    {
      lwgint i;

      for (i = 0; i < hash_table->size; i++)
        lwg_hash_nodes_destroy (hash_table->nodes[i], 
                              hash_table->key_destroy_func,
                              hash_table->value_destroy_func);
      lwg_free (hash_table->nodes);
      lwg_slice_free (LWGHashTable, hash_table);
    }
}

/**
 * lwg_hash_table_destroy:
 * @hash_table: a #LWGHashTable.
 * 
 * Destroys all keys and values in the #LWGHashTable and decrements its
 * reference count by 1. If keys and/or values are dynamically allocated,
 * you should either free them first or create the #LWGHashTable with destroy
 * notifiers using lwg_hash_table_new_full(). In the latter case the destroy
 * functions you supplied will be called on all keys and values during the
 * destruction phase.
 **/
void
lwg_hash_table_destroy (LWGHashTable *hash_table)
{
  lwg_return_if_fail (hash_table != NULL);
  lwg_return_if_fail (hash_table->ref_count > 0);
  
  lwg_hash_table_remove_all (hash_table);
  lwg_hash_table_unref (hash_table);
}

static /*inline*/ GHashNode**
lwg_hash_table_lookup_node (LWGHashTable	*hash_table,
			  lwgconstpointer	 key)
{
  GHashNode **node;
  
  node = &hash_table->nodes
    [(* hash_table->hash_func) (key) % hash_table->size];
  
  /* Hash table lookup needs to be fast.
   *  We therefore remove the extra conditional of testing
   *  whether to call the key_equal_func or not from
   *  the inner loop.
   */
  if (hash_table->key_equal_func)
    while (*node && !(*hash_table->key_equal_func) ((*node)->key, key))
      node = &(*node)->next;
  else
    while (*node && (*node)->key != key)
      node = &(*node)->next;
  
  return node;
}

/**
 * lwg_hash_table_lookup:
 * @hash_table: a #LWGHashTable.
 * @key: the key to look up.
 * 
 * Looks up a key in a #LWGHashTable. Note that this function cannot
 * distinguish between a key that is not present and one which is present
 * and has the value %NULL. If you need this distinction, use
 * lwg_hash_table_lookup_extended().
 * 
 * Return value: the associated value, or %NULL if the key is not found.
 **/
lwgpointer
lwg_hash_table_lookup (LWGHashTable	  *hash_table,
		     lwgconstpointer key)
{
  GHashNode *node;
  
  lwg_return_val_if_fail (hash_table != NULL, NULL);
  
  node = *lwg_hash_table_lookup_node (hash_table, key);
  
  return node ? node->value : NULL;
}

/**
 * lwg_hash_table_lookup_extended:
 * @hash_table: a #LWGHashTable.
 * @lookup_key: the key to look up.
 * @orilwg_key: returns the original key.
 * @value: returns the value associated with the key.
 * 
 * Looks up a key in the #LWGHashTable, returning the original key and the
 * associated value and a #lwgboolean which is %TRUE if the key was found. This 
 * is useful if you need to free the memory allocated for the original key, 
 * for example before calling lwg_hash_table_remove().
 * 
 * Return value: %TRUE if the key was found in the #LWGHashTable.
 **/
lwgboolean
lwg_hash_table_lookup_extended (LWGHashTable    *hash_table,
			      lwgconstpointer  lookup_key,
			      lwgpointer	    *orilwg_key,
			      lwgpointer	    *value)
{
  GHashNode *node;
  
  lwg_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = *lwg_hash_table_lookup_node (hash_table, lookup_key);
  
  if (node)
    {
      if (orilwg_key)
	*orilwg_key = node->key;
      if (value)
	*value = node->value;
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * lwg_hash_table_insert:
 * @hash_table: a #LWGHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #LWGHashTable.
 * 
 * If the key already exists in the #LWGHashTable its current value is replaced
 * with the new value. If you supplied a @value_destroy_func when creating the 
 * #LWGHashTable, the old value is freed using that function. If you supplied
 * a @key_destroy_func when creating the #LWGHashTable, the passed key is freed 
 * using that function.
 **/
void
lwg_hash_table_insert (LWGHashTable *hash_table,
		     lwgpointer	 key,
		     lwgpointer	 value)
{
  GHashNode **node;
  
  lwg_return_if_fail (hash_table != NULL);
  lwg_return_if_fail (hash_table->ref_count > 0);
  
  node = lwg_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      /* do not reset node->key in this place, keeping
       * the old key is the intended behaviour. 
       * lwg_hash_table_replace() can be used instead.
       */

      /* free the passed key */
      if (hash_table->key_destroy_func)
	hash_table->key_destroy_func (key);
      
      if (hash_table->value_destroy_func)
	hash_table->value_destroy_func ((*node)->value);

      (*node)->value = value;
    }
  else
    {
      *node = lwg_hash_node_new (key, value);
      hash_table->nnodes++;
      LWG_HASH_TABLE_RESIZE (hash_table);
    }
}

/**
 * lwg_hash_table_replace:
 * @hash_table: a #LWGHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #LWGHashTable similar to 
 * lwg_hash_table_insert(). The difference is that if the key already exists 
 * in the #LWGHashTable, it gets replaced by the new key. If you supplied a 
 * @value_destroy_func when creating the #LWGHashTable, the old value is freed 
 * using that function. If you supplied a @key_destroy_func when creating the 
 * #LWGHashTable, the old key is freed using that function. 
 **/
void
lwg_hash_table_replace (LWGHashTable *hash_table,
		      lwgpointer	  key,
		      lwgpointer	  value)
{
  GHashNode **node;
  
  lwg_return_if_fail (hash_table != NULL);
  lwg_return_if_fail (hash_table->ref_count > 0);
  
  node = lwg_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      if (hash_table->key_destroy_func)
	hash_table->key_destroy_func ((*node)->key);
      
      if (hash_table->value_destroy_func)
	hash_table->value_destroy_func ((*node)->value);

      (*node)->key   = key;
      (*node)->value = value;
    }
  else
    {
      *node = lwg_hash_node_new (key, value);
      hash_table->nnodes++;
      LWG_HASH_TABLE_RESIZE (hash_table);
    }
}

/**
 * lwg_hash_table_remove:
 * @hash_table: a #LWGHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #LWGHashTable.
 *
 * If the #LWGHashTable was created using lwg_hash_table_new_full(), the
 * key and value are freed using the supplied destroy functions, otherwise
 * you have to make sure that any dynamically allocated values are freed 
 * yourself.
 * 
 * Return value: %TRUE if the key was found and removed from the #LWGHashTable.
 **/
lwgboolean
lwg_hash_table_remove (LWGHashTable	   *hash_table,
		     lwgconstpointer  key)
{
  GHashNode **node, *dest;
  
  lwg_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = lwg_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      lwg_hash_node_destroy (dest, 
			   hash_table->key_destroy_func,
			   hash_table->value_destroy_func);
      hash_table->nnodes--;
  
      LWG_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}

/**
 * lwg_hash_table_remove_all:
 * @hash_table: a #LWGHashTable
 *
 * Removes all keys and their associated values from a #LWGHashTable.
 *
 * If the #LWGHashTable was created using lwg_hash_table_new_full(), the keys
 * and values are freed using the supplied destroy functions, otherwise you
 * have to make sure that any dynamically allocated values are freed
 * yourself.
 *
 * Since: 2.12
 **/
void
lwg_hash_table_remove_all (LWGHashTable *hash_table)
{
  lwguint i;

  lwg_return_if_fail (hash_table != NULL);

  for (i = 0; i < hash_table->size; i++)
    {
      lwg_hash_nodes_destroy (hash_table->nodes[i],
                            hash_table->key_destroy_func,
                            hash_table->value_destroy_func);
      hash_table->nodes[i] = NULL;
    }
  hash_table->nnodes = 0;
  
  LWG_HASH_TABLE_RESIZE (hash_table);
}

/**
 * lwg_hash_table_steal:
 * @hash_table: a #LWGHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #LWGHashTable without
 * calling the key and value destroy functions.
 *
 * Return value: %TRUE if the key was found and removed from the #LWGHashTable.
 **/
lwgboolean
lwg_hash_table_steal (LWGHashTable    *hash_table,
                    lwgconstpointer  key)
{
  GHashNode **node, *dest;
  
  lwg_return_val_if_fail (hash_table != NULL, FALSE);
  
  node = lwg_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      lwg_hash_node_destroy (dest, NULL, NULL);
      hash_table->nnodes--;
  
      LWG_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}

/**
 * lwg_hash_table_steal_all:
 * @hash_table: a #LWGHashTable.
 *
 * Removes all keys and their associated values from a #LWGHashTable 
 * without calling the key and value destroy functions.
 *
 * Since: 2.12
 **/
void
lwg_hash_table_steal_all (LWGHashTable *hash_table)
{
  lwguint i;

  lwg_return_if_fail (hash_table != NULL);

  for (i = 0; i < hash_table->size; i++)
    {
      lwg_hash_nodes_destroy (hash_table->nodes[i], NULL, NULL);
      hash_table->nodes[i] = NULL;
    }

  hash_table->nnodes = 0;

  LWG_HASH_TABLE_RESIZE (hash_table);
}

/**
 * lwg_hash_table_foreach_remove:
 * @hash_table: a #LWGHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #LWGHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #LWGHashTable. If you supplied key or value destroy functions when creating
 * the #LWGHashTable, they are used to free the memory allocated for the removed
 * keys and values.
 * 
 * Return value: the number of key/value pairs removed.
 **/
lwguint
lwg_hash_table_foreach_remove (LWGHashTable	*hash_table,
			     LWGHRFunc	 func,
			     lwgpointer	 user_data)
{
  lwg_return_val_if_fail (hash_table != NULL, 0);
  lwg_return_val_if_fail (func != NULL, 0);
  
  return lwg_hash_table_foreach_remove_or_steal (hash_table, func, user_data, TRUE);
}

/**
 * lwg_hash_table_foreach_steal:
 * @hash_table: a #LWGHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #LWGHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #LWGHashTable, but no key or value destroy functions are called.
 * 
 * Return value: the number of key/value pairs removed.
 **/
lwguint
lwg_hash_table_foreach_steal (LWGHashTable *hash_table,
                            LWGHRFunc	func,
                            lwgpointer	user_data)
{
  lwg_return_val_if_fail (hash_table != NULL, 0);
  lwg_return_val_if_fail (func != NULL, 0);
  
  return lwg_hash_table_foreach_remove_or_steal (hash_table, func, user_data, FALSE);
}

static lwguint
lwg_hash_table_foreach_remove_or_steal (LWGHashTable *hash_table,
                                      LWGHRFunc	  func,
                                      lwgpointer	  user_data,
                                      lwgboolean    notify)
{
  GHashNode *node, *prev;
  lwgint i;
  lwguint deleted = 0;
  
  for (i = 0; i < hash_table->size; i++)
    {
    restart:
      
      prev = NULL;
      
      for (node = hash_table->nodes[i]; node; prev = node, node = node->next)
	{
	  if ((* func) (node->key, node->value, user_data))
	    {
	      deleted += 1;
	      
	      hash_table->nnodes -= 1;
	      
	      if (prev)
		{
		  prev->next = node->next;
		  lwg_hash_node_destroy (node,
				       notify ? hash_table->key_destroy_func : NULL,
				       notify ? hash_table->value_destroy_func : NULL);
		  node = prev;
		}
	      else
		{
		  hash_table->nodes[i] = node->next;
		  lwg_hash_node_destroy (node,
				       notify ? hash_table->key_destroy_func : NULL,
				       notify ? hash_table->value_destroy_func : NULL);
		  goto restart;
		}
	    }
	}
    }
  
  LWG_HASH_TABLE_RESIZE (hash_table);
  
  return deleted;
}

/**
 * lwg_hash_table_foreach:
 * @hash_table: a #LWGHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each of the key/value pairs in the
 * #LWGHashTable.  The function is passed the key and value of each
 * pair, and the given @user_data parameter.  The hash table may not
 * be modified while iterating over it (you can't add/remove
 * items). To remove all items matching a predicate, use
 * lwg_hash_table_foreach_remove().
 **/
void
lwg_hash_table_foreach (LWGHashTable *hash_table,
		      LWGHFunc	  func,
		      lwgpointer	  user_data)
{
  GHashNode *node;
  lwgint i;
  
  lwg_return_if_fail (hash_table != NULL);
  lwg_return_if_fail (func != NULL);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      (* func) (node->key, node->value, user_data);
}

/**
 * lwg_hash_table_find:
 * @hash_table: a #LWGHashTable.
 * @predicate:  function to test the key/value pairs for a certain property.
 * @user_data:  user data to pass to the function.
 * 
 * Calls the given function for key/value pairs in the #LWGHashTable until 
 * @predicate returns %TRUE.  The function is passed the key and value of 
 * each pair, and the given @user_data parameter. The hash table may not
 * be modified while iterating over it (you can't add/remove items). 
 *
 * Return value: The value of the first key/value pair is returned, for which 
 * func evaluates to %TRUE. If no pair with the requested property is found, 
 * %NULL is returned.
 *
 * Since: 2.4
 **/
lwgpointer
lwg_hash_table_find (LWGHashTable	   *hash_table,
                   LWGHRFunc	    predicate,
                   lwgpointer	    user_data)
{
  GHashNode *node;
  lwgint i;
  
  lwg_return_val_if_fail (hash_table != NULL, NULL);
  lwg_return_val_if_fail (predicate != NULL, NULL);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      if (predicate (node->key, node->value, user_data))
        return node->value;       
  return NULL;
}

/**
 * lwg_hash_table_size:
 * @hash_table: a #LWGHashTable.
 * 
 * Returns the number of elements contained in the #LWGHashTable.
 * 
 * Return value: the number of key/value pairs in the #LWGHashTable.
 **/
lwguint
lwg_hash_table_size (LWGHashTable *hash_table)
{
  lwg_return_val_if_fail (hash_table != NULL, 0);
  
  return hash_table->nnodes;
}

static void
lwg_hash_table_resize (LWGHashTable *hash_table)
{
  GHashNode **new_nodes;
  GHashNode *node;
  GHashNode *next;
  lwguint hash_val;
  lwgint new_size;
  lwgint i;

  new_size = lwg_spaced_primes_closest (hash_table->nnodes);
  new_size = CLAMP (new_size, HASH_TABLE_MIN_SIZE, HASH_TABLE_MAX_SIZE);
 
  new_nodes = lwg_new0 (GHashNode*, new_size);
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = next)
      {
	next = node->next;

	hash_val = (* hash_table->hash_func) (node->key) % new_size;

	node->next = new_nodes[hash_val];
	new_nodes[hash_val] = node;
      }
  
  lwg_free (hash_table->nodes);
  hash_table->nodes = new_nodes;
  hash_table->size = new_size;
}

static GHashNode*
lwg_hash_node_new (lwgpointer key,
		 lwgpointer value)
{
  GHashNode *hash_node = lwg_slice_new (GHashNode);
  
  hash_node->key = key;
  hash_node->value = value;
  hash_node->next = NULL;
  
  return hash_node;
}

static void
lwg_hash_node_destroy (GHashNode      *hash_node,
		     LWGDestroyNotify  key_destroy_func,
		     LWGDestroyNotify  value_destroy_func)
{
  if (key_destroy_func)
    key_destroy_func (hash_node->key);
  if (value_destroy_func)
    value_destroy_func (hash_node->value);
  lwg_slice_free (GHashNode, hash_node);
}

static void
lwg_hash_nodes_destroy (GHashNode *hash_node,
		      LWGFreeFunc  key_destroy_func,
		      LWGFreeFunc  value_destroy_func)
{
  while (hash_node)
    {
      GHashNode *next = hash_node->next;
      if (key_destroy_func)
	key_destroy_func (hash_node->key);
      if (value_destroy_func)
	value_destroy_func (hash_node->value);
      lwg_slice_free (GHashNode, hash_node);
      hash_node = next;
    }
}

lwguint
lwg_direct_hash(lwgconstpointer ptr)
{
    union { lwgconstpointer ptr; lwguint ptrval; } tmp;
    tmp.ptr = ptr;
    return tmp.ptrval;
}

/*
#define __LWG_HASH_C__
#include "galiasdef.c"
*/
