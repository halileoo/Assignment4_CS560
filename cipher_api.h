#ifndef CIPHER_API_H
#define CIPHER_API_H

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

typedef void* cipher_t;

extern "C" {
    EXPORT cipher_t cipher_create_caesar(int key);
    EXPORT cipher_t cipher_create_vigenere(const char* key);

    EXPORT char* cipher_encrypt(cipher_t cipher, const char* text);
    EXPORT char* cipher_decrypt(cipher_t cipher, const char* text);

    EXPORT void  cipher_destroy(cipher_t cipher);
    EXPORT void  cipher_free(char* str);
}

#endif
