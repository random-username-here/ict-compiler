///
/// IVM's base devices
///
include once;

func ivm_tty_putc(character int8);

type ivm_crt_cmd enum int8 {
    IVM_CRT_MOVE 0,
    IVM_CRT_LINE 1
}

func ivm_crt_command(cmd ivm_crt_cmd, x uint16, y uint16);

type ivm_keystate enum int8 {
    IVM_KBD_RELEASE     0,
    IVM_KBD_PRESS       1,
    IVM_KBD_REPEAT      2
}

func ivm_kbd_get_keycode() int16;
func ivm_kbd_get_state() ivm_keystate;
func ivm_kbd_get_mods() int8;
