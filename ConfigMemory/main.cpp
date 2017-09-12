#include <cox.h>

char keyBuf[128];

#define MEMORY ConfigMemory2

static void printMenu() {
  printf("Usage:\n");
  printf("* read {addr} {len}\n");
  printf("* write {addr} {single byte of data}\n");
  printf("* length\n");
  printf("* writestring {addr} {string}\n");
}

static uint8_t copyUntil(char *dst, char *src, uint8_t len, char terminator) {
  uint8_t i, j = 0;

  for (i = 0; i < len; i++) {
    if (src[i] == terminator) {
      i++;
      dst[j++] = '\0';
      break;
    } else {
      dst[j++] = src[i];
    }
  }

  return i;
}

static void eventSerialReceived(SerialPort &) {
  char tmpBuf[256];
  uint8_t i;

  if (strcmp(keyBuf, "length") == 0) {
    printf("* length: 0x%lX\n", (uint32_t) MEMORY.length());
  } else if (strncmp(keyBuf, "read ", 5) == 0) {
    uint32_t addr, len;

    i = 5;
    i += copyUntil(tmpBuf, &keyBuf[i], 255 - i, ' ');
    addr = strtoul(tmpBuf, NULL, 0);

    i += copyUntil(tmpBuf, &keyBuf[i], 255 - i, '\0');
    len = strtoul(tmpBuf, NULL, 0);

    printf("* read %lu bytes from address 0x%lX:\n", len, addr);

    if (len > 0) {
      uint8_t *dump = (uint8_t *) dynamicMalloc(len);
      if (dump) {
        MEMORY.read(dump, addr, len);

        for (uint32_t x = 0; x < len; x++) {
          uint8_t index = x % 16;

          if (index == 0) {
            printf("0x%08lX |", addr + x);
          }
          printf(" %02X", dump[x]);
          if (index == 7) {
            printf(" ");
          } else if (x % 16 == 15) {
            printf("\n");
          }
        }
        printf("\n");
        dynamicFree(dump);
      }
    }
  } else if (strncmp(keyBuf, "write ", 6) == 0) {
    uint32_t addr;
    uint8_t data;

    i = 6;
    i += copyUntil(tmpBuf, &keyBuf[i], 255 - i, ' ');
    addr = strtoul(tmpBuf, NULL, 0);

    i += copyUntil(tmpBuf, &keyBuf[i], 255 - i, '\0');
    data = (uint8_t) strtoul(tmpBuf, NULL, 0);

    printf("* write 0x%02X to address 0x%lX:\n", data, addr);

    MEMORY.write(addr, data);
  } else if (strncmp(keyBuf, "writestring ", 12) == 0) {
    uint32_t addr;
    const char *data;

    i = 12;
    i += copyUntil(tmpBuf, &keyBuf[i], 255 - i, ' ');
    addr = strtoul(tmpBuf, NULL, 0);

    data = keyBuf + i;
    printf("* writestring \"%s\" to address 0x%lX:\n", data, addr);

    MEMORY.write((const uint8_t *) data, addr, strlen(data));
  } else {
    printf("* Unknown command\n");
  }

  printf("\n");
  printMenu();
  Serial.inputKeyboard(keyBuf, sizeof(keyBuf) - 1);
}

void setup() {
  Serial.begin(115200);
  printf("\n*** [Nol.Board] Config Memory Test ***\n");

  Serial.onReceive(eventSerialReceived);
  Serial.inputKeyboard(keyBuf, sizeof(keyBuf) - 1);
  Serial.listen();
  printMenu();
}
