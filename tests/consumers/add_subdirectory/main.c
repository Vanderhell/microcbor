#include "mcbor.h"

int main(void)
{
    mcbor_dec_t dec;
    static const uint8_t data[] = {0xF6};

    return mcbor_dec_init(&dec, data, sizeof(data)) == MCBOR_OK ? 0 : 1;
}
