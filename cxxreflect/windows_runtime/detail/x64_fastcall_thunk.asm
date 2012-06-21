
; //                            Copyright James P. McNellis 2011 - 2012.                            //
; //                   Distributed under the Boost Software License, Version 1.0.                   //
; //     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //



;
; cxxreflect_windows_runtime_x64_fastcall_thunk -- Thunk for invoking an x64 fastcall function.
;
; Declared in C as:
;
;     int cxxreflect_windows_runtime_x64_fastcall_thunk(void const*   fp,
;                                                       void const*   arguments,
;                                                       void const*   types,
;                                                       uint64_t      count);
;
; 'fp' is the address of the function to be called.
;
; 'arguments' is a pointer to the initial element of an array of arguments.  Each element is eight
; bytes in size.  'count' is the cardinality of the 'arguments' array.
;
; 'types' is a pointer to the initial element of an array of type enumerators.  Each element is
; eight bytes in size.  'count' is the cardinality of the 'types' array (i.e., 'arguments' and 
; 'types' have the same cardinality).  Each element in the 'types' array must have one of the
; following values:
;
;     0   Integer or pointer argument
;     1   Double-precision real argument
;     2   Single-precision real argument
;
; All integer arguments should be promoted to 64-bit prior to the call.  Real arguments should be
; converted to the correct type for the call.  A double-precision (64-bit) real argument will occupy
; the entire slot in the arguments array.  A single-precision (32-bit) real argument should be
; aligned such that its initial byte is at the beginning of its slot. 
;

_TEXT SEGMENT

$fp           =  0    ; copied from rcx
$arguments    =  8    ; copied from rdx
$types        = 16    ; copied from r8
$count        = 24    ; copied from r9

cxxreflect_windows_runtime_x64_fastcall_thunk PROC FRAME

    ; Save nonvolatile registers that are used in this function:
    push        rbp
    .pushreg    rbp

    push        r12
    .pushreg    r12

    push        r13
    .pushreg    r13

    push        r14
    .pushreg    r14

    ; We don't actually use r15, but the frame offset must be a multiple of 16.
    push        r15
    .pushreg    r15



    ; Initialize rbp as the frame pointer; it points to the spill space for the initial argument ('fp'):
    lea         rbp, [rsp + 48]
    .setframe   rbp, 48



    .endprolog



    ; Compute the size of the arguments frame for the call and sufficiently enlarge the stack:
    mov     rax, r9
    shl     rax, 3
    sub     rsp, rax



    ; Copy the function arguments into the spill space:
    mov     QWORD PTR $fp       [rbp], rcx
    mov     QWORD PTR $arguments[rbp], rdx
    mov     QWORD PTR $types    [rbp], r8
    mov     QWORD PTR $count    [rbp], r9



    ; These are our four loop variables, stored in r10-r13:
    mov     r10, QWORD PTR $count    [rbp]    ; Number of remaining arguments
    xor     r11, r11                          ; Number of processed arguments

    mov     r12, QWORD PTR $arguments[rbp]    ; Pointer to the current argument
    mov     r13, QWORD PTR $types    [rbp]    ; Pointer to the current type



ArgumentLoopBegin:

    ; Test to see if the end of the loop has been reached.  If it has, break out of the loop:
    test    r10, r10
    jz      ArgumentLoopEnd



    ; Up to four arguments get enregistered; if this is a later argument, place it on the stack:
    mov     rax, 4
    cmp     r11, rax
    jge     EmplaceArgumentOnStack



    ; If this is an integer argument, enregister it into one of the integer registers:
    mov     rax, QWORD PTR [r13]
    test    rax, rax
    jz      EnregisterIntegerArgument

    ; If this is a double-precision real argument, enregister it into one of the real registers:
    dec     rax
    test    rax, rax
    jz      EnregisterDoubleArgument

    ; Otherwise, this is a single-precision real argument...



