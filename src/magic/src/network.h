#ifndef SHANDALAR_MAGIC_NETWORK_H
#define SHANDALAR_MAGIC_NETWORK_H

extern int g_network_result_value;
extern int g_network_result_packet_type;

void TENTATIVE_wait_for_network_result(int player, int packet_type);
void TENTATIVE_send_network_result(int player, int packet_type);

#endif
