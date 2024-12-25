#pragma once

#include "cryptonote_core.h"



namespace cryptonote
{

#define XCASH_WALLET_LENGTH 98 // The length of a XCA addres
#define NODE_TO_NETWORK_DATA_NODES_GET_CURRENT_BLOCK_VERIFIERS_LIST_MESSAGE "{\r\n \"message_settings\": \"NODE_TO_NETWORK_DATA_NODES_GET_CURRENT_BLOCK_VERIFIERS_LIST\",\r\n}"
#define NODE_TO_NETWORK_DATA_NODES_GET_CURRENT_BLOCK_VERIFIERS_LIST_ERROR_MESSAGE "Could not get a list of the current block verifiers"
#define NODE_TO_BLOCK_VERIFIERS_CHECK_IF_CURRENT_BLOCK_VERIFIER_MESSAGE "{\r\n \"message_settings\": \"NODE_TO_BLOCK_VERIFIERS_CHECK_IF_CURRENT_BLOCK_VERIFIER\",\r\n}"
#define NODE_TO_BLOCK_VERIFIERS_GET_RESERVE_BYTES_DATABASE_HASH_ERROR_MESSAGE "Could not get the network blocks reserve bytes database hash"
#define BLOCKCHAIN_RESERVED_BYTES_START "7c424c4f434b434841494e5f52455345525645445f42595445535f53544152547c"
#define BLOCKCHAIN_STEALTH_ADDRESS_END "a30101"
#define RANDOM_STRING_LENGTH 100 // The length of the random string
#define DATA_HASH_LENGTH 128 // The length of the SHA2-512 hash
#define BUFFER_SIZE 200000

#define VRF_PUBLIC_KEY_LENGTH 64
#define VRF_SECRET_KEY_LENGTH 128
#define VRF_PROOF_LENGTH 160
#define VRF_BETA_LENGTH 128

#define pointer_reset(pointer) \
free(pointer); \
pointer = NULL;

// bool check_block_verifier_node_signed_block(const block bl, const std::size_t current_block_height);

bool check_block_validity(const block bl, const std::size_t block_height);
}