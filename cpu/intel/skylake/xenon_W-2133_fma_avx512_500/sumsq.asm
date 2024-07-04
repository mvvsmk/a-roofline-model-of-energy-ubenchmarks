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
	vmovapd zmm0, [rdi - 256]
	vmovapd zmm1, [rdi - 192]
	vmovapd zmm2, [rdi - 128]
	vmovapd zmm3, [rdi - 64]
	vmovapd zmm4, [rdi]
	vmovapd zmm5, [rdi + 64]
	vmovapd zmm6, [rdi + 128]
	vmovapd zmm7, [rdi + 192]



%rep MAD_PER_ELEMENT
	vfmadd213ps zmm0, zmm0, zmm0
	vfmadd213ps zmm1, zmm1, zmm1
	vfmadd213ps zmm2, zmm2, zmm2
	vfmadd213ps zmm3, zmm3, zmm3
	vfmadd213ps zmm4, zmm4, zmm4
	vfmadd213ps zmm5, zmm5, zmm5
	vfmadd213ps zmm6, zmm6, zmm6
	vfmadd213ps zmm7, zmm7, zmm7

%endrep
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


%rep MAD_PER_ELEMENT
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
