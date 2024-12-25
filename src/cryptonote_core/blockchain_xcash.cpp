#include <vector>

#include <unordered_map>
#include <string>
#include <tuple>

#include "blockchain_xcash.h"

#include "cryptonote_config.h"
#include "cryptonote_core.h"
#include "common/send_and_receive_data.h"


namespace cryptonote {

void fix_data_hash(const std::size_t block_height, std::string &data_hash)
{
    static const std::unordered_map<std::size_t, std::unordered_map<std::string, std::string>> data_hash_map = {
        {808874, {{"7c2748de805cefcf59d7f0fce292d67c5a824561625eec4d8ead0758ce207a1e5242a13c381b17142ffbb061a788c8944ac0b05e8db8a0062900fe87e6695314", "82f14cfb3e87aedc0c5eff17e51ad65e29b28852372e593d6605b0c4db7487c530d23fe77f29b34f0929f9066b7bdc8689e263923c981c2fd471620a400b314a"}}},
        {809080, {{"046fa0e4a6b67aa5bf79dd051a34c7b4903c0343be6c6c48700445fc190fc375e71edc9d82c4d77726becd0d568d9e5c4b48158306e061848bfcb02a576e8971", "4a7aaa564815995b9316ab7fc6da3e6cbcd32bcdf4fd13ddac028632da1e4f24668c95b5a6a109821de117ac8092cc845381b5284bfd303f3427031be9a209cc"}}},
        {814024, {{"535eaccc8650ebd8f52b9f77aaedceaf166b4500d8532dfe6233a9a2c50f67ec50ff4b7083ba1ee7767b45801857cbce417f7ab695d73edd7de0808ffc4c75d3", "5363a5b4a077e4707a8d01e4cee81a9c798728fa2baefd0f91ee3aed78f97acf7d28c6153827b6ba150c39fb96286a5c6609257ff4abd4a5b467e54a968b804b"}}},
        {819279, {{"6fc3dd9d1d0ff0a423ae42e137d6a3568149bd34f7884bec18e7a07668b9f6548a64c365142dabf8a4451c71039afe8c0224cd08454fa74c520480c69148370f", "11cb9a737266c2f152e586dff0ea93609588a33215216a7a8dd51756372b6785b14cb50f66d51e33ecc719a3dd660ef2c9435f3a2f41ad81907b7cc84828a6b4"}}},
        {820980, {{"71cabbc78313369ce14398c7cf59ddaf048056b3f7f216beff8eea438d1ec075fe120b2b5097cfa38787a5948501b33ef2c1f7cd6fd9d13e720e754c1718a6a8", "dc63a8c27b8d57a037e27db7e68de3514fc4ceb3eb0d141d6ae0605e4bda267ff94213b7a337584d745b0c40080185dd717f47e99319d12f73bb95dfa13e4208"}}},
        {832613, {{"10d51f26de8a0d1c9c38f0cc332048a5d6b370a45f1c11771221cb5362784cf99971d16ce360627263a8198f217aa608aa92d66dd00e8b3bd036d1f127dad6e3", "29e39749cec86f763463ddff8107ba8fac8ca09d0118a1382949a8271b052c10d2ce884fdb19cd9badc5697767343a287b8c5d1360042176c2dec60f6e374e47"}}},
        {835849, {{"c71acd1f7627a069a3b1c8886a0de991bfa6ddeb0007a0ad8e123901557dc6fe5c8d193d121bb66d175a940d0ac0503d4e826133501e2a95154101f4b8d9e18b", "dde80e6cb7946acc1cc65065aa62a419e6571405172f62ef549cb5bb4929f97e66c2533bd2c408f11d314a3b58241560f038960b8d4643c8201eb214467d272a"}}},
        {837829, {{"2d5585dd9c50ea378ae12240d745bb14c65f5344ec66f1bb37ae040f04b076aa92321f674e127e4ec16f11830e8b7d6c1aab975983c7902c1aa5d3d2ea45b703", "d3f8b038749c0ddb5b0c5cdc2be93827acabc0ceba228ccad307fc220a825fbed319a56ca14cca1a720b39ff8fe952d57538a91158d4fc2cb64d697810a99378"}}},
        {838889, {{"f884d613b064fa88b135cf900d1e9cae31dce8e825f8cb162c785b9d40cb4ae47419d5a569eee490c359b0a4abb29d8af2553501274c09b269fc5a0a9d5feb1a", "38dd05e193ae7526204ff384697fa592411ceb595fbda8842427ea250f2c6af35915f5cba2efc11163f0a18a4a6ca9fbb8ccab06e785be93c0a38582f6bf3d3e"}}},
        {839057, {{"0aba90f391b2082154365ed081641684cd6aa7945d1c1a457fc36a13957f12864baf8069e3d6750107ac2ceda20ae9d49fbd1fb9e6f1dc362ca938e4a4cd8b5a", "a6a1fbf9eb2cfa166f4bd384def3c344d70a62a5790db4d742de9a902deca9c39a9ab59dc9fb27e0f0c1d9a9eb21825157c605d75d1ea91214d94b96618ac41b"}}},
        {839059, {{"c4036e9357fe26e4d29b5a307e9e8d883bccd8cfc050bcdc459bea33d3a24e13e0e74f5957dec0b8992b0c9a4ee01ab186905768434c04928dffcf15bb4631bb", "98df417b18765e04e202fd1f093756cb2d18f1d3b19ee3d35181d831705984cc3b2a7565d498e496a4800e3b42b4bb6b850bec56b0529c8795228673593f46b5"}}},
        {839346, {{"52c72c2cc60dd0d69192c0540ccbbeb99a6729f7112c65b5df969f2694f447220147e22637467225e3d2847749291635d8efc12a9bc45a2f49d3e818caa6ac23", "dcca9a5a373c2d5fd94c3a0ea877c0b9ad289957ae7f630a85acdcedea748e6cf898354b4bc1f870b8e788a1364d5d2032ce3be59326821dfd539b69f2353da4"}}},
        {840251, {{"284060c03f54dcc043e0b11455237ba9be9739f5f7e3d7d0bbe13aac2016b89ed87b790100d559793845527094aeb5f6cd4053452a944d9ccd1243188e8824cf", "dc8e868b659a1a6f57e6bf2b29ceaddb795e17f89c1b1719a543ad135d345cec4cd6c7349d287c93a13f1f3380b69ab7a23eb400b808ffbd8833e52ab11bd024"}}},
        {840274, {{"ef6cb40d052d70f63ab32266ba995c748e408e5687b428669cc5a71cce228991bb86b024395316dd54dc167af8b5b2e435ba1dd99cf43c2b3b5afef78d57147f", "f31c67d012b7ce12aeab8cfd326f42bfee73c9ef78a2af1b0c2b71605aac57ee5545a1c7370f2159b0bbbe7d9cd18d68c9d2a0f0d318008796e05c74ec2fa309"}}},
        {840404, {{"501ee3b1bd4619c02fbf14b19faa00bebc2e515848ebc05df97f74675d24299f467b7782c7b2dbe604b18ea10e12fd68e90859c74c9c7432066b9e1f83b2e150", "70fc629eb3d7bb0d09b2eb69ae5596015ac4aa5f5237679d6453ffebbe1cc36fe9ab3ed64992b655bc98cf3c171071ceb530f5d942393d02fbf1291d9035b188"}}},
        {840667, {{"be96a44922ada8dcdec6c62e7aaeb3c5b82daad9f09a5618b35e0791169648fc5da1332a2053eb8378904167764045caa0ff5bdbdb970b11ff1b950f84d3fba4", "86bcf0d395f5785c944f421a74ef686e3be9bf1ff927106e61ca9a391424e3b060ba43cef6b3961e4aa4a8413c5c6d0a471dc33f86d9976f91ba45b96f858c1d"}}},
        {841298, {{"33e161f5ed86b892cae64f8989075cea19f11373320e76df0e0212662069dc84fbce768551d009753ab1491e068532df1d5961a1c4001cd3dd981e3db62eb421", "16498692c9e020f6ada5225fe50626372864cb2c61e8bad9c3f582418d9483de0a0f0f0697ba41fb26fc8b6cc71f40f42825749fc8808afd5dcce6a332b32402"}}},
        {841305, {{"ce9c913ec42a31e200cf9e077a6010c69185412612471bb0f5228c76868aad7f818b10db9ea6fab062c18936fd4b4761c55ba8f997825c91d8ea0902aaf52c65", "b8276ef2194b7f7c2f3e28070ad5cf8292efbd20b502dc2dbcdc3e45d43fa84580dacc21065790fa224198d86f858add12eabbc6f38575a9f4d0d06ea4f9b781"}}},
        {878085, {{"68ca7556a872231f5573c4a087b5e7b357cd552650a1ba0b675c6f79b6e6361739300200bf6062285872f9c26908fa5204ddb11aaa3ec59dbadaf6c4a6e2a17f", "bd8659d8e169b97e705cfbd2d0f1b882713a3a4cdba2a2ca2a76f44974d3bed66389d2d33027083bd1cdbacc15c17a392ca85cf42ec21afca40a6f5075c8e99e"}}}
    };

    auto it = data_hash_map.find(block_height);
    if (it != data_hash_map.end())
    {
        auto hash_it = it->second.find(data_hash);
        if (hash_it != it->second.end())
        {
            data_hash = hash_it->second;
        }
    }
}

bool check_block_validity(const block bl, const std::size_t block_height) {
      if (block_height < BLOCK_HEIGHT_SF_V_2_2_0) {
        return true;
      };

      // if syncing from trusted nodes, no need to verify the block
      if (block_height <= xcash_trusted_sync_block) {
        return true;
      }

      std::vector<std::string> servers = {
        "seed1.xcash.tech",
        "seed2.xcash.tech",
        "seed3.xcash.tech",
        "seed5.xcash.tech"
    };


    std::vector<std::string> hashes;
    xcash_net::get_block_hashes(block_height, servers, hashes);

    std::string network_block_string = epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(bl));
    std::string data_hash = network_block_string.substr(network_block_string.find(BLOCKCHAIN_RESERVED_BYTES_START)+sizeof(BLOCKCHAIN_RESERVED_BYTES_START)-1,DATA_HASH_LENGTH);

    // fix history blocks wich was stored corrupted
    fix_data_hash(block_height, data_hash);

    int valid_hashes = 0;
    for (const auto& hash : hashes) {
      if (hash == data_hash) {
        valid_hashes++;
      }
    }

    if (valid_hashes >= BLOCK_VERIFIERS_VALID_AMOUNT) {
      return true;
    }

    return false;
}



}