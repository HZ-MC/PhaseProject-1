kmain()
{
  char* vidmen = (char*)0xb8000;
  vidmen[0] = 'A';
  vidmen[1] = 0x07;
}
