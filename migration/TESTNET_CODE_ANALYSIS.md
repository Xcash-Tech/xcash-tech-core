# Testnet Configuration Analysis

## Что влияет на поведение testnet в коде

### 1. Директории данных
**Файл:** `src/cryptonote_core/cryptonote_core.cpp`, `src/rpc/core_rpc_server.cpp`, `src/wallet/wallet2.cpp`
- При `--testnet` данные сохраняются в подпапку `testnet`
- Пример: `~/.xcash/` → `~/.xcash/testnet/`

### 2. Network ID и P2P
**Файл:** `src/p2p/net_node.inl`
```cpp
if (m_nettype == cryptonote::TESTNET)
{
  memcpy(&m_network_id, &::config::testnet::NETWORK_ID, 16);
  full_addrs = get_seed_nodes(cryptonote::TESTNET);
}
```
- Устанавливает уникальный network_id для P2P изоляции
- Загружает seed nodes для testnet
- Проверяет соответствие портов

### 3. RPC Responses
**Файл:** `src/rpc/core_rpc_server.cpp`
```cpp
res.testnet = m_nettype == TESTNET;
res.nettype = m_nettype == MAINNET ? "mainnet" : m_nettype == TESTNET ? "testnet" : ...;
```
- Возвращает флаг `testnet: true/false` в RPC ответах
- Показывает тип сети в `get_info`

### 4. Quick Sync Height
**Файл:** `src/cryptonote_core/cryptonote_core.cpp:1087`
```cpp
static const uint64_t quick_height = m_nettype == TESTNET ? 801219 : m_nettype == MAINNET ? 1220516 : 0;
```
- Влияет на оптимизацию синхронизации блоков

### 5. Protocol Sync
**Файл:** `src/cryptonote_protocol/cryptonote_protocol_handler.inl:305`
```cpp
uint64_t last_block_v1 = m_core.get_nettype() == TESTNET ? HF_BLOCK_HEIGHT_PROOF_OF_STAKE-1 : ...;
```
- Определяет высоту последнего блока v1 для синхронизации

### 6. Wallet Specifics
**Файл:** `src/wallet/wallet2.cpp`

#### Genesis Block Check
```cpp
std::string what("Genesis block mismatch. You probably use wallet without testnet flag...");
```
- Проверяет соответствие genesis блока между wallet и daemon

#### Blockchain Height Estimation
```cpp
static const uint64_t approximate_testnet_rolled_back_blocks = 303967;
if (m_nettype == TESTNET && approx_blockchain_height > approximate_testnet_rolled_back_blocks)
  approx_blockchain_height -= approximate_testnet_rolled_back_blocks;
```
- Корректирует оценку высоты блокчейна с учетом rollback

#### Segregation Fork (Line 11652)
```cpp
if (m_nettype == TESTNET)
  // special handling for testnet
```

#### V2 Height (Line 5145)
```cpp
uint64_t v2height = m_nettype == TESTNET ? 624634 : m_nettype == STAGENET ? 32000 : 1009827;
```

### 7. Default Ports
**Файл:** `src/p2p/net_node.inl:536`
```cpp
if ((m_nettype == cryptonote::TESTNET && m_port != std::to_string(::config::testnet::P2P_DEFAULT_PORT)))
```
- Предупреждает если используются нестандартные порты

## Критичные моменты для миграционной сети

✅ **Что уже настроено правильно:**
1. Уникальный `NETWORK_ID` - P2P изоляция работает
2. Порты 58280/58281/58282 - изолированы от mainnet
3. Seed nodes на xcash.testnet
4. Address prefix как в mainnet - адреса совместимы
5. Genesis TX как в mainnet - blockchain совместим

⚠️ **На что обратить внимание:**
1. **Quick sync height (801219)** - возможно нужно обновить для миграционной сети
2. **Approximate rolled back blocks (303967)** - специфично для legacy testnet
3. **v2height (624634)** - может не соответствовать миграционной высоте

## Рекомендации

Для миграционной сети (testnet с mainnet данными) возможно потребуется:

1. **Опциональная настройка константы quick_height:**
   - Использовать mainnet значение (1220516) вместо testnet (801219)
   
2. **Отключить корректировку rolled_back_blocks:**
   - Миграционная сеть использует mainnet LMDB, там нет rollback
   
3. **Использовать mainnet v2height:**
   - Вместо testnet значения (624634)

Эти изменения нужны только если возникнут проблемы с синхронизацией или wallet операциями.

## Файлы требующие внимания

При реальных проблемах проверить:
- `src/cryptonote_core/cryptonote_core.cpp:1087` - quick_height
- `src/wallet/wallet2.cpp:10229` - approximate_testnet_rolled_back_blocks  
- `src/wallet/wallet2.cpp:5145` - v2height
- `src/cryptonote_protocol/cryptonote_protocol_handler.inl:305` - last_block_v1

**Текущая конфигурация должна работать "из коробки"**, но эти константы могут потребовать корректировки при проблемах.
