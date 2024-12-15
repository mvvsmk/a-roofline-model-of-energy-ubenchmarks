
section .text align=64
global sumsq
global sumsqf

sumsq:
	sub rdi, -256
	vzeroall
	mov rax, 512
	sub rsi, 64
	
	jb .restore
	align 64
.process_by_32:
    
	vfmadd213pd ymm0, ymm0, ymm0
	vfmadd213pd ymm1, ymm1, ymm1
	vfmadd213pd ymm2, ymm2, ymm2
	vfmadd213pd ymm3, ymm3, ymm3
	vfmadd213pd ymm4, ymm4, ymm4
	vfmadd213pd ymm5, ymm5, ymm5
	vfmadd213pd ymm6, ymm6, ymm6
	vfmadd213pd ymm7, ymm7, ymm7
	vfmadd213pd ymm8, ymm8, ymm8
	vfmadd213pd ymm9, ymm9, ymm9
	vfmadd213pd ymm10, ymm10, ymm10
	vfmadd213pd ymm11, ymm11, ymm11
	vfmadd213pd ymm12, ymm12, ymm12
	vfmadd213pd ymm13, ymm13, ymm13
	vfmadd213pd ymm14, ymm14, ymm14
	vfmadd213pd ymm15, ymm15, ymm15


	add rdi, rax
	sub rsi, 64
	jae .process_by_32
.restore:
	add rsi, 64
	jz .finish
	int 3
.finish:
	vzeroupper
	ret

sumsqf:
	sub rdi, -128
	mov eax, 256
	vzeroall
	sub rsi, 64
	
	jb .restore
	align 32
.process_by_64:
	vmovaps ymm0, [rdi - 128]
	vmovaps ymm1, [rdi - 96]
	vmovaps ymm2, [rdi - 64]
	vmovaps ymm3, [rdi - 32]
	vmovaps ymm4, [rdi]
	vmovaps ymm5, [rdi + 32]
	vmovaps ymm6, [rdi + 64]
	vmovaps ymm7, [rdi + 96]


%rep 1
	vmulps ymm0, ymm0, ymm0
	vaddps ymm8, ymm8, ymm0
	vmulps ymm1, ymm1, ymm1
	vaddps ymm9, ymm9, ymm1
	vmulps ymm2, ymm2, ymm2
	vaddps ymm10, ymm10, ymm2
	vmulps ymm3, ymm3, ymm3
	vaddps ymm11, ymm11, ymm3
	vmulps ymm4, ymm4, ymm4
	vaddps ymm12, ymm12, ymm4
	vmulps ymm5, ymm5, ymm5
	vaddps ymm13, ymm13, ymm5
	vmulps ymm6, ymm6, ymm6
	vaddps ymm14, ymm14, ymm6
	vmulps ymm7, ymm7, ymm7
	vaddps ymm15, ymm15, ymm7
%endrep
	add rdi, rax
	sub rsi, 64
	jae .process_by_64
.restore:
	add rsi, 64
	jz .finish
	int 3
.finish:
	vzeroupper
	ret
