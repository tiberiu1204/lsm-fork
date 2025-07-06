int main() {
  unsigned char shellcode[] = "\x31\xc0" // xor eax,eax
                              "\xc3";    // ret

  void (*func)() = (void (*)())shellcode;
  func(); // Should segfault

  return 0;
}
