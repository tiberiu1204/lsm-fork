int main() {
  unsigned char shellcode[] = "\x90"; // nop

  void (*func)() = (void (*)())shellcode;
  func(); // Should segfault

  return 0;
}
