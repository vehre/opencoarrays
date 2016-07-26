/* One-sided MPI implementation of Libcaf

Copyright (c) 2012-2016, Sourcery, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Sourcery, Inc., nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL SOURCERY, INC., BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#ifndef LIBCAF_H
#define LIBCAF_H

#include <stdint.h>	/* For int32_t.  */
#include <stddef.h>	/* For size_t.  */
#include <stdbool.h>

#include "libcaf-gfortran-descriptor.h"

#ifndef __GNUC__
#define __attribute__(x)
#define likely(x)       (x)
#define unlikely(x)     (x)
#else
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif

#ifdef PREFIX_NAME
#define PREFIX3(X,Y) X ## Y
#define PREFIX2(X,Y) PREFIX3(X,Y)
#define PREFIX(X) PREFIX2(PREFIX_NAME,X)
#else
#define PREFIX(X) X
#endif


/* Definitions of the Fortran 2008 standard; need to kept in sync with
   ISO_FORTRAN_ENV, cf. libgfortran.h.  */
#define STAT_UNLOCKED		0
#define STAT_LOCKED		1
#define STAT_LOCKED_OTHER_IMAGE	2
#define STAT_DUP_SYNC_IMAGES    3
#define STAT_STOPPED_IMAGE 	6000

/* Describes what type of array we are registerring. Keep in sync with
   gcc/fortran/trans.h.  */
typedef enum caf_register_t {
  CAF_REGTYPE_COARRAY_STATIC,
  CAF_REGTYPE_COARRAY_ALLOC,
  CAF_REGTYPE_LOCK_STATIC,
  CAF_REGTYPE_LOCK_ALLOC,
  CAF_REGTYPE_CRITICAL,
  CAF_REGTYPE_EVENT_STATIC,
  CAF_REGTYPE_EVENT_ALLOC
}
caf_register_t;

typedef struct caf_token_t_struct {
  void* main_token;
  void* win_addr;
  void* local_addr;
}
caf_token_t_struct;

typedef void* caf_token_t;

/* Linked list of static coarrays registered.  */
typedef struct caf_static_t {
  caf_token_t token;
  struct caf_static_t *prev;
}
caf_static_t;

/* When there is a vector subscript in this dimension, nvec == 0, otherwise,
   lower_bound, upper_bound, stride contains the bounds relative to the declared
   bounds; kind denotes the integer kind of the elements of vector[].  */
typedef struct caf_vector_t {
  size_t nvec;
  union {
    struct {
      void *vector;
      int kind;
    } v;
    struct {
      ptrdiff_t lower_bound, upper_bound, stride;
    } triplet;
  } u;
}
caf_vector_t;

typedef enum caf_ref_type_t {
  /* Reference a component of a derived type, either regular one or an
     allocatable or pointer type.  For regular ones idx in caf_reference_t is
     set to -1.  */
  CAF_REF_COMPONENT,
  /* Reference an array.  */
  CAF_REF_ARRAY
} caf_ref_type_t;

typedef enum caf_array_ref_t {
  /* No array ref.  This terminates the array ref.  */
  CAF_ARR_REF_NONE = 0,
  /* Reference array elements given by a vector.  Only for this mode
     caf_reference_t.u.a.dim[i].v is valid.  */
  CAF_ARR_REF_VECTOR,
  /* A full array ref where the full array ref (:) is *not* given in the
     source code, meaning reallocation allowed.  */
  CAF_ARR_REF_IMP_FULL,
  /* A full array ref with the (:) explicitly given in the source code.
     I.e. no reallocation allowed.  */
  CAF_ARR_REF_EXP_FULL,
  /* Reference a range on elements given by start, end and stride.  */
  CAF_ARR_REF_RANGE,
  /* Only a single item is referenced given in the start member.  */
  CAF_ARR_REF_SINGLE,
  /* An array ref of the kind (i:), where i is an arbitrary valid index in the
     array.  The index i is given in the start member.  */
  CAF_ARR_REF_OPEN_END,
  /* An array ref of the kind (:i), where the lower bound of the array ref
     is given by the remote side.  The index i is given in the end member.  */
  CAF_ARR_REF_OPEN_START
} caf_array_ref_t;

