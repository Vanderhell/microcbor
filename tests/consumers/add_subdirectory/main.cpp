#include "mcbor.h"

int main()
{
    mcbor_enc_t enc;
    uint8_t buf[1];
    return mcbor_enc_init(&enc, buf, sizeof(buf)) == MCBOR_OK ? 0 : 1;
}
