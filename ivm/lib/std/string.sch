///
/// strings.h port
///
include once;
include "std/types.sch";

func str_len(string *int8) size;
func str_cmp(a *int8, b *int8) int8;
func str_ncmp(a *int8, b *int8, len size) int8;
func str_dup(s *int8) *int8;
