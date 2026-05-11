///
/// Memory management
///
include once;
include "std/types.sch";

func mem_init();
func mem_alloc(count size) *void;
func mem_free(chunk *void);
func mem_size(chunk *void);

func mem_cpy(dest *void, src *void, n size);
