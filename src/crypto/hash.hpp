#ifndef SIMQ_CRYPTO_HASH 
#define SIMQ_CRYPTO_HASH 

#include <openssl/sha.h>
#include <string.h>

namespace simq::crypto {

    const unsigned int HASH_LENGTH = SHA256_DIGEST_LENGTH;

    class Hash {
        public:
        static void hash( const char *password, unsigned char *hash );
    };

    void Hash::hash( const char *password, unsigned char *hash ) {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, password, strlen(password));

        SHA256_Final(hash, &sha256);
    }

}

#endif
