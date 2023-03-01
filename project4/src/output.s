.text
_start_T:
str x30, [sp, #0]
str x29, [sp, #-8]
add x29, sp, #-8
add sp, sp, #-16
ldr x30, =_frameSize_T
ldr w30, [x30, #0]
sub sp, sp, w30
str x9, [sp, #8]
str x10, [sp, #16]
str x11, [sp, #24]
str x12, [sp, #32]
str x13, [sp, #40]
str x14, [sp, #48]
str x15, [sp, #56]
str x19, [sp, #64]
str x20, [sp, #72]
str x21, [sp, #80]
str x22, [sp, #88]
str x23, [sp, #96]
str x24, [sp, #104]
str x25, [sp, #112]
str x26, [sp, #120]
str x27, [sp, #128]
str x28, [sp, #136]
str x29, [sp, #144]
str s16, [sp, #152]
str s17, [sp, #156]
str s18, [sp, #160]
str s19, [sp, #164]
str s20, [sp, #168]
str s21, [sp, #172]
str s22, [sp, #176]
str s23, [sp, #180]
.data
_CONSTANT_0: .ascii "true\n\000"
.align 3
.text
ldr x9, =_CONSTANT_0
mov x0, x9
bl _write_str
.data
_CONSTANT_1: .word 1
.align 3
.text
ldr w9, _CONSTANT_1
mov w0, w9
b _end_T
_end_T:
ldr x9, [sp, #8]
ldr x10, [sp, #16]
ldr x11, [sp, #24]
ldr x12, [sp, #32]
ldr x13, [sp, #40]
ldr x14, [sp, #48]
ldr x15, [sp, #56]
ldr x19, [sp, #64]
ldr x20, [sp, #72]
ldr x21, [sp, #80]
ldr x22, [sp, #88]
ldr x23, [sp, #96]
ldr x24, [sp, #104]
ldr x25, [sp, #112]
ldr x26, [sp, #120]
ldr x27, [sp, #128]
ldr x28, [sp, #136]
ldr x29, [sp, #144]
ldr s16, [sp, #152]
ldr s17, [sp, #156]
ldr s18, [sp, #160]
ldr s19, [sp, #164]
ldr s20, [sp, #168]
ldr s21, [sp, #172]
ldr s22, [sp, #176]
ldr s23, [sp, #180]
ldr x30, [x29, #8]
mov sp, x29
add sp, sp, #8
ldr x29, [x29,#0]
RET x30
.data
_frameSize_T: .word 184
.text
_start_F:
str x30, [sp, #0]
str x29, [sp, #-8]
add x29, sp, #-8
add sp, sp, #-16
ldr x30, =_frameSize_F
ldr w30, [x30, #0]
sub sp, sp, w30
str x9, [sp, #8]
str x10, [sp, #16]
str x11, [sp, #24]
str x12, [sp, #32]
str x13, [sp, #40]
str x14, [sp, #48]
str x15, [sp, #56]
str x19, [sp, #64]
str x20, [sp, #72]
str x21, [sp, #80]
str x22, [sp, #88]
str x23, [sp, #96]
str x24, [sp, #104]
str x25, [sp, #112]
str x26, [sp, #120]
str x27, [sp, #128]
str x28, [sp, #136]
str x29, [sp, #144]
str s16, [sp, #152]
str s17, [sp, #156]
str s18, [sp, #160]
str s19, [sp, #164]
str s20, [sp, #168]
str s21, [sp, #172]
str s22, [sp, #176]
str s23, [sp, #180]
.data
_CONSTANT_2: .ascii "false\n\000"
.align 3
.text
ldr x9, =_CONSTANT_2
mov x0, x9
bl _write_str
.data
_CONSTANT_3: .word 0
.align 3
.text
ldr w9, _CONSTANT_3
mov w0, w9
b _end_F
_end_F:
ldr x9, [sp, #8]
ldr x10, [sp, #16]
ldr x11, [sp, #24]
ldr x12, [sp, #32]
ldr x13, [sp, #40]
ldr x14, [sp, #48]
ldr x15, [sp, #56]
ldr x19, [sp, #64]
ldr x20, [sp, #72]
ldr x21, [sp, #80]
ldr x22, [sp, #88]
ldr x23, [sp, #96]
ldr x24, [sp, #104]
ldr x25, [sp, #112]
ldr x26, [sp, #120]
ldr x27, [sp, #128]
ldr x28, [sp, #136]
ldr x29, [sp, #144]
ldr s16, [sp, #152]
ldr s17, [sp, #156]
ldr s18, [sp, #160]
ldr s19, [sp, #164]
ldr s20, [sp, #168]
ldr s21, [sp, #172]
ldr s22, [sp, #176]
ldr s23, [sp, #180]
ldr x30, [x29, #8]
mov sp, x29
add sp, sp, #8
ldr x29, [x29,#0]
RET x30
.data
_frameSize_F: .word 184
.text
_start_main:
str x30, [sp, #0]
str x29, [sp, #-8]
add x29, sp, #-8
add sp, sp, #-16
ldr x30, =_frameSize_main
ldr w30, [x30, #0]
sub sp, sp, w30
str x9, [sp, #8]
str x10, [sp, #16]
str x11, [sp, #24]
str x12, [sp, #32]
str x13, [sp, #40]
str x14, [sp, #48]
str x15, [sp, #56]
str x19, [sp, #64]
str x20, [sp, #72]
str x21, [sp, #80]
str x22, [sp, #88]
str x23, [sp, #96]
str x24, [sp, #104]
str x25, [sp, #112]
str x26, [sp, #120]
str x27, [sp, #128]
str x28, [sp, #136]
str x29, [sp, #144]
str s16, [sp, #152]
str s17, [sp, #156]
str s18, [sp, #160]
str s19, [sp, #164]
str s20, [sp, #168]
str s21, [sp, #172]
str s22, [sp, #176]
str s23, [sp, #180]
.data
_CONSTANT_4: .ascii "test1:\n\000"
.align 3
.text
ldr x9, =_CONSTANT_4
mov x0, x9
bl _write_str
_IF_0:
bl _start_T
mov w9, w0
bl _start_F
mov w10, w0
cmp w9, #0
cset w9, ne
cmp w10, #0
cset w10, ne
orr w9, w9, w10
cmp w9, #0
beq _elseLabel_0
beq _elseLabel_0
b _ifExitLabel_0
_elseLabel_0:
_ifExitLabel_0:
.data
_CONSTANT_5: .ascii "test2:\n\000"
.align 3
.text
ldr x9, =_CONSTANT_5
mov x0, x9
bl _write_str
_IF_1:
bl _start_F
mov w9, w0
bl _start_T
mov w10, w0
cmp w9, #0
cset w9, ne
cmp w10, #0
cset w10, ne
and w9, w9, w10
cmp w9, #0
beq _elseLabel_1
beq _elseLabel_1
b _ifExitLabel_1
_elseLabel_1:
_ifExitLabel_1:
.data
_CONSTANT_6: .ascii "test3:\n\000"
.align 3
.text
ldr x9, =_CONSTANT_6
mov x0, x9
bl _write_str
_IF_2:
bl _start_T
mov w9, w0
bl _start_F
mov w10, w0
cmp w9, #0
cset w9, ne
cmp w10, #0
cset w10, ne
and w9, w9, w10
bl _start_F
mov w10, w0
bl _start_T
mov w11, w0
cmp w10, #0
cset w10, ne
cmp w11, #0
cset w11, ne
orr w10, w10, w11
cmp w9, #0
cset w9, ne
cmp w10, #0
cset w10, ne
orr w9, w9, w10
cmp w9, #0
beq _elseLabel_2
beq _elseLabel_2
b _ifExitLabel_2
_elseLabel_2:
_ifExitLabel_2:
.data
_CONSTANT_7: .ascii "test4:\n\000"
.align 3
.text
ldr x9, =_CONSTANT_7
mov x0, x9
bl _write_str
_IF_3:
bl _start_T
mov w9, w0
bl _start_F
mov w10, w0
cmp w9, #0
cset w9, ne
cmp w10, #0
cset w10, ne
orr w9, w9, w10
bl _start_F
mov w10, w0
bl _start_T
mov w11, w0
cmp w10, #0
cset w10, ne
cmp w11, #0
cset w11, ne
orr w10, w10, w11
cmp w9, #0
cset w9, ne
cmp w10, #0
cset w10, ne
orr w9, w9, w10
cmp w9, #0
beq _elseLabel_3
beq _elseLabel_3
b _ifExitLabel_3
_elseLabel_3:
_ifExitLabel_3:
.data
_CONSTANT_8: .word 0
.align 3
.text
ldr w9, _CONSTANT_8
mov w0, w9
b _end_main
_end_main:
ldr x9, [sp, #8]
ldr x10, [sp, #16]
ldr x11, [sp, #24]
ldr x12, [sp, #32]
ldr x13, [sp, #40]
ldr x14, [sp, #48]
ldr x15, [sp, #56]
ldr x19, [sp, #64]
ldr x20, [sp, #72]
ldr x21, [sp, #80]
ldr x22, [sp, #88]
ldr x23, [sp, #96]
ldr x24, [sp, #104]
ldr x25, [sp, #112]
ldr x26, [sp, #120]
ldr x27, [sp, #128]
ldr x28, [sp, #136]
ldr x29, [sp, #144]
ldr s16, [sp, #152]
ldr s17, [sp, #156]
ldr s18, [sp, #160]
ldr s19, [sp, #164]
ldr s20, [sp, #168]
ldr s21, [sp, #172]
ldr s22, [sp, #176]
ldr s23, [sp, #180]
ldr x30, [x29, #8]
mov sp, x29
add sp, sp, #8
ldr x29, [x29,#0]
RET x30
.data
_frameSize_main: .word 184
