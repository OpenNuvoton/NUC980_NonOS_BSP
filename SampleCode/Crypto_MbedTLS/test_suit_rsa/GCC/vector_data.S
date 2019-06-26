;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright(c) 2019 Nuvoton Technology Corp. All rights reserved.                                         */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


	.syntax	unified

	.globl	VectorBase_RSA
	.globl	VectorLimit_RSA

	.align	4
 
	.text
        
VectorBase_RSA:
    .incbin  "../../test_suite_rsa.data"
VectorLimit_RSA:
    .space   4

    .end
    
    