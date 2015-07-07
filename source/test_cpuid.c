#include <hartallo/hl_cpu.h>
#include <hartallo/hl_types.h>
// http://msdn.microsoft.com/en-us/library/hskdteyh(v=vs.90).aspx
// cpuid.cpp
// processor: x86, x64
// Use the __cpuid intrinsic to get information about a CPU

#include <stdio.h>
#include <string.h>

const char* szFeatures[] = {
    "x87 FPU On Chip",
    "Virtual-8086 Mode Enhancement",
    "Debugging Extensions",
    "Page Size Extensions",
    "Time Stamp Counter",
    "RDMSR and WRMSR Support",
    "Physical Address Extensions",
    "Machine Check Exception",
    "CMPXCHG8B Instruction",
    "APIC On Chip",
    "Unknown1",
    "SYSENTER and SYSEXIT",
    "Memory Type Range Registers",
    "PTE Global Bit",
    "Machine Check Architecture",
    "Conditional Move/Compare Instruction",
    "Page Attribute Table",
    "36-bit Page Size Extension",
    "Processor Serial Number",
    "CFLUSH Extension",
    "Unknown2",
    "Debug Store",
    "Thermal Monitor and Clock Ctrl",
    "MMX Technology",
    "FXSAVE/FXRSTOR",
    "SSE Extensions",
    "SSE2 Extensions",
    "Self Snoop",
    "Multithreading Technology",
    "Thermal Monitor",
    "Unknown4",
    "Pending Break Enable"
};

