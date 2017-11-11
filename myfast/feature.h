
struct syn_flood{
  unsigned char data[54];
  unsigned char IPsrc[16];
  unsigned int IPsrcflag;
  unsigned char IPdst[16];
  unsigned int IPdst_synflag;
  unsigned char syn;
  unsigned char ack;
  unsigned char heibaiflag;
  long firsttime;
}
