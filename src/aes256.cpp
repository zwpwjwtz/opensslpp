﻿#include "../include/opensslpp/aes256.h"

#include "../include/opensslpp/base64.h"
#include "../include/opensslpp/random.h"

#include "common.h"

std::unique_ptr<opensslpp::Aes256> opensslpp::Aes256::createNewKey()
{
    auto random = Random::create();
    if (!random)
        return nullptr;

    Key key;
    if (!random->getRandomBytes(key.data(), key.size()))
        return nullptr;

    return std::unique_ptr<Aes256>(new Aes256(std::move(random), std::move(key)));
}

std::unique_ptr<opensslpp::Aes256> opensslpp::Aes256::createWithKey(const std::string& base64Key)
{
    auto random = Random::create();
    if (!random)
        return nullptr;

    Key key;

    if (Base64::decode(base64Key, key.data(), key.size()) != key.size())
        return nullptr;

    return std::unique_ptr<Aes256>(new Aes256(std::move(random), std::move(key)));
}

std::string opensslpp::Aes256::base64Key() const
{
    return Base64::encode(key_.data(), key_.size());
}

const opensslpp::Aes256::Key& opensslpp::Aes256::key() const
{
    return key_;
}

bool opensslpp::Aes256::encrypt(const std::string& plainText, std::vector<uint8_t>& cipher, Iv& iv) const
{
    return encrypt(reinterpret_cast<const uint8_t*>(plainText.c_str()), plainText.size(), cipher, iv);
}

bool opensslpp::Aes256::encrypt(const uint8_t* plainData, size_t plainDataSize, std::vector<uint8_t>& cipher, Iv& iv) const
{
    if (!random_->getRandomBytes(iv.data(), iv.size()))
        return false;

    CipherContextPtr context(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!context)
        return false;

    if (EVP_EncryptInit_ex(context.get(), EVP_aes_256_cbc(), nullptr, key_.data(), iv.data()) != Success)
        return false;

    const size_t cipherSize = getCipherSize(plainDataSize);
    cipher.resize(cipherSize);

    int size = 0;
    if (EVP_EncryptUpdate(context.get(), cipher.data(), &size, plainData, plainDataSize) != Success)
        return false;

    if (EVP_EncryptFinal_ex(context.get(), cipher.data() + size, &size) != Success)
        return false;

    return true;
}

bool opensslpp::Aes256::decrypt(const std::vector<uint8_t>& cipher, const Iv& iv, std::vector<uint8_t>& plainData) const
{
    CipherContextPtr context(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!context)
        return false;

    if (EVP_DecryptInit_ex(context.get(), EVP_aes_256_cbc(), nullptr, key_.data(), iv.data()) != Success)
        return false;

    plainData.resize(cipher.size());

    int size = 0;
    if (EVP_DecryptUpdate(context.get(), plainData.data(), &size, cipher.data(), cipher.size()) != Success)
        return false;

    auto plainDataSize = size;

    if (EVP_DecryptFinal_ex(context.get(), plainData.data() + size, &size) != Success)
        return false;

    plainDataSize += size;

    plainData.resize(plainDataSize);

    return true;
}

size_t opensslpp::Aes256::getCipherSize(size_t plainSize)
{
    return (plainSize / IvSize + 1) * IvSize;
}

opensslpp::Aes256::Aes256(std::unique_ptr<Random>&& random, Key&& key)
    : random_(std::move(random))
    , key_(std::move(key))
{
}

opensslpp::Aes256::~Aes256()
{
}
