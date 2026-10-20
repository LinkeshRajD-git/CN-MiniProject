// 8-bit one's-complement checksum demonstration.
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    char input[512];
    printf("Enter text data: ");
    if (!fgets(input, sizeof(input), stdin)) return 1;
    size_t n = strcspn(input, "\r\n");
    input[n] = '\0';
    if (n == 0) { puts("Empty input."); return 1; }

    unsigned int sum = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned int word = (unsigned char)input[i];
        unsigned int before = sum;
        sum += word;
        unsigned int carry = sum >> 8;
        sum = (sum & 0xFF) + carry; // end-around carry
        printf("Byte %zu (%u): sum before=%u, raw sum=%u, carry=%u, folded=%u\n",
               i, word, before, before + word, carry, sum);
    }
    unsigned int checksum = (~sum) & 0xFF;
    printf("Sender checksum = 0x%02X (%u)\n", checksum, checksum);

    // Receiver adds all bytes plus checksum. Valid if result is 0xFF.
    unsigned int verify = sum + checksum;
    verify = (verify & 0xFF) + (verify >> 8);
    printf("Receiver final sum = 0x%02X\n", verify);
    puts(verify == 0xFF ? "Verification: no error detected"
                        : "Verification: error detected");
    puts("To demonstrate an error, change one input byte at receiver before checking.");
    return 0;
}
