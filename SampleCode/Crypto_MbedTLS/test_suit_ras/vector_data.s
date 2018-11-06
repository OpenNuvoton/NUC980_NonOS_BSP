;/*---------------------------------------------------------------------------------------------------------*/
;/*                                                                                                         */
;/* Copyright(c) 2010 Nuvoton Technology Corp. All rights reserved.                                         */
;/*                                                                                                         */
;/*---------------------------------------------------------------------------------------------------------*/


    AREA _image, DATA, READONLY

    EXPORT  VectorBase_RSA
    EXPORT  VectorLimit_RSA

    ALIGN   4
        
VectorBase_RSA
    INCBIN .\test_suite_rsa.data
VectorLimit_RSA

    END