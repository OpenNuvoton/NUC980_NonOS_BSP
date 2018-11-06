;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright(c) 2010 Nuvoton Technology Corp. All rights reserved.                                         */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


    AREA _image, DATA, READONLY

    EXPORT  VectorBase_PKCS1_V15
    EXPORT  VectorLimit_PKCS1_V15

    ALIGN   4
        
VectorBase_PKCS1_V15
    INCBIN .\test_suite_pkcs1_v15.data
VectorLimit_PKCS1_V15

    END