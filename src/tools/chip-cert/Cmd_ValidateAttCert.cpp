/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements the command handler for the 'chip-cert' tool
 *      that validates a CHIP attestation certificate chain.
 *
 */

#include "chip-cert.h"

#include <credentials/attestation_verifier/DeviceAttestationVerifier.h>

namespace {

using namespace chip;
using namespace chip::ArgParser;
using namespace chip::Credentials;
using namespace chip::Crypto;
using namespace chip::ASN1;

#define CMD_NAME "chip-cert validate-att-cert"

bool HandleOption(const char * progName, OptionSet * optSet, int id, const char * name, const char * arg);

// clang-format off
OptionDef gCmdOptionDefs[] =
{
    { "dac",            kArgumentRequired,  'd' },
    { "pai",            kArgumentRequired,  'i' },
    { "paa",            kArgumentRequired,  'a' },
    { }
};

const char * const gCmdOptionHelp =
    "  -d, --dac <cert-file>\n"
    "\n"
    "       A file containing Device Attestation Certificate (DAC) to be\n"
    "       validated. The DAC is provided in the DER encoded format.\n"
    "\n"
    "  -i, --pai <cert-file>\n"
    "\n"
    "       A file containing Product Attestation Intermediate (PAI) Certificate.\n"
    "       The PAI is provided in the DER encoded format.\n"
    "\n"
    "  -a, --paa <cert-file>\n"
    "\n"
    "       A file containing trusted Product Attestation Authority (PAA) Certificate.\n"
    "       The PAA is provided in the DER encoded format.\n"
    "\n"
    ;

OptionSet gCmdOptions =
{
    HandleOption,
    gCmdOptionDefs,
    "COMMAND OPTIONS",
    gCmdOptionHelp
};

HelpOptions gHelpOptions(
    CMD_NAME,
    "Usage: " CMD_NAME " [ <options...> ]\n",
    CHIP_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Validate a chain of CHIP attestation certificates"
);

OptionSet * gCmdOptionSets[] =
{
    &gCmdOptions,
    &gHelpOptions,
    nullptr
};
// clang-format on

const char * gDACFileName = nullptr;
const char * gPAIFileName = nullptr;
const char * gPAAFileName = nullptr;

bool HandleOption(const char * progName, OptionSet * optSet, int id, const char * name, const char * arg)
{
    switch (id)
    {
    case 'd':
        gDACFileName = arg;
        break;
    case 'i':
        gPAIFileName = arg;
        break;
    case 'a':
        gPAAFileName = arg;
        break;
    default:
        PrintArgError("%s: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

AttestationVerificationResult MapError(CertificateChainValidationResult certificateChainValidationResult)
{
    switch (certificateChainValidationResult)
    {
    case CertificateChainValidationResult::kRootFormatInvalid:
        return AttestationVerificationResult::kPaaFormatInvalid;

    case CertificateChainValidationResult::kRootArgumentInvalid:
        return AttestationVerificationResult::kPaaArgumentInvalid;

    case CertificateChainValidationResult::kICAFormatInvalid:
        return AttestationVerificationResult::kPaiFormatInvalid;

    case CertificateChainValidationResult::kICAArgumentInvalid:
        return AttestationVerificationResult::kPaiArgumentInvalid;

    case CertificateChainValidationResult::kLeafFormatInvalid:
        return AttestationVerificationResult::kDacFormatInvalid;

    case CertificateChainValidationResult::kLeafArgumentInvalid:
        return AttestationVerificationResult::kDacArgumentInvalid;

    case CertificateChainValidationResult::kChainInvalid:
        return AttestationVerificationResult::kDacSignatureInvalid;

    case CertificateChainValidationResult::kNoMemory:
        return AttestationVerificationResult::kNoMemory;

    case CertificateChainValidationResult::kInternalFrameworkError:
        return AttestationVerificationResult::kInternalError;

    default:
        return AttestationVerificationResult::kInternalError;
    }
}

} // namespace

bool Cmd_ValidateAttCert(int argc, char * argv[])
{
    uint8_t dacBuf[kMaxDERCertLength] = { 0 };
    uint8_t paiBuf[kMaxDERCertLength] = { 0 };
    uint8_t paaBuf[kMaxDERCertLength] = { 0 };
    MutableByteSpan dac(dacBuf);
    MutableByteSpan pai(paiBuf);
    MutableByteSpan paa(paaBuf);

    AttestationVerificationResult attestationResult = AttestationVerificationResult::kSuccess;
    CHIP_ERROR error                                = CHIP_NO_ERROR;

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        return true;
    }

    VerifyOrReturnError(ParseArgs(CMD_NAME, argc, argv, gCmdOptionSets), false);

    if (gDACFileName == nullptr)
    {
        fprintf(stderr, "Please specify the DAC certificate file name using the --dac option.\n");
        return false;
    }

    if (gPAIFileName == nullptr)
    {
        fprintf(stderr, "Please specify the PAI certificate file name using the --pai option.\n");
        return false;
    }

    if (gPAAFileName == nullptr)
    {
        fprintf(stderr, "Please specify the PAA certificate file name using the --paa option.\n");
        return false;
    }

    if (!ReadCertDERRaw(gDACFileName, dac))
    {
        fprintf(stderr, "Unable to open DAC Certificate File: %s\n", gDACFileName);
        return false;
    }
    if (!ReadCertDERRaw(gPAIFileName, pai))
    {
        fprintf(stderr, "Unable to open PAI Certificate File: %s\n", gPAIFileName);
        return false;
    }
    if (!ReadCertDERRaw(gPAAFileName, paa))
    {
        fprintf(stderr, "Unable to open PAA Certificate File: %s\n", gPAAFileName);
        return false;
    }

    // Verify certificate validity information
    {
        VerifyOrExit((error = IsCertificateValidAtCurrentTime(dac)) == CHIP_NO_ERROR,
                     attestationResult = AttestationVerificationResult::kDacExpired);

        VerifyOrExit((error = IsCertificateValidAtIssuance(dac, pai)) == CHIP_NO_ERROR,
                     attestationResult = AttestationVerificationResult::kPaiExpired);

        VerifyOrExit((error = IsCertificateValidAtIssuance(dac, paa)) == CHIP_NO_ERROR,
                     attestationResult = AttestationVerificationResult::kPaaExpired);
    }

    // Verify that VID and PID in the certificates match.
    {
        AttestationCertVidPid dacVidPid;
        AttestationCertVidPid paiVidPid;
        AttestationCertVidPid paaVidPid;

        VerifyOrExit((error = ExtractVIDPIDFromX509Cert(dac, dacVidPid)) == CHIP_NO_ERROR,
                     attestationResult = AttestationVerificationResult::kDacFormatInvalid);
        VerifyOrExit((error = ExtractVIDPIDFromX509Cert(pai, paiVidPid)) == CHIP_NO_ERROR,
                     attestationResult = AttestationVerificationResult::kPaiFormatInvalid);
        VerifyOrExit((error = ExtractVIDPIDFromX509Cert(paa, paaVidPid)) == CHIP_NO_ERROR,
                     attestationResult = AttestationVerificationResult::kPaaFormatInvalid);

        VerifyOrExit(dacVidPid.mVendorId.HasValue() && dacVidPid.mVendorId == paiVidPid.mVendorId,
                     attestationResult = AttestationVerificationResult::kDacVendorIdMismatch);

        if (paaVidPid.mVendorId.HasValue())
        {
            VerifyOrExit(dacVidPid.mVendorId == paaVidPid.mVendorId,
                         attestationResult = AttestationVerificationResult::kPaiVendorIdMismatch);
        }

        if (paiVidPid.mProductId.HasValue())
        {
            VerifyOrExit(dacVidPid.mProductId == paiVidPid.mProductId,
                         attestationResult = AttestationVerificationResult::kDacProductIdMismatch);
        }

        VerifyOrExit(!paaVidPid.mProductId.HasValue(), attestationResult = AttestationVerificationResult::kPaaFormatInvalid);
    }

    // Validate certificate chain.
    chip::Crypto::CertificateChainValidationResult chainValidationResult;
    error = ValidateCertificateChain(paa.data(), paa.size(), pai.data(), pai.size(), dac.data(), dac.size(), chainValidationResult);
    VerifyOrExit(error == CHIP_NO_ERROR, attestationResult = MapError(chainValidationResult));

exit:
    if (error != CHIP_NO_ERROR)
    {
        fprintf(stderr, "Error: %" CHIP_ERROR_FORMAT "\n", error.Format());
    }

    if (attestationResult != AttestationVerificationResult::kSuccess)
    {
        fprintf(stderr, "Attestation certificate validation failed with result: %d\n", static_cast<int>(attestationResult));
        return false;
    }

    return true;
}
