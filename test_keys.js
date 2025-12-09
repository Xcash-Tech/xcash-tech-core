#!/usr/bin/env node
'use strict';

const { cnBase58 } = require('@xmr-core/xmr-b58');
const { keccak_256 } = require('js-sha3');

function hexToBytes(hex) {
  if (hex.length % 2 !== 0) throw new Error('Invalid hex length');
  const bytes = new Uint8Array(hex.length / 2);
  for (let i = 0; i < bytes.length; i++) {
    bytes[i] = parseInt(hex.substr(i * 2, 2), 16);
  }
  return bytes;
}

function decodeVarint(buffer, offset = 0) {
  let value = 0n;
  let shift = 0n;
  let i = offset;
  while (true) {
    if (i >= buffer.length) throw new Error('Malformed varint');
    const byte = buffer[i];
    value |= BigInt(byte & 0x7f) << shift;
    if ((byte & 0x80) === 0) {
      return { value: Number(value), bytes: i - offset + 1 };
    }
    shift += 7n;
    i++;
  }
}

function deriveKeysFromAddress(address) {
  const decoded = hexToBytes(cnBase58.decode(address));
  const { value: prefix, bytes } = decodeVarint(decoded);
  const spendPublic = Buffer.from(decoded.slice(bytes, bytes + 32)).toString('hex');
  const viewPublic = Buffer.from(decoded.slice(bytes + 32, bytes + 64)).toString('hex');
  const body = decoded.slice(0, bytes + 64);
  const checksum = Buffer.from(decoded.slice(bytes + 64, bytes + 68)).toString('hex');
  const expected = Buffer.from(keccak_256.arrayBuffer(body)).slice(0, 4).toString('hex');
  if (checksum !== expected) {
    throw new Error('Address checksum mismatch');
  }
  return { prefix, spendPublic, viewPublic };
}

// Test with our delegate address
const ADDRESS = '***REMOVED***';

console.log('=== Decoding X-CASH Address ===');
console.log('Address:', ADDRESS);
console.log('');

try {
  const keys = deriveKeysFromAddress(ADDRESS);
  console.log('Network prefix:', keys.prefix);
  console.log('Spend public key:', keys.spendPublic);
  console.log('View public key:', keys.viewPublic);
  console.log('');
  console.log('=== Expected from daemon ===');
  console.log('C++ address.m_spend_public_key should be:', keys.spendPublic);
  console.log('C++ derived leader_pubkey (from secret):', '795f5a4acb9ac8390d314cefbbf64a58df6924f7f8cb566422af348c3064a62e');
  console.log('');
  console.log('NOTE: These should be DIFFERENT!');
  console.log('  - spendPublic is from the address structure');
  console.log('  - leader_pubkey is derived from secret key hash');
} catch (err) {
  console.error('Error:', err.message);
  process.exit(1);
}
