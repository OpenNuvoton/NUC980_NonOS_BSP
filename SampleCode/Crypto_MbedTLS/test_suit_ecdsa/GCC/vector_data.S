;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright(c) 2019 Nuvoton Technology Corp. All rights reserved.                                         */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


	.syntax	unified

	.globl	VectorBase_ECDSA
	.globl	VectorLimit_ECDSA

	.align	4
 
	.text
        
VectorBase_ECDSA:
    .incbin  "../../test_suite_ecdsa.data"
VectorLimit_ECDSA:
    .space   4

    .end
    
    