#pragma once

#define VER_FILEVERSION 1, 0, 0, 0
#define VER_FILEVERSION_STR "1.0.0.0\0"

#define VER_PRODUCTVERSION 1, 0, 0, 0
#define VER_PRODUCTVERSION_STR "1.0\0"

#define VER_COMPANYNAME_STR "Microsoft Corporation"
#define VER_LEGALCOPYRIGHT_STR "Copyright (C) Microsoft Corporation"
#define VER_LEGALTRADEMARKS1_STR "All Rights Reserved"
#define VER_LEGALTRADEMARKS2_STR ""
#define VER_PRODUCTNAME_STR "Microsoft Display Capture Tools"

#ifndef DEBUG
#define VER_DEBUG 0
#else
#define VER_DEBUG VS_FF_DEBUG
#endif

// These can be set to VER_FF_PRIVATEBUILD and VER_FF_PRERELEASE if they are used, and then the string for PrivateBuild should be set
#define VER_PRIVATEBUILD 0
#define VER_PRERELEASE 0

VS_VERSION_INFO VERSIONINFO
FILEVERSION     VER_FILEVERSION
PRODUCTVERSION  VER_PRODUCTVERSION
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)
FILEOS          VOS__WINDOWS32
FILETYPE        VFT_DLL
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904E4"
        BEGIN
            VALUE "CompanyName",      VER_COMPANYNAME_STR
            VALUE "FileDescription",  VER_FILEDESCRIPTION_STR
            VALUE "FileVersion",      VER_FILEVERSION_STR
            VALUE "InternalName",     VER_INTERNALNAME_STR
            VALUE "LegalCopyright",   VER_LEGALCOPYRIGHT_STR
            VALUE "LegalTrademarks1", VER_LEGALTRADEMARKS1_STR
            VALUE "LegalTrademarks2", VER_LEGALTRADEMARKS2_STR
            VALUE "OriginalFilename", VER_ORIGINALFILENAME_STR
            VALUE "ProductName",      VER_PRODUCTNAME_STR
            VALUE "ProductVersion",   VER_PRODUCTVERSION_STR
        END
    END

    BLOCK "VarFileInfo"
    BEGIN
        /* The following line should only be modified for localized versions.     */
        /* It consists of any number of WORD,WORD pairs, with each pair           */
        /* describing a language,codepage combination supported by the file.      */
        /*                                                                        */
        /* For example, a file might have values "0x409,1252" indicating that it  */
        /* supports English language (0x409) in the Windows ANSI codepage (1252). */

        VALUE "Translation", 0x409, 1252

    END
END
