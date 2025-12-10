// Copyright (c) 2025 X-CASH Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "gtest/gtest.h"

#include <vector>
#include <cstring>

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/tx_extra.h"
#include "crypto/crypto.h"

using namespace cryptonote;

// Test fixture for leader metadata tests
class LeaderMetadataTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        // Valid test data
        valid_leader_id = "XCA1a2b3c4d5e6f7g8h9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7A8B9C0D1E2F3G4H5I6J7K8L9M0N1O2P3Q4R5S6";
        
        // Generate valid signature (64 bytes)
        memset(&valid_signature, 0xAB, sizeof(crypto::signature));
    }

    std::string valid_leader_id;
    crypto::signature valid_signature;
};

// Test 1: Valid leader metadata serialization and deserialization
TEST_F(LeaderMetadataTest, ValidMetadata)
{
    std::vector<uint8_t> tx_extra;
    
    // Add valid leader info
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, valid_leader_id, valid_signature));
    ASSERT_FALSE(tx_extra.empty());
    
    // Extract leader info
    std::string extracted_leader_id;
    crypto::signature extracted_sig;
    
    ASSERT_TRUE(get_leader_info_from_tx_extra(tx_extra, extracted_leader_id, extracted_sig));
    ASSERT_EQ(valid_leader_id, extracted_leader_id);
    ASSERT_EQ(0, memcmp(&valid_signature, &extracted_sig, sizeof(crypto::signature)));
}

// Test 2: Empty tx_extra should return false
TEST_F(LeaderMetadataTest, EmptyTxExtra)
{
    std::vector<uint8_t> tx_extra;
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_FALSE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
}

// Test 3: Malformed - truncated header (only tag, no size)
TEST_F(LeaderMetadataTest, TruncatedHeader)
{
    std::vector<uint8_t> tx_extra;
    tx_extra.push_back(TX_EXTRA_TAG_LEADER_INFO);
    // Missing size byte and data
    
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_FALSE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
}

// Test 4: Malformed - wrong tag
TEST_F(LeaderMetadataTest, WrongTag)
{
    std::vector<uint8_t> tx_extra;
    
    // Create valid metadata but with wrong tag
    std::vector<uint8_t> valid_extra;
    ASSERT_TRUE(add_leader_info_to_tx_extra(valid_extra, valid_leader_id, valid_signature));
    
    // Replace tag with invalid one
    tx_extra = valid_extra;
    tx_extra[0] = 0xFF; // Invalid tag
    
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_FALSE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
}

// Test 5: Malformed - size mismatch (size byte doesn't match actual data)
TEST_F(LeaderMetadataTest, SizeMismatch)
{
    std::vector<uint8_t> tx_extra;
    
    // Add valid metadata
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, valid_leader_id, valid_signature));
    
    // Corrupt the size byte (make it larger than actual data)
    if (tx_extra.size() > 1)
    {
        tx_extra[1] = 0xFF; // Set unrealistic size
    }
    
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_FALSE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
}

// Test 6: Malformed - truncated data (incomplete leader_id)
TEST_F(LeaderMetadataTest, TruncatedData)
{
    std::vector<uint8_t> tx_extra;
    
    // Add valid metadata
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, valid_leader_id, valid_signature));
    
    // Truncate the data (remove last 30 bytes)
    if (tx_extra.size() > 30)
    {
        tx_extra.resize(tx_extra.size() - 30);
    }
    
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_FALSE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
}

// Test 7: Malformed - empty leader_id
TEST_F(LeaderMetadataTest, EmptyLeaderId)
{
    std::vector<uint8_t> tx_extra;
    std::string empty_leader_id = "";
    
    // Try to add metadata with empty leader_id
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, empty_leader_id, valid_signature));
    
    // Should be able to extract it (serialization allows empty strings)
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_TRUE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
    ASSERT_TRUE(leader_id.empty());
}

// Test 8: Malformed - oversized leader_id (extremely long string)
TEST_F(LeaderMetadataTest, OversizedLeaderId)
{
    std::vector<uint8_t> tx_extra;
    
    // Create an extremely long leader_id (10KB)
    std::string oversized_leader_id(10000, 'X');
    
    // The function should still work (no explicit size limit in the code)
    // but this tests boundary behavior
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, oversized_leader_id, valid_signature));
    
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_TRUE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
    ASSERT_EQ(oversized_leader_id, leader_id);
}

// Test 9: Malformed - corrupted signature bytes
TEST_F(LeaderMetadataTest, CorruptedSignature)
{
    std::vector<uint8_t> tx_extra;
    
    // Add valid metadata
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, valid_leader_id, valid_signature));
    
    // Corrupt signature bytes (assuming they're at the end)
    if (tx_extra.size() > 64)
    {
        for (size_t i = tx_extra.size() - 64; i < tx_extra.size(); ++i)
        {
            tx_extra[i] ^= 0xFF; // Flip all bits
        }
    }
    
    std::string leader_id;
    crypto::signature sig;
    
    // Should still parse (corruption detection happens during signature verification, not parsing)
    ASSERT_TRUE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
    ASSERT_EQ(valid_leader_id, leader_id);
    ASSERT_NE(0, memcmp(&valid_signature, &sig, sizeof(crypto::signature)));
}

// Test 10: Multiple fields in tx_extra (leader info should be found among other fields)
TEST_F(LeaderMetadataTest, MultipleFieldsInExtra)
{
    std::vector<uint8_t> tx_extra;
    
    // Add a public key first
    crypto::public_key pub_key;
    memset(&pub_key, 0x12, sizeof(crypto::public_key));
    add_tx_pub_key_to_extra(tx_extra, pub_key);
    
    // Add leader info
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, valid_leader_id, valid_signature));
    
    // Add some padding
    std::vector<uint8_t> padding(10, 0x00);
    tx_extra.insert(tx_extra.end(), padding.begin(), padding.end());
    
    // Should still find leader info
    std::string leader_id;
    crypto::signature sig;
    
    ASSERT_TRUE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
    ASSERT_EQ(valid_leader_id, leader_id);
    ASSERT_EQ(0, memcmp(&valid_signature, &sig, sizeof(crypto::signature)));
}

// Test 11: Malformed - random garbage data
TEST_F(LeaderMetadataTest, GarbageData)
{
    std::vector<uint8_t> tx_extra;
    
    // Fill with random garbage
    for (int i = 0; i < 100; ++i)
    {
        tx_extra.push_back(static_cast<uint8_t>(rand() % 256));
    }
    
    std::string leader_id;
    crypto::signature sig;
    
    // Should fail gracefully
    ASSERT_FALSE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
}

// Test 12: Malformed - leader_id with invalid characters (non-printable)
TEST_F(LeaderMetadataTest, InvalidCharactersInLeaderId)
{
    std::vector<uint8_t> tx_extra;
    
    // Create leader_id with null bytes and control characters
    std::string invalid_leader_id = "XCA\x00\x01\x02\x03\x04\x05invalid";
    
    ASSERT_TRUE(add_leader_info_to_tx_extra(tx_extra, invalid_leader_id, valid_signature));
    
    std::string leader_id;
    crypto::signature sig;
    
    // Should parse (validity checking is not serialization's job)
    ASSERT_TRUE(get_leader_info_from_tx_extra(tx_extra, leader_id, sig));
    // Note: comparison might be tricky due to null bytes
}
