#ifndef SHANDALAR_MAGIC_NETWORK_H
#define SHANDALAR_MAGIC_NETWORK_H

typedef struct
{
  char packet_type;
  char pad_1;
  short packet_number;
  int result;
} result_network_packet_t;

extern result_network_packet_t g_network_result_packet;

void TENTATIVE_wait_for_network_result(int player, int packet_type);
void TENTATIVE_send_network_result(int player, int packet_type);

#endif
