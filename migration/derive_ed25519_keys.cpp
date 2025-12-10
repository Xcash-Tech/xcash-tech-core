// Utility to derive Ed25519 public keys from DPoS secret keys
// Compile: g++ -o derive_ed25519_keys derive_ed25519_keys.cpp -I../src -I../external/easylogging++ -I../contrib/epee/include -L../build/lib -lcncrypto -lsodium -std=c++14

#include <iostream>
#include <string>
#include <cstring>
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/crypto-ops.h"
#include "common/util.h"
#include "string_tools.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <128-char-hex-secret-key>" << std::endl;
        return 1;
    }

    std::string delegate_secret_key = argv[1];
    
    if (delegate_secret_key.size() != 128) {
        std::cerr << "Error: Secret key must be 128 hex characters (64 bytes)" << std::endl;
        return 1;
    }

    // Parse hex to binary
    std::string ecdsa_key_binary;
    if (!epee::string_tools::parse_hexstr_to_binbuff(delegate_secret_key, ecdsa_key_binary)) {
        std::cerr << "Error: Failed to parse hex" << std::endl;
        return 1;
    }

    // Hash to 32 bytes
    crypto::hash ecdsa_key_hash;
    crypto::cn_fast_hash(ecdsa_key_binary.data(), ecdsa_key_binary.size(), ecdsa_key_hash);
    
    crypto::secret_key leader_seckey;
    memcpy(&leader_seckey, &ecdsa_key_hash, sizeof(crypto::secret_key));

    // Apply scalar reduction
    sc_reduce32(reinterpret_cast<unsigned char*>(&leader_seckey));

    // Derive public key
    crypto::public_key derived_pubkey;
    if (!crypto::secret_key_to_public_key(leader_seckey, derived_pubkey)) {
        std::cerr << "Error: Failed to derive public key" << std::endl;
        return 1;
    }

    std::cout << epee::string_tools::pod_to_hex(derived_pubkey) << std::endl;
    return 0;
}