void dump_cpuid()
{
    char CPUString[0x20];
    char CPUBrandString[0x40];
    int CPUInfo[4] = {-1};
    int nSteppingID = 0;
    int nModel = 0;
    int nFamily = 0;
    int nProcessorType = 0;
    int nExtendedmodel = 0;
    int nExtendedfamily = 0;
    int nBrandIndex = 0;
    int nCLFLUSHcachelinesize = 0;
    int nLogicalProcessors = 0;
    int nAPICPhysicalID = 0;
    int nFeatureInfo = 0;
    int nCacheLineSize = 0;
    int nL2Associativity = 0;
    int nCacheSizeK = 0;
    int nPhysicalAddress = 0;
    int nVirtualAddress = 0;
    int nRet = 0;

    int nCores = 0;
    int nCacheType = 0;
    int nCacheLevel = 0;
    int nMaxThread = 0;
    int nSysLineSize = 0;
    int nPhysicalLinePartitions = 0;
    int nWaysAssociativity = 0;
    int nNumberSets = 0;

    unsigned    nIds, nExIds, i;

    hl_bool_t    bSSE3Instructions = HL_FALSE;
    hl_bool_t    bMONITOR_MWAIT = HL_FALSE;
    hl_bool_t    bCPLQualifiedDebugStore = HL_FALSE;
    hl_bool_t    bVirtualMachineExtensions = HL_FALSE;
    hl_bool_t    bEnhancedIntelSpeedStepTechnology = HL_FALSE;
    hl_bool_t    bThermalMonitor2 = HL_FALSE;
    hl_bool_t    bSupplementalSSE3 = HL_FALSE;
    hl_bool_t    bL1ContextID = HL_FALSE;
    hl_bool_t    bCMPXCHG16B = HL_FALSE;
    hl_bool_t    bxTPRUpdateControl = HL_FALSE;
    hl_bool_t    bPerfDebugCapabilityMSR = HL_FALSE;
    hl_bool_t    bSSE41Extensions = HL_FALSE;
    hl_bool_t    bSSE42Extensions = HL_FALSE;
    hl_bool_t    bPOPCNT = HL_FALSE;

    hl_bool_t    bMultithreading = HL_FALSE;

    hl_bool_t    bLAHF_SAHFAvailable = HL_FALSE;
    hl_bool_t    bCmpLegacy = HL_FALSE;
    hl_bool_t    bSVM = HL_FALSE;
    hl_bool_t    bExtApicSpace = HL_FALSE;
    hl_bool_t    bAltMovCr8 = HL_FALSE;
    hl_bool_t    bLZCNT = HL_FALSE;
    hl_bool_t    bSSE4A = HL_FALSE;
    hl_bool_t    bMisalignedSSE = HL_FALSE;
    hl_bool_t    bPREFETCH = HL_FALSE;
    hl_bool_t    bSKINITandDEV = HL_FALSE;
    hl_bool_t    bSYSCALL_SYSRETAvailable = HL_FALSE;
    hl_bool_t    bExecuteDisableBitAvailable = HL_FALSE;
    hl_bool_t    bMMXExtensions = HL_FALSE;
    hl_bool_t    bFFXSR = HL_FALSE;
    hl_bool_t    b1GBSupport = HL_FALSE;
    hl_bool_t    bRDTSCP = HL_FALSE;
    hl_bool_t    b64Available = HL_FALSE;
    hl_bool_t    b3DNowExt = HL_FALSE;
    hl_bool_t    b3DNow = HL_FALSE;
    hl_bool_t    bNestedPaging = HL_FALSE;
    hl_bool_t    bLBRVisualization = HL_FALSE;
    hl_bool_t    bFP128 = HL_FALSE;
    hl_bool_t    bMOVOptimization = HL_FALSE;

    hl_bool_t    bSelfInit = HL_FALSE;
    hl_bool_t    bFullyAssociative = HL_FALSE;

    // __cpuid with an InfoType argument of 0 returns the number of
    // valid Ids in CPUInfo[0] and the CPU identification string in
    // the other three array elements. The CPU identification string is
    // not in linear order. The code below arranges the information
    // in a human readable form.
    __cpuid(CPUInfo, 0);
    nIds = CPUInfo[0];
    memset(CPUString, 0, sizeof(CPUString));
    *((int*)CPUString) = CPUInfo[1];
    *((int*)(CPUString+4)) = CPUInfo[3];
    *((int*)(CPUString+8)) = CPUInfo[2];

    // Get the information associated with each valid Id
    for (i=0; i<=nIds; ++i) {
        __cpuid(CPUInfo, i);
        printf_s("\nFor InfoType %d\n", i);
        printf_s("CPUInfo[0] = 0x%x\n", CPUInfo[0]);
        printf_s("CPUInfo[1] = 0x%x\n", CPUInfo[1]);
        printf_s("CPUInfo[2] = 0x%x\n", CPUInfo[2]);
        printf_s("CPUInfo[3] = 0x%x\n", CPUInfo[3]);

        // Interpret CPU feature information.
        if  (i == 1) {
            nSteppingID = CPUInfo[0] & 0xf;
            nModel = (CPUInfo[0] >> 4) & 0xf;
            nFamily = (CPUInfo[0] >> 8) & 0xf;
            nProcessorType = (CPUInfo[0] >> 12) & 0x3;
            nExtendedmodel = (CPUInfo[0] >> 16) & 0xf;
            nExtendedfamily = (CPUInfo[0] >> 20) & 0xff;
            nBrandIndex = CPUInfo[1] & 0xff;
            nCLFLUSHcachelinesize = ((CPUInfo[1] >> 8) & 0xff) * 8;
            nLogicalProcessors = ((CPUInfo[1] >> 16) & 0xff);
            nAPICPhysicalID = (CPUInfo[1] >> 24) & 0xff;
            bSSE3Instructions = (CPUInfo[2] & 0x1) || HL_FALSE;
            bMONITOR_MWAIT = (CPUInfo[2] & 0x8) || HL_FALSE;
            bCPLQualifiedDebugStore = (CPUInfo[2] & 0x10) || HL_FALSE;
            bVirtualMachineExtensions = (CPUInfo[2] & 0x20) || HL_FALSE;
            bEnhancedIntelSpeedStepTechnology = (CPUInfo[2] & 0x80) || HL_FALSE;
            bThermalMonitor2 = (CPUInfo[2] & 0x100) || HL_FALSE;
            bSupplementalSSE3 = (CPUInfo[2] & 0x200) || HL_FALSE;
            bL1ContextID = (CPUInfo[2] & 0x300) || HL_FALSE;
            bCMPXCHG16B= (CPUInfo[2] & 0x2000) || HL_FALSE;
            bxTPRUpdateControl = (CPUInfo[2] & 0x4000) || HL_FALSE;
            bPerfDebugCapabilityMSR = (CPUInfo[2] & 0x8000) || HL_FALSE;
            bSSE41Extensions = (CPUInfo[2] & 0x80000) || HL_FALSE;
            bSSE42Extensions = (CPUInfo[2] & 0x100000) || HL_FALSE;
            bPOPCNT= (CPUInfo[2] & 0x800000) || HL_FALSE;
            nFeatureInfo = CPUInfo[3];
            bMultithreading = (nFeatureInfo & (1 << 28)) || HL_FALSE;
        }
    }

    // Calling __cpuid with 0x80000000 as the InfoType argument
    // gets the number of valid extended IDs.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];
    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    // Get the information associated with each extended ID.
    for (i=0x80000000; i<=nExIds; ++i) {
        __cpuid(CPUInfo, i);
        printf_s("\nFor InfoType %x\n", i);
        printf_s("CPUInfo[0] = 0x%x\n", CPUInfo[0]);
        printf_s("CPUInfo[1] = 0x%x\n", CPUInfo[1]);
        printf_s("CPUInfo[2] = 0x%x\n", CPUInfo[2]);
        printf_s("CPUInfo[3] = 0x%x\n", CPUInfo[3]);

        if  (i == 0x80000001) {
            bLAHF_SAHFAvailable = (CPUInfo[2] & 0x1) || HL_FALSE;
            bCmpLegacy = (CPUInfo[2] & 0x2) || HL_FALSE;
            bSVM = (CPUInfo[2] & 0x4) || HL_FALSE;
            bExtApicSpace = (CPUInfo[2] & 0x8) || HL_FALSE;
            bAltMovCr8 = (CPUInfo[2] & 0x10) || HL_FALSE;
            bLZCNT = (CPUInfo[2] & 0x20) || HL_FALSE;
            bSSE4A = (CPUInfo[2] & 0x40) || HL_FALSE;
            bMisalignedSSE = (CPUInfo[2] & 0x80) || HL_FALSE;
            bPREFETCH = (CPUInfo[2] & 0x100) || HL_FALSE;
            bSKINITandDEV = (CPUInfo[2] & 0x1000) || HL_FALSE;
            bSYSCALL_SYSRETAvailable = (CPUInfo[3] & 0x800) || HL_FALSE;
            bExecuteDisableBitAvailable = (CPUInfo[3] & 0x10000) || HL_FALSE;
            bMMXExtensions = (CPUInfo[3] & 0x40000) || HL_FALSE;
            bFFXSR = (CPUInfo[3] & 0x200000) || HL_FALSE;
            b1GBSupport = (CPUInfo[3] & 0x400000) || HL_FALSE;
            bRDTSCP = (CPUInfo[3] & 0x8000000) || HL_FALSE;
            b64Available = (CPUInfo[3] & 0x20000000) || HL_FALSE;
            b3DNowExt = (CPUInfo[3] & 0x40000000) || HL_FALSE;
            b3DNow = (CPUInfo[3] & 0x80000000) || HL_FALSE;
        }

        // Interpret CPU brand string and cache information.
        if  (i == 0x80000002) {
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        }
        else if  (i == 0x80000003) {
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        }
        else if  (i == 0x80000004) {
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
        }
        else if  (i == 0x80000006) {
            nCacheLineSize = CPUInfo[2] & 0xff;
            nL2Associativity = (CPUInfo[2] >> 12) & 0xf;
            nCacheSizeK = (CPUInfo[2] >> 16) & 0xffff;
        }
        else if  (i == 0x80000008) {
            nPhysicalAddress = CPUInfo[0] & 0xff;
            nVirtualAddress = (CPUInfo[0] >> 8) & 0xff;
        }
        else if  (i == 0x8000000A) {
            bNestedPaging = (CPUInfo[3] & 0x1) || HL_FALSE;
            bLBRVisualization = (CPUInfo[3] & 0x2) || HL_FALSE;
        }
        else if  (i == 0x8000001A) {
            bFP128 = (CPUInfo[0] & 0x1) || HL_FALSE;
            bMOVOptimization = (CPUInfo[0] & 0x2) || HL_FALSE;
        }
    }

    // Display all the information in user-friendly format.

    printf_s("\n\nCPU String: %s\n", CPUString);

    if  (nIds >= 1) {
        if  (nSteppingID) {
            printf_s("Stepping ID = %d\n", nSteppingID);
        }
        if  (nModel) {
            printf_s("Model = %d\n", nModel);
        }
        if  (nFamily) {
            printf_s("Family = %d\n", nFamily);
        }
        if  (nProcessorType) {
            printf_s("Processor Type = %d\n", nProcessorType);
        }
        if  (nExtendedmodel) {
            printf_s("Extended model = %d\n", nExtendedmodel);
        }
        if  (nExtendedfamily) {
            printf_s("Extended family = %d\n", nExtendedfamily);
        }
        if  (nBrandIndex) {
            printf_s("Brand Index = %d\n", nBrandIndex);
        }
        if  (nCLFLUSHcachelinesize)
            printf_s("CLFLUSH cache line size = %d\n",
                     nCLFLUSHcachelinesize);
        if (bMultithreading && (nLogicalProcessors > 0)) {
            printf_s("Logical Processor Count = %d\n", nLogicalProcessors);
        }
        if  (nAPICPhysicalID) {
            printf_s("APIC Physical ID = %d\n", nAPICPhysicalID);
        }

        if  (nFeatureInfo || bSSE3Instructions ||
                bMONITOR_MWAIT || bCPLQualifiedDebugStore ||
                bVirtualMachineExtensions || bEnhancedIntelSpeedStepTechnology ||
                bThermalMonitor2 || bSupplementalSSE3 || bL1ContextID ||
                bCMPXCHG16B || bxTPRUpdateControl || bPerfDebugCapabilityMSR ||
                bSSE41Extensions || bSSE42Extensions || bPOPCNT ||
                bLAHF_SAHFAvailable || bCmpLegacy || bSVM ||
                bExtApicSpace || bAltMovCr8 ||
                bLZCNT || bSSE4A || bMisalignedSSE ||
                bPREFETCH || bSKINITandDEV || bSYSCALL_SYSRETAvailable ||
                bExecuteDisableBitAvailable || bMMXExtensions || bFFXSR || b1GBSupport ||
                bRDTSCP || b64Available || b3DNowExt || b3DNow || bNestedPaging ||
                bLBRVisualization || bFP128 || bMOVOptimization ) {
            printf_s("\nThe following features are supported:\n");

            if  (bSSE3Instructions) {
                printf_s("\tSSE3\n");
            }
            if  (bMONITOR_MWAIT) {
                printf_s("\tMONITOR/MWAIT\n");
            }
            if  (bCPLQualifiedDebugStore) {
                printf_s("\tCPL Qualified Debug Store\n");
            }
            if  (bVirtualMachineExtensions) {
                printf_s("\tVirtual Machine Extensions\n");
            }
            if  (bEnhancedIntelSpeedStepTechnology) {
                printf_s("\tEnhanced Intel SpeedStep Technology\n");
            }
            if  (bThermalMonitor2) {
                printf_s("\tThermal Monitor 2\n");
            }
            if  (bSupplementalSSE3) {
                printf_s("\tSupplemental Streaming SIMD Extensions 3\n");
            }
            if  (bL1ContextID) {
                printf_s("\tL1 Context ID\n");
            }
            if  (bCMPXCHG16B) {
                printf_s("\tCMPXCHG16B Instruction\n");
            }
            if  (bxTPRUpdateControl) {
                printf_s("\txTPR Update Control\n");
            }
            if  (bPerfDebugCapabilityMSR) {
                printf_s("\tPerf\\Debug Capability MSR\n");
            }
            if  (bSSE41Extensions) {
                printf_s("\tSSE4.1 Extensions\n");
            }
            if  (bSSE42Extensions) {
                printf_s("\tSSE4.2 Extensions\n");
            }
            if  (bPOPCNT) {
                printf_s("\tPPOPCNT Instruction\n");
            }

            i = 0;
            nIds = 1;
            while (i < (sizeof(szFeatures)/sizeof(const char*))) {
                if  (nFeatureInfo & nIds) {
                    printf_s("\t");
                    printf_s(szFeatures[i]);
                    printf_s("\n");
                }

                nIds <<= 1;
                ++i;
            }
            if (bLAHF_SAHFAvailable) {
                printf_s("\tLAHF/SAHF in 64-bit mode\n");
            }
            if (bCmpLegacy) {
                printf_s("\tCore multi-processing legacy mode\n");
            }
            if (bSVM) {
                printf_s("\tSecure Virtual Machine\n");
            }
            if (bExtApicSpace) {
                printf_s("\tExtended APIC Register Space\n");
            }
            if (bAltMovCr8) {
                printf_s("\tAltMovCr8\n");
            }
            if (bLZCNT) {
                printf_s("\tLZCNT instruction\n");
            }
            if (bSSE4A) {
                printf_s("\tSSE4A (EXTRQ, INSERTQ, MOVNTSD, MOVNTSS)\n");
            }
            if (bMisalignedSSE) {
                printf_s("\tMisaligned SSE mode\n");
            }
            if (bPREFETCH) {
                printf_s("\tPREFETCH and PREFETCHW Instructions\n");
            }
            if (bSKINITandDEV) {
                printf_s("\tSKINIT and DEV support\n");
            }
            if (bSYSCALL_SYSRETAvailable) {
                printf_s("\tSYSCALL/SYSRET in 64-bit mode\n");
            }
            if (bExecuteDisableBitAvailable) {
                printf_s("\tExecute Disable Bit\n");
            }
            if (bMMXExtensions) {
                printf_s("\tExtensions to MMX Instructions\n");
            }
            if (bFFXSR) {
                printf_s("\tFFXSR\n");
            }
            if (b1GBSupport) {
                printf_s("\t1GB page support\n");
            }
            if (bRDTSCP) {
                printf_s("\tRDTSCP instruction\n");
            }
            if (b64Available) {
                printf_s("\t64 bit Technology\n");
            }
            if (b3DNowExt) {
                printf_s("\t3Dnow Ext\n");
            }
            if (b3DNow) {
                printf_s("\t3Dnow! instructions\n");
            }
            if (bNestedPaging) {
                printf_s("\tNested Paging\n");
            }
            if (bLBRVisualization) {
                printf_s("\tLBR Visualization\n");
            }
            if (bFP128) {
                printf_s("\tFP128 optimization\n");
            }
            if (bMOVOptimization) {
                printf_s("\tMOVU Optimization\n");
            }
        }
    }

    if  (nExIds >= 0x80000004) {
        printf_s("\nCPU Brand String: %s\n", CPUBrandString);
    }

    if  (nExIds >= 0x80000006) {
        printf_s("Cache Line Size = %d\n", nCacheLineSize);
        printf_s("L2 Associativity = %d\n", nL2Associativity);
        printf_s("Cache Size = %dK\n", nCacheSizeK);
    }


    for (i=0;; i++) {
        __cpuidex(CPUInfo, 0x4, i);
        if(!(CPUInfo[0] & 0xf0)) {
            break;
        }

        if(i == 0) {
            nCores = CPUInfo[0] >> 26;
            printf_s("\n\nNumber of Cores = %d\n", nCores + 1);
        }

        nCacheType = (CPUInfo[0] & 0x1f);
        nCacheLevel = (CPUInfo[0] & 0xe0) >> 5;
        bSelfInit = (CPUInfo[0] & 0x100) >> 8;
        bFullyAssociative = (CPUInfo[0] & 0x200) >> 9;
        nMaxThread = (CPUInfo[0] & 0x03ffc000) >> 14;
        nSysLineSize = (CPUInfo[1] & 0x0fff);
        nPhysicalLinePartitions = (CPUInfo[1] & 0x03ff000) >> 12;
        nWaysAssociativity = (CPUInfo[1]) >> 22;
        nNumberSets = CPUInfo[2];

        printf_s("\n");

        printf_s("ECX Index %d\n", i);
        switch (nCacheType) {
        case 0:
            printf_s("   Type: Null\n");
            break;
        case 1:
            printf_s("   Type: Data Cache\n");
            break;
        case 2:
            printf_s("   Type: Instruction Cache\n");
            break;
        case 3:
            printf_s("   Type: Unified Cache\n");
            break;
        default:
            printf_s("   Type: Unknown\n");
        }

        printf_s("   Level = %d\n", nCacheLevel + 1);
        if (bSelfInit) {
            printf_s("   Self Initializing\n");
        }
        else {
            printf_s("   Not Self Initializing\n");
        }
        if (bFullyAssociative) {
            printf_s("   Is Fully Associatve\n");
        }
        else {
            printf_s("   Is Not Fully Associatve\n");
        }
        printf_s("   Max Threads = %d\n",
                 nMaxThread+1);
        printf_s("   System Line Size = %d\n",
                 nSysLineSize+1);
        printf_s("   Physical Line Partions = %d\n",
                 nPhysicalLinePartitions+1);
        printf_s("   Ways of Associativity = %d\n",
                 nWaysAssociativity+1);
        printf_s("   Number of Sets = %d\n",
                 nNumberSets+1);
    }
}

HL_ERROR_T hl_test_cpuid()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    // Dump CPUID
    dump_cpuid();

    return err;
}
