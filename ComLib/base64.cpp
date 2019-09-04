#include "base64.h"

static mstring _base64_encode(mstring str, const char* encodeChars)
{
    mstring ret;

    unsigned char c1, c2, c3;
    size_t i = 0;
    size_t len = str.length();

    while (i < len)
    {
        // read the first byte
        c1 = str[i++];
        if (i == len)	   // pad with "="
        {
            ret += encodeChars[c1 >> 2];
            ret += encodeChars[(c1 & 0x3) << 4];
            ret += "==";
            break;
        }

        // read the second byte
        c2 = str[i++];
        if (i == len)	   // pad with "="
        {
            ret += encodeChars[c1 >> 2];
            ret += encodeChars[((c1 & 0x3) << 4) | ((c2 & 0xF0) >> 4)];
            ret += encodeChars[(c2 & 0xF) << 2];
            ret += "=";
            break;
        }

        // read the third byte
        c3 = str[i++];
        // convert into four bytes mstring
        ret += encodeChars[c1 >> 2];
        ret += encodeChars[((c1 & 0x3) << 4) | ((c2 & 0xF0) >> 4)];
        ret += encodeChars[((c2 & 0xF) << 2) | ((c3 & 0xC0) >> 6)];
        ret += encodeChars[c3 & 0x3F];
    }

    return ret;
}

static mstring _base64_decode(mstring str, const char* decodeChars)
{
    char c1, c2, c3, c4;
    size_t i = 0;
    size_t len = str.length();
    mstring ret;

    while (i < len)
    {
        // read the first byte
        do
        {
            c1 = decodeChars[str[i++]];
        } while (i < len && c1 == -1);

        if (c1 == -1)
        {
            break;
        }

        // read the second byte
        do
        {
            c2 = decodeChars[str[i++]];
        } while (i < len && c2 == -1);

        if (c2 == -1)
        {
            break;
        }

        // assamble the first byte
        ret += char((c1 << 2) | ((c2 & 0x30) >> 4));

        // read the third byte
        do
        {
            c3 = (char)str[i++];
            if (c3 == 61)	   // meet with "=", break
            {
                return ret;
            }
            c3 = decodeChars[c3];
        } while (i < len && c3 == -1);

        if (c3 == -1)
        {
            break;
        }

        // assamble the second byte
        ret += char(((c2 & 0XF) << 4) | ((c3 & 0x3C) >> 2));

        // read the fourth byte
        do
        {
            c4 = (char)str[i++];
            if (c4 == 61)	   // meet with "=", break
            {
                return ret;
            }
            c4 = decodeChars[c4];
        } while (i < len && c4 == -1);

        if (c4 == -1)
        {
            break;
        }

        // assamble the third byte
        ret += char(((c3 & 0x03) << 6) | c4);
    }

    return ret;
}

mstring base64encode(mstring str)
{
    static const char s_std_base64_encode_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    return _base64_encode(str, s_std_base64_encode_chars);
}

mstring base64decode(mstring str)
{
    static const char s_std_base64_decode_chars[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
    };

    return _base64_decode(str, s_std_base64_decode_chars);
}
