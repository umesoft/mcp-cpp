# ifndef __BASE64_H__
# define __BASE64_H__

# ifdef __cplusplus
extern "C" {
# endif
# if 0
}
# endif

typedef enum tagBASE64_TYPE {
	BASE64_TYPE_STANDARD,
	BASE64_TYPE_MIME,
	BASE64_TYPE_URL
} BASE64_TYPE;

char *base64Encode(const char *data, const size_t size, const BASE64_TYPE type);
char *base64Decode(const char *base64, size_t *retSize, const BASE64_TYPE type);

# if 0
{
# endif
# ifdef __cplusplus
}
# endif

# endif // !__BASE64_H__
