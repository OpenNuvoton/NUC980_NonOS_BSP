;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright(c) 2019 Nuvoton Technology Corp. All rights reserved.                                         */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


	.syntax	unified

	.globl	VectorBase_PKCS1_V15
	.globl	VectorLimit_PKCS1_V15

	.align	4
 
	.text
        
VectorBase_PKCS1_V15:
    .incbin  "../../test_suite_pkcs1_v15.data"
VectorLimit_PKCS1_V15:
    .space   4

    .end
    
    