;
; Single-precision real argument enregistration
;
EnregisterSingleArgument:

    ; Compute the register into which this argument is to be placed and enregister the argument:

    mov     rax, 0
    cmp     rax, r11
    je      EnregisterSingleArgumentIntoXmm0

    mov     rax, 1
    cmp     rax, r11
    je      EnregisterSingleArgumentIntoXmm1

    mov     rax, 2
    cmp     rax, r11
    je      EnregisterSingleArgumentIntoXmm2

    ; Otherwise, this must be the fourth argument:
    jmp     EnregisterSingleArgumentIntoXmm3

EnregisterSingleArgumentIntoXmm0:
    movss   xmm0, DWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterSingleArgumentIntoXmm1:
    movss   xmm1, DWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterSingleArgumentIntoXmm2:
    movss   xmm2, DWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterSingleArgumentIntoXmm3:
    movss   xmm3, DWORD PTR [r12]
    jmp     ContinueWithNextArgument



;
; Double-precision real argument enregistration
;
EnregisterDoubleArgument:

    ; Compute the register into which this argument is to be placed and enregister the argument:

    mov     rax, 0
    cmp     rax, r11
    je      EnregisterDoubleArgumentIntoXmm0

    mov     rax, 1
    cmp     rax, r11
    je      EnregisterDoubleArgumentIntoXmm1

    mov     rax, 2
    cmp     rax, r11
    je      EnregisterDoubleArgumentIntoXmm2

    ; Otherwise, this must be the fourth argument:
    jmp     EnregisterDoubleArgumentIntoXmm3

EnregisterDoubleArgumentIntoXmm0:
    movsd   xmm0, QWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterDoubleArgumentIntoXmm1:
    movsd   xmm1, QWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterDoubleArgumentIntoXmm2:
    movsd   xmm2, QWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterDoubleArgumentIntoXmm3:
    movsd   xmm3, QWORD PTR [r12]
    jmp     ContinueWithNextArgument



;
; Integer argument enregistration
;
EnregisterIntegerArgument:

    ; Compute the register into which this argument is to be placed and enregister the argument:

    mov     rax, 0
    cmp     rax, r11
    je      EnregisterIntegerArgumentIntoRCX

    mov     rax, 1
    cmp     rax, r11
    je      EnregisterIntegerArgumentIntoRDX

    mov     rax, 2
    cmp     rax, r11
    je      EnregsiterIntegerArgumentIntoR8

    ; Otherwise, this must be the fourth argument:
    jmp     EnregisterIntegerArgumentIntoR9

EnregisterIntegerArgumentIntoRCX:
    mov     rcx, QWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterIntegerArgumentIntoRDX:
    mov     rdx, QWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregsiterIntegerArgumentIntoR8:
    mov     r8, QWORD PTR [r12]
    jmp     ContinueWithNextArgument

EnregisterIntegerArgumentIntoR9:
    mov     r9, QWORD PTR [r12]
    jmp     ContinueWithNextArgument



;
; Stack argument emplacement
;
EmplaceArgumentOnStack:

    ; Compute the offset of this argument using its offset into the 'arguments' array:
    mov     rax, r12
    mov     r14, $arguments[rbp]
    sub     rax, r14
    add     rax, rsp



    ; Emplace the argument into the correct position on the stack:
    mov     r14, QWORD PTR [r12]
    mov     QWORD PTR [rax], r14



ContinueWithNextArgument:

    ; Update for the next iteration
    dec     r10
    inc     r11
    add     r12, 8    ; Move to the next element in the 'arguments' array
    add     r13, 8    ; Move to the next element in the 'types' array
    jmp     ArgumentLoopBegin



ArgumentLoopEnd:

    ; Call the function:
    mov     rax, QWORD PTR $fp[rbp]
    call    rax



    ; Pop the arguments from the stack:
    mov     r10, QWORD PTR $count[rbp]
    shl     r10, 3
    add     rsp, r10



    ; Restore nonvolatile registers that are used in this function:
    pop     r15
    pop     r14
    pop     r13
    pop     r12



    ; Restore the frame pointer:
    pop     rbp



    ; Balance has been returned to the stack.  Return:
    ret     0

cxxreflect_windows_runtime_x64_fastcall_thunk ENDP

_TEXT ENDS

END

; // AMDG //
