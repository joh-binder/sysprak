#ifndef PERFORM_CONNECTION
#define PERFORM_CONNECTION

void receive_msg(int sock, char *recmsg);

void send_msg(int sock, char *sendmsg);

void performConnection(int sock, char *gameID);

#endif