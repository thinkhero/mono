
#ifndef __MONO_METADATA_INTERNALS_H__
#define __MONO_METADATA_INTERNALS_H__

#include "mono/metadata/image.h"
#include "mono/metadata/blob.h"
#include "mono/metadata/mempool.h"
#include "mono/utils/mono-hash.h"

struct _MonoAssembly {
	int   ref_count;
	char *basedir;
	MonoAssemblyName aname;
	GModule *aot_module;
	MonoImage *image;
	guint8 in_gac;
	guint8 dynamic;
	guint8 corlib_internal;
	gboolean ref_only;
	/* security manager flags (one bit is for lazy initialization) */
	guint32 ecma:2;		/* Has the ECMA key */
	guint32 aptc:2;		/* Has the [AllowPartiallyTrustedCallers] attributes */
	guint32 fulltrust:2;	/* Has FullTrust permission */
	guint32 unmanaged:2;	/* Has SecurityPermissionFlag.UnmanagedCode permission */
};

typedef struct {
	const char* data;
	guint32  size;
} MonoStreamHeader;

struct _MonoTableInfo {
	const char *base;
	guint       rows     : 24;
	guint       row_size : 8;

	/*
	 * Tables contain up to 9 columns and the possible sizes of the
	 * fields in the documentation are 1, 2 and 4 bytes.  So we
	 * can encode in 2 bits the size.
	 *
	 * A 32 bit value can encode the resulting size
	 *
	 * The top eight bits encode the number of columns in the table.
	 * we only need 4, but 8 is aligned no shift required. 
	 */
	guint32   size_bitfield;
};

struct _MonoImage {
	int   ref_count;
	FILE *file_descr;
	/* if file_descr is NULL the image was loaded from raw data */
	char *raw_data;
	guint32 raw_data_len;
	guint8 raw_data_allocated;

	/* Whenever this is a dynamically emitted module */
	guint8 dynamic;

	char *name;
	const char *assembly_name;
	const char *module_name;
	const char *version;
	char *guid;
	void *image_info;
	MonoMemPool         *mempool;

	char                *raw_metadata;
			    
	guint8               idx_string_wide, idx_guid_wide, idx_blob_wide;
			    
	MonoStreamHeader     heap_strings;
	MonoStreamHeader     heap_us;
	MonoStreamHeader     heap_blob;
	MonoStreamHeader     heap_guid;
	MonoStreamHeader     heap_tables;
			    
	const char          *tables_base;

	/**/
	MonoTableInfo        tables [MONO_TABLE_NUM];

	/*
	 * references is initialized only by using the mono_assembly_open
	 * function, and not by using the lowlevel mono_image_open.
	 *
	 * It is NULL terminated.
	 */
	MonoAssembly **references;

	MonoImage **modules;
	guint32 module_count;

	MonoImage **files;

	/*
	 * The Assembly this image was loaded from.
	 */
	MonoAssembly *assembly;

	/*
	 * Indexed by method tokens and typedef tokens.
	 */
	GHashTable *method_cache;
	GHashTable *class_cache;
	/*
	 * Indexed by fielddef and memberref tokens
	 */
	GHashTable *field_cache;

	/* indexed by typespec tokens. */
	GHashTable *typespec_cache;
	/* indexed by token */
	GHashTable *memberref_signatures;
	GHashTable *helper_signatures;

	/*
	 * Indexes namespaces to hash tables that map class name to typedef token.
	 */
	GHashTable *name_cache;

	/*
	 * Indexed by ((rank << 24) | (typedef & 0xffffff)), which limits us to a
	 * maximal rank of 255
	 */
	GHashTable *array_cache;

	/*
	 * indexed by MonoMethodSignature 
	 */
	GHashTable *delegate_begin_invoke_cache;
	GHashTable *delegate_end_invoke_cache;
	GHashTable *delegate_invoke_cache;

	/*
	 * indexed by MonoMethod pointers 
	 */
	GHashTable *runtime_invoke_cache;
	GHashTable *managed_wrapper_cache;
	GHashTable *native_wrapper_cache;
	GHashTable *remoting_invoke_cache;
	GHashTable *synchronized_cache;
	GHashTable *unbox_wrapper_cache;

	void *reflection_info;

	/*
	 * user_info is a public field and is not touched by the
	 * metadata engine
	 */
	void *user_info;

	/* dll map entries */
	GHashTable *dll_map;
};

enum {
	MONO_SECTION_TEXT,
	MONO_SECTION_RSRC,
	MONO_SECTION_RELOC,
	MONO_SECTION_MAX
};

