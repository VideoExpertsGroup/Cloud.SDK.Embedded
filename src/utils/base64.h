#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Encodes null-terminated input 'string' using Base64 algorithm.
   Resulting null-terminated string is placed into 'outbuf'. No more than 'outlen' bytes are returned.
*/
void Encode64(char* outbuf, char* string, int outlen);

/* Encodes input 'string' using Base64 algorithm.
   Resulting string is placed into 'outbuf', it is null-terminated if there is enough space. No more than 'outlen' bytes are placed.
   Return value is length of resulting string
   If it is greater than 'outlen', this means overflow
*/
int Encode64_2(char* outbuf, int outlen, char* string, int inlen);

/* Decodes null-terminated input 'string' using Base64 algorithm.
   Resulting null-terminated string is placed into 'outbuf'. No more than 'outlen' bytes are returned.
   Return value is length of resulting string excluding trailing '0'.
   Negative value means error or overflow
*/
int  Decode64(char* outbuf, char* string, int outlen);

/* Decodes input 'string' using Base64 algorithm.
   Resulting data is placed into 'outbuf'. No more than 'outlen' bytes are returned.
   Return value is length of resulting data.
   If negative, it indicates the first invalid input byte as shown below: -1 for the 1st byte (string[0]), -2 for the 2nd,...
   If greater than 'outlen', it means overflow
*/
int  Decode64_2 (char* outbuf, int outlen, const char* string, int inlen);

#ifdef __cplusplus
};
#endif

#endif /* __BASE64_H__ */

