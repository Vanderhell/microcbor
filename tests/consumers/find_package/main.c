#include "mcbor.h"

int main(void)
{
    mcbor_enc_t enc;
    uint8_t buf[8];

    return mcbor_enc_init(&enc, buf, sizeof(buf)) == MCBOR_OK ? 0 : 1;
}
