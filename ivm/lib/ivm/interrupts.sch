///
/// Interrupts configuration
///
include once;

type ivm_int_id enum int8 {
    IVM_INT_TIMER       0x01,
    IVM_INT_KEYBOARD    0x08,
    IVM_INT_EXCEPTION   0x0f
}

func ivm_get_int(which ivm_int_id) *func() void;
func ivm_set_int(which ivm_int_id, handler *func() void);

func ivm_get_exception_type() int8;
func ivm_get_exception_pc() *void;
func ivm_get_segfault_addr() *void;
