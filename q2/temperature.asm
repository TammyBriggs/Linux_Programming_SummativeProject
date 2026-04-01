section .data
    filename db "temperature_data.txt", 0
    err_msg db "Error: Cannot open temperature_data.txt.", 10
    err_len equ $ - err_msg
    msg_total db "Total readings: ", 0
    len_total equ $ - msg_total
    msg_valid db "Valid readings: ", 0
    len_valid equ $ - msg_valid

section .bss
    ; Buffer to load file contents into memory (64KB chunk)
    buffer resb 65536      
    ; Buffer to convert integer counts into printable ASCII strings
    num_str resb 20        

section .text
    global _start

_start:
    ; ==========================================
    ; FILE HANDLING LOGIC: OPEN
    ; ==========================================
    mov rax, 2             ; syscall number for sys_open
    lea rdi, [filename]    ; arg1: pointer to filename string
    mov rsi, 0             ; arg2: flags (O_RDONLY = 0)
    mov rdx, 0             ; arg3: mode (not needed for read-only)
    syscall

    ; --- Error Handling for File Open ---
    cmp rax, 0
    jl file_error          ; If rax is negative, sys_open failed
    mov r8, rax            ; Save the valid file descriptor in r8

    ; --- Initialize Counters ---
    xor r12, r12           ; r12 = Total lines count (starts at 0)
    xor r13, r13           ; r13 = Valid lines count (starts at 0)
    xor r14, r14           ; r14 = Flag: Does current line have data? (0 = No, 1 = Yes)

read_loop:
    ; ==========================================
    ; FILE HANDLING LOGIC: READ INTO MEMORY
    ; ==========================================
    mov rax, 0             ; syscall number for sys_read
    mov rdi, r8            ; arg1: file descriptor
    lea rsi, [buffer]      ; arg2: pointer to our memory buffer
    mov rdx, 65536         ; arg3: maximum bytes to read
    syscall

    cmp rax, 0
    je end_of_file         ; If 0 bytes were read, we reached EOF
    jl file_error          ; If negative, a read error occurred

    mov r9, rax            ; Store number of bytes actually read in r9
    lea r10, [buffer]      ; r10 points to the start of the buffer
    xor r11, r11           ; r11 is our current byte index (starts at 0)

parse_loop:
    ; ==========================================
    ; STRING TRAVERSAL & LINE COUNTING LOGIC
    ; ==========================================
    cmp r11, r9
    jge read_loop          ; If index >= bytes read, fetch the next file chunk

    mov al, byte [r10 + r11] ; Load the current character into AL register

    ; Handle varying line endings (CRLF vs LF)
    cmp al, 13             ; Check for Carriage Return (\r)
    je next_byte           ; Ignore it entirely. We wait for the \n to count the line.

    cmp al, 10             ; Check for Line Feed (\n)
    je process_newline     ; If it's a newline, process our counters

    ; If it's neither \r nor \n, it's valid data (a number, minus sign, etc.)
    mov r14, 1             ; Flag that the current line has valid characters
    jmp next_byte

process_newline:
    inc r12                ; Increment total line count (including empty ones)
    
    cmp r14, 1             ; Did this line have valid data?
    jne reset_line_state   ; If not, skip valid increment
    inc r13                ; If yes, increment the valid readings count

reset_line_state:
    xor r14, r14           ; Reset the valid data flag to 0 for the next line

next_byte:
    inc r11                ; Move to the next byte
    jmp parse_loop         ; Loop back

end_of_file:
    ; Edge case: The file might end with data but NO trailing newline
    cmp r14, 1
    jne close_file
    inc r12                ; Count the final line
    inc r13                ; Count the valid data

close_file:
    ; ==========================================
    ; FILE HANDLING LOGIC: CLOSE
    ; ==========================================
    mov rax, 3             ; syscall number for sys_close
    mov rdi, r8            ; arg1: file descriptor
    syscall

    ; ==========================================
    ; OUTPUT LOGIC
    ; ==========================================
    ; Print "Total readings: "
    mov rax, 1             ; sys_write
    mov rdi, 1             ; stdout
    lea rsi, [msg_total]
    mov rdx, len_total
    syscall

    ; Convert r12 to string and print
    mov rax, r12
    call print_number

    ; Print "Valid readings: "
    mov rax, 1             ; sys_write
    mov rdi, 1             ; stdout
    lea rsi, [msg_valid]
    mov rdx, len_valid
    syscall

    ; Convert r13 to string and print
    mov rax, r13
    call print_number

    ; Set exit code to 0 (Success)
    mov rdi, 0
    jmp exit_program

file_error:
    ; Print error message
    mov rax, 1             
    mov rdi, 1             
    lea rsi, [err_msg]
    mov rdx, err_len
    syscall
    mov rdi, 1             ; Set exit code to 1 (Failure)

exit_program:
    ; ==========================================
    ; CONTROL FLOW: TERMINATION
    ; ==========================================
    mov rax, 60            ; syscall number for sys_exit
    syscall

; =======================================================
; HELPER ROUTINE: print_number
; Converts an integer in RAX to an ASCII string and prints
; =======================================================
print_number:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    
    lea r12, [num_str + 19] ; Point to the end of our string buffer
    mov byte [r12], 10      ; Add a newline character at the end
    mov rbx, 10             ; We divide by 10 to isolate digits
    
.convert_loop:
    xor rdx, rdx            ; Clear RDX before division
    div rbx                 ; Divide RAX by 10. Quotient -> RAX, Remainder -> RDX
    add dl, '0'             ; Convert the integer remainder to an ASCII character
    dec r12                 ; Move backwards in our string buffer
    mov byte [r12], dl      ; Store the character
    test rax, rax           ; Is the quotient 0?
    jnz .convert_loop       ; If not, loop and divide again

    ; Calculate the string length and print
    lea r13, [num_str + 20] 
    sub r13, r12            ; Length = end of buffer - current pointer
    
    mov rax, 1              ; sys_write
    mov rdi, 1              ; stdout
    mov rsi, r12            ; Start of the ASCII string
    mov rdx, r13            ; String length
    syscall
    
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
