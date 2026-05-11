///
/// Intrinsics of IVM
///
include once;

func ivm_get_sf() *void #inline;
func ivm_get_sp() *void #inline;
func ivm_get_pc() *void #inline;

func ivm_cli() #inline;
func ivm_sti() #inline; 
func ivm_int(interrupt int8) #inline;

include "./details/intrinsics.ict";
