#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>


int encrypt(unsigned char* cipher, unsigned char* plain, unsigned int plainLen, unsigned char* key, unsigned char *iv){
	EVP_CIPHER_CTX ctx;//cipher context = ctx
	int addLen=0, outLen=0;
	unsigned long err=0;

	ERR_load_crypto_strings();
	EVP_CIPHER_CTX_init(&ctx);

	if(EVP_EncryptInit_ex(&ctx, EVP_aes_128_gcm(), NULL, key, iv) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_Encryptinit - %s\n", ERR_error_string(err,NULL));

		return -1;
	}

	if(EVP_EncryptUpdate(&ctx, cipher, &outLen, plain, plainLen) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_EncryptUpdate - %s\n", ERR_error_string(err, NULL));

		return -1;
	}
	
	if(EVP_EncryptFinal_ex(&ctx, &cipher[outLen], &addLen) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_EncryptFinal() - %s\n", ERR_error_string(err, NULL));

		return -1;
	}
			
	EVP_CIPHER_CTX_cleanup(&ctx);
	ERR_free_strings();
	printf("addLen = %d, outLen = %d\n", addLen, outLen);

	return addLen + outLen;
}

int decrypt(unsigned char *plain, unsigned char *cipher, unsigned int cipherLen, unsigned char *key, unsigned char *iv){
	EVP_CIPHER_CTX ctx;
	unsigned long err=0;
	int subLen = 0, outLen=0;

	ERR_load_crypto_strings();
	EVP_CIPHER_CTX_init(&ctx);

	if (EVP_DecryptInit_ex(&ctx, EVP_aes_128_gcm(), NULL, key, iv) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_Decryptinit - %s\n", ERR_error_string(err,NULL));
		return -1;
	}
	if (EVP_DecryptUpdate(&ctx, plain, &outLen, cipher, cipherLen) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_DecryptUpdate - %s\n", ERR_error_string(err,NULL));
		return -1;
	}
/*	if(EVP_DecryptFinal_ex(&ctx, &plain[outLen], &subLen) != 1){
		err = ERR_get_error();
		printf("ERR:EVP_DecryptFinal() - %s\n", ERR_error_string(err, NULL));
		return -1;
	}
*/
	EVP_CIPHER_CTX_cleanup(&ctx);
	ERR_free_strings();
	printf("subLen = %d, outLen = %d\n", subLen, outLen);
	
	return subLen+outLen;

}

int main(void){
	int clen;
	char key[17] = "1234567890123456"; //sizeof(key)=17(actually 4 but..) 16+NULL. but using 16.
	char iv[13] = "123456789012";  //AES_GCM has IV which is size 96bit (12Byte)
	char p[] = "123456789012345";
	printf("plainLen %d\n", strlen(p));
	char *c = (char*)calloc(1024, sizeof(char));
	char *p2 = (char*)calloc(1024, sizeof(char));
	clen = encrypt(c, p, strlen(p)+1, key, iv);	
	printf("%s\n", c);
	decrypt(p2, c, clen, key, iv);
	printf("%s\n", p2);
	return 0;
}