typedef struct {
	GHashTable *hash;
	char *data;
	guint32 alloc_size; /* malloced bytes */
	guint32 index;
	guint32 offset; /* from start of metadata */
} MonoDynamicStream;

typedef struct {
	guint32 alloc_rows;
	guint32 rows;
	guint8  row_size; /*  calculated later with column_sizes */
	guint8  columns;
	guint32 next_idx;
	guint32 *values; /* rows * columns */
} MonoDynamicTable;

struct _MonoDynamicAssembly {
	MonoAssembly assembly;
	char *strong_name;
	guint32 strong_name_size;
	guint8 run;
	guint8 save;
};

struct _MonoDynamicImage {
	MonoImage image;
	guint32 meta_size;
	guint32 text_rva;
	guint32 metadata_rva;
	guint32 image_base;
	guint32 cli_header_offset;
	guint32 iat_offset;
	guint32 idt_offset;
	guint32 ilt_offset;
	guint32 imp_names_offset;
	struct {
		guint32 rva;
		guint32 size;
		guint32 offset;
		guint32 attrs;
	} sections [MONO_SECTION_MAX];
	GHashTable *typespec;
	GHashTable *typeref;
	GHashTable *handleref;
	MonoGHashTable *tokens;
	GHashTable *blob_cache;
	GList *array_methods;
	GPtrArray *gen_params;
	MonoGHashTable *token_fixups;
	GHashTable *method_to_table_idx;
	GHashTable *field_to_table_idx;
	GHashTable *method_aux_hash;
	gboolean run;
	gboolean save;
	gboolean initial_image;
	guint32 pe_kind, machine;
	char *strong_name;
	guint32 strong_name_size;
	char *win32_res;
	guint32 win32_res_size;
	MonoDynamicStream sheap;
	MonoDynamicStream code; /* used to store method headers and bytecode */
	MonoDynamicStream resources; /* managed embedded resources */
	MonoDynamicStream us;
	MonoDynamicStream blob;
	MonoDynamicStream tstream;
	MonoDynamicStream guid;
	MonoDynamicTable tables [MONO_TABLE_NUM];
};

/* for use with allocated memory blocks (assumes alignment is to 8 bytes) */
guint mono_aligned_addr_hash (gconstpointer ptr);

const char *   mono_meta_table_name              (int table);
void           mono_metadata_compute_table_bases (MonoImage *meta);

MonoClass**
mono_metadata_interfaces_from_typedef_full  (MonoImage             *image,
					     guint32                table_index,
					     guint                 *count,
					     MonoGenericContext    *context);

MonoArrayType *
mono_metadata_parse_array_full              (MonoImage             *image,
					     MonoGenericContext    *generic_context,
					     const char            *ptr,
					     const char           **rptr);

MonoType *
mono_metadata_parse_type_full               (MonoImage             *image,
					     MonoGenericContext    *generic_context,
					     MonoParseTypeMode      mode,
					     short                  opt_attrs,
					     const char            *ptr,
					     const char           **rptr);

MonoType *
mono_type_create_from_typespec_full         (MonoImage             *image,
					     MonoGenericContext    *generic_context,
					     guint32                type_spec);

MonoMethodSignature *
mono_metadata_parse_signature_full          (MonoImage             *image,
					     MonoGenericContext    *generic_context,
					     guint32                token);

MonoMethodSignature *
mono_metadata_parse_method_signature_full   (MonoImage             *image,
					     MonoGenericContext    *generic_context,
					     int                     def,
					     const char             *ptr,
					     const char            **rptr);

MonoMethodHeader *
mono_metadata_parse_mh_full                 (MonoImage             *image,
					     MonoGenericContext    *generic_context,
					     const char            *ptr);

guint
mono_metadata_generic_method_hash           (MonoGenericMethod     *gmethod);

gboolean
mono_metadata_generic_method_equal          (MonoGenericMethod     *g1,
					     MonoGenericMethod     *g2);

MonoGenericInst *
mono_metadata_parse_generic_inst            (MonoImage             *image,
					     MonoGenericContext    *generic_context,
					     int                    count,
					     const char            *ptr,
					     const char           **rptr);

MonoGenericInst *
mono_metadata_lookup_generic_inst           (MonoGenericInst       *ginst);

MonoGenericClass *
mono_metadata_lookup_generic_class          (MonoGenericClass      *gclass);

MonoGenericInst *
mono_metadata_inflate_generic_inst          (MonoGenericInst       *ginst,
					     MonoGenericContext    *context);

void mono_dynamic_stream_reset (MonoDynamicStream* stream);
void mono_assembly_addref      (MonoAssembly *assembly);

#endif /* __MONO_METADATA_INTERNALS_H__ */