/* References to remote components of a derived type.  */
typedef struct caf_reference_t {
  /* A pointer to the next ref or NULL.  */
  struct caf_reference_t *next;
  /* The type of the reference.  */
  /* caf_ref_type_t */
  int type;
  /* The size of an item referenced in bytes.  I.e. in an array ref this is
     the factor to advance the array pointer with to get to the next item.
     For component refs this gives just the size of the element referenced.  */
  size_t item_size;
  union {
    struct {
      /* The offset (in bytes) of the component in the derived type.
	 Unused for allocatable or pointer components.  */
      ptrdiff_t offset;
      /* The index of the allocatable or pointer component in the derived
	 type.  -1 for all other components (then offset is valid).  */
      int idx;
    } c;
    struct {
      /* The mode of the array ref.  See CAF_ARR_REF_*.  */
      /* caf_array_ref_t */
      unsigned char mode[GFC_MAX_DIMENSIONS];
      /* Subscript refs (s) or vector refs (v).  */
      union {
	struct {
	  /* The start and end boundary of the ref and the stride.  */
	  int start, end, stride;
	} s;
	struct {
	  /* nvec entries of kind giving the elements to reference.  */
	  void *vector;
	  /* The number of entries in vector.  */
	  size_t nvec;
	  /* The integer kind used for the elements in vector.  */
	  int kind;
	} v;
      } dim[GFC_MAX_DIMENSIONS];
    } a;
  } u;
} caf_reference_t;

/* Common auxiliary functions: caf_auxiliary.c.  */

bool PREFIX (is_contiguous) (gfc_descriptor_t *);


/* Header for the specific implementation.  */

void PREFIX (init) (int *, char ***);
void PREFIX (finalize) (void);

int PREFIX (this_image) (int);
int PREFIX (num_images) (int, int);

void *PREFIX (register) (size_t, caf_register_t, caf_token_t *, int *, char *,
			int);
void PREFIX (deregister) (caf_token_t *, int *, char *, int);

void PREFIX (caf_get) (caf_token_t, size_t, int, gfc_descriptor_t *,
		       caf_vector_t *, gfc_descriptor_t *, int, int);
void PREFIX (caf_send) (caf_token_t, size_t, int, gfc_descriptor_t *,
                        caf_vector_t *, gfc_descriptor_t *, int, int);

void PREFIX (caf_sendget) (caf_token_t, size_t, int, gfc_descriptor_t *,
			   caf_vector_t *, caf_token_t, size_t, int,
			   gfc_descriptor_t *, caf_vector_t *, int, int);

void PREFIX (co_max) (gfc_descriptor_t *, int, int *, char *, int, int);
void PREFIX (co_min) (gfc_descriptor_t *, int, int *, char *, int, int);
void PREFIX (co_sum) (gfc_descriptor_t *, int, int *, char *, int);

void PREFIX (sync_all) (int *, char *, int);
void PREFIX (sync_images) (int, int[], int *, char *, int);

void PREFIX (error_stop_str) (const char *, int32_t)
     __attribute__ ((noreturn));
void PREFIX (error_stop) (int32_t) __attribute__ ((noreturn));

void PREFIX (atomic_define) (caf_token_t, size_t, int, void *, int *, int, int);
void PREFIX (atomic_ref) (caf_token_t, size_t, int, void *, int *, int, int);
void PREFIX (atomic_cas) (caf_token_t, size_t, int, void *, void *,
			  void *, int *, int, int);
void PREFIX (atomic_op) (int, caf_token_t, size_t, int, void *, void *,
			 int *, int, int);

void PREFIX (lock) (caf_token_t, size_t, int, int *, int *, char *, int);
void PREFIX (unlock) (caf_token_t, size_t, int, int *, char *, int);
void PREFIX (event_post) (caf_token_t, size_t, int, int *, char *, int);
void PREFIX (event_wait) (caf_token_t, size_t, int, int *, char *, int);
void PREFIX (event_query) (caf_token_t, size_t, int, int *, int *);
#endif  /* LIBCAF_H  */
