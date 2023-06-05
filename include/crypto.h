
#ifndef _CRYPTO_H_
#define _CRYPTO_H

#include <stdint.h>
#include <sys/types.h>
/*
typedef uint64_t i64_t;
typedef uint32_t i32_t;
typedef uint16_t i16_t;
typedef uint8_t  i8_t;
*/
void rsa_encrypt(uint8_t  *plaintext, uint16_t *ciphertext, uint64_t len, uint16_t e, uint16_t n);
void rsa_decrypt(uint8_t  *plaintext, uint16_t *ciphertext, uint64_t len, uint16_t d, uint16_t n);
void rsa_key_gen(uint16_t *p_e, uint16_t *p_d, uint16_t *p_n);

/* Maximum message size 
 */
//#define MAX_MSG_SZ (2048u)

/* Example main
 *
int main(int argc, char *argv[]) {

    i16_t e;
    i16_t n;
    i8_t plaintext[MAX_MSG_SZ];
    i16_t ciphertext[MAX_MSG_SZ];
    i8_t *p_cipher = (i8_t *)ciphertext;
    i64_t msg_sz = 0;

    i16_t d;

    // Generate the keys 
    rsa_key_gen(&e, &d, &n);

    // Print the values
    printf("e = %d\n", e);
    printf("d = %d\n", d);
    printf("n = %d\n", n);

    strcpy((char*)plaintext, "Roger isXX best 123#");
    msg_sz = strlen((char*)plaintext);
    printf("Message:%s Len:%d\n", plaintext, (int)msg_sz);
    
    // Encrypt the message
    rsa_encrypt(plaintext, ciphertext, msg_sz, e, n);

    // Print the ciphertext
    for (int i = 0; i < 2 * msg_sz; i++) {
        printf("%c", p_cipher[i]);
    }

    printf("\n--------------------------\n");

    // Decrypt the message 
    rsa_decrypt(plaintext, ciphertext, msg_sz, d, n);

    // Print the plaintext 
    for (int i = 0; i < msg_sz; i++) {
        printf("%c", plaintext[i]);
    }

    
    return 0;
}
*/

#endif /* _CRYPTO_H_ */
